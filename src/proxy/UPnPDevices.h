#ifndef UPNPDEVICES_H
#define UPNPDEVICES_H

#include <QUdpSocket>
#include "UPnPRouter.h"

namespace proxy
{
class UPnPRouter;

class UPnPDevices : public QObject
{
    Q_OBJECT
public:
    UPnPDevices(QObject *parent = nullptr);
    virtual ~UPnPDevices();

public slots:
    void discover();

private slots:
    void onReadyRead();
    void onError(QAbstractSocket::SocketError e);

signals:
    void discovered(UPnPRouter *router);

private:
    UPnPRouter *findRouter(const QUrl &location);
    UPnPRouter *parseResponse(const QByteArray &data);

    QUdpSocket m_mcsocket;
    QList<UPnPRouter *> routers;
    QList<UPnPRouter *> updatingRouters;
};

}

#endif //UPNPDEVICES_H
