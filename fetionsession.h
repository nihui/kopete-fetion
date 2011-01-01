#ifndef FETIONSESSION_H
#define FETIONSESSION_H

#include <QObject>

#include <kopeteonlinestatus.h>
#include <QHash>
#include <QNetworkAccessManager>
// #include <QNetworkReply>

class FetionSipEvent;
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
//         void replyFinished( QNetworkReply* reply );
        void getSystemConfigFinished();
        void ssiAuthFinished();
        void getCodePicFinished();
        void handleSipEvent( const FetionSipEvent& sipEvent );

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
//         void slotMessageReceived( const QString& sId, const QString& msgContent, const QString& qsipuri );
//         void slotNewThreadEntered( FetionSip* sip, User* user );
//         void slotPresenceChanged( const QString& sId, StateType state );
    private:
        bool m_isConnected;
        QString m_accountId;
        QString m_password;
        QNetworkAccessManager* manager;

        QString picid;
        QString vcode;
        QString algorithm;
        QString type;
        FetionSipNotifier* notifier;

        QString m_ssiAppSignInV2Uri;
        QString m_getPicCodeUri;
        QString m_sipcProxyAddress;
        QString m_sipcSslProxyAddress;
        QString m_httpTunnelAddress;
        QString m_nouce;

        QString m_userId;
        QString m_from;

//         QHash<QString, Conversation*> contactConvHash;
};

#endif // FETIONSESSION_H
