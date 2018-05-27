#ifndef DATATRANSFORMER_H
#define DATATRANSFORMER_H

#include <QByteArray>
#include <QString>
#include "resourcerecord.h"

class DnsDataStream
{
public:
    DnsDataStream( QByteArray *dnsPacket );

    DnsDataStream& domainName( QString &domain );
    DnsDataStream& domainName( QByteArray &domain );
    DnsDataStream& number16( quint16 &number );
    DnsDataStream& number32( quint32 &number );
    DnsDataStream& question( DnsQuestion &question );
    DnsDataStream& resourceData(DnsResourceRecord &resData);
    DnsDataStream& resourceRecord( DnsResourceRecord &resRec );

//static
    static QByteArray transformDomain( const QByteArray & domain );
    static QByteArray transformDomain( const QString    & domain );

    static QByteArray fromDomainToBytes(const QByteArray & domain, int *posOfEnd = nullptr );
    static QString    fromDomainToString( const QByteArray & domain, int *posOfEnd = nullptr );

    static QByteArray fromDomainToBytes(const QByteArray & packet, int start, int *posOfEnd = nullptr );
    static QString    fromDomainToString( const QByteArray & packet, int start, int *posOfEnd = nullptr );

protected:
    QByteArray *m_dnsPacket;
    int         m_position;
};

#endif // DATATRANSFORMER_H
