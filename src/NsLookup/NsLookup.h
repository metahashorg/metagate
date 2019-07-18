#ifndef NSLOOKUP_H
#define NSLOOKUP_H

#include <QObject>

#include <vector>
#include <map>
#include <deque>
#include <mutex>
#include <atomic>

#include "duration.h"

#include "qt_utilites/TimerClass.h"
#include "Network/SimpleClient.h"

#include "Network/UdpSocketClient.h"

#include "qt_utilites/CallbackWrapper.h"
#include "qt_utilites/ManagerWrapper.h"

#include "TaskManager.h"

#include "NsLookupStructs.h"

struct TypedException;

namespace nslookup {
class FullWorker;
class SimpleWorker;
class RefreshIpWorker;
class RefreshNodeWorker;
class FindEmptyNodesWorker;
class PrintNodesWorker;
class MiddleWorker;
}

class NsLookup : public ManagerWrapper, public TimerClass {
friend class nslookup::FullWorker;
friend class nslookup::SimpleWorker;
friend class nslookup::RefreshIpWorker;
friend class nslookup::RefreshNodeWorker;
friend class nslookup::FindEmptyNodesWorker;
friend class nslookup::PrintNodesWorker;
friend class nslookup::MiddleWorker;
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

protected:

    void beginResolve(std::map<NodeType::Node, std::vector<NodeInfo>> &allNodesForTypesNew, std::vector<QString> &ipsTemp, const std::function<void()> &finalizeLookup, const std::function<void(std::map<QString, NodeType>::const_iterator node)> &beginPing);

    void continueResolve(std::map<QString, NodeType>::const_iterator node, std::map<NodeType::Node, std::vector<NodeInfo>> &allNodesForTypesNew, std::vector<QString> &ipsTemp, const std::function<void()> &finalizeLookup, const std::function<void(std::map<QString, NodeType>::const_iterator node)> &beginPing);

    void continuePing(std::vector<QString>::const_iterator ipsIter, const NodeType &node, std::map<NodeType::Node, std::vector<NodeInfo>> &allNodesForTypesNew, const std::vector<QString> &ipsTemp, const std::function<void()> &continueResolve);

    void continuePingSafe(const NodeType &node, const std::vector<QString> &ipsTemp, const std::function<void()> &continueResolve);

    void finalizeLookup(bool isFullFill, std::map<NodeType::Node, std::vector<NodeInfo>> &allNodesForTypesNew);

    void finalizeLookup(const NodeType::Node &node, const std::vector<NodeInfo> &allNodesForTypesNew);

    void saveAll(bool isFullFill);



    size_t findCountUpdatedIp(const QString &address) const;

    void processRefreshIp(const QString &address, std::vector<QString> &ipsTemp, const std::function<void(const NodeType &node)> &beginPing);

    void finalizeRefreshIp(const NodeType::Node &node, const std::map<NodeType::Node, std::vector<NodeInfo>> &allNodesForTypesNew);


    std::map<QString, NodeType>::const_iterator getBeginNodesIterator() const;

    void fillNodeStruct(const QString &nodeStr, NodeType &node, std::vector<QString> &ipsTemp);

    bool fillNodeStruct(std::map<QString, NodeType>::const_iterator &node, std::vector<QString> &ipsTemp);

    size_t countWorkedNodes(const NodeType::Node &node) const;

    size_t countWorkedNodes(const QString &nodeStr) const;


    void findAndRefreshEmptyNodes();


    void printNodes() const;

private:

    void process();

    void sortAll();

    system_time_point fillNodesFromFile(const QString &file, const std::map<QString, NodeType> &expectedNodes);

    void saveToFile(const QString &file, const system_time_point &tp, const std::map<QString, NodeType> &expectedNodes);

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

    size_t countWorkedNodes(const std::vector<NodeInfo> &nodes) const;

private:

    UdpSocketClient udpClient;

    QString savedNodesPath;

    system_time_point filledFileTp;

    std::map<QString, NodeType> nodes;

    std::map<NodeType::Node, std::vector<NodeInfo>> allNodesForTypes;

    std::map<NodeType::Node, std::vector<NodeInfo>> allNodesForTypesBackup;

    SimpleClient client;

    std::atomic<bool> isResetFilledFile{false};

    CacheDns cacheDns;

    seconds timeoutRequestNodes;

    QString dnsServerName;

    int dnsServerPort;

    std::vector<std::pair<QString, size_t>> defectiveTorrents;

    DnsErrorDetails dnsErrorDetails;

    size_t updateNumber = 0;

    nslookup::TaskManager taskManager;
};

#endif // NSLOOKUP_H
