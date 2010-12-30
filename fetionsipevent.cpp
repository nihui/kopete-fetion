#include "fetionsipevent.h"

#include <QDebug>

FetionSipEvent::FetionSipEvent( SipType sipType, EventType eventType )
{
    m_sipType = sipType;
    m_eventType = eventType;
}

FetionSipEvent::FetionSipEvent( const QString& typeStr, const QString& headerStr )
{
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

FetionSipEvent::EventType FetionSipEvent::eventType() const
{
    return m_eventType;
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

QString FetionSipEvent::toString() const
{
    QString str;
    str += FetionSipEvent::sipTypeToString( m_sipType ) + ' ' + m_typeAddition + "\r\n";//" fetion.com.cn SIP-C/4.0\r\n";

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
        "R",            // SipRegister
        "S",            // SipService
        "SUB",          // SipSubscription
        "NB",           // SipNotification
        "I",            // SipInvitation
        "IN",           // SipIncoming
        "O",            // SipOption
        "M",            // SipMessage
        "SIP-C/4.0",    // SipSipc_4_0
        "A"             // SipAcknowledge
    };

    if ( sipType < 0 || sipType > 11 ) {
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
        return FetionSipEvent::SipSubScription;
    if ( sipTypeStr == QLatin1String( "NB" ) )
        return FetionSipEvent::SipNotification;
    if ( sipTypeStr == QLatin1String( "I" ) )
        return FetionSipEvent::SipInvitation;
    if ( sipTypeStr == QLatin1String( "IN" ) )
        return FetionSipEvent::SipIncoming;
    if ( sipTypeStr == QLatin1String( "O" ) )
        return FetionSipEvent::SipOption;
    if ( sipTypeStr == QLatin1String( "M" ) )
        return FetionSipEvent::SipMessage;
    if ( sipTypeStr == QLatin1String( "SIP-C/4.0" ) )
        return FetionSipEvent::SipSipc_4_0;
    if ( sipTypeStr == QLatin1String( "A" ) )
        return FetionSipEvent::SipAcknowledge;
    return FetionSipEvent::SipUnknown;
}

QString FetionSipEvent::eventTypeToString( EventType eventType )
{
    static const QString eventTypeString[] = {
        "PresenceV4",               // EventPresence
        "SetPresenceV4",            // EventSetPresence
        "",                         // TODO EventContact
        "Conversation",             // EventConversation
        "CatMsg",                   // EventCatMessage
        "SendCatSMS",               // EventSendCatMessage
        "StartChat",                // EventStartChat
        "InviteBuddy",              // EventInviteBuddy
        "GetContactInfoV4",         // EventGetContactInfo
        "CreateBuddyList",          // EventCreateBuddyList
        "DeleteBuddyList",          // EventDeleteBuddyList
        "SetContactInfoV4",         // EventSetContactInfo
        "SetUserInfoV4",            // EventSetUserInfo
        "SetBuddyListInfo",         // EventSetBuddyListInfo
        "DeleteBuddyV4",            // EventDeleteBuddy
        "AddBuddyV4",               // EventAddBuddy
        "KeepAlive",                // EventKeepAlive
        "DirectSMS",                // EventDirectSMS
        "SendDirectCatSMS",         // EventSendDirectCatSMS
        "HandleContactRequestV4",   // EventHandleContactRequest
        "PGGetGroupList",           // EventPGGetGroupList
        "PGGetGroupInfo",           // EventPGGetGroupInfo
        "PGGetGroupMembers",        // EventPGGetGroupMembers
        "PGSendCatSMS",             // EventPGSendCatSMS
        "PGPresence"                // EventPGPresence
    };

    if ( eventType < 0 || eventType > 24 ) {
        qWarning() << "Unexpected event type" << (int)eventType;
        return QString();
    }
    return eventTypeString[ (int)eventType ];
}

FetionSipEvent::EventType FetionSipEvent::StringToEventType( const QString& eventTypeStr )
{
    if ( eventTypeStr == QLatin1String( "PresenceV4" ) )
        return FetionSipEvent::EventPresence;
    if ( eventTypeStr == QLatin1String( "SetPresenceV4" ) )
        return FetionSipEvent::EventSetPresence;
    if ( eventTypeStr == QLatin1String( "Conversation" ) )
        return FetionSipEvent::EventConversation;
    if ( eventTypeStr == QLatin1String( "CatMsg" ) )
        return FetionSipEvent::EventCatMessage;
    if ( eventTypeStr == QLatin1String( "SendCatSMS" ) )
        return FetionSipEvent::EventSendCatMessage;
    if ( eventTypeStr == QLatin1String( "StartChat" ) )
        return FetionSipEvent::EventStartChat;
    if ( eventTypeStr == QLatin1String( "InviteBuddy" ) )
        return FetionSipEvent::EventInviteBuddy;
    if ( eventTypeStr == QLatin1String( "GetContactInfoV4" ) )
        return FetionSipEvent::EventGetContactInfo;
    if ( eventTypeStr == QLatin1String( "CreateBuddyList" ) )
        return FetionSipEvent::EventCreateBuddyList;
    if ( eventTypeStr == QLatin1String( "DeleteBuddyList" ) )
        return FetionSipEvent::EventDeleteBuddyList;
    if ( eventTypeStr == QLatin1String( "SetContactInfoV4" ) )
        return FetionSipEvent::EventSetContactInfo;
    if ( eventTypeStr == QLatin1String( "SetUserInfoV4" ) )
        return FetionSipEvent::EventSetUserInfo;
    if ( eventTypeStr == QLatin1String( "SetBuddyListInfo" ) )
        return FetionSipEvent::EventSetBuddyListInfo;
    if ( eventTypeStr == QLatin1String( "DeleteBuddyV4" ) )
        return FetionSipEvent::EventDeleteBuddy;
    if ( eventTypeStr == QLatin1String( "AddBuddyV4" ) )
        return FetionSipEvent::EventAddBuddy;
    if ( eventTypeStr == QLatin1String( "KeepAlive" ) )
        return FetionSipEvent::EventKeepAlive;
    if ( eventTypeStr == QLatin1String( "DirectSMS" ) )
        return FetionSipEvent::EventDirectSMS;
    if ( eventTypeStr == QLatin1String( "SendDirectCatSMS" ) )
        return FetionSipEvent::EventSendDirectCatSMS;
    if ( eventTypeStr == QLatin1String( "HandleContactRequestV4" ) )
        return FetionSipEvent::EventHandleContactRequest;
    if ( eventTypeStr == QLatin1String( "PGGetGroupList" ) )
        return FetionSipEvent::EventPGGetGroupList;
    if ( eventTypeStr == QLatin1String( "PGGetGroupInfo" ) )
        return FetionSipEvent::EventPGGetGroupInfo;
    if ( eventTypeStr == QLatin1String( "PGGetGroupMembers" ) )
        return FetionSipEvent::EventPGGetGroupMembers;
    if ( eventTypeStr == QLatin1String( "PGSendCatSMS" ) )
        return FetionSipEvent::EventPGSendCatSMS;
    if ( eventTypeStr == QLatin1String( "PGPresence" ) )
        return FetionSipEvent::EventPGPresence;
    return FetionSipEvent::EventUnknown;
}
