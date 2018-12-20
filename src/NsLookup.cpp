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
        info.node = NodeType::Node(settings.value("node").toString());
        info.port = settings.value("port").toString();
        LOG << "node " << info.type << ". " << info.node.str() << ". " << info.port << ".";
        nodes[info.type] = info;
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
        allNodesForTypes.swap(allNodesForTypesNew);
    }
    sortAll();
    lock.unlock();
    if (!isSafeCheck) {
        saveToFile(savedNodesPath, system_now(), nodes);
    }

    const time_point stopScan = ::now();
    LOG << "Dns scan time " << std::chrono::duration_cast<seconds>(stopScan - startScanTime).count() << " seconds";

    milliseconds msTimer;
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
        } else {
            LOG << "Dns safe check not success. Start full scan";
            msTimer = 1ms;
        }
    } else {
        msTimer = UPDATE_PERIOD;
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

    allNodesForTypesNew.clear();

    LOG << "Dns scan start";
    continueResolve(nodes.begin());
END_SLOT_WRAPPER
}

std::vector<QString> NsLookup::requestDns(const NodeType &node) const {
    QUdpSocket udp;

    DnsPacket requestPacket;
    requestPacket.addQuestion(DnsQuestion::getIp(node.node.str()));
    requestPacket.setFlags(DnsFlag::MyFlag);
    udp.writeDatagram(requestPacket.toByteArray(), QHostAddress("8.8.8.8"), 53);

    udp.waitForReadyRead(milliseconds(2s).count());
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

void NsLookup::continueResolve(std::map<QString, NodeType>::const_iterator node) {
    if (isStopped.load()) {
        return;
    }
    if (node == nodes.end()) {
        finalizeLookup();
        return;
    }

    if (allNodesForTypesNew.find(node->second.node) != allNodesForTypesNew.end()) {
        continueResolve(std::next(node));
        return;
    }

    ipsTemp = requestDns(node->second);
    posInIpsTemp = 0;

    continuePing(node);
}

QString NsLookup::makeAddress(const QString &ip, const QString &port) {
    return "http://" + ip + ":" + port;
}

void NsLookup::continuePing(std::map<QString, NodeType>::const_iterator node) {
    if (isStopped.load()) {
        return;
    }

    if (!isSafeCheck) {
        if (posInIpsTemp >= ipsTemp.size()) {
            continueResolve(std::next(node));
            return;
        }

        const size_t countSteps = std::min(size_t(10), ipsTemp.size() - posInIpsTemp);

        std::shared_ptr<size_t> requestsInProcess = std::make_shared<size_t>(countSteps);

        for (size_t i = 0; i < countSteps; i++) {
            const QString &ip = ipsTemp[posInIpsTemp];
            posInIpsTemp++;
            client.ping(ip, [this, node, requestsInProcess](const QString &address, const milliseconds &time, const std::string &message) {
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

                    allNodesForTypesNew[node->second.node].emplace_back(info);
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
                    continuePing(node);
                }
            }, 2s);
        }
    } else {
        if (allNodesForTypes[node->second.node].empty()) {
            continueResolve(std::next(node));
            return;
        }

        std::vector<size_t> processVectPos;
        for (size_t i = 0; i < allNodesForTypes[node->second.node].size(); i++) {
            NodeInfo &element = allNodesForTypes[node->second.node][i];
            if (element.ping == MAX_PING.count()) {
                // skip
            } else if (std::find(ipsTemp.begin(), ipsTemp.end(), element.address) == ipsTemp.end()) {
                element.ping = MAX_PING.count();
            } else {
                processVectPos.emplace_back(i);
            }
            if (processVectPos.size() >= ACCEPTABLE_COUNT_ADDRESSES + 2) {
                break;
            }
        }
        std::shared_ptr<size_t> requestsInProcess = std::make_shared<size_t>(processVectPos.size());
        for (size_t i = 0; i < processVectPos.size(); i++) {
            const size_t posInIpsTempSave = processVectPos[i];
            const QString &address = allNodesForTypes[node->second.node][posInIpsTempSave].address;
            client.ping(address, [this, node, posInIpsTempSave, requestsInProcess](const QString &address, const milliseconds &time, const std::string &message) {
                try {
                    NodeInfo &info = allNodesForTypes[node->second.node][posInIpsTempSave];
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
                    continueResolve(std::next(node));
                }
            }, 2s);
        }
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

static std::string calcHashNodes(const std::map<QString, NodeType> &expectedNodes) {
    const std::string strNodes = std::accumulate(expectedNodes.begin(), expectedNodes.end(), std::string(), [](const std::string &a, const auto &b) {
        return a + b.second.type.toStdString() + b.second.node.str().toStdString() + b.second.port.toStdString();
    });
    return QString(QCryptographicHash::hash(QString::fromStdString(strNodes).toUtf8(), QCryptographicHash::Md5).toHex()).toStdString();
}

system_time_point NsLookup::fillNodesFromFile(const QString &file, const std::map<QString, NodeType> &expectedNodes) {
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

            allNodesForTypes[nodes[type].node].emplace_back(info);
        }
    }

    sortAll();

    createSymlink(file);

    return timePoint;
}

void NsLookup::saveToFile(const QString &file, const system_time_point &tp, const std::map<QString, NodeType> &expectedNodes) {
    std::string content;
    content += CURRENT_VERSION + "\n";
    content += calcHashNodes(expectedNodes) + "\n";
    content += std::to_string(systemTimePointToInt(tp)) + "\n";
    for (const auto &nodeTypeIter: nodes) {
        const NodeType &nodeType = nodeTypeIter.second;
        for (const NodeInfo &node: allNodesForTypes[nodeType.node]) {
            content += nodeType.type.toStdString() + " ";
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
    auto foundType = nodes.find(type);
    if (foundType == nodes.end()) {
        return {};
    }
    auto found = allNodesForTypes.find(foundType->second.node);
    if (found == allNodesForTypes.end()) {
        return {};
    }
    const std::vector<NodeInfo> &nodes = found->second;

    std::vector<NodeInfo> filterNodes;
    std::copy_if(nodes.begin(), nodes.end(), std::back_inserter(filterNodes), [](const NodeInfo &node) {
        return node.ping < MAX_PING.count();
    });
    return ::getRandom<QString>(filterNodes, limit, count, process);
}

void NsLookup::resetFile() {
     isResetFilledFile = true;
}
