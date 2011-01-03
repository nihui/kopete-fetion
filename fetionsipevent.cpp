#include "fetionsipevent.h"

#include <QDebug>

int FetionSipEvent::m_callid = 0;

FetionSipEvent::FetionSipEvent( SipType sipType )
{
    m_sipType = sipType;
    m_typeAddition = "fetion.com.cn SIP-C/4.0";
}

FetionSipEvent::FetionSipEvent( const QString& typeStr, const QString& headerStr )
{
//     qWarning() << "construct sip event" << typeStr << headerStr;
    m_sipType = FetionSipEvent::stringToSipType( typeStr.section( QLatin1Char( ' ' ), 0, 0, QString::SectionSkipEmpty ) );
    m_typeAddition = typeStr.section( QLatin1Char( ' ' ), 1, -1, QString::SectionSkipEmpty );

    QStringList list = headerStr.split( "\r\n", QString::SkipEmptyParts );
    QStringList::ConstIterator it = list.constBegin();
    QStringList::ConstIterator end = list.constEnd();
    while ( it != end ) {
        QStringList keyvalue = (*it).split( ": ", QString::SkipEmptyParts );
        m_header << QPair<QString, QString>( keyvalue.at( 0 ), keyvalue.at( 1 ) );
        ++it;
    }
}

FetionSipEvent::~FetionSipEvent()
{
}

FetionSipEvent::SipType FetionSipEvent::sipType() const
{
    return m_sipType;
}

void FetionSipEvent::setTypeAddition( const QString& addition )
{
    m_typeAddition = addition;
}

QString FetionSipEvent::typeAddition() const
{
    return m_typeAddition;
}

void FetionSipEvent::addHeader( const QString& key, const QString& value )
{
    m_header << QPair<QString, QString>( key, value );
}

QStringList FetionSipEvent::getValues( const QString& key ) const
{
    QStringList values;

    QList<QPair<QString, QString> >::ConstIterator it = m_header.constBegin();
    QList<QPair<QString, QString> >::ConstIterator end = m_header.constEnd();
    while ( it != end ) {
        if ( (*it).first == key )
            values << (*it).second;
        ++it;
    }

    return values;
}

QString FetionSipEvent::getFirstValue( const QString& key ) const
{
    QList<QPair<QString, QString> >::ConstIterator it = m_header.constBegin();
    QList<QPair<QString, QString> >::ConstIterator end = m_header.constEnd();
    while ( it != end ) {
        if ( (*it).first == key )
            return (*it).second;
        ++it;
    }
    return QString();
}

void FetionSipEvent::setContent( const QString& content )
{
    m_content = content;
}

QString FetionSipEvent::getContent() const
{
    return m_content;
}

QString FetionSipEvent::toString() const
{
    QString str;
    str += FetionSipEvent::sipTypeToString( m_sipType ) + ' ' + m_typeAddition + "\r\n";

    QList<QPair<QString, QString> >::ConstIterator it = m_header.constBegin();
    QList<QPair<QString, QString> >::ConstIterator end = m_header.constEnd();
    while ( it != end ) {
        str += (*it).first + ": " + (*it).second + "\r\n";
        ++it;
    }

    if ( m_content.isEmpty() ) {
        str += "\r\n";
    }
    else {
        str += "L: " + QString::number( m_content.size() ) + "\r\n\r\n";
        str += m_content;
    }

    return str;
}

QString FetionSipEvent::sipTypeToString( SipType sipType )
{
    static const QString sipTypeString[] = {
        "",             // SipUnknown
        "A",            // SipAcknowledge
        "BN",           // SipBENotify
        "B",            // SipBye
        "",             // SipCancel TODO
        "IN",           // SipInfo
        "I",            // SipInvite
        "M",            // SipMessage
        "",             // SipNegotiate TODO
        "NB",           // SipNotify
        "O",            // SipOption
        "",             // SipRefer TODO
        "R",            // SipRegister
        "S",            // SipService
        "SUB",          // SipSubscribe
        "SIP-C/4.0",    // SipSipc_4_0
    };

    if ( sipType < 0 || sipType > 15 ) {
        qWarning() << "Unexpected sip type" << (int)sipType;
        return QString();
    }
    return sipTypeString[ (int)sipType ];
}

FetionSipEvent::SipType FetionSipEvent::stringToSipType( const QString& sipTypeStr )
{
    if ( sipTypeStr == QLatin1String( "R" ) )
        return FetionSipEvent::SipRegister;
    if ( sipTypeStr == QLatin1String( "S" ) )
        return FetionSipEvent::SipService;
    if ( sipTypeStr == QLatin1String( "SUB" ) )
        return FetionSipEvent::SipSubScribe;
    if ( sipTypeStr == QLatin1String( "NB" ) )
        return FetionSipEvent::SipNotify;
    if ( sipTypeStr == QLatin1String( "I" ) )
        return FetionSipEvent::SipInvite;
    if ( sipTypeStr == QLatin1String( "IN" ) )
        return FetionSipEvent::SipInfo;
    if ( sipTypeStr == QLatin1String( "O" ) )
        return FetionSipEvent::SipOption;
    if ( sipTypeStr == QLatin1String( "M" ) )
        return FetionSipEvent::SipMessage;
    if ( sipTypeStr == QLatin1String( "SIP-C/4.0" ) )
        return FetionSipEvent::SipSipc_4_0;
    if ( sipTypeStr == QLatin1String( "A" ) )
        return FetionSipEvent::SipAcknowledge;
    if ( sipTypeStr == QLatin1String( "BN" ) )
        return FetionSipEvent::SipBENotify;
    return FetionSipEvent::SipUnknown;
}

void FetionSipEvent::resetCallid()
{
    FetionSipEvent::m_callid = 0;
}

int FetionSipEvent::nextCallid()
{
    return FetionSipEvent::m_callid++;
}
