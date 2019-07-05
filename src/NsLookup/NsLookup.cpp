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
#include "QRegister.h"
#include "ManagerWrapperImpl.h"

#include "algorithms.h"

#include "NslWorker.h"

SET_LOG_NAMESPACE("NSL");

using namespace nslookup;

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

static QString getAddressWithoutHttp(const QString &address) {
    if (!address.startsWith("http") && !address.startsWith("https")) {
        return address;
    }
    return QUrl(address).host() + ":" + QString::fromStdString(std::to_string(QUrl(address).port()));
}

NsLookup::NsLookup(QObject *parent)
    : TimerClass(1s, nullptr)
{
    Q_CONNECT(this, &NsLookup::getStatus, this, &NsLookup::onGetStatus);
    Q_CONNECT(this, &NsLookup::rejectServer, this, &NsLookup::onRejectServer);
    Q_CONNECT(this, &NsLookup::getRandomServersWithoutHttp, this, &NsLookup::onGetRandomServersWithoutHttp);
    Q_CONNECT(this, &NsLookup::getRandomServers, this, &NsLookup::onGetRandomServers);

    Q_REG(GetStatusCallback, "GetStatusCallback");
    Q_REG(GetServersCallback, "GetServersCallback");

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

    CHECK(settings.contains("ns_lookup/timeoutRequestNodesSeconds"), "settings ns_lookup/timeoutRequestNodesSeconds field not found");
    timeoutRequestNodes = seconds(settings.value("ns_lookup/timeoutRequestNodesSeconds").toInt());
    CHECK(settings.contains("ns_lookup/dns_server"), "settings ns_lookup/dns_server field not found");
    dnsServerName = settings.value("ns_lookup/dns_server").toString();
    CHECK(settings.contains("ns_lookup/dns_server_port"), "settings ns_lookup/dns_server_port field not found");
    dnsServerPort = settings.value("ns_lookup/dns_server_port").toInt();

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

    Q_CONNECT(&udpClient, &UdpSocketClient::callbackCall, this, &NsLookup::callbackCall);

    client.setParent(this);
    Q_CONNECT(&client, &SimpleClient::callbackCall, this, &NsLookup::callbackCall);

    udpClient.mvToThread(TimerClass::getThread());
    client.moveToThread(TimerClass::getThread());
    moveToThread(TimerClass::getThread());

    udpClient.startTm();
}

NsLookup::~NsLookup() {
    TimerClass::exit();

    if (isResetFilledFile.load()) {
        removeFile(savedNodesPath);
    }
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

std::vector<NodeTypeStatus> NsLookup::getNodesStatus() const {
    std::vector<NodeTypeStatus> result;

    for (const auto &pair: allNodesForTypes) {
        const size_t countSuccess = countWorkedNodes(pair.second);
        const auto bestResult = !pair.second.empty() ? pair.second[0].ping : 0;
        result.emplace_back(pair.first.str(), countSuccess, pair.second.size(), bestResult);
    }

    return result;
}

void NsLookup::printNodes() const {
    const std::vector<NodeTypeStatus> nodesStatuses = getNodesStatus();

    std::string result;
    for (const NodeTypeStatus &stat: nodesStatuses) {
        result += stat.node.toStdString() + ": " + std::to_string(stat.countWorked) + "/" + std::to_string(stat.countAll) + ", " + std::to_string(stat.bestResult) + "; ";
    }

    LOG << "Nodes result: " << result;
}

size_t NsLookup::countWorkedNodes(const std::vector<NodeInfo> &nodes) const {
    const size_t countSuccess = std::accumulate(nodes.begin(), nodes.end(), size_t(0), [](size_t prev, const NodeInfo &subElem) -> size_t {
        if (!subElem.isTimeout) {
            return prev + 1;
        } else {
            return prev + 0;
        }
    });
    return countSuccess;
}

void NsLookup::process() {
    if (taskManager.isCurrentWork()) {
        return;
    }

    if (taskManager.isTaskReady()) {
        const Task task = taskManager.popTask();
        const std::shared_ptr<NslWorker> worker = makeWorker(taskManager, *this, task);
        if (worker->isActual()) {
            updateNumber++;
            worker->runWork(worker);
        }
    }
}

void NsLookup::finalizeLookup() {
    isProcess = false;

    if (!isSafeCheck) {
        allNodesForTypes.swap(allNodesForTypesNew);
    }
    sortAll();
    if (!isSafeCheck) {
        saveToFile(savedNodesPath, system_now(), nodes);
    }

    const time_point stopScan = ::now();
    LOG << "Dns scan time " << std::chrono::duration_cast<seconds>(stopScan - startScanTime).count() << " seconds";

    bool isSuccessFl = false;
    if (isSafeCheck) {
        bool isSuccess = true;
        for (const auto &elem: allNodesForTypes) {
            const size_t countSuccess = countWorkedNodes(elem.second);
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
            isProcess = true;
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
            CHECK(!packet.answers().empty(), "Empty dns response " + toHex(std::string(response.begin(), response.end())));
            LOG << "Dns ok " << node->second.type << ". " << packet.answers().size();
        });

        if (except.isSet()) {
            LOG << "Dns repeat number " << countRepeat - 1;
            const bool res = repeatResolveDns(dnsServerName, dnsServerPort, byteArray, node, now, countRepeat - 1);
            if (!res) {
                dnsErrorDetails.dnsName = dnsServerName;
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

static NodeInfo parseNodeInfo(const QString &address, const milliseconds &time, const std::string &message, size_t updateNumber) {
    NodeInfo info;
    info.address = address;
    info.ping = time.count();
    info.countUpdated = updateNumber;

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
    if (!isSafeCheck) {
        if (ipsIter == ipsTemp.end()) {
            continueResolve(std::next(node));
            return;
        }

        const size_t countSteps = std::min(size_t(10), size_t(std::distance(ipsIter, ipsTemp.cend())));

        CHECK(countSteps != 0, "Incorrect countSteps");
        std::vector<QString> requests(ipsIter, ipsIter + countSteps);

        client.pings(node->second.node.str().toStdString(), requests, [this, node, requestsSize=requests.size(), ipsIter, countSteps](const std::vector<std::tuple<QString, milliseconds, std::string>> &results) {
            const TypedException exception = apiVrapper2([&]{
                CHECK(requestsSize == results.size(), "Incorrect results");
                for (const auto &result: results) {
                    allNodesForTypesNew[node->second.node].emplace_back(parseNodeInfo(std::get<0>(result), std::get<1>(result), std::get<2>(result), updateNumber));
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
                    NodeInfo newInfo = parseNodeInfo(std::get<0>(result), std::get<1>(result), std::get<2>(result), updateNumber);
                    CHECK(info.address == newInfo.address, "Incorrect address");
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

void NsLookup::finalizeRefresh(const NodeType::Node &node) {
    isProcessRefresh = false;

    const std::vector<NodeInfo> &nds = allNodesForTypesNew[node];
    for (auto iter = defectiveTorrents.begin(); iter != defectiveTorrents.end();) {
        if (std::find_if(nds.cbegin(), nds.cend(), [n=*iter](const NodeInfo &info) {
            return getAddressWithoutHttp(info.address) == getAddressWithoutHttp(n.first);
        }) != nds.cend()) {
            iter = defectiveTorrents.erase(iter);
        } else {
            iter++;
        }
    }

    LOG << "Updated ip status. Left " << defectiveTorrents.size();

    if (!isSafeCheck) {
        allNodesForTypes[node] = allNodesForTypesNew[node];
    }
    sortAll();
    saveToFile(savedNodesPath, system_now(), nodes);
}

void NsLookup::continuePingRefresh(std::vector<QString>::const_iterator ipsIter, const NodeType::Node &node) {
    if (ipsIter == ipsTempRefresh.end()) {
        finalizeRefresh(node);
        return;
    }

    const size_t countSteps = std::min(size_t(10), size_t(std::distance(ipsIter, ipsTempRefresh.cend())));

    CHECK(countSteps != 0, "Incorrect countSteps");
    const std::vector<QString> requests(ipsIter, ipsIter + countSteps);

    client.pings(node.str().toStdString(), requests, [this, node, requestsSize=requests.size(), ipsIter, countSteps](const std::vector<std::tuple<QString, milliseconds, std::string>> &results) {
        const TypedException exception = apiVrapper2([&]{
            CHECK(requestsSize == results.size(), "Incorrect results");
            for (const auto &result: results) {
                allNodesForTypesNew[node].emplace_back(parseNodeInfo(std::get<0>(result), std::get<1>(result), std::get<2>(result), updateNumber));
            }
        });

        if (exception.isSet()) {
            LOG << "Exception"; // Ошибка логгируется внутри apiVrapper2;
        }
        continuePingRefresh(std::next(ipsIter, countSteps), node);
    }, 2s);
}

void NsLookup::processRefresh() {
    if (isProcess || isProcessRefresh) {
        return;
    }

    QString address;
    for (const auto &pair: defectiveTorrents) {
        if (pair.second >= 3) {
            address = pair.first;
            break;
        }
    }

    const auto cntPing = [this](const NodeType::Node &type, const std::vector<NodeInfo> &nodes) {
        ipsTempRefresh.clear();
        for (const auto &t: nodes) {
            ipsTempRefresh.emplace_back(t.address);
        }

        isProcessRefresh = true;

        continuePingRefresh(std::begin(ipsTempRefresh), type);
    };

    if (!address.isEmpty()) {
        allNodesForTypesNew.clear();

        for (const auto &pairNodes: allNodesForTypes) {
            if (std::find_if(pairNodes.second.cbegin(), pairNodes.second.cend(), [&address](const NodeInfo &info) {
                return getAddressWithoutHttp(info.address) == getAddressWithoutHttp(address);
            }) != pairNodes.second.cend()) {
                LOG << "Update status for ip: " << address << ". All: " << defectiveTorrents.size();

                cntPing(pairNodes.first, pairNodes.second);
                break;
            }
        }
    } else {
        std::vector<std::map<NodeType::Node, std::vector<NodeInfo>>::const_iterator> pairs;
        for (auto pairNodes = allNodesForTypes.cbegin(); pairNodes != allNodesForTypes.cend(); pairNodes++) {
            const size_t countWorked = countWorkedNodes(pairNodes->second);
            if (countWorked == 0) {
                pairs.emplace_back(pairNodes);
            }
        }

        if (!pairs.empty()) {
            const size_t counter = randomCounter++;
            const auto &pairNodes = pairs[counter % pairs.size()];
            LOG << "Update status for type: " << pairNodes->first.str();

            cntPing(pairNodes->first, pairNodes->second);
        }
    }
}

void NsLookup::sortAll() {
    for (auto &element: allNodesForTypes) {
        std::sort(element.second.begin(), element.second.end(), [](const NodeInfo &first, const NodeInfo &second) {
            return first.address < second.address;
        });
        element.second.erase(std::unique(element.second.begin(), element.second.end(), [](const NodeInfo &first, const NodeInfo &second) {
            return first.address == second.address;
        }), element.second.end());

        std::sort(element.second.begin(), element.second.end(), std::less<NodeInfo>{});
    }
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

            allNodesForTypes[nodes[QString::fromStdString(type)].node].emplace_back(info);

            count++;
        }
    }

    LOG << "Filled nodes: " << count;

    sortAll();

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
}

std::vector<QString> NsLookup::getRandom(const QString &type, size_t limit, size_t count, const std::function<QString(const NodeInfo &node)> &process) const {
    CHECK(count <= limit, "Incorrect count value");

    const auto foundType = nodes.find(type);
    if (foundType == nodes.end()) {
        return {};
    }
    const auto found = allNodesForTypes.find(foundType->second.node);
    if (found == allNodesForTypes.end()) {
        return {};
    }
    std::vector<NodeInfo> nodes = found->second;

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

void NsLookup::onGetStatus(const GetStatusCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this]{
        return std::make_tuple(getNodesStatus(), dnsErrorDetails);
    }, callback);
END_SLOT_WRAPPER
}

void NsLookup::onGetRandomServersWithoutHttp(const QString &type, size_t limit, size_t count, const GetServersCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this]{
        return getRandom(type, limit, count, [](const NodeInfo &node) {return getAddressWithoutHttp(node.address);});
    }, callback);
END_SLOT_WRAPPER
}

void NsLookup::onGetRandomServers(const QString &type, size_t limit, size_t count, const GetServersCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this]{
        return getRandom(type, limit, count, [](const NodeInfo &node) {return node.address;});
    }, callback);
END_SLOT_WRAPPER
}

void NsLookup::onRejectServer(const QString &server) {
BEGIN_SLOT_WRAPPER
    bool isFound = false;
    for (auto &defective: defectiveTorrents) {
        if (getAddressWithoutHttp(defective.first) == getAddressWithoutHttp(server)) {
            defective.second++;
            isFound = true;
            break;
        }
    }

    if (!isFound) {
        defectiveTorrents.emplace_back(server, 1);
    }
END_SLOT_WRAPPER
}
