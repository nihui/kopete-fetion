#ifndef FETIONACCOUNT_H
#define FETIONACCOUNT_H

#include <kopetemessage.h>
#include <kopetepasswordedaccount.h>

class KActionMenu;
class FetionProtocol;
class FetionSession;
class FetionBuddyInfo;

class FetionAccount : public Kopete::PasswordedAccount
{
    Q_OBJECT
    public:
        FetionAccount( FetionProtocol* parent, const QString& accountId );
        virtual ~FetionAccount();
        virtual void connectWithPassword( const QString& password );
        virtual void disconnect();
        virtual void fillActionMenu( KActionMenu* actionMenu );
        virtual void setOnlineStatus( const Kopete::OnlineStatus& status,
                                      const Kopete::StatusMessage& reason = Kopete::StatusMessage(),
                                      const OnlineStatusOptions& options = None );
        virtual void setStatusMessage( const Kopete::StatusMessage& statusMessage );
    public Q_SLOTS:
        void slotSentMessage( const QString& id, const QString& msgContent );
    protected:
        virtual bool createContact( const QString& contactId, Kopete::MetaContact* parentContact );
    private Q_SLOTS:
        void slotLoginSuccessed();
        void slotLogoutSuccessed();
        void slotGotBuddy( const QString& id, const QString& buddyListName );
        void slotGotBuddyList( const QString& buddyListName );
        void slotBuddyStatusUpdated( const QString& id, const QString& statusId );
        void slotBuddyInfoUpdated( const QString& id, const FetionBuddyInfo& buddyInfo );
        void slotGotMessage( const QString& id, const QString& message );

    private:
        FetionSession* m_session;
};

#endif // FETIONACCOUNT_H
