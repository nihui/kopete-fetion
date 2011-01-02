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
            SipAcknowledge              = 1,
            SipBENotify                 = 2,
            SipBye                      = 3,
            SipCancel                   = 4,
            SipInfo                     = 5,
            SipInvite                   = 6,
            SipMessage                  = 7,
            SipNegotiate                = 8,
            SipNotify                   = 9,
            SipOption                   = 10,
            SipRefer                    = 11,
            SipRegister                 = 12,
            SipService                  = 13,
            SipSubScribe                = 14,
            SipSipc_4_0                 = 15
        } SipType;

        explicit FetionSipEvent( SipType sipType );
        explicit FetionSipEvent( const QString& typeStr, const QString& headerStr );
        virtual ~FetionSipEvent();
        SipType sipType() const;
        void setTypeAddition( const QString& addition );
        QString typeAddition() const;
        void addHeader( const QString& key, const QString& value );
        QStringList getValues( const QString& key ) const;
        QString getFirstValue( const QString& key ) const;
        void setContent( const QString& content );
        QString getContent() const;
        QString toString() const;
        static QString sipTypeToString( SipType sipType );
        static SipType stringToSipType( const QString& sipTypeStr );
        static void resetCallid();
        static int nextCallid();
    private:
        SipType m_sipType;
        QString m_typeAddition;
        QList<QPair<QString, QString> > m_header;
        QString m_content;
        static int m_callid;
};

#endif // FETIONSIPEVENT_H
