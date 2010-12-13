#ifndef FETIONSIPNOTIFIER_H
#define FETIONSIPNOTIFIER_H

#include <QThread>

#include <openfetion.h>

class FetionSipNotifier : public QThread
{
    Q_OBJECT
    public:
        FetionSipNotifier( FetionSip* sip, User* user );
        virtual ~FetionSipNotifier();
        void run();
    Q_SIGNALS:
        void newThreadEntered( FetionSip* sip, User* user );
        void messageReceived( const QString& sId, const QString& msgContent, const QString& qsipuri );
        void presenceChanged( const QString& sId, StateType state );
    private:
        FetionSip* sip;
        User* user;
};

#endif // FETIONSIPNOTIFIER_H
