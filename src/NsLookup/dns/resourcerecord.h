#ifndef RESOURCERECORD_H
#define RESOURCERECORD_H

#include <QByteArray>
#include <QString>
#include <QHostAddress>
#include <QDebug>
#include <QVariant>

typedef QPair<quint16, QString> Preference_Domain;

Q_DECLARE_METATYPE( Preference_Domain )
Q_DECLARE_METATYPE( QHostAddress )

namespace RRTypes {
enum  RRType{
    Invalid     = 0,
    A           = 1,    // IP
    NS          = 2,    // сервер DNS
    CNAME       = 5,    // каноническое имя
    PTR         = 12,   // запись указателя
    HINFO       = 13,   // информация о хосте
    MX          = 15,   // запись об обмене почтой
    AXFR        = 252,  // запрос на пепедачу зоны
    ANY         = 255   // запрос всех записей
};
}

namespace RRClasses {
enum RRClass{
    Invalid     = 0,
    Internet    = 1     // Internet
};
}

class DnsQuestion
{
public:
    DnsQuestion();
    DnsQuestion( const QString &domain, RRTypes::RRType type = RRTypes::A, RRClasses::RRClass cls = RRClasses::Internet );
    DnsQuestion( const DnsQuestion & other );
    DnsQuestion(       DnsQuestion &&other ) = default;
    virtual ~DnsQuestion(){}

    DnsQuestion &operator =( const DnsQuestion&  other );
    DnsQuestion &operator =(       DnsQuestion&& other ) = default;

    virtual void clear();
    virtual QByteArray toBytes() const;


    QByteArray domainName() const;
    void setDomainName(const QByteArray &domainName);

    RRTypes::RRType type() const;
    void setType(const RRTypes::RRType &type);

    RRClasses::RRClass reqClass() const;
    void setReqClass(const RRClasses::RRClass &reqClass);

//static
    static DnsQuestion getName( const QString &ip );
    static DnsQuestion getIp( const QString &domainName );
    static DnsQuestion getCanonicalName( const QString &domainName);
    static DnsQuestion getMailExchanger( const QString &domainName);

protected:
    QByteArray          m_domainName;
    RRTypes::RRType     m_type;
    RRClasses::RRClass  m_class;
};

class DnsResourceRecord : public DnsQuestion
{
public:
    DnsResourceRecord();
    DnsResourceRecord( const DnsResourceRecord & dnsResRec ) = default;
    DnsResourceRecord(       DnsResourceRecord &&dnsResRec ) = default;
    DnsResourceRecord &operator = ( const DnsResourceRecord&  other ) = default;
    DnsResourceRecord &operator = (       DnsResourceRecord&& other ) = default;

    void clear() override;
    QByteArray toBytes() const override;

    quint32 ttl() const;
    void setTtl(const quint32 &ttl);

    void setVariantResourceData(const QVariant & resData);
    QVariant variantResourceData() const;

    QString  toString() const;

protected:
    quint32         m_ttl;
    QVariant        m_resourceData;
};

#endif // RESOURCERECORD_H
