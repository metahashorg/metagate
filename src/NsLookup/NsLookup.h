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

#include "CallbackWrapper.h"
#include "ManagerWrapper.h"

#include "TaskManager.h"

#include "NsLookupStructs.h"

struct TypedException;

class NsLookup : public ManagerWrapper, public TimerClass {
    Q_OBJECT
private:

    struct CacheDns {
        std::map<QString, std::vector<QString>> cache;
        time_point lastUpdate;
    };

public:

    using GetStatusCallback = CallbackWrapper<void(const std::vector<NodeTypeStatus> &nodeStatuses, const DnsErrorDetails &dnsError)>;

    using GetServersCallback = CallbackWrapper<void(const std::vector<QString> &servers)>;

public:
    explicit NsLookup(QObject *parent = nullptr);

    ~NsLookup() override;

    void resetFile();

protected:

    void startMethod() override;

    void timerMethod() override;

    void finishMethod() override;

signals:

    void getStatus(const GetStatusCallback &callback);

    void getRandomServersWithoutHttp(const QString &type, size_t limit, size_t count, const GetServersCallback &callback);

    void getRandomServers(const QString &type, size_t limit, size_t count, const GetServersCallback &callback);

public slots:

    void onGetStatus(const GetStatusCallback &callback);

    void onGetRandomServersWithoutHttp(const QString &type, size_t limit, size_t count, const GetServersCallback &callback);

    void onGetRandomServers(const QString &type, size_t limit, size_t count, const GetServersCallback &callback);

signals:

    void rejectServer(const QString &server);

public slots:

    void onRejectServer(const QString &server);

signals:

    void finished();

    void serversFlushed(const TypedException &exception);

public:

    void beginResolve(std::map<NodeType::Node, std::vector<NodeInfo>> &allNodesForTypesNew, std::vector<QString> &ipsTemp, const std::function<void()> &finalizeLookup, const std::function<void(std::map<QString, NodeType>::const_iterator node)> &beginPing);

    void continueResolve(std::map<QString, NodeType>::const_iterator node, std::map<NodeType::Node, std::vector<NodeInfo>> &allNodesForTypesNew, std::vector<QString> &ipsTemp, const std::function<void()> &finalizeLookup, const std::function<void(std::map<QString, NodeType>::const_iterator node)> &beginPing);

    void continuePing(bool isSafeCheck, std::vector<QString>::const_iterator ipsIter, const NodeType &node, std::map<NodeType::Node, std::vector<NodeInfo>> &allNodesForTypesNew, std::vector<QString> &ipsTemp, const std::function<void()> &continueResolve);

    void finalizeLookup(std::map<NodeType::Node, std::vector<NodeInfo>> &allNodesForTypesNew, const std::function<void()> &endLookup);

private:

    void process();

    void sortAll();

    system_time_point fillNodesFromFile(const QString &file, const std::map<QString, NodeType> &expectedNodes);

    void saveToFile(const QString &file, const system_time_point &tp, const std::map<QString, NodeType> &expectedNodes);

    /*void continuePingRefresh(std::vector<QString>::const_iterator ipsIter, const NodeType::Node &node);

    void finalizeRefresh(const NodeType::Node &node);

    void processRefresh();*/

    std::vector<QString> getRandom(const QString &type, size_t limit, size_t count, const std::function<QString(const NodeInfo &node)> &process) const;

    bool repeatResolveDns(
        const QString &dnsServerName,
        int dnsServerPort,
        const QByteArray &byteArray,
        std::map<QString, NodeType>::const_iterator node,
        time_point now,
        size_t countRepeat,
        std::vector<QString> &ipsTemp,
        const std::function<void()> &beginPing
    );

    std::vector<NodeTypeStatus> getNodesStatus() const;

    void printNodes() const;

    size_t countWorkedNodes(const std::vector<NodeInfo> &nodes) const;

private:

    UdpSocketClient udpClient;

    QString savedNodesPath;

    std::map<QString, NodeType> nodes;

    std::vector<QString> ipsTempRefresh;

    std::map<NodeType::Node, std::vector<NodeInfo>> allNodesForTypes;

    milliseconds msTimer = 10s;

    time_point prevCheckTime;

    SimpleClient client;

    time_point startScanTime;

    std::atomic<bool> isResetFilledFile{false};

    milliseconds passedTime;

    CacheDns cacheDns;

    seconds timeoutRequestNodes;

    QString dnsServerName;

    int dnsServerPort;

    time_point prevPrintTime;

    std::vector<std::pair<QString, size_t>> defectiveTorrents;

    DnsErrorDetails dnsErrorDetails;

    size_t randomCounter = 0;

    size_t updateNumber = 0;

    nslookup::TaskManager taskManager;
};

#endif // NSLOOKUP_H
