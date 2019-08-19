#ifndef NSLOOKUPSTRUCTS_H
#define NSLOOKUPSTRUCTS_H

#include <QString>

#include "duration.h"

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

    milliseconds ping;

    size_t countUpdated = 1;
    bool isTimeout = false;

    bool operator< (const NodeInfo &second) const {
        if (this->isTimeout) {
            return false;
        } else if (second.isTimeout) {
            return true;
        } else {
            const auto max = std::numeric_limits<decltype(countUpdated)>::max();
            return std::make_pair(max - countUpdated, ping) < std::make_pair(max - second.countUpdated, second.ping);
        }
    }
};

struct DnsErrorDetails {
    QString dnsName;

    void clear() {
        dnsName.clear();
    }

    bool isEmpty() const {
        return dnsName.isEmpty();
    }
};

struct NodeTypeStatus {
    QString node;
    size_t countWorked;
    size_t countAll;
    size_t bestResult;

    NodeTypeStatus(const QString &node, size_t countWorked, size_t countAll, size_t bestResult)
        : node(node)
        , countWorked(countWorked)
        , countAll(countAll)
        , bestResult(bestResult)
    {}
};

struct NodeResponse {
    bool isSuccess;

    NodeResponse(bool isSuccess)
        : isSuccess(isSuccess)
    {}
};

#endif // NSLOOKUPSTRUCTS_H
