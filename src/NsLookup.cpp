#include "NsLookup.h"

#include <QApplication>
#include <QCryptographicHash>

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

SET_LOG_NAMESPACE("NSL");

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
    : TimerClass(1s, nullptr)
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
        nodes[info.type] = info;
    }
    LOG << "nodes types " << nodes.size();
    settings.endArray();

    CHECK(settings.contains("ns_lookup/countSuccessTests"), "settings ns_lookup/countSuccessTests field not found");
    countSuccessTestsForP2PNodes = settings.value("ns_lookup/countSuccessTests").toInt();
    CHECK(settings.contains("ns_lookup/timeoutRequestNodesSeconds"), "settings ns_lookup/timeoutRequestNodesSeconds field not found");
    timeoutRequestNodes = seconds(settings.value("ns_lookup/timeoutRequestNodesSeconds").toInt());
    CHECK(settings.contains("ns_lookup/dns_server"), "settings ns_lookup/dns_server field not found");
    dnsServerName = settings.value("ns_lookup/dns_server").toString();
    CHECK(settings.contains("ns_lookup/dns_server_port"), "settings ns_lookup/dns_server_port field not found");
    dnsServerPort = settings.value("ns_lookup/dns_server_port").toInt();
    CHECK(settings.contains("ns_lookup/use_users_servers"), "settings ns_lookup/use_users_servers field not found");
    useUsersServers = settings.value("ns_lookup/use_users_servers").toBool();

    savedNodesPath = makePath(getNsLookupPath(), FILL_NODES_PATH);
    const system_time_point lastFill = fillNodesFromFile(savedNodesPath, nodes);
    const system_time_point now = system_now();
    passedTime = std::chrono::duration_cast<milliseconds>(now - lastFill);
    if (lastFill - now >= hours(1)) {
        // Защита от перевода времени назад
        passedTime = UPDATE_PERIOD;
    }

    if (passedTime >= UPDATE_PERIOD) {
        isSafeCheck = false;
    } else {
        isSafeCheck = true;
    }
    LOG << "Start ns resolve after " << 1 << " ms " << isSafeCheck;

    CHECK(connect(&udpClient, &UdpSocketClient::callbackCall, this, &NsLookup::callbackCall), "not connect callbackCall");

    client.setParent(this);
    CHECK(connect(&client, &SimpleClient::callbackCall, this, &NsLookup::callbackCall), "not connect callbackCall");

    udpClient.mvToThread(TimerClass::getThread());
    client.moveToThread(TimerClass::getThread());
    moveToThread(TimerClass::getThread());

    udpClient.startTm();
}

NsLookup::~NsLookup() {
    isStopped = true;
    TimerClass::exit();

    if (isResetFilledFile.load()) {
        removeFile(savedNodesPath);
    }
}

void NsLookup::callbackCall(SimpleClient::ReturnCallback callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void NsLookup::startMethod() {
    process();
}

void NsLookup::timerMethod() {
    process();
}

void NsLookup::finishMethod() {
    // empty
}

void NsLookup::process() {
BEGIN_SLOT_WRAPPER
    if (now() - prevCheckTime >= msTimer) {
        msTimer = 60s; // На случай, если что-то пойдет не так, повторная проверка запустится через это время
        startScanTime = ::now();

        allNodesForTypesNew.clear();

        LOG << "Dns scan start";
        continueResolve(nodes.begin());

        prevCheckTime = now();
    }
END_SLOT_WRAPPER
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

        if (useUsersServers) {
            startScanTime = ::now();
            continueResolveP2P(std::begin(nodes));
        }
    }
    isSafeCheck = false;
}

bool NsLookup::repeatResolveDns(
    const QString &dnsServerName,
    int dnsServerPort,
    const QByteArray &byteArray,
    std::map<QString, NodeType>::const_iterator node,
    time_point now,
    size_t countRepeat
) {
    if (countRepeat == 0) {
        return false;
    }
    udpClient.sendRequest(QHostAddress(dnsServerName), dnsServerPort, std::vector<char>(byteArray.begin(), byteArray.end()), [this, node, now, dnsServerName, dnsServerPort, byteArray, countRepeat](const std::vector<char> &response, const UdpSocketClient::SocketException &exception) {
        DnsPacket packet;
        const TypedException except = apiVrapper2([&](){
            CHECK(!exception.isSet(), "Dns exception: " + exception.toString());
            CHECK(response.size() > 0, "Incorrect response dns " + node->second.type.toStdString());
            packet = DnsPacket::fromBytesArary(QByteArray(response.data(), response.size()));

            LOG << "Dns ok " << node->second.type << ". " << packet.answers().size();
            CHECK(!packet.answers().empty(), "Empty dns response " + toHex(std::string(response.begin(), response.end())));
        });

        if (except.isSet()) {
            LOG << "Dns repeat number " << countRepeat - 1;
            const bool res = repeatResolveDns(dnsServerName, dnsServerPort, byteArray, node, now, countRepeat - 1);
            if (!res) {
                throw except;
            } else {
                return;
            }
        }

        ipsTemp.clear();
        for (const auto &record : packet.answers()) {
            ipsTemp.emplace_back(::makeAddress(record.toString(), node->second.port));
        }

        cacheDns.cache[node->second.node.str()] = ipsTemp;
        cacheDns.lastUpdate = now;

        continuePing(std::begin(ipsTemp), node);
    }, timeoutRequestNodes);

    return true;
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
        DnsPacket requestPacket;
        requestPacket.addQuestion(DnsQuestion::getIp(node->second.node.str()));
        requestPacket.setFlags(DnsFlag::MyFlag);
        const auto byteArray = requestPacket.toByteArray();
        LOG << "Dns " << node->second.type << ".";
        repeatResolveDns(dnsServerName, dnsServerPort, byteArray, node, now, 3);
    } else {
        continuePing(std::begin(ipsTemp), node);
    }
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

void NsLookup::continuePing(std::vector<QString>::const_iterator ipsIter, std::map<QString, NodeType>::const_iterator node) {
    if (isStopped.load()) {
        return;
    }

    if (!isSafeCheck) {
        if (ipsIter == ipsTemp.end()) {
            continueResolve(std::next(node));
            return;
        }

        const size_t countSteps = std::min(size_t(10), size_t(std::distance(ipsIter, ipsTemp.cend())));

        CHECK(countSteps != 0, "Incorrect countSteps");
        std::vector<QString> requests;
        std::transform(ipsIter, ipsIter + countSteps, std::back_inserter(requests), [](const auto &r) {
            return r;
        });

        client.pings(node->second.node.str().toStdString(), requests, [this, node, requestsSize=requests.size(), ipsIter, countSteps](const std::vector<std::tuple<QString, milliseconds, std::string>> &results) {
            const TypedException exception = apiVrapper2([&]{
                CHECK(requestsSize == results.size(), "Incorrect results");
                for (const auto &result: results) {
                    allNodesForTypesNew[node->second.node].emplace_back(parseNodeInfo(std::get<0>(result), std::get<1>(result), std::get<2>(result)));
                }
            });

            if (exception.isSet()) {
                LOG << "Exception"; // Ошибка логгируется внутри apiVrapper2;
            }
            continuePing(std::next(ipsIter, countSteps), node);
        }, 2s);
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
        std::vector<QString> requests;
        for (const size_t posInIpsTemp: processVectPos) {
            const QString &address = allNodesForTypes[node->second.node][posInIpsTemp].address;
            requests.emplace_back(address);
        }
        if (processVectPos.empty()) {
            continueResolve(std::next(node));
        }
        client.pings(node->second.node.str().toStdString(), requests, [this, node, processVectPos](const std::vector<std::tuple<QString, milliseconds, std::string>> &results) {
            const TypedException exception = apiVrapper2([&]{
                CHECK(processVectPos.size() == results.size(), "Incorrect result");
                for (size_t i = 0; i < results.size(); i++) {
                    const auto &result = results[i];
                    const size_t index = processVectPos[i];
                    NodeInfo &info = allNodesForTypes[node->second.node][index];
                    NodeInfo newInfo = parseNodeInfo(std::get<0>(result), std::get<1>(result), std::get<2>(result));
                    CHECK(info.address == newInfo.address, "Incorrect address");
                    newInfo.isChecked = true;
                    if (!newInfo.isTimeout) {
                        newInfo.ping = info.ping;
                    }

                    info = newInfo;
                }
            });

            if (exception.isSet()) {
                LOG << "Exception"; // Ошибка логгируется внутри apiVrapper2;
            }
            continueResolve(std::next(node));
        }, 2s);
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
    CHECK(root.contains("result") && root.value("result").isObject(), "p2p result field not found");
    const QJsonObject data = root.value("result").toObject();
    CHECK(data.contains("nodes") && data.value("nodes").isArray(), "p2p nodes field not found");
    const QJsonArray nodes = data.value("nodes").toArray();

    std::vector<P2PNodeResult> result;
    for (const QJsonValue &node1: nodes) {
        CHECK(node1.isObject(), "p2p node field not found");
        const QJsonObject node = node1.toObject();

        P2PNodeResult res;
        CHECK(node.contains("node") && node.value("node").isString(), "p2p node field not found");
        res.node = node.value("node").toString();
        CHECK(node.contains("count") && node.value("count").isDouble(), "p2p count field not found");
        res.count = node.value("count").toInt();
        CHECK(node.contains("ip") && node.value("ip").isString(), "p2p ip field not found");
        res.ip = makeAddress(node.value("ip").toString());
        CHECK(node.contains("type") && node.value("type").isString(), "p2p type field not found");
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
            LOG << "P2P get nodes exception: " << exception.toString();
            continueResolveP2P(std::next(node));
            return;
        }
        const std::vector<P2PNodeResult> nodes = parseNodesP2P(response);

        ipsTempP2P.clear();
        std::transform(nodes.begin(), nodes.end(), std::back_inserter(ipsTempP2P), [](const P2PNodeResult &node) {
            return std::make_pair(node.type, node.ip);
        });

        continuePingP2P(std::begin(ipsTempP2P), node, nodeNetworkTorrent, nodeNetworkProxy);
    }, timeoutRequestNodes);
}

void NsLookup::continuePingP2P(std::vector<std::pair<NodeType::SubType, QString>>::const_iterator ipsIter, std::map<QString, NodeType>::const_iterator node, const NodeType &nodeTorrent, const NodeType &nodeProxy) {
    if (isStopped.load()) {
        return;
    }

    if (ipsIter == ipsTempP2P.end()) {
        continueResolveP2P(std::next(node));
        return;
    }

    const size_t countSteps = std::min(size_t(10), size_t(std::distance(ipsIter, ipsTempP2P.cend())));

    CHECK(countSteps != 0, "Incorrect count steps");
    std::vector<QString> requests;
    std::transform(ipsIter, ipsIter + countSteps, std::back_inserter(requests), [](const auto &pair) {
        return pair.second;
    });
    std::vector<NodeType::SubType> types;
    std::transform(ipsIter, ipsIter + countSteps, std::back_inserter(types), [](const auto &pair) {
        return pair.first;
    });

    client.pings(node->second.node.str().toStdString(), requests, [this, types, ipsIter, countSteps, node, nodeTorrent, nodeProxy](const std::vector<std::tuple<QString, milliseconds, std::string>> &results) {
        const TypedException exception = apiVrapper2([&]{
            CHECK(types.size() == results.size(), "Incorrect results");
            for (size_t i = 0; i < results.size(); i++) {
                const auto &result = results[i];
                const NodeType::SubType &type = types[i];
                const NodeInfo info = parseNodeInfo(std::get<0>(result), std::get<1>(result), std::get<2>(result));

                std::unique_lock<std::mutex> lock(nodeMutex);
                if (type == NodeType::SubType::torrent) {
                    allNodesForTypesP2P[nodeTorrent.node].emplace_back(info); // TODO добавлять сразу к сортированному массиву
                } else if (type == NodeType::SubType::proxy) {
                    allNodesForTypesP2P[nodeProxy.node].emplace_back(info);
                }
            }
        });

        if (exception.isSet()) {
            LOG << "Exception"; // Ошибка логгируется внутри apiVrapper2;
        }
        continuePingP2P(std::next(ipsIter, countSteps), node, nodeTorrent, nodeProxy);
    }, 2s);
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
    size_t count = 0;
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

            count++;
        }
    }

    LOG << "Filled nodes: " << count;

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

    if (useUsersServers) {
        const auto foundP2P = allNodesForTypesP2P.find(foundType->second.node);
        if (foundP2P != allNodesForTypesP2P.end()) {
            nodes.insert(nodes.end(), foundP2P->second.begin(), foundP2P->second.end());
        }
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
