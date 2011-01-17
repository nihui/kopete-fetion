#include "fetionaccount.h"

#include "fetioncontact.h"
#include "fetionprotocol.h"
#include "fetionsession.h"
#include "fetionsipnotifier.h"
#include "fetionvcodedialog.h"

#include <kopetechatsession.h>
#include <kopetecontactlist.h>
#include <kopetegroup.h>
#include <kopetemetacontact.h>
#include <kopetestatusmessage.h>

#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include <KDebug>

FetionAccount::FetionAccount( FetionProtocol* parent, const QString& accountId )
: Kopete::PasswordedAccount( parent, accountId )
{
    setMyself( new FetionContact( this, accountId, Kopete::ContactList::self()->myself() ) );
    myself()->setOnlineStatus( FetionProtocol::protocol()->fetionOffline );
    m_session = new FetionSession( this );
}

FetionAccount::~FetionAccount()
{
    qWarning() << "FetionAccount::~FetionAccount()";
}

void FetionAccount::connectWithPassword( const QString& password )
{
    QObject::connect( m_session, SIGNAL(gotBuddy(const QString&,const QString&)),
                      this, SLOT(slotGotBuddy(const QString&,const QString&)) );
    QObject::connect( m_session, SIGNAL(gotBuddyList(const QString&)),
                      this, SLOT(slotGotBuddyList(const QString&)) );
    QObject::connect( m_session, SIGNAL(buddyInfoUpdated(const QString&,const FetionBuddyInfo&)),
                      this, SLOT(slotBuddyInfoUpdated(const QString&,const FetionBuddyInfo&)) );

    QObject::connect( m_session, SIGNAL(gotMessage(const QString&,const QString&)),
                      this, SLOT(slotGotMessage(const QString&,const QString&)) );
    QObject::connect( m_session, SIGNAL(contactStatusChanged(const QString&,const Kopete::OnlineStatus&)),
                      this, SLOT(slotContactStatusChanged(const QString&,const Kopete::OnlineStatus&)) );
    m_session->setLoginInformation( accountId(), password );
    m_session->login();
    /// TODO: connecting stuff end
    myself()->setOnlineStatus( FetionProtocol::protocol()->fetionOnline );
}

void FetionAccount::disconnect()
{
    /// TODO: disconnecting stuff here
    qWarning() << "FetionAccount::disconnect()";
    myself()->setOnlineStatus( FetionProtocol::protocol()->fetionOffline );
//     m_session->disconnect();
}

void FetionAccount::fillActionMenu( KActionMenu* actionMenu )
{
    Kopete::Account::fillActionMenu( actionMenu );
}

void FetionAccount::setOnlineStatus( const Kopete::OnlineStatus& status,
                                     const Kopete::StatusMessage& reason,
                                     const OnlineStatusOptions& options )
{
    /// TODO: status changing
    if ( status.status() == Kopete::OnlineStatus::Online ) {
        connect( Kopete::OnlineStatus::Online );
    }
//     m_session->setStatus( status );
}

void FetionAccount::setStatusMessage( const Kopete::StatusMessage& statusMessage )
{
    m_session->setStatusMessage( statusMessage.message() );
}

void FetionAccount::slotSentMessage( const QString& id, const QString& msgContent )
{
    qWarning() << "slotSentMessage" << id << msgContent;
    m_session->sendClientMessage( id, msgContent );
}

bool FetionAccount::createContact( const QString& contactId, Kopete::MetaContact* parentContact )
{
    if ( !contacts().value( contactId ) ) {
        FetionContact* newContact = new FetionContact( this, contactId, parentContact );
        return newContact != NULL;
    }
    return false;
}

void FetionAccount::slotGotBuddy( const QString& id, const QString& buddyListName )
{
    qWarning() << "slotGotBuddy" << id << buddyListName;
    Kopete::Contact* contact = contacts().value( id );
    if ( !contact ) {
        if ( buddyListName.isEmpty() )
            Kopete::MetaContact* mc = addContact( id, id );
        else {
            Kopete::Group* group = Kopete::ContactList::self()->findGroup( buddyListName );
            Kopete::MetaContact* mc = addContact( id, id, group );
        }
    }
}

void FetionAccount::slotGotBuddyList( const QString& buddyListName )
{
    qWarning() << "slotGotBuddyList" << buddyListName;
    Kopete::Group* group = Kopete::ContactList::self()->findGroup( buddyListName );
    if ( !group ) {
        group = new Kopete::Group( buddyListName );
        Kopete::ContactList::self()->addGroup( group );
    }
}

void FetionAccount::slotBuddyInfoUpdated( const QString& id, const FetionBuddyInfo& buddyInfo )
{
    qWarning() << "slotBuddyInfoUpdated" << id;
    FetionContact* contact = qobject_cast<FetionContact*>(contacts().value( id ));
    if ( !contact ) {
        qWarning() << "unknown contact info update" << id;
        return;
    }
    contact->setNickName( buddyInfo.nick.isEmpty() ? id : buddyInfo.nick );
    contact->setStatusMessage( buddyInfo.smsg );
    contact->metaContact()->setDisplayName( contact->nickName() );
}

void FetionAccount::slotContactStatusChanged( const QString& sId, const Kopete::OnlineStatus& status )
{
    Kopete::Contact* c = contacts().value( sId );
    if ( c ) {
        c->setOnlineStatus( status );
    }
}

void FetionAccount::slotGotMessage( const QString& id, const QString& message )
{
    Kopete::Contact* contact = contacts().value( id );
    if ( !contact ) {
        qWarning() << "unknown sender" << id << message;
        return;
    }

    Kopete::Message m = Kopete::Message( contact, myself() );
    m.setPlainBody( message );
    m.setDirection( Kopete::Message::Inbound );
    contact->manager( Kopete::Contact::CanCreate )->appendMessage( m );

    qWarning() << id << "say:" << message;
}

