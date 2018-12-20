#ifndef NSLOOKUP_H
#define NSLOOKUP_H

#include <QObject>
#include <QThread>
#include <QTimer>

#include <vector>
#include <map>
#include <deque>
#include <mutex>
#include <atomic>

#include "duration.h"

#include "client.h"

struct NodeType {
    QString type;
    QString node;
    QString port;
};

struct NodeInfo {
    QString address;

    size_t ping;

    bool operator< (const NodeInfo &second) const {
        return this->ping < second.ping;
    }
};

class NsLookup : public QObject
{
    Q_OBJECT
public:
    explicit NsLookup(QObject *parent = nullptr);

    ~NsLookup() override;

    void start();

    std::vector<QString> getRandomWithoutHttp(const QString &type, size_t limit, size_t count) const;

    std::vector<QString> getRandom(const QString &type, size_t limit, size_t count) const;

    void resetFile();

signals:

    void finished();

public slots:

    void run();

    void uploadEvent();

    void callbackCall(SimpleClient::ReturnCallback callback);

private:

    void sortAll();

    system_time_point fillNodesFromFile(const QString &file, const std::map<QString, NodeType> &expectedNodes);

    void saveToFile(const QString &file, const system_time_point &tp, const std::map<QString, NodeType> &expectedNodes);

    void continueResolve(std::map<QString, NodeType>::const_iterator node);

    void continuePing(std::map<QString, NodeType>::const_iterator node);

    void finalizeLookup();

    std::vector<QString> getRandom(const QString &type, size_t limit, size_t count, const std::function<QString(const NodeInfo &node)> &process) const;

    std::vector<QString> requestDns(const NodeType &node) const;

    static QString makeAddress(const QString &ip, const QString &port);

private:

    QString savedNodesPath;

    std::map<QString, NodeType> nodes;

    std::vector<QString> ipsTemp;

    size_t posInIpsTemp;

    struct CmpNodeType {
        bool operator()(const NodeType& a, const NodeType& b) const {
            return a.node < b.node;
        }
    };

    std::map<NodeType, std::vector<NodeInfo>, CmpNodeType> allNodesForTypes;

    std::map<NodeType, std::vector<NodeInfo>, CmpNodeType> allNodesForTypesNew;

    mutable std::mutex nodeMutex;

    QThread thread1;

    QTimer qtimer;

    SimpleClient client;

    time_point startScanTime;

    std::atomic<bool> isResetFilledFile{false};

    bool isSafeCheck = false;

    milliseconds passedTime;

    std::atomic<bool> isStopped{false};

};

#endif // NSLOOKUP_H
