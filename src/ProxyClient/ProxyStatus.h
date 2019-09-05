#ifndef PROXYSTATUS_H
#define PROXYSTATUS_H

#include <QString>

namespace proxy_client {

struct ProxyStatus {
    enum class Status {
        not_set, connect_to_server_error, started, begin_test, error_begin_test, success_test, failure_test
    };

    Status status = Status::not_set;

    QString description;
};

} // namespace proxy_client

#endif // PROXYSTATUS_H
