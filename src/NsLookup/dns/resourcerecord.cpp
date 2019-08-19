#include "resourcerecord.h"

#include "datatransformer.h"
#include <QDataStream>
#include <QStringList>

DnsQuestion::DnsQuestion()
{
    clear();
}

DnsQuestion::DnsQuestion(const QString &domain, RRTypes::RRType type, RRClasses::RRClass cls)
{
    m_domainName    = domain.toUtf8();
    m_type          = type;
    m_class         = cls;
}

DnsQuestion::DnsQuestion(const DnsQuestion &other)
{
    m_domainName    = other.m_domainName;
    m_type          = other.m_type;
    m_class         = other.m_class;
}

DnsQuestion &DnsQuestion::operator =(const DnsQuestion &other)
{
    m_domainName    = other.m_domainName;
    m_type          = other.m_type;
    m_class         = other.m_class;
    return *this;
}

void DnsQuestion::clear()
{
    m_domainName.clear();
    m_type  = RRTypes::Invalid;
    m_class = RRClasses::Invalid;
}

QByteArray DnsQuestion::toBytes() const
{
    QByteArray result = DnsDataStream::transformDomain( m_domainName );
    QDataStream( &result, QIODevice::Append ) << (quint16)m_type << (quint16)m_class;

    return result;
}

QByteArray DnsQuestion::domainName() const
{
    return m_domainName;
}

void DnsQuestion::setDomainName(const QByteArray &domainName)
{
    m_domainName = domainName;
}

RRTypes::RRType DnsQuestion::type() const
{
    return m_type;
}

void DnsQuestion::setType(const RRTypes::RRType &type)
{
    m_type = type;
}

RRClasses::RRClass DnsQuestion::reqClass() const
{
    return m_class;
}

void DnsQuestion::setReqClass(const RRClasses::RRClass &reqClass)
{
    m_class = reqClass;
}

DnsQuestion DnsQuestion::getName(const QString &ip)
{
    QStringList ipList = ip.split('.', QString::SkipEmptyParts);
    Q_ASSERT( ipList.size() == 4 );
    std::swap( ipList[0], ipList[3] );
    std::swap( ipList[1], ipList[2] );
    QString arpinetDomain = ipList.join('.') + QString::fromUtf8(".in-addr.arpa.");
    return DnsQuestion( arpinetDomain, RRTypes::PTR );
}

DnsQuestion DnsQuestion::getIp(const QString &domainName)
{
    return DnsQuestion( domainName, RRTypes::A );
}

DnsQuestion DnsQuestion::getCanonicalName(const QString &domainName)
{
    return DnsQuestion( domainName, RRTypes::CNAME );
}

DnsQuestion DnsQuestion::getMailExchanger(const QString &domainName)
{
    return DnsQuestion( domainName, RRTypes::MX );
}



/////////////////////////////////////////////////
/// \brief DnsResourceRecord
///

DnsResourceRecord::DnsResourceRecord()
{
    clear();
}

void DnsResourceRecord::clear()
{
    DnsQuestion::clear();
    m_ttl = 0;
    m_resourceData.clear();
}

QByteArray DnsResourceRecord::toBytes() const
{
    QByteArray res = DnsQuestion::toBytes();
    //QDataStream( &res, QIODevice::Append ) << m_ttl << quint16( m_resourceData.size() );

    return res;// + m_resourceData;
}

void DnsResourceRecord::setVariantResourceData(const QVariant &resData)
{
    m_resourceData = resData;
}

quint32 DnsResourceRecord::ttl() const
{
    return m_ttl;
}

void DnsResourceRecord::setTtl(const quint32 &ttl)
{
    m_ttl = ttl;
}

QVariant DnsResourceRecord::variantResourceData() const
{
    return m_resourceData;
}

QString DnsResourceRecord::toString() const
{
    switch (m_type) {
    case RRTypes::A     : return variantResourceData().value<QHostAddress>().toString();
    case RRTypes::PTR   : return variantResourceData().toString();
    case RRTypes::CNAME : return variantResourceData().toString();
    case RRTypes::MX    :
    {
                          Preference_Domain data = variantResourceData().value<Preference_Domain>();
                          return QString::number(data.first) + " " + data.second;
    }
    default:
        break;
    }
    return QString();
}
