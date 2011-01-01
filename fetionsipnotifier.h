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
    Q_SIGNALS:
        void sipEventReceived( const FetionSipEvent& sipEvent );
    private Q_SLOTS:
        void slotReadyRead();
    private:
        QTcpSocket m_socket;
};

#endif // FETIONSIPNOTIFIER_H
