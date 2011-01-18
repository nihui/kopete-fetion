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
    QObject::connect( m_session, SIGNAL(gotBuddy(const QString&,const QString&)),
                      this, SLOT(slotGotBuddy(const QString&,const QString&)) );
    QObject::connect( m_session, SIGNAL(gotBuddyList(const QString&)),
                      this, SLOT(slotGotBuddyList(const QString&)) );
    QObject::connect( m_session, SIGNAL(buddyStatusUpdated(const QString&,const QString&)),
                      this, SLOT(slotBuddyStatusUpdated(const QString&,const QString&)) );
    QObject::connect( m_session, SIGNAL(buddyInfoUpdated(const QString&,const FetionBuddyInfo&)),
                      this, SLOT(slotBuddyInfoUpdated(const QString&,const FetionBuddyInfo&)) );
    QObject::connect( m_session, SIGNAL(gotMessage(const QString&,const QString&)),
                      this, SLOT(slotGotMessage(const QString&,const QString&)) );
}

FetionAccount::~FetionAccount()
{
    qWarning() << "FetionAccount::~FetionAccount()";
}

void FetionAccount::connectWithPassword( const QString& password )
{
    if ( m_session->isConnected() || m_session->isConnecting() )
        return;

    m_session->setLoginInformation( accountId(), password );
    m_session->login();
    /// TODO: connecting stuff end
    myself()->setOnlineStatus( FetionProtocol::protocol()->fetionOnline );
}

void FetionAccount::disconnect()
{
    /// TODO: disconnecting stuff here
    qWarning() << "FetionAccount::disconnect()";
    m_session->logout();
    myself()->setOnlineStatus( FetionProtocol::protocol()->fetionOffline );
}

void FetionAccount::fillActionMenu( KActionMenu* actionMenu )
{
    Kopete::Account::fillActionMenu( actionMenu );
}

void FetionAccount::setOnlineStatus( const Kopete::OnlineStatus& status,
                                     const Kopete::StatusMessage& reason,
                                     const OnlineStatusOptions& options )
{
    Kopete::OnlineStatus oldStatus = myself()->onlineStatus();
    if ( oldStatus == status )
        return;

    /// TODO: status changing
    if ( !oldStatus.isDefinitelyOnline() && status.isDefinitelyOnline() ) {
        connect();
    }
    else if ( oldStatus.isDefinitelyOnline() && !status.isDefinitelyOnline() ) {
        disconnect();
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

void FetionAccount::slotBuddyStatusUpdated( const QString& id, const QString& statusId )
{
    qWarning() << "slotBuddyStatusUpdated" << id << statusId;
    Kopete::Contact* contact = contacts().value( id );
    if ( !contact ) {
        qWarning() << "unknown sender" << id << statusId;
        return;
    }

    Kopete::OnlineStatus status;
    switch ( statusId.toInt() ) {
        case P_OFFLINE:
            status = FetionProtocol::protocol()->fetionOffline;
            break;
        case P_HIDDEN:
            status = FetionProtocol::protocol()->fetionInvisible;
            break;
        case P_AWAY:
            status = FetionProtocol::protocol()->fetionAway;
            break;
        case P_ONTHEPHONE:
            status = FetionProtocol::protocol()->fetionBusy;
            break;
        case P_RIGHTBACK:
            status = FetionProtocol::protocol()->fetionAway;
            break;
        case P_ONLINE:
            status = FetionProtocol::protocol()->fetionOnline;
            break;
        case P_OUTFORLAUNCH:
            status = FetionProtocol::protocol()->fetionBusy;
            break;
        case P_BUSY:
            status = FetionProtocol::protocol()->fetionBusy;
            break;
        case P_DONOTDISTURB:
            status = FetionProtocol::protocol()->fetionBusy;
            break;
        case P_MEETING:
            status = FetionProtocol::protocol()->fetionBusy;
            break;
        default:
            qWarning() << "unknown status id" << statusId;
            status = FetionProtocol::protocol()->fetionOffline;
            break;
    }
    contact->setOnlineStatus( status );
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

