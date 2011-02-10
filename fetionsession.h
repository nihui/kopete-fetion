#ifndef FETIONSESSION_H
#define FETIONSESSION_H

#include <QObject>

#include <QNetworkAccessManager>
#include <QQueue>
#include <QDomNamedNodeMap>
#include <QHash>
#include <QTimer>
#include <QVariant>

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
        typedef void (FetionSession::*FetionSipEventCallback)(bool,const FetionSipEvent&,const QVariant&);
        typedef struct { FetionSipEventCallback cb; QVariant data; } FetionSipEventCallbackData;
        explicit FetionSession( QObject* parent = 0 );
        virtual ~FetionSession();
        void setLoginInformation( const QString& accountId, const QString& password );
        void login();
        void logout();
        QString accountId() const;
        bool isConnecting() const;
        bool isConnected() const;
        void setStatusId( const QString& statusId );
        void setStatusMessage( const QString& statusMessage );
        void requestChatChannel();
        void sendClientMessage( const QString& id, const QString& message, FetionSipNotifier* chatChannel = 0 );
        void sendClientNudge( FetionSipNotifier* chatChannel );
        void sendMobilePhoneMessage( const QString& id, const QString& message );
        void sendMobilePhoneMessageToMyself( const QString& message );
        void requestBuddyDetail( const QString& id );
        void requestBuddyPortrait( const QString& id );

    private Q_SLOTS:
        void getSystemConfigFinished();
        void ssiAuthFinished();
        void getCodePicFinished();
        void requestBuddyPortraitFinished();
        void handleSipcRegisterReplyEvent( const FetionSipEvent& sipEvent );
        void handleSipEvent( const FetionSipEvent& sipEvent );

        void sendKeepAlive();

    Q_SIGNALS:
        void loginSuccessed();
        void logoutSuccessed();
        void gotBuddyList( const QString& buddyListName );
        void gotBuddy( const QString& id, const QString& buddyListName );
        void chatChannelEstablished( FetionSipNotifier* chatChannel );
        void chatChannelAccepted( const QString& id, FetionSipNotifier* chatChannel );
        void buddyStatusUpdated( const QString& id, const QString& statusId );
        void buddyInfoUpdated( const QString& id, const FetionBuddyInfo& buddyInfo );
        void gotMessage( const QString& id, const QString& message );
        void sendClientMessageSuccessed( const QString& id );
        void gotBuddyDetail( const QString& id, const QDomNamedNodeMap& detailMap );
        void buddyPortraitUpdated( const QString& id, const QImage& portrait );
        void gotNudge( const QString& id );

    private:
        void sendSipcAuthEvent();

    private:
        void sendKeepAliveCB( bool isSuccessed, const FetionSipEvent& callbackEvent, const QVariant& data );
        void requestChatChannelCB( bool isSuccessed, const FetionSipEvent& callbackEvent, const QVariant& data );
        void sendClientMessageCB( bool isSuccessed, const FetionSipEvent& callbackEvent, const QVariant& data );
        void requestBuddyDetailCB( bool isSuccessed, const FetionSipEvent& callbackEvent, const QVariant& data );

        bool m_isConnecting;
        bool m_isConnected;
        QString m_accountId;
        QString m_password;
        QNetworkAccessManager* manager;
        QHash<int,FetionSipEventCallbackData> m_callidCallback;

        QHash<QNetworkReply*,QString> m_portraitReplyId;

        QString m_ssicCookie;

        QString picid;
        QString vcode;
        QString algorithm;
        QString type;
        QByteArray response;
        FetionSipNotifier* m_notifier;

        QString m_ssiAppSignInV2Uri;
        QString m_getPicCodeUri;
        QString m_sipcProxyAddress;
        QString m_sipcSslProxyAddress;
        QString m_httpTunnelAddress;
        QString m_getUri;

        QString m_getPortraitUri;
        QString m_portraitFileTypes;

        QString m_nouce;

        QString m_sipUri;
        QString m_userId;
        QString m_from;

        QTimer* m_hearter;

        QHash<int, QString> m_buddyListIdNames;
        QHash<QString, QString> m_buddyIdSipUri;
};

#endif // FETIONSESSION_H
