#include "dnspacket.h"

#include "datatransformer.h"

DnsPacket::DnsPacket()
{
    clear();
}

void DnsPacket::clear()
{
    m_id        = 0;
    m_flags     = 0;

    m_questions.clear();
    m_answers.clear();
    m_accessRights.clear();
    m_addInform.clear();
}

void DnsPacket::setFlags(DnsFlags flags)
{
    m_flags = flags;
}

quint16 DnsPacket::generateId()
{
    m_id = rand() % USHRT_MAX ;
    return m_id;
}

void DnsPacket::addDomainName(const QString &domainName)
{
    m_questions.append( DnsQuestion(domainName) );
}

QByteArray DnsPacket::toByteArray() const
{
    quint16 questionsCount      = m_questions.size();
    quint16 answersCount        = m_answers.size();
    quint16 accessRightsCount   = m_accessRights.size();
    quint16 addInformCount      = m_addInform.size();
    addInformCount = 1;

    QByteArray result;
    QDataStream( &result, QIODevice::WriteOnly ) << m_id << (quint16)m_flags << questionsCount << answersCount
                                                 << accessRightsCount << addInformCount;

    auto quesVecToBytes = []( const QVector<DnsQuestion> &source ){
        QByteArray _result;
        for ( const auto & rr : source ){
            _result += rr.toBytes();
        }
        return _result;
    };

    auto resVecToBytes = []( const QVector<DnsResourceRecord> &source ){
        QByteArray _result;
        for ( const auto & rr : source ){
            _result += rr.toBytes();
        }
        return _result;
    };

    result += quesVecToBytes( m_questions );
    result += resVecToBytes( m_answers );
    result += resVecToBytes( m_accessRights );
    result += resVecToBytes( m_addInform );

    result += QByteArray::fromHex("000029F000000000000000");

    return result;
}

DnsPacket DnsPacket::makeRequestPacket(const QString &domainName)
{
    return makeRequestPacket( QList< QString >{ domainName } );
}

DnsPacket DnsPacket::makeRequestPacket(const QList<QString> &domainNames)
{
    DnsPacket result;
    result.generateId();
    result.setFlags( DnsFlag::RD );

    for ( const auto & domain : domainNames ){
        result.addDomainName( domain );
    }

    return result;
}

DnsPacket DnsPacket::fromBytesArary( QByteArray source )
{
    DnsPacket result;

    quint16 questionsCount      = 0;
    quint16 answersCount        = 0;
    quint16 accessRightsCount   = 0;
    quint16 addInformCount      = 0;
    quint16 flags               = 0;

    DnsDataStream dnsStream( &source );
    dnsStream.number16( result.m_id ).number16( flags ).number16( questionsCount )
             .number16( answersCount ).number16( accessRightsCount ).number16( addInformCount );

    result.m_flags = (DnsFlags)flags;

    auto bytesToQuestionsVec = [ &dnsStream ]( int _count ){
        QVector<DnsQuestion> _result;
        for ( int i = 0; i < _count; ++i ){
            DnsQuestion question;
            dnsStream.question( question );
            _result << std::move( question );
        }
        return _result;
    };

    auto bytesToResRecVec = [ &dnsStream ]( int _count ){
        QVector<DnsResourceRecord> _result;
        for ( int i = 0; i < _count; ++i ){
            DnsResourceRecord resRec;
            dnsStream.resourceRecord( resRec );
            _result << std::move( resRec );
        }
        return _result;
    };

    result.m_questions      = bytesToQuestionsVec( questionsCount );
    result.m_answers        = bytesToResRecVec( answersCount );
    result.m_accessRights   = bytesToResRecVec( accessRightsCount );
    result.m_addInform      = bytesToResRecVec( addInformCount );

    return result;
}

const QVector<DnsQuestion> & DnsPacket::questions() const
{
    return m_questions;
}

void DnsPacket::setQuestions(const QVector<DnsQuestion> &questions)
{
    m_questions = questions;
}

void DnsPacket::addQuestion(const DnsQuestion &quession)
{
    m_questions.append( quession );
}

const QVector<DnsResourceRecord> & DnsPacket::answers() const
{
    return m_answers;
}

QDataStream &operator <<(QDataStream &inStream, const DnsPacket &source)
{
    inStream << source.toByteArray();
    return inStream;
}

