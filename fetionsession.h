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


class FetionSipEvent;
class FetionSipNotifier;

class FetionSession : public QObject
{
    Q_OBJECT
    public:
        typedef void (FetionSession::*FetionSipEventCallback)(bool,const FetionSipEvent&);
        explicit FetionSession( QObject* parent = 0 );
        virtual ~FetionSession();
        void setLoginInformation( const QString& accountId, const QString& password );
        void login();
        void logout();
        QString accountId() const;
        bool isConnecting() const;
        bool isConnected() const;
        void setVisibility( bool isVisible );
        void setStatusId( const QString& statusId );
        void setStatusMessage( const QString& statusMessage );
        void sendClientMessage( const QString& id, const QString& message );
        void sendMobilePhoneMessage( const QString& id, const QString& message );
        void sendMobilePhoneMessageToMyself( const QString& message );
        void requestBuddyDetail( const QString& id );

    private Q_SLOTS:
        void getSystemConfigFinished();
        void ssiAuthFinished();
        void getCodePicFinished();
        void handleSipcRegisterReplyEvent( const FetionSipEvent& sipEvent );
        void handleSipEvent( const FetionSipEvent& sipEvent );

        void sendKeepAlive();

    Q_SIGNALS:
        void loginSuccessed();
        void logoutSuccessed();
        void gotBuddyList( const QString& buddyListName );
        void gotBuddy( const QString& id, const QString& buddyListName );
        void buddyStatusUpdated( const QString& id, const QString& statusId );
        void buddyInfoUpdated( const QString& id, const FetionBuddyInfo& buddyInfo );
        void gotMessage( const QString& id, const QString& message );

    private:
        void sendClientMessageCB( bool isSuccessed, const FetionSipEvent& callbackEvent );
        void requestBuddyDetailCB( bool isSuccessed, const FetionSipEvent& callbackEvent );

        bool m_isConnecting;
        bool m_isConnected;
        QString m_accountId;
        QString m_password;
        QNetworkAccessManager* manager;
        QHash<int,FetionSipEventCallback> m_callidCallback;

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
