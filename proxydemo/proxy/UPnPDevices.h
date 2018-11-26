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
    UPnPDevices();
    virtual ~UPnPDevices();

//    /// Get the number of routers discovered
//    quint32 getNumDevicesDiscovered() const;

//    /// Find a router using it's server name
//    UPnPRouter* findDevice(const QString & name);

    /*/// Save all routers to a file (for convenience at startup)
        void saveRouters(const QString & file);

        /// Load all routers from a file
        void loadRouters(const QString & file);*/

public slots:
    void discover();

private slots:
    void onReadyRead();
    void onError(QAbstractSocket::SocketError e);
    //void onXmlFileDownloaded(UPnPRouter *r, bool success);

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
