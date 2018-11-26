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
    UPnPRouter(const QString & server,const QUrl &location);
    virtual ~UPnPRouter();

    QString server() const;
    QUrl location() const;

    QString friendlyName() const;
    QString manufacturer() const;
    QString modelDescription() const;
    QString modelName() const;
    QString modelNumber() const;


    void addPortMapping(quint16 localPort, quint16 externalPort, Protocol protocol);
    void deletePortMapping(quint16 externalPort, Protocol protocol);

    void downloadXMLFile();

signals:
    void xmlFileDownloaded(UPnPRouter *r, bool success);

private:
    void sendSoapQuery(const QString &query, const QString &soapact, const QString &controlurl);

    bool parseXML(QByteArray &data);
    void parseXMLRoot(QXmlStreamReader &xml);
    void parseXMLDevice(QXmlStreamReader &xml);
    void parseXMLServiceList(QXmlStreamReader &xml);
    void parseXMLService(QXmlStreamReader &xml);

    QString m_server;
    QUrl m_location;

    QString m_friendlyName;
    QString m_manufacturer;
    QString m_modelDescription;
    QString m_modelName;
    QString m_modelNumber;

    QNetworkAccessManager m_manager;

    QList<UPnPService> services;
};

}

#endif
