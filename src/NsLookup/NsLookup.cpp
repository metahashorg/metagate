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
#include "utilites/utils.h"
#include "duration.h"
#include "Log.h"
#include "qt_utilites/SlotWrapper.h"
#include "Paths.h"
#include "qt_utilites/QRegister.h"
#include "qt_utilites/ManagerWrapperImpl.h"
#include "Network/SimpleClient.h"

#include "utilites/algorithms.h"

#include "NslWorker.h"
#include "Workers/FullWorker.h"
#include "Workers/SimpleWorker.h"
#include "Workers/RefreshIpWorker.h"
#include "Workers/RefreshNodeWorker.h"
#include "Workers/FindEmptyNodesWorker.h"
#include "Workers/PrintNodesWorker.h"
#include "Workers/MiddleWorker.h"
#include "InfrastructureNsLookup.h"

SET_LOG_NAMESPACE("NSL");

using namespace std::placeholders;

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

NsLookup::NsLookup(InfrastructureNsLookup &infrastructureNsl)
    : TimerClass(1s, nullptr)
    , infrastructureNsl(infrastructureNsl)
    , client(new SimpleClient(this))
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
    filledFileTp = lastFill;
    const system_time_point now = system_now();
    milliseconds passedTime = std::chrono::duration_cast<milliseconds>(now - lastFill);
    if (lastFill - now >= hours(1)) {
        // Защита от перевода времени назад
        passedTime = UPDATE_PERIOD;
    }

    if (passedTime >= UPDATE_PERIOD) {
        taskManager.addTask(FullWorker::makeTask(0s));
    } else {
        taskManager.addTask(SimpleWorker::makeTask(0s));
        taskManager.addTask(FullWorker::makeTask(std::chrono::duration_cast<seconds>(UPDATE_PERIOD - passedTime)));
    }

    taskManager.addTask(MiddleWorker::makeTask(3min));
    taskManager.addTask(FindEmptyNodesWorker::makeTask(0s));
    taskManager.addTask(PrintNodesWorker::makeTask(0s));

    Q_CONNECT(&udpClient, &UdpSocketClient::callbackCall, this, &NsLookup::callbackCall);

    Q_CONNECT(client, &SimpleClient::callbackCall, this, &NsLookup::callbackCall);

    udpClient.mvToThread(TimerClass::getThread());
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
        const auto bestResult = !pair.second.empty() ? pair.second[0].ping.count() : 0;
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

size_t NsLookup::countWorkedNodes(const NodeType::Node &node) const {
    return countWorkedNodes(allNodesForTypes.at(node));
}

size_t NsLookup::countWorkedNodes(const QString &nodeStr) const {
    return countWorkedNodes(NodeType::Node(nodeStr));
}

void NsLookup::findAndRefreshEmptyNodes() {
    for (const auto &pair: allNodesForTypes) {
        if (countWorkedNodes(pair.second) == 0) {
            taskManager.addTask(RefreshNodeWorker::makeTask(0s, pair.first.str()));
        }
    }
}

void NsLookup::process() {
    if (taskManager.isCurrentWork()) {
        return;
    }

    while (taskManager.isTaskReady()) {
        const Task task = taskManager.popTask();
        const std::shared_ptr<NslWorker> worker = makeWorker(taskManager, *this, task);
        if (worker->isActual()) {
            updateNumber++;
            taskManager.runWork(worker);
            break;
        }
    }
}

void NsLookup::saveAll(bool isFullFill) {
    sortAll();
    if (isFullFill) {
        filledFileTp = system_now();
    }
    saveToFile(savedNodesPath, filledFileTp, nodes);
}

void NsLookup::finalizeLookup(bool isFullFill) {
    emit serversFlushed(TypedException());

    saveAll(isFullFill);
}

void NsLookup::finalizeLookup(bool isFullFill, std::map<NodeType::Node, std::vector<NodeInfo>> &allNodesForTypesNew) {
    allNodesForTypes.swap(allNodesForTypesNew);

    emit serversFlushed(TypedException());

    saveAll(isFullFill);

    defectiveTorrents.clear();
}

void NsLookup::finalizeLookup(const NodeType::Node &node, const std::vector<NodeInfo> &allNodesForTypesNew) {
    allNodesForTypes[node] = allNodesForTypesNew;
    saveAll(false);

    defectiveTorrents.clear();
}

bool NsLookup::repeatResolveDns(
    const QString &dnsServerName,
    int dnsServerPort,
    const QByteArray &byteArray,
    std::map<QString, NodeType>::const_iterator node,
    time_point now,
    size_t countRepeat,
    std::vector<QString> &ipsTemp,
    const std::function<void()> &beginPing
) {
    if (countRepeat == 0) {
        return false;
    }
    udpClient.sendRequest(QHostAddress(dnsServerName), dnsServerPort, std::vector<char>(byteArray.begin(), byteArray.end()), [this, &ipsTemp, beginPing, node, now, dnsServerName, dnsServerPort, byteArray, countRepeat](const std::vector<char> &response, const UdpSocketClient::SocketException &exception) {
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
            const bool res = repeatResolveDns(dnsServerName, dnsServerPort, byteArray, node, now, countRepeat - 1, ipsTemp, beginPing);
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

        beginPing();
    }, timeoutRequestNodes);

    return true;
}

void NsLookup::beginResolve(std::map<NodeType::Node, std::vector<NodeInfo>> &allNodesForTypesNew, std::vector<QString> &ipsTemp, const std::function<void()> &finalizeLookup, const std::function<void(std::map<QString, NodeType>::const_iterator node)> &beginPing) {
    continueResolve(std::begin(nodes), allNodesForTypesNew, ipsTemp, finalizeLookup, beginPing);
}

void NsLookup::continueResolve(std::map<QString, NodeType>::const_iterator node, std::map<NodeType::Node, std::vector<NodeInfo>> &allNodesForTypesNew, std::vector<QString> &ipsTemp, const std::function<void()> &finalizeLookup, const std::function<void(std::map<QString, NodeType>::const_iterator node)> &beginPing) {
    if (node == nodes.end()) {
        finalizeLookup();
        return;
    }

    if (allNodesForTypesNew.find(node->second.node) != allNodesForTypesNew.end()) {
        continueResolve(std::next(node), allNodesForTypesNew, ipsTemp, finalizeLookup, beginPing);
        return;
    }

    const time_point now = ::now();
    if (now - cacheDns.lastUpdate >= 1h) {
        cacheDns.cache.clear();
    }
    ipsTemp = cacheDns.cache[node->second.node.str()];
    const auto bPing = std::bind(beginPing, node);
    if (ipsTemp.empty()) {
        DnsPacket requestPacket;
        requestPacket.addQuestion(DnsQuestion::getIp(node->second.node.str()));
        requestPacket.setFlags(DnsFlag::MyFlag);
        const auto byteArray = requestPacket.toByteArray();
        LOG << "Dns " << node->second.type << ".";
        repeatResolveDns(dnsServerName, dnsServerPort, byteArray, node, now, 3, ipsTemp, bPing);
    } else {
        bPing();
    }
}

static NodeInfo preParseNodeInfo(const QString &address, const SimpleClient::Response &response, size_t updateNumber) {
    NodeInfo info;
    info.address = address;
    info.ping = response.time;
    if (info.ping >= MAX_PING) {
        info.isTimeout = true;
    }
    info.countUpdated = updateNumber;

    if (response.exception.isTimeout()) {
        info.ping = MAX_PING;
        info.isTimeout = true;
    }

    return info;
}

static NodeResponse defaultResponseParser(const QByteArray &response, const std::string &error) {
    if (response.isEmpty() && error.empty()) {
        return NodeResponse(false);
    } else {
        return NodeResponse(true);
    }
}

void NsLookup::continuePing(std::vector<QString>::const_iterator ipsIter, const NodeType &node, std::map<NodeType::Node, std::vector<NodeInfo>> &allNodesForTypesNew, const std::vector<QString> &ipsTemp, const std::function<void()> &continueResolve) {
    if (ipsIter == ipsTemp.end()) {
        continueResolve();
        return;
    }

    const auto distance = std::distance(ipsIter, ipsTemp.cend());
    const auto countSteps = std::min(decltype(distance)(10), distance);

    CHECK(countSteps != 0, "Incorrect countSteps");
    std::vector<QString> currentIps(ipsIter, ipsIter + countSteps);

    emit infrastructureNsl.getRequestFornode(node.type, InfrastructureNsLookup::GetFormatRequestCallback([this, currentIps, &allNodesForTypesNew, &ipsTemp, continueResolve, node, ipsIter, countSteps](bool found, const QString &get, const QByteArray &post, const std::function<NodeResponse(const QByteArray &response, const std::string &error)> &processResponse){
        std::function<NodeResponse(const QByteArray &response, const std::string &error)> pResponse = found ? processResponse : defaultResponseParser;
        std::vector<QUrl> getRequests;
        getRequests.reserve(currentIps.size());
        std::transform(currentIps.begin(), currentIps.end(), std::back_inserter(getRequests), [&get](const QString &ip) {
            QUrl getRequest = ip;
            getRequest.setPath(get);
            return getRequest;
        });
        client->sendMessagesPost(node.node.str().toStdString(), getRequests, post, [this, &allNodesForTypesNew, &ipsTemp, continueResolve, node, currentIps, ipsIter, countSteps, pResponse](const std::vector<SimpleClient::Response> &results) {
            const TypedException exception = apiVrapper2([&]{
                CHECK(currentIps.size() == results.size(), "Incorrect results");
                for (size_t i = 0; i < results.size(); i++) {
                    const SimpleClient::Response &result = results[i];
                    const QString &ip = currentIps[i];
                    NodeInfo nodeInfo = preParseNodeInfo(ip, result, updateNumber);
                    const NodeResponse r = pResponse(result.response, result.exception.content);
                    if (!r.isSuccess) {
                        nodeInfo.isTimeout = true;
                        nodeInfo.ping = MAX_PING;
                    }
                    allNodesForTypesNew[node.node].emplace_back(nodeInfo);
                }
            });

            if (exception.isSet()) {
                LOG << "Exception"; // Ошибка логгируется внутри apiVrapper2;
            }
            continuePing(std::next(ipsIter, countSteps), node, allNodesForTypesNew, ipsTemp, continueResolve);
        }, 2s);
    }, [](const TypedException &exception) {
        LOG << "Error: " << exception.description;
    }, signalFunc));
}

void NsLookup::continuePingSafe(const NodeType &node, const std::vector<QString> &ipsTemp, const std::function<void()> &continueResolve) {
    if (allNodesForTypes[node.node].empty()) {
        continueResolve();
        return;
    }

    std::vector<size_t> processVectPos;
    for (size_t i = 0; i < allNodesForTypes[node.node].size(); i++) {
        NodeInfo &element = allNodesForTypes[node.node][i];
        if (element.ping == MAX_PING) {
            element.isTimeout = true;
        } else if (std::find(ipsTemp.begin(), ipsTemp.end(), element.address) == ipsTemp.end()) {
            element.ping = MAX_PING;
            element.isTimeout = true;
        } else {
            processVectPos.emplace_back(i);
        }
        if (processVectPos.size() >= ACCEPTABLE_COUNT_ADDRESSES + 2) {
            break;
        }
    }
    std::vector<QString> currentIps;
    for (const size_t posInIpsTemp: processVectPos) {
        const QString &address = allNodesForTypes[node.node][posInIpsTemp].address;
        currentIps.emplace_back(address);
    }
    if (processVectPos.empty()) {
        continueResolve();
        return;
    }
    emit infrastructureNsl.getRequestFornode(node.type, InfrastructureNsLookup::GetFormatRequestCallback([this, currentIps, continueResolve, node, processVectPos](bool found, const QString &get, const QByteArray &post, const std::function<NodeResponse(const QByteArray &response, const std::string &error)> &processResponse){
        std::function<NodeResponse(const QByteArray &response, const std::string &error)> pResponse = found ? processResponse : defaultResponseParser;
        std::vector<QUrl> getRequests;
        getRequests.reserve(currentIps.size());
        std::transform(currentIps.begin(), currentIps.end(), std::back_inserter(getRequests), [&get](const QString &ip) {
            QUrl getRequest = ip;
            getRequest.setPath(get);
            return getRequest;
        });
        client->sendMessagesPost(node.node.str().toStdString(), getRequests, post, [this, continueResolve, node, currentIps, processVectPos, pResponse](const std::vector<SimpleClient::Response> &results) {
            const TypedException exception = apiVrapper2([&]{
                CHECK(processVectPos.size() == results.size(), "Incorrect result");
                for (size_t i = 0; i < results.size(); i++) {
                    const auto &result = results[i];
                    const size_t index = processVectPos[i];
                    const QString address = currentIps[i];
                    NodeInfo &info = allNodesForTypes[node.node][index];
                    NodeInfo newInfo = preParseNodeInfo(address, result, updateNumber);
                    const NodeResponse nodeResponse = pResponse(result.response, result.exception.content);
                    if (!nodeResponse.isSuccess) {
                        newInfo.isTimeout = true;
                        newInfo.ping = MAX_PING;
                    }
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
            continueResolve();
        }, 2s);
    }, [](const TypedException &exception) {
        LOG << "Error: " << exception.description;
    }, signalFunc));
}

void NsLookup::finalizeRefreshIp(const NodeType::Node &node, const std::map<NodeType::Node, std::vector<NodeInfo>> &allNodesForTypesNew) {
    const std::vector<NodeInfo> &nds = allNodesForTypesNew.at(node);
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

    allNodesForTypes[node] = allNodesForTypesNew.at(node);
    saveAll(false);
}

std::map<QString, NodeType>::const_iterator NsLookup::getBeginNodesIterator() const {
    return std::cbegin(nodes);
}

void NsLookup::fillNodeStruct(const QString &nodeStr, NodeType &node, std::vector<QString> &ipsTemp) {
    for (const auto &pair: nodes) {
        if (pair.second.node.str() == nodeStr) {
            node = pair.second;
            std::transform(allNodesForTypes[node.node].begin(), allNodesForTypes[node.node].end(), std::back_inserter(ipsTemp), [](const NodeInfo &info) {
                return info.address;
            });
            return;
        }
    }
    throwErr("Not found node for str " + nodeStr.toStdString());
}

bool NsLookup::fillNodeStruct(std::map<QString, NodeType>::const_iterator &node, std::vector<QString> &ipsTemp) {
    if (node == nodes.end()) {
        return false;
    }

    NodeType tmp;
    fillNodeStruct(node->second.node.str(), tmp, ipsTemp);

    return true;
}

size_t NsLookup::findCountUpdatedIp(const QString &address) const {
    for (const auto &pair: allNodesForTypes) {
        for (const NodeInfo &info: pair.second) {
            if (getAddressWithoutHttp(address) == getAddressWithoutHttp(info.address)) {
                return info.countUpdated;
            }
        }
    }
    return 0;
}

void NsLookup::processRefreshIp(const QString &address, std::vector<QString> &ipsTemp, const std::function<void(const NodeType &node)> &beginPing) {
    for (const auto &pairNodes: allNodesForTypes) {
        if (std::find_if(pairNodes.second.cbegin(), pairNodes.second.cend(), [&address](const NodeInfo &info) {
            return getAddressWithoutHttp(info.address) == getAddressWithoutHttp(address);
        }) != pairNodes.second.cend()) {
            LOG << "Update status for ip: " << address << ". All: " << defectiveTorrents.size();

            for (const auto &t: pairNodes.second) {
                ipsTemp.emplace_back(t.address);
            }

            const auto foundNodeType = std::find_if(nodes.begin(), nodes.end(), [n=pairNodes.first](const auto &pair) {
                return pair.second.node.str() == n.str();
            });
            CHECK(foundNodeType != nodes.end(), "Not found node for type");
            beginPing(foundNodeType->second);
            break;
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
            long timeMs;
            ss >> type >> address >> timeMs >> isTimeout;
            info.ping = milliseconds(timeMs);
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

    allNodesForTypesBackup = allNodesForTypes;

    return timePoint;
}

void NsLookup::saveToFile(const QString &file, const system_time_point &tp, const std::map<QString, NodeType> &expectedNodes) {
    std::string content;
    content += CURRENT_VERSION + "\n";
    content += calcHashNodes(expectedNodes) + "\n";
    content += std::to_string(systemTimePointToInt(tp)) + "\n";
    for (const auto &nodeTypeIter: nodes) {
        const NodeType &nodeType = nodeTypeIter.second;
        const auto processOneNode = [](const NodeInfo &node, const NodeType &nodeType) {
            std::string content;
            content += nodeType.type.toStdString() + " ";
            content += node.address.toStdString() + " ";
            content += std::to_string(node.ping.count()) + " ";
            content += (node.isTimeout ? "1" : "0") + std::string(" ");
            content += "\n";
            return content;
        };
        if (countWorkedNodes(allNodesForTypes[nodeType.node]) != 0 || allNodesForTypesBackup[nodeType.node].empty()) {
            for (const NodeInfo &node: allNodesForTypes[nodeType.node]) {
                content += processOneNode(node, nodeType);
            }
            allNodesForTypesBackup[nodeType.node] = allNodesForTypes[nodeType.node];
        } else {
            for (const NodeInfo &node: allNodesForTypesBackup[nodeType.node]) {
                content += processOneNode(node, nodeType);
            }
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
    QString address;
    for (auto &defective: defectiveTorrents) {
        if (getAddressWithoutHttp(defective.first) == getAddressWithoutHttp(server)) {
            defective.second++;
            if (defective.second >= 3) {
                address = defective.first;
            }
            isFound = true;
            break;
        }
    }

    if (!isFound) {
        defectiveTorrents.emplace_back(server, 1);
    }

    if (!address.isEmpty()) {
        const size_t countUpdated = findCountUpdatedIp(address);
        taskManager.addTask(RefreshIpWorker::makeTask(0s, address, countUpdated));
    }
END_SLOT_WRAPPER
}
