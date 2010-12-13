#ifndef FETIONSESSION_H
#define FETIONSESSION_H

#include <QObject>

#include <kopeteonlinestatus.h>
#include <QHash>
#include <openfetion.h>

class FetionSipNotifier;

class FetionSession : public QObject
{
    Q_OBJECT
    public:
        explicit FetionSession( const QString& accountId );
        virtual ~FetionSession();
        bool isConnected() const;
        Conversation* getConversation( const QString& sId );
    public Q_SLOTS:
        void connect( const QString& password );
        void disconnect();
        void setStatus( const Kopete::OnlineStatus& status );
    Q_SIGNALS:
        void connectionSuccessed();
        void connectionFailed();
        void contactStatusChanged( const QString& sId, const Kopete::OnlineStatus& status );
        void gotContact( const Contact* contact );
        void gotGroup( const Group* group );
        void gotMessage( const QString& sId, const QString& msgContent );
        void statusChanged( const Kopete::OnlineStatus& status );
    private Q_SLOTS:
        void slotMessageReceived( const QString& sId, const QString& msgContent, const QString& qsipuri );
        void slotNewThreadEntered( FetionSip* sip, User* user );
        void slotPresenceChanged( const QString& sId, StateType state );
    private:
        bool m_isConnected;
        QString m_accountId;
        FetionSipNotifier* notifier;
        User* me;
        Config* config;
        struct userlist* ul;
        QHash<QString, Conversation*> contactConvHash;
};

#endif // FETIONSESSION_H
