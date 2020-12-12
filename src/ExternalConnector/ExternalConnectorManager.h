#ifndef EXTERNALCONNECTORMANAGER_H
#define EXTERNALCONNECTORMANAGER_H

#include "qt_utilites/ManagerWrapper.h"
#include "qt_utilites/TimerClass.h"

class ExternalConnector;

namespace localconnection
{
class LocalServer;
class LocalServerRequest;
class LocalClient;
} // namespace localconnection

class ExternalConnectorManager : public ManagerWrapper, public TimerClass
{
    Q_OBJECT
public:
    ExternalConnectorManager(ExternalConnector &externalConnector, QObject* parent = nullptr);
    ~ExternalConnectorManager() override;

protected:
    void startMethod() override;

    void timerMethod() override;

    void finishMethod() override;

public slots:
    void urlChanged(const QString &url);

private slots:
    void onRequest(std::shared_ptr<localconnection::LocalServerRequest> request);

private:
    ExternalConnector &externalConnector;

    localconnection::LocalServer* localServer;
    localconnection::LocalClient* localClient;
};

#endif // EXTERNALCONNECTORMANAGER_H
