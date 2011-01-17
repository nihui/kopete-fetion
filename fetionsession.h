#ifndef FETIONSESSION_H
#define FETIONSESSION_H

#include <QObject>

#include <kopeteonlinestatus.h>
#include <QNetworkAccessManager>
#include <QQueue>
#include <QHash>

class FetionBuddyInfo {
    public:
        QString sid;
        QString sipuri;
        QString mobileno;
        QString nick;
        QString smsg;
};

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
        QString accountId() const;
        void setVisibility( bool isVisible );
        void setStatusMessage( const QString& status );
        void sendClientMessage( const QString& id, const QString& message );
        void sendMobilePhoneMessage( const QString& id, const QString& message );
        void sendMobilePhoneMessageToMyself( const QString& message );

    private Q_SLOTS:
        void getSystemConfigFinished();
        void ssiAuthFinished();
        void getCodePicFinished();
        void handleSipcRegisterReplyEvent( const FetionSipEvent& sipEvent );
        void handleSipEvent( const FetionSipEvent& sipEvent );

    Q_SIGNALS:
        void loginSuccessed();
        void gotBuddyList( const QString& buddyListName );
        void gotBuddy( const QString& id, const QString& buddyListName );
        void buddyStatusUpdated( const QString& id, const QString& statusId );
        void buddyInfoUpdated( const QString& id, const FetionBuddyInfo& buddyInfo );
        void gotMessage( const QString& id, const QString& message );

        void contactStatusChanged( const QString& sId, const Kopete::OnlineStatus& status );
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

        QString m_sipUri;
        QString m_userId;
        QString m_from;

        QHash<int, QString> m_buddyListIdNames;
        QHash<QString, QString> m_buddyIdSipUri;
};

#endif // FETIONSESSION_H
