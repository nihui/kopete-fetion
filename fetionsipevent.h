#ifndef FETIONSIPEVENT_H
#define FETIONSIPEVENT_H

#include <QObject>
#include <QList>
#include <QPair>
#include <QStringList>


class FetionSipEvent : public QObject
{
    public:
        typedef enum {
            SipUnknown                  = 0,
            SipRegister                 = 1,
            SipService                  = 2,
            SipSubScription             = 3,
            SipNotification             = 4,
            SipInvitation               = 5,
            SipIncoming                 = 6,
            SipOption                   = 7,
            SipMessage                  = 8,
            SipSipc_4_0                 = 9,
            SipAcknowledge              = 10
        } SipType;

        typedef enum {
            EventUnknown                = -1,
            EventPresence               = 0,
            EventSetPresence            = 1,
            EventContact                = 2,
            EventConversation           = 3,
            EventCatMessage             = 4,
            EventSendCatMessage         = 5,
            EventStartChat              = 6,
            EventInviteBuddy            = 7,
            EventGetContactInfo         = 8,
            EventCreateBuddyList        = 9,
            EventDeleteBuddyList        = 10,
            EventSetContactInfo         = 11,
            EventSetUserInfo            = 12,
            EventSetBuddyListInfo       = 13,
            EventDeleteBuddy            = 14,
            EventAddBuddy               = 15,
            EventKeepAlive              = 16,
            EventDirectSMS              = 17,
            EventSendDirectCatSMS       = 18,
            EventHandleContactRequest   = 19,
            EventPGGetGroupList         = 20,
            EventPGGetGroupInfo         = 21,
            EventPGGetGroupMembers      = 22,
            EventPGSendCatSMS           = 23,
            EventPGPresence             = 24
        } EventType;

        explicit FetionSipEvent( SipType sipType, EventType eventType );
        explicit FetionSipEvent( const QString& typeStr, const QString& headerStr );
        virtual ~FetionSipEvent();
        SipType sipType() const;
        EventType eventType() const;
        void setTypeAddition( const QString& addition );
        QString typeAddition() const;
        void addHeader( const QString& key, const QString& value );
        QStringList getValues( const QString& key ) const;
        QString getFirstValue( const QString& key ) const;
        void setContent( const QString& content );
        QString toString() const;
        static QString sipTypeToString( SipType sipType );
        static SipType stringToSipType( const QString& sipTypeStr );
        static QString eventTypeToString( EventType eventType );
        static EventType StringToEventType( const QString& eventTypeStr );
        static void resetCallid();
        static int nextCallid();
    private:
        SipType m_sipType;
        EventType m_eventType;
        QString m_typeAddition;
        QList<QPair<QString, QString> > m_header;
        QString m_content;
        static int m_callid;
};

#endif // FETIONSIPEVENT_H
