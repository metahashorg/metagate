#include "NsLookup.h"

#include <set>

#include <QUdpSocket>
#include <QApplication>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include <QSettings>

#include "dns/dnspacket.h"
#include "check.h"
#include "utils.h"
#include "duration.h"
#include "Log.h"
#include "SlotWrapper.h"
#include "Paths.h"

#include "algorithms.h"

const static QString NODES_FILE = "nodes.txt";

const static QString FILL_NODES_PATH = "fill_nodes.txt";

const static std::string CURRENT_VERSION = "v2";

const static milliseconds MAX_PING = 100s;

const static milliseconds UPDATE_PERIOD = days(1);

const static size_t ACCEPTABLE_COUNT_ADDRESSES = 3;

NsLookup::NsLookup(QObject *parent)
    : QObject(parent)
{
    QSettings settings(getSettingsPath(), QSettings::IniFormat);
    const int size = settings.beginReadArray("nodes");
    for (int i = 0; i < size; i++) {
        settings.setArrayIndex(i);
        NodeType info;
        info.type = settings.value("type").toString();
        info.node = settings.value("node").toString();
        info.port = settings.value("port").toString();
        LOG << "node " << info.type << ". " << info.node << ". " << info.port << ".";
        nodes.emplace_back(info);
    }
    settings.endArray();

    savedNodesPath = makePath(getNsLookupPath(), FILL_NODES_PATH);
    const system_time_point lastFill = fillNodesFromFile(savedNodesPath, nodes);
    const system_time_point now = system_now();
    passedTime = std::chrono::duration_cast<milliseconds>(now - lastFill);
    if (lastFill - now >= hours(1)) {
        // Защита от перевода времени назад
        passedTime = UPDATE_PERIOD;
    }

    CHECK(QObject::connect(&thread1,SIGNAL(started()),this,SLOT(run())), "not connect started");
    CHECK(QObject::connect(this,SIGNAL(finished()),&thread1,SLOT(terminate())), "not connect finished");

    milliseconds msTimer = 1ms;
    if (passedTime >= UPDATE_PERIOD) {
        isSafeCheck = false;
    } else {
        isSafeCheck = true;
    }
    LOG << "Start ns resolve after " << msTimer.count() << " ms";
    qtimer.moveToThread(&thread1);
    qtimer.setInterval(msTimer.count());
    qtimer.setSingleShot(true);
    CHECK(connect(&qtimer, SIGNAL(timeout()), this, SLOT(uploadEvent())), "not connect");
    CHECK(qtimer.connect(&thread1, SIGNAL(started()), SLOT(start())), "not connect");
    CHECK(qtimer.connect(&thread1, SIGNAL(finished()), SLOT(stop())), "not connect");

    client.setParent(this);
    CHECK(connect(&client, &SimpleClient::callbackCall, this, &NsLookup::callbackCall), "not connect callbackCall");

    client.moveToThread(&thread1);
    moveToThread(&thread1);
}

NsLookup::~NsLookup() {
    isStopped = true;
    thread1.quit();
    if (!thread1.wait(3000)) {
        thread1.terminate();
        thread1.wait();
    }

    if (isResetFilledFile.load()) {
        removeFile(savedNodesPath);
    }
}

void NsLookup::callbackCall(SimpleClient::ReturnCallback callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void NsLookup::start() {
    thread1.start();
}

void NsLookup::run() {
    // empty
}

void NsLookup::finalizeLookup() {
    std::unique_lock<std::mutex> lock(nodeMutex);
    if (!isSafeCheck) {
        allNodes.swap(allNodesNew);
        allNodesForTypes.swap(allNodesForTypesNew);
    }
    sortAll();
    if (!isSafeCheck) {
        saveToFile(savedNodesPath, system_now(), nodes);
    }
    lock.unlock();

    const time_point stopScan = ::now();
    LOG << "Dns scan time " << std::chrono::duration_cast<seconds>(stopScan - startScanTime).count() << " seconds";

    milliseconds msTimer;
    bool isSuccessFl = false;
    if (isSafeCheck) {
        bool isSuccess = true;
        for (const auto &elem: allNodesForTypes) {
            const size_t countSuccess = std::accumulate(elem.second.begin(), elem.second.end(), size_t(0), [](size_t prev, const NodeInfo &subElem) -> size_t {
                if (subElem.ping != MAX_PING.count()) {
                    return prev + 1;
                } else {
                    return prev + 0;
                }
            });
            if (countSuccess < ACCEPTABLE_COUNT_ADDRESSES) {
                isSuccess = false;
            }
        }
        if (isSuccess) {
            LOG << "Dns safe check success";
            msTimer = UPDATE_PERIOD - passedTime;
            isSuccessFl = true;
        } else {
            LOG << "Dns safe check not success. Start full scan";
            msTimer = 1ms;
        }
    } else {
        msTimer = UPDATE_PERIOD;
        isSuccessFl = true;
    }
    if (isSuccessFl) {
        emit serversFlushed(TypedException());
    }
    isSafeCheck = false;
    qtimer.setInterval(msTimer.count());
    qtimer.setSingleShot(true);
}

void NsLookup::uploadEvent() {
BEGIN_SLOT_WRAPPER
    qtimer.setSingleShot(true);
    qtimer.start(milliseconds(10min).count()); // В случае, если что-то не удастся, через 10 минут произойдет повторная попытка

    startScanTime = ::now();

    posInNodes = 0;
    allNodesForTypesNew.clear();
    allNodesNew.clear();

    LOG << "Dns scan start";
    continueResolve();
END_SLOT_WRAPPER
}

std::vector<QString> NsLookup::requestDns(const NodeType &node) const {
    QUdpSocket udp;

    DnsPacket requestPacket;
    requestPacket.addQuestion(DnsQuestion::getIp(node.node));
    requestPacket.setFlags(DnsFlag::MyFlag);
    udp.writeDatagram(requestPacket.toByteArray(), QHostAddress("8.8.8.8"), 53);

    udp.waitForReadyRead();
    std::vector<char> data(512 * 1000, 0);
    const qint64 size = udp.readDatagram(data.data(), data.size());
    CHECK(size > 0, "Incorrect response dns");
    const DnsPacket packet = DnsPacket::fromBytesArary(QByteArray(data.data(), size));

    LOG << "dns ok " << node.type << ". " << packet.answers().size();
    CHECK(!packet.answers().empty(), "Empty dns response");

    std::vector<QString> result;
    for (const auto &record : packet.answers()) {
        result.emplace_back(makeAddress(record.toString(), node.port));
    }
    return result;
}

void NsLookup::continueResolve() {
    if (isStopped.load()) {
        return;
    }
    if (posInNodes >= nodes.size()) {
        finalizeLookup();
        return;
    }

    const NodeType &node = nodes[posInNodes];
    posInNodes++;

    ipsTemp = requestDns(node);
    posInIpsTemp = 0;
    countSuccessfullTemp = 0;

    continuePing();
}

QString NsLookup::makeAddress(const QString &ip, const QString &port) {
    return "http://" + ip + ":" + port;
}

void NsLookup::continuePing() {
    if (isStopped.load()) {
        return;
    }

    const NodeType &nodeType = nodes[posInNodes - 1];

    if (!isSafeCheck) {
        if (posInIpsTemp >= ipsTemp.size()) {
            continueResolve();
            return;
        }

        const size_t countSteps = std::min(size_t(10), ipsTemp.size() - posInIpsTemp);

        std::shared_ptr<size_t> requestsInProcess = std::make_shared<size_t>(countSteps);

        for (size_t i = 0; i < countSteps; i++) {
            const QString &ip = ipsTemp[posInIpsTemp];
            posInIpsTemp++;
            client.ping(ip, [this, type=nodeType.type, requestsInProcess](const QString &address, const milliseconds &time, const std::string &message) {
                try {
                    NodeInfo info;
                    info.address = address;
                    info.ping = time.count();

                    if (message.empty()) {
                        info.ping = MAX_PING.count();
                    } else {
                        QJsonParseError parseError;
                        QJsonDocument::fromJson(QString::fromStdString(message).toUtf8(), &parseError);
                        if (parseError.error != QJsonParseError::NoError) {
                            info.ping = MAX_PING.count();
                        }
                    }

                    addNode(type, info, true);
                } catch (const Exception &e) {
                    LOG << "Error " << e;
                } catch (const std::exception &e) {
                    LOG << "Error " << e.what();
                } catch (const TypedException &e) {
                    LOG << "Error typed " << e.description;
                } catch (...) {
                    LOG << "Unknown error";
                }

                (*requestsInProcess)--;
                if ((*requestsInProcess) == 0) {
                    continuePing();
                }
            }, 2s);
        }
    } else {
        if (posInIpsTemp >= allNodesForTypes[nodeType.type].size()) {
            continueResolve();
            return;
        }

        const size_t countIterations = allNodesForTypes[nodeType.type].size() - posInIpsTemp;
        std::vector<size_t> processVectPos;
        for (size_t i = 0; i < countIterations; i++) {
            NodeInfo &element = allNodesForTypes[nodeType.type][posInIpsTemp];
            if (element.ping == MAX_PING.count()) {
                // skip
            } else if (std::find(ipsTemp.begin(), ipsTemp.end(), element.address) == ipsTemp.end()) {
                element.ping = MAX_PING.count();
            } else {
                processVectPos.emplace_back(posInIpsTemp);
            }
            posInIpsTemp++;
            if (processVectPos.size() >= ACCEPTABLE_COUNT_ADDRESSES) {
                break;
            }
        }
        std::shared_ptr<size_t> requestsInProcess = std::make_shared<size_t>(processVectPos.size());
        for (size_t i = 0; i < processVectPos.size(); i++) {
            const size_t posInIpsTempSave = processVectPos[i];
            const QString &address = allNodesForTypes[nodeType.type][posInIpsTempSave].get().address;
            client.ping(address, [this, type=nodeType.type, posInIpsTempSave, requestsInProcess](const QString &address, const milliseconds &time, const std::string &message) {
                try {
                    NodeInfo &info = allNodesForTypes[type][posInIpsTempSave];
                    CHECK(info.address == address, "Incorrect address");

                    if (message.empty()) {
                        info.ping = MAX_PING.count();
                    } else {
                        QJsonParseError parseError;
                        QJsonDocument::fromJson(QString::fromStdString(message).toUtf8(), &parseError);
                        if (parseError.error != QJsonParseError::NoError) {
                            info.ping = MAX_PING.count();
                        }
                    }

                    if (info.ping != MAX_PING.count()) {
                        countSuccessfullTemp++;
                    }
                } catch (const Exception &e) {
                    LOG << "Error " << e;
                } catch (const std::exception &e) {
                    LOG << "Error " << e.what();
                } catch (const TypedException &e) {
                    LOG << "Error typed " << e.description;
                } catch (...) {
                    LOG << "Unknown error";
                }

                (*requestsInProcess)--;
                if ((*requestsInProcess) == 0) {
                    if (countSuccessfullTemp >= ACCEPTABLE_COUNT_ADDRESSES) {
                        continueResolve();
                    } else {
                        continuePing();
                    }
                }
            }, 2s);
        }
    }
}

void NsLookup::addNode(const QString &type, const NodeInfo &node, bool isNew) {
    auto processNode = [](std::deque<NodeInfo> &allNodes, std::map<QString, std::vector<std::reference_wrapper<NodeInfo>>> &allNodesForTypes, const QString &type, const NodeInfo &node) {
        allNodes.emplace_back(node);
        NodeInfo &ref = allNodes.back();
        allNodesForTypes[type].emplace_back(std::ref(ref));
    };

    if (isNew) {
        processNode(allNodesNew, allNodesForTypesNew, type, node);
    } else {
        processNode(allNodes, allNodesForTypes, type, node);
    }
}

void NsLookup::sortAll() {
    for (auto &element: allNodesForTypes) {
        std::sort(element.second.begin(), element.second.end(), std::less<NodeInfo>{});
    }
}

static void createSymlink(const QString &file) {
    const QString symlink = makePath(QApplication::applicationDirPath(), "fill_nodes_symlink.lnk");
    QFile::link(file, symlink);
}

static std::string calcHashNodes(const std::vector<NodeType> &expectedNodes) {
    std::vector<NodeType> copyNodes = expectedNodes;
    std::sort(copyNodes.begin(), copyNodes.end(), [](const NodeType &first, const NodeType &second) {
        return first.type < second.type;
    });
    const std::string strNodes = std::accumulate(copyNodes.begin(), copyNodes.end(), std::string(), [](const std::string &a, const NodeType &b) {
        return a + b.type.toStdString() + b.node.toStdString() + b.port.toStdString();
    });
    return QString(QCryptographicHash::hash(QString::fromStdString(strNodes).toUtf8(), QCryptographicHash::Md5).toHex()).toStdString();
}

system_time_point NsLookup::fillNodesFromFile(const QString &file, const std::vector<NodeType> &expectedNodes) {
    QFile inputFile(file);
    if(!inputFile.open(QIODevice::ReadOnly)) {
        return intToSystemTimePoint(0);
    }
    QTextStream in(&inputFile);
    CHECK(!in.atEnd(), "Incorrect file " + file.toStdString());
    const std::string versionStr = in.readLine().toStdString();
    if (versionStr != CURRENT_VERSION) {
        return intToSystemTimePoint(0);
    }

    const std::string hashNodes = calcHashNodes(expectedNodes);
    const std::string hashNodesStr = in.readLine().toStdString();
    if (hashNodes != hashNodesStr) {
        return intToSystemTimePoint(0);
    }

    const std::string timePointStr = in.readLine().toStdString();
    const system_time_point timePoint = intToSystemTimePoint(std::stoull(timePointStr));
    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (!line.isNull() && !line.isEmpty()) {
            NodeInfo info;
            const int spacePos1 = line.indexOf(' ');
            CHECK(spacePos1 != -1, "Incorrect file " + file.toStdString());
            const QString type = line.mid(0, spacePos1);
            const int spacePos2 = line.indexOf(' ', spacePos1 + 1);
            CHECK(spacePos2 != -1, "Incorrect file " + file.toStdString());
            info.address = line.mid(spacePos1 + 1, spacePos2 - spacePos1 - 1);
            if (!info.address.startsWith("http")) {
                info.address = "http://" + info.address;
            }
            info.ping = std::stoull(line.mid(spacePos2 + 1).toStdString());

            addNode(type, info, false);
        }
    }

    sortAll();

    createSymlink(file);

    return timePoint;
}

void NsLookup::saveToFile(const QString &file, const system_time_point &tp, const std::vector<NodeType> &expectedNodes) {
    std::string content;
    content += CURRENT_VERSION + "\n";
    content += calcHashNodes(expectedNodes) + "\n";
    content += std::to_string(systemTimePointToInt(tp)) + "\n";
    for (const auto &element1: allNodesForTypes) {
        for (const NodeInfo &node: element1.second) {
            content += element1.first.toStdString() + " ";
            content += node.address.toStdString() + " ";
            content += std::to_string(node.ping) + "\n";
        }
    }

    writeToFile(file, content, false);

    createSymlink(file);
}

std::vector<QString> NsLookup::getRandomWithoutHttp(const QString &type, size_t limit, size_t count) const {
    return getRandom(type, limit, count, [](const NodeInfo &node) {return QUrl(node.address).host() + ":" + QString::fromStdString(std::to_string(QUrl(node.address).port()));});
}

std::vector<QString> NsLookup::getRandom(const QString &type, size_t limit, size_t count) const {
    return getRandom(type, limit, count, [](const NodeInfo &node) {return node.address;});
}

std::vector<QString> NsLookup::getRandom(const QString &type, size_t limit, size_t count, const std::function<QString(const NodeInfo &node)> &process) const {
    CHECK(count <= limit, "Incorrect count value");

    std::lock_guard<std::mutex> lock(nodeMutex);
    auto found = allNodesForTypes.find(type);
    if (found == allNodesForTypes.end()) {
        return {};
    }
    const std::vector<std::reference_wrapper<NodeInfo>> &nodes = found->second;

    std::vector<std::reference_wrapper<NodeInfo>> filterNodes;
    std::copy_if(nodes.begin(), nodes.end(), std::back_inserter(filterNodes), [](const NodeInfo &node) {
        return node.ping < MAX_PING.count();
    });
    return ::getRandom<QString>(filterNodes, limit, count, process);
}

void NsLookup::resetFile() {
     isResetFilledFile = true;
}
