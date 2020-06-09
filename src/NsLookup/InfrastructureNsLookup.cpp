#include "InfrastructureNsLookup.h"

#include "check.h"
#include "Log.h"
#include "Paths.h"

#include "qt_utilites/QRegister.h"
#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/ManagerWrapperImpl.h"

#include "NsLookup.h"

#include <QSettings>

SET_LOG_NAMESPACE("NSL");

InfrastructureNsLookup::Nodes::Nodes(const QString &torrent, const QString &proxy, const QString &contractTorrent)
    : torrent(torrent)
    , proxy(proxy)
    , contractTorrent(contractTorrent)
{}

InfrastructureNsLookup::InfrastructureNsLookup() {
    Q_CONNECT(this, &InfrastructureNsLookup::getTorrents, this, &InfrastructureNsLookup::onGetTorrents);
    Q_CONNECT(this, &InfrastructureNsLookup::getProxy, this, &InfrastructureNsLookup::onGetProxy);
    Q_CONNECT(this, &InfrastructureNsLookup::getContractTorrent, this, &InfrastructureNsLookup::onGetContractTorrent);
    Q_CONNECT(this, &InfrastructureNsLookup::getRequestFornode, this, &InfrastructureNsLookup::onGetRequestFornode);

    Q_REG(InfrastructureNsLookup::GetServersCallback, "InfrastructureNsLookup::GetServersCallback");
    Q_REG(GetFormatRequestCallback, "GetFormatRequestCallback");

    QSettings settings(getSettingsPath(), QSettings::IniFormat);
    const int size = settings.beginReadArray("infrastructure");
    for (int i = 0; i < size; i++) {
        settings.setArrayIndex(i);
        const QString currency = settings.value("currency").toString();
        const Nodes nodes(settings.value("torrent").toString(), settings.value("proxy").toString(), settings.value("contract_torrent", "").toString());
        infrastructure.emplace(currency, nodes);
    }
    LOG << "infrastructure size " << infrastructure.size();
    settings.endArray();
}

void InfrastructureNsLookup::setNsLookup(NsLookup *nsl) {
    nsLookup = nsl;
}

template<typename Member>
void InfrastructureNsLookup::getServers(const QString &currency, const Member &member, size_t limit, size_t count, const InfrastructureNsLookup::GetServersCallback &callback) {
    runAndEmitErrorCallback([&]{
        const auto found = infrastructure.find(currency);
        if (found == infrastructure.end()) {
            callback.emitCallback(std::vector<QString>());
            return;
        }
        const QString type = member(found->second);
        if (type.isEmpty()) {
            callback.emitCallback(std::vector<QString>());
            return;
        }
        CHECK(nsLookup != nullptr, "NsLookup not set");
        emit nsLookup->getRandomServers(type, limit, count, callback);
    }, callback);
}

void InfrastructureNsLookup::onGetTorrents(const QString &currency, size_t limit, size_t count, const InfrastructureNsLookup::GetServersCallback &callback) {
BEGIN_SLOT_WRAPPER
    getServers(currency, std::mem_fn(&Nodes::torrent), limit, count, callback);
END_SLOT_WRAPPER
}

void InfrastructureNsLookup::onGetProxy(const QString &currency, size_t limit, size_t count, const InfrastructureNsLookup::GetServersCallback &callback) {
BEGIN_SLOT_WRAPPER
    getServers(currency, std::mem_fn(&Nodes::proxy), limit, count, callback);
END_SLOT_WRAPPER
}

void InfrastructureNsLookup::onGetContractTorrent(const QString &currency, size_t limit, size_t count, const InfrastructureNsLookup::GetServersCallback &callback) {
BEGIN_SLOT_WRAPPER
    getServers(currency, std::mem_fn(&Nodes::contractTorrent), limit, count, callback);
END_SLOT_WRAPPER
}

static NodeResponse proxyResponseParser(const QByteArray &response, const std::string &error) {
    if (response.isEmpty() && error.empty()) {
        return NodeResponse(false);
    } else {
        return NodeResponse(true);
    }
}

static NodeResponse torrentResponseParser(const QByteArray &response, const std::string &/*error*/) {
    if (response.isEmpty()) {
        return NodeResponse(false);
    } else {
        if (response.front() == '{' && response.back() == '}') {
            return NodeResponse(true);
        } else {
            return NodeResponse(false);
        }
    }
}

void InfrastructureNsLookup::onGetRequestFornode(const QString &type, const GetFormatRequestCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]() -> std::tuple<bool, QString, QByteArray, std::function<NodeResponse(const QByteArray &response, const std::string &error)>>{
        for (const auto &pair: infrastructure) {
            const Nodes &node = pair.second;
            if (type == node.proxy) {
                return std::make_tuple(true, "", QByteArray(), proxyResponseParser);
            } else if (type == node.torrent || type == node.contractTorrent) {
                return std::make_tuple(true, "/status", QByteArray(), torrentResponseParser);
            }
        }
        return std::make_tuple(false, "", QByteArray(), nullptr);
    }, callback);
END_SLOT_WRAPPER
}
