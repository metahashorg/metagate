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

    std::map<NodeType::Node, std::vector<NodeInfo>> allNodesForTypes;

    std::map<NodeType::Node, std::vector<NodeInfo>> allNodesForTypesNew;

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
