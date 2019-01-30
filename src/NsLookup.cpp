#include "NsLookup.h"

#include <set>
#include <sstream>

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

const static std::string CURRENT_VERSION = "v3";

const static milliseconds MAX_PING = 100s;

const static milliseconds UPDATE_PERIOD = days(1);

const static size_t ACCEPTABLE_COUNT_ADDRESSES = 3;

static QString makeAddress(const QString &ipAndPort) {
    return "http://" + ipAndPort;
}

static QString makeAddress(const QString &ip, const QString &port) {
    return makeAddress(ip + ":" + port);
}

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
        if (settings.contains("network")) {
            info.network = settings.value("network").toString();
        }
        if (settings.contains("subtype")) {
            const QString subtype = settings.value("subtype").toString();
            if (subtype == "torrent") {
                info.subtype = NodeType::SubType::torrent;
            } else if (subtype == "proxy") {
                info.subtype = NodeType::SubType::proxy;
            } else {
                throwErr("Incorrect subtype: " + subtype.toStdString());
            }
        }
        LOG << "node " << info.type << ". " << info.node.str() << ". " << info.port << ". " << info.network;
        nodes[info.type] = info;
    }
    settings.endArray();

    CHECK(settings.contains("ns_lookup/countSuccessTests"), "ns_lookup/countSuccessTests field not found");
    countSuccessTestsForP2PNodes = settings.value("ns_lookup/countSuccessTests").toInt();
    CHECK(settings.contains("ns_lookup/timeoutRequestNodesSeconds"), "ns_lookup/timeoutRequestNodesSeconds field not found");
    timeoutRequestNodes = seconds(settings.value("ns_lookup/timeoutRequestNodesSeconds").toInt());

    savedNodesPath = makePath(getNsLookupPath(), FILL_NODES_PATH);
    const system_time_point lastFill = fillNodesFromFile(savedNodesPath, nodes);
    const system_time_point now = system_now();
    passedTime = std::chrono::duration_cast<milliseconds>(now - lastFill);
    if (lastFill - now >= hours(1)) {
        // Защита от перевода времени назад
        passedTime = UPDATE_PERIOD;
    }

    CHECK(connect(&thread1, &QThread::started, this, &NsLookup::run), "not connect started");
    CHECK(connect(this, &NsLookup::finished, &thread1, &QThread::terminate), "not connect finished");

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
    CHECK(connect(&qtimer, &QTimer::timeout, this, &NsLookup::uploadEvent), "not connect uploadEvent");
    CHECK(connect(&thread1, &QThread::started, &qtimer, QOverload<>::of(&QTimer::start)), "not connect start");
    CHECK(connect(&thread1, &QThread::finished, &qtimer, &QTimer::stop), "not connect stop");

    client.setParent(this);
    CHECK(connect(&client, &SimpleClient::callbackCall, this, &NsLookup::callbackCall), "not connect callbackCall");

    client.moveToThread(&thread1);
    moveToThread(&thread1);
}

NsLookup::~NsLookup() {
    isStopped = true;
    thread1.quit();
    if (!thread1.wait(8000)) {
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

    udp.waitForReadyRead(); // Не ставить timeout, так как это вызывает странное поведение
    if (isStopped) {
        return {};
    }
    std::vector<char> data(512 * 1000, 0);
    const qint64 size = udp.readDatagram(data.data(), data.size());
    CHECK(size > 0, "Incorrect response dns " + node.type.toStdString());
    const DnsPacket packet = DnsPacket::fromBytesArary(QByteArray(data.data(), size));

    LOG << "dns ok " << node.type << ". " << packet.answers().size();
    CHECK(!packet.answers().empty(), "Empty dns response " + toHex(std::string(data.begin(), data.begin() + size)));

    std::vector<QString> result;
    for (const auto &record : packet.answers()) {
        result.emplace_back(::makeAddress(record.toString(), node.port));
    }
    return result;
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
    bool isSuccessFl = false;
    if (isSafeCheck) {
        bool isSuccess = true;
        for (const auto &elem: allNodesForTypes) {
            const size_t countSuccess = std::accumulate(elem.second.begin(), elem.second.end(), size_t(0), [](size_t prev, const NodeInfo &subElem) -> size_t {
                if (subElem.isChecked && !subElem.isTimeout) {
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

        startScanTime = ::now();
        continueResolveP2P(std::begin(nodes));
    }
    isSafeCheck = false;
    qtimer.setInterval(msTimer.count());
    qtimer.setSingleShot(true);
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

    const time_point now = ::now();
    if (now - cacheDns.lastUpdate >= 1h) {
        cacheDns.cache.clear();
    }
    ipsTemp = cacheDns.cache[node->second.node.str()];
    if (ipsTemp.empty()) {
        ipsTemp = requestDns(node->second);
        cacheDns.cache[node->second.node.str()] = ipsTemp;
        cacheDns.lastUpdate = now;
    }
    posInIpsTemp = 0;

    continuePing(node);
}

static NodeInfo parseNodeInfo(const QString &address, const milliseconds &time, const std::string &message) {
    NodeInfo info;
    info.address = address;
    info.ping = time.count();
    info.isChecked = true;

    if (message.empty()) {
        info.ping = MAX_PING.count();
        info.isTimeout = true;
    } else {
        QJsonParseError parseError;
        QJsonDocument::fromJson(QString::fromStdString(message).toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            info.ping = MAX_PING.count();
            info.isTimeout = true;
        }
    }

    return info;
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

        if (countSteps == 0) {
            continuePing(node);
        }
        for (size_t i = 0; i < countSteps; i++) {
            const QString &ip = ipsTemp[posInIpsTemp];
            posInIpsTemp++;
            client.ping(ip, [this, node, requestsInProcess](const QString &address, const milliseconds &time, const std::string &message) {
                try {
                    allNodesForTypesNew[node->second.node].emplace_back(parseNodeInfo(address, time, message));
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
                element.isTimeout = true;
            } else if (std::find(ipsTemp.begin(), ipsTemp.end(), element.address) == ipsTemp.end()) {
                element.ping = MAX_PING.count();
                element.isTimeout = true;
            } else {
                processVectPos.emplace_back(i);
            }
            if (processVectPos.size() >= ACCEPTABLE_COUNT_ADDRESSES + 2) {
                break;
            }
        }
        std::shared_ptr<size_t> requestsInProcess = std::make_shared<size_t>(processVectPos.size());
        if (processVectPos.empty()) {
            continueResolve(std::next(node));
        }
        for (size_t i = 0; i < processVectPos.size(); i++) {
            const size_t posInIpsTempSave = processVectPos[i];
            const QString &address = allNodesForTypes[node->second.node][posInIpsTempSave].address;
            client.ping(address, [this, node, posInIpsTempSave, requestsInProcess](const QString &address, const milliseconds &time, const std::string &message) {
                try {
                    NodeInfo &info = allNodesForTypes[node->second.node][posInIpsTempSave];
                    NodeInfo newInfo = parseNodeInfo(address, time, message);
                    CHECK(info.address == newInfo.address, "Incorrect address");
                    newInfo.isChecked = true;
                    if (!newInfo.isTimeout) {
                        newInfo.ping = info.ping;
                    }

                    info = newInfo;
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

void NsLookup::finalizeLookupP2P() {
    size_t countAll = 0;
    std::unique_lock<std::mutex> lock(nodeMutex);
    for (auto &element: allNodesForTypesP2P) {
        std::sort(element.second.begin(), element.second.end(), std::less<NodeInfo>{});
        countAll += element.second.size();
    }
    lock.unlock();

    const time_point stopScan = ::now();
    LOG << "Dns scan p2p time " << std::chrono::duration_cast<seconds>(stopScan - startScanTime).count() << " seconds. Count new nodes " << countAll;
}

struct P2PNodeResult {
    QString node;
    NodeType::SubType type = NodeType::SubType::none;
    int count;
    QString ip;
};

static std::vector<P2PNodeResult> parseNodesP2P(const std::string &resp) {
    const QJsonDocument response = QJsonDocument::fromJson(QString::fromStdString(resp).toUtf8());
    const QJsonObject root = response.object();
    CHECK(root.contains("result") && root.value("result").isObject(), "result field not found");
    const QJsonObject data = root.value("result").toObject();
    CHECK(data.contains("nodes") && data.value("nodes").isArray(), "nodes field not found");
    const QJsonArray nodes = data.value("nodes").toArray();

    std::vector<P2PNodeResult> result;
    for (const QJsonValue &node1: nodes) {
        CHECK(node1.isObject(), "node field not found");
        const QJsonObject node = node1.toObject();

        P2PNodeResult res;
        CHECK(node.contains("node") && node.value("node").isString(), "node field not found");
        res.node = node.value("node").toString();
        CHECK(node.contains("count") && node.value("count").isDouble(), "count field not found");
        res.count = node.value("count").toInt();
        CHECK(node.contains("ip") && node.value("ip").isString(), "ip field not found");
        res.ip = makeAddress(node.value("ip").toString());
        CHECK(node.contains("type") && node.value("type").isString(), "type field not found");
        const QString type = node.value("type").toString();
        if (type == "proxy") {
            res.type = NodeType::SubType::proxy;
        } else if (type == "torrent") {
            res.type = NodeType::SubType::torrent;
        }
        result.emplace_back(res);
    }
    return result;
};

void NsLookup::continueResolveP2P(std::map<QString, NodeType>::const_iterator node) {
    if (isStopped.load()) {
        return;
    }
    if (node == nodes.end()) {
        finalizeLookupP2P();
        return;
    }

    if (node->second.subtype != NodeType::SubType::torrent) {
        continueResolveP2P(std::next(node));
        return;
    }

    const QString network = node->second.network;
    NodeType nodeNetworkTorrent;
    NodeType nodeNetworkProxy;
    for (const auto &pair: nodes) {
        if (pair.second.network == network) {
            if (pair.second.subtype == NodeType::SubType::proxy) {
                nodeNetworkProxy = pair.second;
            } else if (pair.second.subtype == NodeType::SubType::torrent) {
                nodeNetworkTorrent = pair.second;
            }
        }
    }

    if (
        (nodeNetworkTorrent.subtype != NodeType::SubType::none && allNodesForTypesP2P.find(nodeNetworkTorrent.node) != allNodesForTypesP2P.end()) ||
        (nodeNetworkProxy.subtype != NodeType::SubType::none && allNodesForTypesP2P.find(nodeNetworkProxy.node) != allNodesForTypesP2P.end())
    ) {
        // Эту структуру мы уже заполняли, пропускаем
        continueResolveP2P(std::next(node));
        return;
    }

    if (allNodesForTypes[node->second.node].empty()) {
        continueResolveP2P(std::next(node));
        return;
    }
    const QString postRequest = QString::fromStdString("{\"id\":1,\"params\":{\"count_tests\": " + std::to_string(countSuccessTestsForP2PNodes) + "},\"method\":\"get-all-last-nodes-count\", \"pretty\": false}");
    client.sendMessagePost(allNodesForTypes[node->second.node][0].address, postRequest, [this, node, nodeNetworkProxy, nodeNetworkTorrent](const std::string &response, const SimpleClient::ServerException &exception){
        if (exception.isSet()) {
            LOG << "P2P get nodes exception: " << exception.description << " " << exception.content;
            continueResolveP2P(std::next(node));
            return;
        }
        const std::vector<P2PNodeResult> nodes = parseNodesP2P(response);

        ipsTempP2P.clear();
        std::transform(nodes.begin(), nodes.end(), std::back_inserter(ipsTempP2P), [](const P2PNodeResult &node) {
            return std::make_pair(node.type, node.ip);
        });
        posInIpsTemp = 0;

        continuePingP2P(node, nodeNetworkTorrent, nodeNetworkProxy);
    }, timeoutRequestNodes);
}

void NsLookup::continuePingP2P(std::map<QString, NodeType>::const_iterator node, const NodeType &nodeTorrent, const NodeType &nodeProxy) {
    if (isStopped.load()) {
        return;
    }

    if (posInIpsTemp >= ipsTempP2P.size()) {
        continueResolveP2P(std::next(node));
        return;
    }

    const size_t countSteps = std::min(size_t(10), ipsTempP2P.size() - posInIpsTemp);

    std::shared_ptr<size_t> requestsInProcess = std::make_shared<size_t>(countSteps);

    if (countSteps == 0) {
        continuePing(node);
    }
    for (size_t i = 0; i < countSteps; i++) {
        const auto &ipPair = ipsTempP2P[posInIpsTemp];
        posInIpsTemp++;
        client.ping(ipPair.second, [this, type=ipPair.first, node, nodeTorrent, nodeProxy, requestsInProcess](const QString &address, const milliseconds &time, const std::string &message) {
            try {
                const NodeInfo info = parseNodeInfo(address, time, message);

                std::unique_lock<std::mutex> lock(nodeMutex);
                if (type == NodeType::SubType::torrent) {
                    allNodesForTypesP2P[nodeTorrent.node].emplace_back(info); // TODO добавлять сразу к сортированному массиву
                } else if (type == NodeType::SubType::proxy) {
                    allNodesForTypesP2P[nodeProxy.node].emplace_back(info);
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
                continuePingP2P(node, nodeTorrent, nodeProxy);
            }
        }, 2s);
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
            std::istringstream ss(line.toStdString());
            NodeInfo info;
            std::string type;
            std::string address;
            int isTimeout;
            ss >> type >> address >> info.ping >> isTimeout;
            CHECK(!ss.fail(), "Incorrect file " + file.toStdString());

            info.address = QString::fromStdString(address);
            if (!info.address.startsWith("http")) {
                info.address = "http://" + info.address;
            }
            info.isTimeout = isTimeout == 1;
            info.isChecked = false;

            allNodesForTypes[nodes[QString::fromStdString(type)].node].emplace_back(info);
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
            content += std::to_string(node.ping) + " ";
            content += (node.isTimeout ? "1" : "0") + std::string(" ");
            content += "\n";
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

    std::unique_lock<std::mutex> lock(nodeMutex);
    const auto foundType = nodes.find(type);
    if (foundType == nodes.end()) {
        return {};
    }
    const auto found = allNodesForTypes.find(foundType->second.node);
    if (found == allNodesForTypes.end()) {
        return {};
    }
    std::vector<NodeInfo> nodes = found->second;

    const auto foundP2P = allNodesForTypesP2P.find(foundType->second.node);
    if (foundP2P != allNodesForTypesP2P.end()) {
        nodes.insert(nodes.end(), foundP2P->second.begin(), foundP2P->second.end());
    }
    lock.unlock();

    std::sort(nodes.begin(), nodes.end(), std::less<NodeInfo>{});

    std::vector<NodeInfo> filterNodes;
    std::copy_if(nodes.begin(), nodes.end(), std::back_inserter(filterNodes), [](const NodeInfo &node) {
        return !node.isTimeout;
    });
    return ::getRandom<QString>(filterNodes, limit, count, process);
}

void NsLookup::resetFile() {
     isResetFilledFile = true;
}
