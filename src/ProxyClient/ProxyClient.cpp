#include "ProxyClient.h"

#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"
#include "qt_utilites/ManagerWrapperImpl.h"

namespace proxy_client {

ProxyClient::ProxyClient()
    : proxyClient("metagate_proxy")
{
    Q_CONNECT(&proxyClient, &LocalClient::callbackCall, this, &ProxyClient::callbackCall);

    Q_CONNECT(this, &ProxyClient::refreshStatus, this, &ProxyClient::onRefreshStatus);

    Q_REG(RefreshStatusCallback, "RefreshStatusCallback");
}

void ProxyClient::mvToThread(QThread *th) {
    proxyClient.mvToThread(th);
    this->moveToThread(th);
}

void ProxyClient::onRefreshStatus(const RefreshStatusCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitErrorCallback([&]{
        proxyClient.sendMessage("status", [callback](const LocalClient::Response &response) {
            ProxyStatus status;
            if (response.exception.isSet()) {
                status.status = ProxyStatus::Status::connect_to_server_error;
                status.description = QString::fromStdString(response.exception.description);
            } else {

            }
            callback.emitCallback(status);
        });
    }, callback);
END_SLOT_WRAPPER
}

} // namespace proxy_client
