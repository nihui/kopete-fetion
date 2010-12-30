#ifndef FETIONSIPNOTIFIER_H
#define FETIONSIPNOTIFIER_H

#include <QObject>
#include <QTcpSocket>

class FetionSipEvent;

class FetionSipNotifier : public QObject
{
    Q_OBJECT
    public:
        FetionSipNotifier( QObject* parent = 0 );
        virtual ~FetionSipNotifier();
        void connectToHost( const QString& hostAddress, int port );
        void sendSipEvent( const FetionSipEvent& sipEvent );
//         void run();
    Q_SIGNALS:
        void messageReceived( const QString& sId, const QString& msgContent, const QString& qsipuri );
//         void presenceChanged( const QString& sId, StateType state );
    private Q_SLOTS:
        void slotReadyRead();
    private:
        QTcpSocket m_socket;
};

#endif // FETIONSIPNOTIFIER_H
