#ifndef NSLOOKUP_H
#define NSLOOKUP_H

#include <QObject>
#include <QThread>
#include <QTimer>

#include <vector>
#include <map>
#include <deque>
#include <mutex>

#include "duration.h"

#include "client.h"

struct NodeType {
    QString type;
    QString node;
    QString port;
};

struct NodeInfo {
    QString ipAndPort;

    size_t ping;

    bool operator< (const NodeInfo &second) const {
        return this->ping < second.ping;
    }
};

class NsLookup : public QObject
{
    Q_OBJECT
public:
    explicit NsLookup(const QString &pagesPath, QObject *parent = nullptr);

    ~NsLookup() override;

    void start();

    std::vector<QString> getRandom(const QString &type, size_t limit, size_t count) const;

signals:

    void finished();

public slots:

    void run();

    void timerEvent();

    void callbackCall(ReturnCallback callback);

private:

    void addNode(const QString &type, const NodeInfo &node, bool isNew);

    void sortAll();

    system_time_point fillNodesFromFile(const QString &file);

    void saveToFile(const QString &file, const system_time_point &tp);

    void continueResolve();

    void continuePing();

    void finalizeLookup();

private:

    QString pagesPath;

    QString savedNodesPath;

    std::vector<NodeType> nodes;

    size_t posInNodes = 0;

    std::vector<QString> ipsTemp;

    size_t posInIpsTemp;

    std::deque<NodeInfo> allNodes;

    std::deque<NodeInfo> allNodesNew;

    size_t requestsInProcess = 0;

    std::map<QString, std::vector<std::reference_wrapper<const NodeInfo>>> allNodesForTypes;

    std::map<QString, std::vector<std::reference_wrapper<const NodeInfo>>> allNodesForTypesNew;

    mutable std::mutex nodeMutex;

    QThread thread1;

    QTimer qtimer;

    SimpleClient client;

    time_point startScanTime;

};

#endif // NSLOOKUP_H
