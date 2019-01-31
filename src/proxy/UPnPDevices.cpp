#include "UPnPDevices.h"

#include "UPnPRouter.h"

#include <QNetworkDatagram>
#include "check.h"
#include "Log.h"
#include "SlotWrapper.h"

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

UPnPDevices::UPnPDevices(QObject *parent)
    : QObject(parent)
{
    CHECK(connect(&m_mcsocket, &QUdpSocket::readyRead,
                     this, &UPnPDevices::onReadyRead), "onReadyRead is not connected");
    CHECK(connect(&m_mcsocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
                     this, &UPnPDevices::onError), "onError is not connected");

    CHECK(m_mcsocket.bind(QHostAddress::AnyIPv4, mcSearchPort, QUdpSocket::ShareAddress), "bind error");
    CHECK(m_mcsocket.joinMulticastGroup(QHostAddress(mcSearchHost)), "joinMulticastGroup error");
}


UPnPDevices::~UPnPDevices()
{
    m_mcsocket.leaveMulticastGroup(QHostAddress(mcSearchHost));
}

void UPnPDevices::discover()
{
BEGIN_SLOT_WRAPPER
    LOG << "Start to find UPnP routers on the local network";

    m_mcsocket.writeDatagram(upnpSearchHeader, QHostAddress(mcSearchHost), mcSearchPort);
    m_mcsocket.writeDatagram(tr64SearchHeader, QHostAddress(mcSearchHost), mcSearchPort);
END_SLOT_WRAPPER
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
BEGIN_SLOT_WRAPPER
    while (m_mcsocket.hasPendingDatagrams()) {
        QNetworkDatagram dgram = m_mcsocket.receiveDatagram();
        UPnPRouter *r = parseResponse(dgram.data());
        if (r) {
            CHECK(connect(r, &UPnPRouter::xmlFileDownloaded,
                             this, [r, this](bool success) {
                updatingRouters.removeAll(r);
                if (success) {
                          QUrl location = r->location();
                          if (findRouter(location)) {
                              r->deleteLater();
                          } else {
                              routers.append(r);
                              emit discovered(r);
                          }
                } else {
                    r->deleteLater();
                }
            }), "");
            r->downloadXMLFile();
            updatingRouters.append(r);
        }
    }
END_SLOT_WRAPPER
}

void UPnPDevices::onError(QAbstractSocket::SocketError e)
{
BEGIN_SLOT_WRAPPER
    Q_UNUSED(e);
    LOG << "MCast error : " << m_mcsocket.errorString();
END_SLOT_WRAPPER
}

UPnPRouter *UPnPDevices::parseResponse(const QByteArray &data)
{
    const QString response = QString::fromLatin1(data);
    QVector<QStringRef> lines = response.splitRef("\r\n");
    QString server;
    QUrl location;

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
    for (int idx = 0; idx < lines.count() && !validDevice; idx++) {
        line = lines[idx];
        if ((line.contains(QLatin1String("ST:")) || line.contains(QLatin1String("NT:"))) && line.contains(QLatin1String("InternetGatewayDevice"))) {
            validDevice = true;
        }
    }
    if (!validDevice) {
        return nullptr;
    }

    // read all lines and try to find the server and location fields
    for (int i = 1; i < lines.count(); i++) {
        line = lines[i];
        if (line.startsWith(QLatin1String("location"), Qt::CaseInsensitive)) {
            location = QUrl(line.mid(line.indexOf(':') + 1).trimmed().toString());
            if (!location.isValid())
                return nullptr;
        } else if (line.startsWith(QLatin1String("server"), Qt::CaseInsensitive)) {
            server = line.mid(line.indexOf(':') + 1).trimmed().toString();
            if (server.isEmpty())
                return nullptr;
        }
    }

    if (findRouter(location)) {
        return nullptr;
    } else {
        // everything OK, make a new UPnPRouter
        return new UPnPRouter(server, location);
    }
}

}
