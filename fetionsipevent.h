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
            SipUnknown                  = -1,
            SipAcknowledge              = 0,
            SipBENotify                 = 1,
            SipBye                      = 2,
            SipCancel                   = 3,
            SipInfo                     = 4,
            SipInvite                   = 5,
            SipMessage                  = 6,
            SipNegotiate                = 7,
            SipNotify                   = 8,
            SipOption                   = 9,
            SipRefer                    = 10,
            SipRegister                 = 11,
            SipService                  = 12,
            SipSubScribe                = 13,
            SipSipc_4_0                 = 14
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
