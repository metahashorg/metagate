#ifndef UPNPROUTER_H
#define UPNPROUTER_H

#include <QUrl>
#include <QNetworkAccessManager>
#include <QObject>

class QXmlStreamReader;

namespace proxy
{

enum Protocol
{
    TCP,
    UDP
};

struct UPnPService
{
    QString serviceid;
    QString servicetype;
    QString controlurl;
};

class UPnPRouter : public QObject
{
    Q_OBJECT
public:
    using PortMappingCallback = std::function<void(bool result, const QString &error)>;

    UPnPRouter(const QString &server, const QUrl &location);
    virtual ~UPnPRouter();

    QString server() const;
    QUrl location() const;

    QString friendlyName() const;
    QString manufacturer() const;
    QString modelDescription() const;
    QString modelName() const;
    QString modelNumber() const;
    QString serialNumber() const;
    QString udn() const;


    void addPortMapping(quint16 localPort, quint16 externalPort, Protocol protocol, const PortMappingCallback &callback);
    void deletePortMapping(quint16 externalPort, Protocol protocol, const PortMappingCallback &callback);

    void downloadXMLFile();

signals:
    void xmlFileDownloaded( bool success);

private:
    void sendSoapQuery(const QString &query, const QString &soapact, const QString &controlurl, const PortMappingCallback &callback);

    void parseXML(QByteArray &data);
    void parseXMLRoot(QXmlStreamReader &xml);
    void parseXMLDeviceList(QXmlStreamReader &xml);
    void parseXMLServiceList(QXmlStreamReader &xml);
    void parseXMLDevice(QXmlStreamReader &xml, bool set = false);
    void parseXMLService(QXmlStreamReader &xml);

    QString getLocalAddress(const QString &hostName, quint16 port) const;

    QString m_server;
    QUrl m_location;
    QUrl m_url;
    QString m_serviceType;
    QString m_localAddress;

    QString m_friendlyName;
    QString m_manufacturer;
    QString m_modelDescription;
    QString m_modelName;
    QString m_modelNumber;
    QString m_serialNumber;
    QString m_udn;

    QNetworkAccessManager m_manager;

    QList<UPnPService> services;
};

}

#endif
