#include "UPnPRouter.h"

#include <QPair>
#include <QDomElement>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QBuffer>
#include <QXmlStreamReader>

using SOAPArg = QPair<QString, QString>;

static QString createSOAPCommand(const QString &action, const QString &service, const QList<SOAPArg> &args)
{
    QString ret = QString("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                          "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
                          "<s:Body>"
                          "<u:%1 xmlns:u=\"%2\">").arg(action).arg(service);

    for (const SOAPArg &a : args)
        ret += "<" + a.first + ">" + a.second + "</" + a.first + ">";

    ret += QString("</u:%1></s:Body></s:Envelope>").arg(action);
    return ret;
}

namespace proxy
{
UPnPRouter::UPnPRouter(const QString &server, const QUrl &location)
    : m_server(server)
    , m_location(location)
{
    if(m_location.port() <= 0)
        m_location.setPort(80);
}

UPnPRouter::~UPnPRouter()
{
}

void UPnPRouter::addPortMapping(quint16 internalPort, quint16 externalPort, Protocol protocol)
{
    QString serviceType;
    QString controlUrl;
    for (const UPnPService& s : services) {
        if(s.servicetype.contains("WANIPConnection") || s.servicetype.contains("WANPPPConnection")) {
            serviceType = s.servicetype;
            controlUrl = s.controlurl;
        }
    }
    if (serviceType.isEmpty())
        return;

    // add all the arguments for the command
    QList<SOAPArg> args;
    args.append(SOAPArg("NewRemoteHost", ""));
    args.append(SOAPArg("NewExternalPort", QString::number(externalPort)));
    args.append(SOAPArg("NewProtocol", protocol == TCP ? "TCP" : "UDP"));
    args.append(SOAPArg("NewInternalPort", QString::number(internalPort)));
    args.append(SOAPArg("NewInternalClient", "192.168.100.104"));
    args.append(SOAPArg("NewEnabled", "1"));
    args.append(SOAPArg("NewPortMappingDescription", "MetaGate"));
    args.append(SOAPArg("NewLeaseDuration", "0"));

    QString action = "AddPortMapping";
    QString comm = createSOAPCommand(action, serviceType, args);
    qDebug() << comm;

    /*Forwarding fw = {port, 0, srv};
    // erase old forwarding if one exists
    QList<Forwarding>::iterator itr = fwds.begin();
    while(itr != fwds.end())
    {
        Forwarding& fwo = *itr;
        if(fwo.port == port && fwo.service == srv)
            itr = fwds.erase(itr);
        else
            ++itr;
    }*/

    sendSoapQuery(comm, serviceType + "#" + action, controlUrl);
    //connect(fw.pending_req, SIGNAL(result(HTTPRequest*)), parent, SLOT(forwardResult(HTTPRequest*)));
    //fwds.append(fw);
}

void UPnPRouter::deletePortMapping(quint16 externalPort, Protocol protocol)
{
    QString serviceType;
    QString controlUrl;
    for (const UPnPService& s : services) {
        if(s.servicetype.contains("WANIPConnection") || s.servicetype.contains("WANPPPConnection")) {
            serviceType = s.servicetype;
            controlUrl = s.controlurl;
        }
    }
    if (serviceType.isEmpty())
        return;


    // add all the arguments for the command
    QList<SOAPArg> args;
    args.append(SOAPArg("NewExternalPort", QString::number(externalPort)));
    args.append(SOAPArg("NewProtocol", protocol == TCP ? "TCP" : "UDP"));

    QString action = "DeletePortMapping";
    QString comm = createSOAPCommand(action, serviceType, args);
    qDebug() << comm;
    sendSoapQuery(comm, serviceType + "#" + action, controlUrl);
}

void UPnPRouter::downloadXMLFile()
{
    QNetworkRequest request(m_location);
    QNetworkReply *reply = m_manager.get(request);
    QObject::connect(reply, &QNetworkReply::finished,
                     [this, reply]() {
        if (reply->error()) {
            //m_error = "Failed to download %1: %2", d->location.toDisplayString(), j->errorString());
            //LOG
            return;
        }

        QByteArray data = reply->readAll();
        parseXML(data);
        //UPnPDescriptionParser desc_parse;
        //bool ret = desc_parse.parse(reply->readAll(), this);
        //CHECK

        emit xmlFileDownloaded(this, true);
        //getExternalIP();
        reply->deleteLater();
    });
}




QString UPnPRouter::server() const
{
    return m_server;
}

QUrl UPnPRouter::location() const
{
    return m_location;
}

QString UPnPRouter::friendlyName() const
{
    return  m_friendlyName;
}

QString UPnPRouter::manufacturer() const
{
    return  m_manufacturer;
}

QString UPnPRouter::modelDescription() const
{
    return  m_modelDescription;
}

QString UPnPRouter::modelName() const
{
    return  m_modelName;
}

QString UPnPRouter::modelNumber() const
{
    return  m_modelNumber;
}

void UPnPRouter::sendSoapQuery(const QString& query, const QString& soapact, const QString& controlurl)
{
    QUrl curl(controlurl);
    QString host = curl.host().isEmpty() ? m_location.host() : curl.host();
    int port = curl.port() != -1 ? curl.port() : m_location.port(80);
    QUrl url(m_location);
    url.setPath(curl.path());

    QNetworkRequest req;
    req.setUrl(url);
    req.setRawHeader("Host", host.toLatin1() + QByteArrayLiteral(":") + QByteArray::number(port));
    req.setRawHeader("User-Agent", "MetaGate");
    req.setHeader(QNetworkRequest::ContentTypeHeader, QLatin1String("text/xml"));
    req.setRawHeader("SOAPAction", QString("\"%1\"").arg(soapact).toLatin1());
    QNetworkReply *reply = m_manager.post(req, query.toLatin1());
    QObject::connect(reply, &QNetworkReply::finished,
                     [reply]() {
        if (reply->error()) {
            //m_error = "Failed to download %1: %2", d->location.toDisplayString(), j->errorString());
            //LOG
            qDebug() << reply->errorString();
            return;
        }

        qDebug() << "ret " << reply->readAll();
        reply->deleteLater();
    });

}

bool UPnPRouter::parseXML(QByteArray &data)
{
    qDebug() << "!!!!!!!";
    QBuffer buffer(&data, this);
    buffer.open(QIODevice::ReadOnly);
    QXmlStreamReader xml(&buffer);

    while (xml.readNextStartElement()) {
        qDebug() << xml.name();
        if (xml.name() == "root") {
            parseXMLRoot(xml);
        } else {
            xml.skipCurrentElement();
        }

    }

}

void UPnPRouter::parseXMLRoot(QXmlStreamReader &xml)
{
    while (xml.readNextStartElement()) {
        qDebug() << xml.name();
        if (xml.name() == "device") {
            parseXMLDevice(xml);
        } else {
            xml.skipCurrentElement();
        }

    }
}

void UPnPRouter::parseXMLDevice(QXmlStreamReader &xml)
{
    while (xml.readNextStartElement()) {
        qDebug() << xml.name();
        if (xml.name() == "serviceList") {
            parseXMLServiceList(xml);
        } else if (xml.name() == "friendlyName") {
            m_friendlyName = xml.readElementText();
        } else if (xml.name() == "manufacturer") {
            m_manufacturer = xml.readElementText();
        } else if (xml.name() == "modelDescription") {
            m_modelDescription = xml.readElementText();
        } else if (xml.name() == "modelName") {
            m_modelName = xml.readElementText();
        } else if (xml.name() == "modelNumber") {
            m_modelNumber = xml.readElementText();
        } else {
            xml.skipCurrentElement();
        }
    }
}

void UPnPRouter::parseXMLServiceList(QXmlStreamReader &xml)
{
    while (xml.readNextStartElement()) {
        qDebug() << xml.name();
        if (xml.name() == "service") {
            parseXMLService(xml);
        } else {
            xml.skipCurrentElement();
        }

    }
}

void UPnPRouter::parseXMLService(QXmlStreamReader &xml)
{
    UPnPService service;
    while (xml.readNextStartElement()) {
        qDebug() << xml.name();
        if (xml.name() == "serviceType") {
            service.servicetype = xml.readElementText();
        } else if (xml.name() == "serviceId") {
            service.serviceid = xml.readElementText();
        } else if (xml.name() == "controlURL") {
            service.controlurl = xml.readElementText();
        } else {
            xml.skipCurrentElement();
        }
    }
    services.append(service);
}

}
