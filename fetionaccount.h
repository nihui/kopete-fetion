#ifndef FETIONACCOUNT_H
#define FETIONACCOUNT_H

#include <kopetemessage.h>
#include <kopetepasswordedaccount.h>

// #include <openfetion.h>

class KActionMenu;
class FetionProtocol;
class FetionSession;

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
        void slotSentMessage( const QString& sId, const QString& msgContent );
    protected:
        virtual bool createContact( const QString& contactId, Kopete::MetaContact* parentContact );
    private Q_SLOTS:
        void slotGotContact( const QString& contactId, const QString& contactName, int groupId );
        void slotGotGroup( int groupId, const QString& groupName );
        void slotGotMessage( const QString& sId, const QString& msgContent );
        void slotContactStatusChanged( const QString& sId, const Kopete::OnlineStatus& status );
    private:
        FetionSession* m_session;
        QHash<int, Kopete::Group*> groupHash;
};

#endif // FETIONACCOUNT_H
