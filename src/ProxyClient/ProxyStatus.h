#ifndef PROXYSTATUS_H
#define PROXYSTATUS_H

#include <QString>

namespace proxy_client {

struct ProxyStatus {
    enum class Status {
        not_set, connect_to_server_error
    };

    Status status = Status::not_set;

    QString description;
};

} // namespace proxy_client

#endif // PROXYSTATUS_H
