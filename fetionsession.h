#ifndef FETIONSESSION_H
#define FETIONSESSION_H

#include <QObject>

#include <kopeteonlinestatus.h>
#include <QNetworkAccessManager>
#include <QQueue>
#include <QHash>
#include <QTimer>

class FetionBuddyInfo {
    public:
        QString sid;
        QString sipuri;
        QString mobileno;
        QString nick;
        QString smsg;
};

#define P_OFFLINE       -1
#define P_HIDDEN        0
#define P_AWAY          100
#define P_ONTHEPHONE    150
#define P_RIGHTBACK     300
#define P_ONLINE        400
#define P_OUTFORLAUNCH  500
#define P_BUSY          600
#define P_DONOTDISTURB  800
#define P_MEETING       850

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
        void setStatusMessage( const QString& statusMessage );
        void sendClientMessage( const QString& id, const QString& message );
        void sendMobilePhoneMessage( const QString& id, const QString& message );
        void sendMobilePhoneMessageToMyself( const QString& message );

    private Q_SLOTS:
        void getSystemConfigFinished();
        void ssiAuthFinished();
        void getCodePicFinished();
        void handleSipcRegisterReplyEvent( const FetionSipEvent& sipEvent );
        void handleSipEvent( const FetionSipEvent& sipEvent );

        void sendKeepAlive();

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

        QTimer* m_hearter;

        QHash<int, QString> m_buddyListIdNames;
        QHash<QString, QString> m_buddyIdSipUri;
};

#endif // FETIONSESSION_H
