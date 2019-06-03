#ifndef NSLOOKUP_H
#define NSLOOKUP_H

#include <QObject>

#include <vector>
#include <map>
#include <deque>
#include <mutex>
#include <atomic>

#include "duration.h"

#include "TimerClass.h"
#include "client.h"

#include "UdpSocketClient.h"

struct TypedException;

struct NodeType {
    enum class SubType {
        none, torrent, proxy
    };

    struct Node {
        QString node;

        Node() = default;

        explicit Node(const QString &node)
            : node(node)
        {}

        const QString &str() const {
            return node;
        }

        bool operator< (const Node &second) const {
            return this->node < second.node;
        }
    };

    QString type;
    Node node;
    QString port;
    QString network;
    SubType subtype = SubType::none;
};

struct NodeInfo {
    QString address;

    size_t ping;

    bool isChecked = false;
    bool isTimeout = false;

    bool operator< (const NodeInfo &second) const {
        if (this->isTimeout) {
            return false;
        } else if (second.isTimeout) {
            return true;
        } else if (this->isChecked && !second.isChecked) {
            return true;
        } else if (!this->isChecked && second.isChecked) {
            return false;
        } else {
            return this->ping < second.ping;
        }
    }
};

class NsLookup : public TimerClass {
    Q_OBJECT
private:

    struct CacheDns {
        std::map<QString, std::vector<QString>> cache;
        time_point lastUpdate;
    };

public:
    explicit NsLookup(QObject *parent = nullptr);

    ~NsLookup() override;

    std::vector<QString> getRandomWithoutHttp(const QString &type, size_t limit, size_t count) const;

    std::vector<QString> getRandom(const QString &type, size_t limit, size_t count) const;

    void resetFile();

signals:

    void finished();

    void serversFlushed(const TypedException &exception);

public slots:

    void run();

    void uploadEvent();

    void callbackCall(SimpleClient::ReturnCallback callback);

private:

    void process();

    void sortAll();

    system_time_point fillNodesFromFile(const QString &file, const std::map<QString, NodeType> &expectedNodes);

    void saveToFile(const QString &file, const system_time_point &tp, const std::map<QString, NodeType> &expectedNodes);

    void continueResolve(std::map<QString, NodeType>::const_iterator node);

    void continuePing(std::vector<QString>::const_iterator ipsIter, std::map<QString, NodeType>::const_iterator node);

    void finalizeLookup();

    void continueResolveP2P(std::map<QString, NodeType>::const_iterator node);

    void continuePingP2P(std::vector<std::pair<NodeType::SubType, QString>>::const_iterator ipsIter, std::map<QString, NodeType>::const_iterator node, const NodeType &nodeTorrent, const NodeType &nodeProxy);

    void finalizeLookupP2P();

    std::vector<QString> getRandom(const QString &type, size_t limit, size_t count, const std::function<QString(const NodeInfo &node)> &process) const;

    bool repeatResolveDns(
        const QString &dnsServerName,
        int dnsServerPort,
        const QByteArray &byteArray,
        std::map<QString, NodeType>::const_iterator node,
        time_point now,
        size_t countRepeat
    );

private:

    UdpSocketClient udpClient;

    QString savedNodesPath;

    std::map<QString, NodeType> nodes;

    std::vector<QString> ipsTemp;

    std::vector<std::pair<NodeType::SubType, QString>> ipsTempP2P;

    std::map<NodeType::Node, std::vector<NodeInfo>> allNodesForTypes;

    std::map<NodeType::Node, std::vector<NodeInfo>> allNodesForTypesNew;

    std::map<NodeType::Node, std::vector<NodeInfo>> allNodesForTypesP2P;

    mutable std::mutex nodeMutex;

    milliseconds msTimer = 10s;

    time_point prevCheckTime;

    SimpleClient client;

    time_point startScanTime;

    std::atomic<bool> isResetFilledFile{false};

    bool isSafeCheck = false;

    milliseconds passedTime;

    std::atomic<bool> isStopped{false};

    CacheDns cacheDns;

    int countSuccessTestsForP2PNodes = 0;

    seconds timeoutRequestNodes;

    QString dnsServerName;

    int dnsServerPort;

    bool useUsersServers = false;

};

#endif // NSLOOKUP_H
