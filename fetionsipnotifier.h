#ifndef FETIONSIPNOTIFIER_H
#define FETIONSIPNOTIFIER_H

#include <QObject>
#include <QTcpSocket>

#include <openfetion.h>

class FetionSipNotifier : public QObject
{
    Q_OBJECT
    public:
        FetionSipNotifier( FetionSip* sip, User* user, QObject* parent = 0 );
        virtual ~FetionSipNotifier();
//         void run();
    Q_SIGNALS:
        void newThreadEntered( FetionSip* sip, User* user );
        void messageReceived( const QString& sId, const QString& msgContent, const QString& qsipuri );
        void presenceChanged( const QString& sId, StateType state );
    private Q_SLOTS:
        void slotReadyRead();
    private:
        void handleSipMessage( SipMsg* sipmsg );
    private:
        FetionSip* sip;
        User* user;
        QTcpSocket m_socket;
};

#endif // FETIONSIPNOTIFIER_H
