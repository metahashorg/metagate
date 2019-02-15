#ifndef DNSPACKET_H
#define DNSPACKET_H

#include <QVector>
#include <QList>
#include <QByteArray>
#include <QFlags>
#include <QDataStream>
#include "resourcerecord.h"

enum class DnsFlag {
    Clear   = 0x0,
    RA      = 0x80,
    RD      = 0x100,
    TC      = 0x200,
    AA      = 0x400,
    StandartRequest     = 0x000,
    InversivRequese     = 0x800,
    ServerStateRequest  = 0x1000,
    QR      = 0x8000,
    MyFlag  = 0x120,
};

class DnsPacket
{
public:
    Q_DECLARE_FLAGS(DnsFlags, DnsFlag)

    DnsPacket();
    DnsPacket( const DnsPacket & other ) = default;
    DnsPacket(       DnsPacket &&other ) = default;

    DnsPacket& operator=(const DnsPacket &) = default;

    void clear();

    void        setFlags( DnsFlags flags );
    quint16     generateId();
    void        addDomainName( const QString &domainName );
    QByteArray  toByteArray() const;

// static
    static DnsPacket makeRequestPacket( const QString & domainName );
    static DnsPacket makeRequestPacket( const QList< QString > & domainNames );
    static DnsPacket fromBytesArary(QByteArray source );

    const QVector<DnsQuestion> &questions() const;
    void setQuestions(const QVector<DnsQuestion> &questions);
    void addQuestion(const DnsQuestion &quession);

    const QVector<DnsResourceRecord> &answers() const;

protected:
    quint16                     m_id;
    DnsFlags                    m_flags;

    QVector< DnsQuestion >       m_questions;
    QVector< DnsResourceRecord > m_answers;
    QVector< DnsResourceRecord > m_accessRights;
    QVector< DnsResourceRecord > m_addInform;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(DnsPacket::DnsFlags)

QDataStream &operator <<( QDataStream & inStream, const DnsPacket &source );

#endif // DNSPACKET_H
