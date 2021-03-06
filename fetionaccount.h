#ifndef FETIONACCOUNT_H
#define FETIONACCOUNT_H

#include <kopetemessage.h>
#include <kopetepasswordedaccount.h>

class QDomNamedNodeMap;
class KActionMenu;
class FetionProtocol;
class FetionSession;
class FetionBuddyInfo;
class FetionContact;
class FetionSipNotifier;

class FetionAccount : public Kopete::PasswordedAccount
{
    Q_OBJECT
    public:
        explicit FetionAccount( FetionProtocol* parent, const QString& accountId );
        virtual ~FetionAccount();
        FetionSession* session() const;
        virtual void connectWithPassword( const QString& password );
        virtual void disconnect();
        virtual void fillActionMenu( KActionMenu* actionMenu );
        virtual void setOnlineStatus( const Kopete::OnlineStatus& status,
                                      const Kopete::StatusMessage& reason = Kopete::StatusMessage(),
                                      const OnlineStatusOptions& options = None );
        virtual void setStatusMessage( const Kopete::StatusMessage& statusMessage );
    protected:
        virtual bool createContact( const QString& contactId, Kopete::MetaContact* parentContact );
    private Q_SLOTS:
        void slotSendSMSToMyself();
        void slotLoginSuccessed();
        void slotLogoutSuccessed();
        void slotGotBuddy( const QString& id, const QString& buddyListName );
        void slotGotBuddyList( const QString& buddyListName );
        void slotChatChannelAccepted( const QString& id, FetionSipNotifier* chatChannel );
        void slotBuddyStatusUpdated( const QString& id, const QString& statusId );
        void slotBuddyInfoUpdated( const QString& id, const FetionBuddyInfo& buddyInfo );
        void slotGotMessage( const QString& id, const QString& message );
        void slotSendClientMessageSuccessed( const QString& id );
        void slotGotBuddyDetail( const QString& id, const QDomNamedNodeMap& detailMap );
        void slotBuddyPortraitUpdated( const QString& id, const QImage& portrait );
        void slotGotNudge( const QString& id );
    private:
        FetionSession* m_session;
};

#endif // FETIONACCOUNT_H
