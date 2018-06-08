#include "NsLookup.h"

#include <QHostAddress>
#include <QDir>
#include <QCoreApplication>
#include <QUdpSocket>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include "dns/dnspacket.h"
#include "check.h"
#include "utils.h"
#include "duration.h"
#include "Log.h"
#include "SlotWrapper.h"

#include "algorithms.h"

const static QString NODES_FILE = "nodes.txt";

const static QString FILL_NODES_PATH = "fill_nodes.txt";

const milliseconds UPDATE_PERIOD = days(1);

NsLookup::NsLookup(const QString &pagesPath, QObject *parent)
    : QObject(parent)
    , pagesPath(pagesPath)
{
    const QString nodesFile = QDir(pagesPath).filePath(NODES_FILE);
    QFile inputFile(nodesFile);
    CHECK(inputFile.open(QIODevice::ReadOnly), "Not open file " + nodesFile.toStdString());
    QTextStream in(&inputFile);
    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (!line.isNull() && !line.isEmpty()) {
            NodeType info;
            const int spacePos1 = line.indexOf(' ');
            CHECK(spacePos1 != -1, "Incorrect file " + nodesFile.toStdString());
            info.type = line.mid(0, spacePos1);
            const int spacePos2 = line.indexOf(' ', spacePos1 + 1);
            CHECK(spacePos2 != -1, "Incorrect file " + nodesFile.toStdString());
            info.node = line.mid(spacePos1 + 1, spacePos2 - spacePos1 - 1);
            info.port = line.mid(spacePos2 + 1);

            LOG << "node " << info.type << ". " << info.node << ". " << info.port << ".";
            nodes.emplace_back(info);
        }
    }

    savedNodesPath = QDir(QCoreApplication::applicationDirPath()).filePath(FILL_NODES_PATH);
    const system_time_point lastFill = fillNodesFromFile(savedNodesPath);
    const system_time_point now = system_now();
    milliseconds passedTime = std::chrono::duration_cast<milliseconds>(now - lastFill);
    if (lastFill - now >= hours(1)) {
        // Защита от перевода времени назад
        passedTime = UPDATE_PERIOD;
    }

    CHECK(QObject::connect(&thread1,SIGNAL(started()),this,SLOT(run())), "not connect started");
    CHECK(QObject::connect(this,SIGNAL(finished()),&thread1,SLOT(terminate())), "not connect finished");

    milliseconds msTimer;
    if (passedTime >= UPDATE_PERIOD) {
        msTimer = 1ms;
    } else {
        msTimer = UPDATE_PERIOD - passedTime;
    }
    LOG << "Start ns resolve after " << msTimer.count() << " ms";
    qtimer.moveToThread(&thread1);
    qtimer.setInterval(msTimer.count());
    qtimer.setSingleShot(true);
    CHECK(connect(&qtimer, SIGNAL(timeout()), this, SLOT(timerEvent())), "not connect");
    CHECK(qtimer.connect(&thread1, SIGNAL(started()), SLOT(start())), "not connect");
    CHECK(qtimer.connect(&thread1, SIGNAL(finished()), SLOT(stop())), "not connect");

    client.setParent(this);
    CHECK(connect(&client, SIGNAL(callbackCall(ReturnCallback)), this, SLOT(callbackCall(ReturnCallback))), "not connect callbackCall");

    client.moveToThread(&thread1);
    moveToThread(&thread1);
}

NsLookup::~NsLookup() {
    thread1.quit();
    if (!thread1.wait(3000)) {
        thread1.terminate();
        thread1.wait();
    }
}

void NsLookup::callbackCall(ReturnCallback callback) {
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
    std::lock_guard<std::mutex> lock(nodeMutex);
    allNodes.swap(allNodesNew);
    allNodesForTypes.swap(allNodesForTypesNew);
    sortAll();
    saveToFile(savedNodesPath, system_now());

    const time_point stopScan = ::now();
    LOG << "Dns scan time " << std::chrono::duration_cast<seconds>(stopScan - startScanTime).count() << " seconds";
}

void NsLookup::timerEvent() {
BEGIN_SLOT_WRAPPER
    qtimer.setInterval(UPDATE_PERIOD.count());
    qtimer.setSingleShot(false);

    startScanTime = ::now();

    posInNodes = 0;
    allNodesForTypesNew.clear();
    allNodesNew.clear();

    LOG << "Dns scan start";
    continueResolve();
END_SLOT_WRAPPER
}

void NsLookup::continueResolve() {
    if (posInNodes >= nodes.size()) {
        finalizeLookup();
        return;
    }

    const NodeType &node = nodes[posInNodes];
    posInNodes++;

    QUdpSocket udp;

    DnsPacket requestPacket;
    requestPacket.addQuestion(DnsQuestion::getIp(node.node));
    requestPacket.setFlags(DnsFlag::MyFlag);
    udp.writeDatagram(requestPacket.toByteArray(), QHostAddress("8.8.8.8"), 53);

    udp.waitForReadyRead();
    std::vector<char> data(512 * 1000, 0);
    const int size = udp.readDatagram(data.data(), data.size());
    CHECK(size > 0, "Incorrect response dns");
    DnsPacket packet = DnsPacket::fromBytesArary(QByteArray(data.data(), size));

    LOG << "dns ok " << node.type << ". " << packet.answers().size();
    ipsTemp.clear();
    for (const auto &record : packet.answers()) {
        ipsTemp.emplace_back(record.toString());
    }
    posInIpsTemp = 0;

    continuePing();
}

void NsLookup::continuePing() {
    if (posInIpsTemp >= ipsTemp.size()) {
        continueResolve();
        return;
    }

    const NodeType &nodeType = nodes[posInNodes - 1];

    const size_t countSteps = std::min(size_t(10), ipsTemp.size() - posInIpsTemp);
    requestsInProcess = countSteps;
    for (size_t i = 0; i < countSteps; i++) {
        const QString &ip = ipsTemp[posInIpsTemp];
        posInIpsTemp++;
        client.ping(ip + ":" + nodeType.port, [this, type=nodeType.type](const QString &address, const milliseconds &time, const std::string &message) {
            const milliseconds MAX_PING = 100s;
            NodeInfo info;
            info.ipAndPort = address;
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

            requestsInProcess--;
            if (requestsInProcess == 0) {
                continuePing();
            }
        }, 2s);
    }
}

void NsLookup::addNode(const QString &type, const NodeInfo &node, bool isNew) {
    auto processNode = [](std::deque<NodeInfo> &allNodes, std::map<QString, std::vector<std::reference_wrapper<const NodeInfo>>> &allNodesForTypes, const QString &type, const NodeInfo &node) {
        allNodes.emplace_back(node);
        const NodeInfo &ref = allNodes.back();
        allNodesForTypes[type].emplace_back(std::cref(ref));
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

system_time_point NsLookup::fillNodesFromFile(const QString &file) {
    QFile inputFile(file);
    if(!inputFile.open(QIODevice::ReadOnly)) {
        return intToSystemTimePoint(0);
    }
    QTextStream in(&inputFile);
    CHECK(!in.atEnd(), "Incorrect file " + file.toStdString());
    const std::string firstLineStr = in.readLine().toStdString();
    const system_time_point timePoint = intToSystemTimePoint(std::stoull(firstLineStr));
    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (!line.isNull() && !line.isEmpty()) {
            NodeInfo info;
            const int spacePos1 = line.indexOf(' ');
            CHECK(spacePos1 != -1, "Incorrect file " + file.toStdString());
            const QString type = line.mid(0, spacePos1);
            const int spacePos2 = line.indexOf(' ', spacePos1 + 1);
            CHECK(spacePos2 != -1, "Incorrect file " + file.toStdString());
            info.ipAndPort = line.mid(spacePos1 + 1, spacePos2 - spacePos1 - 1);
            info.ping = std::stoull(line.mid(spacePos2 + 1).toStdString());

            addNode(type, info, false);
        }
    }

    sortAll();
    return timePoint;
}

void NsLookup::saveToFile(const QString &file, const system_time_point &tp) {
    std::string content;
    content += std::to_string(systemTimePointToInt(tp)) + "\n";
    for (const auto &element1: allNodesForTypes) {
        for (const NodeInfo &node: element1.second) {
            content += element1.first.toStdString() + " ";
            content += node.ipAndPort.toStdString() + " ";
            content += std::to_string(node.ping) + "\n";
        }
    }

    writeToFile(file, content, false);
}

std::vector<QString> NsLookup::getRandom(const QString &type, size_t limit, size_t count) const {
    CHECK(count <= limit, "Incorrect count value");

    std::lock_guard<std::mutex> lock(nodeMutex);
    auto found = allNodesForTypes.find(type);
    if (found == allNodesForTypes.end()) {
        return {};
    }
    const auto &nodes = found->second;

    return ::getRandom<QString>(nodes, limit, count, [](const auto &node) {return node.get().ipAndPort;});
}
