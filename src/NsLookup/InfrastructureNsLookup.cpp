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

InfrastructureNsLookup::Nodes::Nodes(const QString &torrent, const QString &proxy)
    : torrent(torrent)
    , proxy(proxy)
{}

InfrastructureNsLookup::InfrastructureNsLookup(NsLookup &nsLookup, QObject *parent)
    : nsLookup(nsLookup)
{
    Q_CONNECT(this, &InfrastructureNsLookup::getTorrents, this, &InfrastructureNsLookup::onGetTorrents);
    Q_CONNECT(this, &InfrastructureNsLookup::getProxy, this, &InfrastructureNsLookup::onGetProxy);

    Q_REG(InfrastructureNsLookup::GetServersCallback, "InfrastructureNsLookup::GetServersCallback");

    QSettings settings(getSettingsPath(), QSettings::IniFormat);
    const int size = settings.beginReadArray("infrastructure");
    for (int i = 0; i < size; i++) {
        settings.setArrayIndex(i);
        const QString currency = settings.value("currency").toString();
        const Nodes nodes(settings.value("torrent").toString(), settings.value("proxy").toString());
        infrastructure.emplace(currency, nodes);
    }
    LOG << "infrastructure size " << infrastructure.size();
    settings.endArray();
}

template<typename Member>
void InfrastructureNsLookup::getServers(const QString &currency, const Member &member, size_t limit, size_t count, const InfrastructureNsLookup::GetServersCallback &callback) {
    runAndEmitErrorCallback([&]{
        const auto found = infrastructure.find(currency);
        if (found == infrastructure.end()) {
            callback.emitCallback(std::vector<QString>());
            return;
        }
        emit nsLookup.getRandomServers(member(found->second), limit, count, callback);
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
