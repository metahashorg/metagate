#include "UPnPDevices.h"

#include <QNetworkDatagram>

namespace proxy
{

const QString mcSearchHost = QStringLiteral("239.255.255.250");
const quint16 mcSearchPort = 1900;

const QByteArray upnpSearchHeader = "M-SEARCH * HTTP/1.1\r\n"
                                    "HOST: 239.255.255.250:1900\r\n"
                                    "ST:urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n"
                                    "MAN:\"ssdp:discover\"\r\n"
                                    "MX:3\r\n"
                                    "\r\n\0";

const QByteArray tr64SearchHeader = "M-SEARCH * HTTP/1.1\r\n"
                                    "HOST: 239.255.255.250:1900\r\n"
                                    "ST:urn:dslforum-org:device:InternetGatewayDevice:1\r\n"
                                    "MAN:\"ssdp:discover\"\r\n"
                                    "MX:3\r\n"
                                    "\r\n\0";

static bool UrlCompare(const QUrl &a, const QUrl & b)
{
    if (a == b)
        return true;

    return
            a.scheme() == b.scheme() &&
            a.host() == b.host() &&
            a.password() == b.password() &&
            a.port(80) == b.port(80) &&
            a.path() == b.path() &&
            a.query() == b.query(); //TODO check if ported correctly
}

UPnPDevices::UPnPDevices()
{
    QObject::connect(&m_mcsocket, &QUdpSocket::readyRead,
                     this, &UPnPDevices::onReadyRead);
    QObject::connect(&m_mcsocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
                     this, &UPnPDevices::onError);

    //check
    m_mcsocket.bind(QHostAddress::AnyIPv4, mcSearchPort, QUdpSocket::ShareAddress);
    m_mcsocket.joinMulticastGroup(QHostAddress(mcSearchHost));

    /*for (quint32 i = 0;i < 10;i++)
    {
        if (!bind(1900 + i,QUdpSocket::ShareAddress))
            ;//qDebug() << "Cannot bind to UDP port 1900 : " << errorString() << endl;
        else
            break;
    }*/

}


UPnPDevices::~UPnPDevices()
{
    m_mcsocket.leaveMulticastGroup(QHostAddress(mcSearchHost));
}

void UPnPDevices::discover()
{
    qDebug() << "Trying to find UPnP devices on the local network" << endl;

    m_mcsocket.writeDatagram(upnpSearchHeader, QHostAddress(mcSearchHost), mcSearchPort);
    m_mcsocket.writeDatagram(tr64SearchHeader, QHostAddress(mcSearchHost), mcSearchPort);
}

UPnPRouter *UPnPDevices::findRouter(const QUrl &location)
{
    for (UPnPRouter *r : routers) {
        if (UrlCompare(r->location(), location))
            return r;
    }
    return nullptr;
}

void UPnPDevices::onReadyRead()
{
    while (m_mcsocket.hasPendingDatagrams()) {
        QNetworkDatagram dgram = m_mcsocket.receiveDatagram();
        qDebug() << "Received : " << dgram.data();
        // try to make a router of it
        UPnPRouter *r = parseResponse(dgram.data());
        if (r)
        {
            QObject::connect(r, &UPnPRouter::xmlFileDownloaded,
                             this, [this](UPnPRouter *r, bool success) {
                updatingRouters.removeAll(r);
                if (!success)
                {
                    // we couldn't download and parse the XML file so
                    // get rid of it
                    r->deleteLater();
                }
                else
                {
                    // add it to the list and emit the signal
                    QUrl location = r->location();
                    if (findRouter(location))
                    {
                        r->deleteLater();
                    }
                    else
                    {
                        routers.append(r);
                        emit discovered(r);
                    }
                }
            }
            );

            // download it's xml file
            r->downloadXMLFile();
            updatingRouters.append(r);
        }
    }
}

void UPnPDevices::onError(QAbstractSocket::SocketError e)
{
    Q_UNUSED(e);
    qDebug() << "Error : " << m_mcsocket.errorString();
}

UPnPRouter *UPnPDevices::parseResponse(const QByteArray &data)
{
    const QString response = QString::fromLatin1(data);
    QVector<QStringRef> lines = response.splitRef("\r\n");
    QString server;
    QUrl location;

    /*
        Out(SYS_PNP|LOG_DEBUG) << "Received : " << endl;
        for (Uint32 idx = 0;idx < lines.count(); idx++)
            Out(SYS_PNP|LOG_DEBUG) << lines[idx] << endl;
        */

    // first read first line and see if contains a HTTP 200 OK message
    QStringRef line = lines.first();
    if (!line.contains(QLatin1String("HTTP"))) {
        // it is either a 200 OK or a NOTIFY
        if (!line.contains(QLatin1String("NOTIFY")) && !line.contains(QLatin1String("200")))
            return nullptr;
    } else if (line.contains(QLatin1String("M-SEARCH"))) { // ignore M-SEARCH
        return nullptr;
    }

    // quick check that the response being parsed is valid
    bool validDevice = false;
    for (int idx = 0;idx < lines.count() && !validDevice; idx++)
    {
        line = lines[idx];
        if ((line.contains(QLatin1String("ST:")) || line.contains(QLatin1String("NT:"))) && line.contains(QLatin1String("InternetGatewayDevice")))
        {
            validDevice = true;
        }
    }
    if (!validDevice)
    {
        //	qDebug() << "Not a valid Internet Gateway Device" << endl;
        return 0;
    }

    // read all lines and try to find the server and location fields
    for (int i = 1;i < lines.count();i++)
    {
        line = lines[i];
        if (line.startsWith(QLatin1String("location"), Qt::CaseInsensitive))
        {
            location = QUrl(line.mid(line.indexOf(':') + 1).trimmed().toString()); //TODO fromLocalFile()?
            if (!location.isValid())
                return 0;
        }
        else if (line.startsWith(QLatin1String("server"), Qt::CaseInsensitive))
        {
            server = line.mid(line.indexOf(':') + 1).trimmed().toString();
            if (server.length() == 0)
                return 0;

        }
    }

    if (findRouter(location)) {
        return nullptr;
    } else {
        qDebug() << "Detected IGD " << server;
        // everything OK, make a new UPnPRouter
        return new UPnPRouter(server, location);
    }
}

}
