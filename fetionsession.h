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
        explicit FetionSession( QObject* parent = 0 );
        virtual ~FetionSession();
        void setLoginInformation( const QString& accountId, const QString& password );
        void login();
        void logout();
        bool isLoggedIn() const;
        QString accountId() const;
        void setVisibility( bool isVisible );
        void setStatusMessage( const QString& status );
        void sendClientMessage( const QString& sId, const QString& message );
        void sendMobilePhoneMessage( const QString& sId, const QString& message );

    private Q_SLOTS:
//         void disconnect();
//         void startLoginRequest();
        /** */
//         bool isConnected() const;
//         Conversation* getConversation( const QString& sId );
//     public Q_SLOTS:
//         void connect( const QString& password );
//         void disconnect();
//         void setStatus( const Kopete::OnlineStatus& status );
    Q_SIGNALS:
//         void connectionSuccessed();
//         void connectionFailed();
        void contactStatusChanged( const QString& sId, const Kopete::OnlineStatus& status );
        void gotContact( const QString& contactId, const QString& contactName, int groupId );
        void gotGroup( int groupId, const QString& groupName );
        void gotMessage( const QString& sId, const QString& msgContent );
        void statusChanged( const Kopete::OnlineStatus& status );
    private Q_SLOTS:
        void slotMessageReceived( const QString& sId, const QString& msgContent, const QString& qsipuri );
        void slotNewThreadEntered( FetionSip* sip, User* user );
        void slotPresenceChanged( const QString& sId, StateType state );
    private:
        bool m_isConnected;
        QString m_accountId;
        QString m_password;
        FetionSipNotifier* notifier;
        User* me;
        Config* config;
        struct userlist* ul;
//         QHash<QString, Conversation*> contactConvHash;
};

#endif // FETIONSESSION_H
