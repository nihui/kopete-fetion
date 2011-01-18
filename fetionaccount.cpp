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
    QObject::connect( m_session, SIGNAL(loginSuccessed()),
                      this, SLOT(slotLoginSuccessed()) );
    QObject::connect( m_session, SIGNAL(logoutSuccessed()),
                      this, SLOT(slotLogoutSuccessed()) );
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
}

void FetionAccount::disconnect()
{
    m_session->logout();
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

    if ( !oldStatus.isDefinitelyOnline() && status.isDefinitelyOnline() ) {
        connect();
        return;
    }
    if ( oldStatus.isDefinitelyOnline() && !status.isDefinitelyOnline() ) {
        disconnect();
        return;
    }

    /// change status id
    QString statusId;
    switch ( status.internalStatus() ) {
        case 1:/* offline */
            statusId = "-1";
            break;
        case 2:/* hidden */
            statusId = "0";
            break;
        case 3:/* away */
            statusId = "100";
            break;
        case 4:/* outforlaunch */
            statusId = "150";
            break;
        case 5:/* rightback */
            statusId = "300";
            break;
        case 6:/* online */
            statusId = "400";
            break;
        case 7:/* onthephone */
            statusId = "500";
            break;
        case 8:/* busy */
            statusId = "600";
            break;
        case 9:/* donotdisturb */
            statusId = "800";
            break;
        case 10:/* meeting */
            statusId = "850";
            break;
        default:/* any -> online */
            statusId = "400";
            break;
    }
    qWarning() << "change status id to" << statusId;
    m_session->setStatusId( statusId );
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

void FetionAccount::slotLoginSuccessed()
{
    qWarning() << "slotLoginSuccessed";
    myself()->setOnlineStatus( FetionProtocol::protocol()->fetionOnline );
}

void FetionAccount::slotLogoutSuccessed()
{
    qWarning() << "slotLogoutSuccessed";
    myself()->setOnlineStatus( FetionProtocol::protocol()->fetionOffline );
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
        case P_OUTFORLAUNCH:
            status = FetionProtocol::protocol()->fetionOutForLaunch;
            break;
        case P_RIGHTBACK:
            status = FetionProtocol::protocol()->fetionRightBack;
            break;
        case P_ONLINE:
            status = FetionProtocol::protocol()->fetionOnline;
            break;
        case P_ONTHEPHONE:
            status = FetionProtocol::protocol()->fetionOnThePhone;
            break;
        case P_BUSY:
            status = FetionProtocol::protocol()->fetionBusy;
            break;
        case P_DONOTDISTURB:
            status = FetionProtocol::protocol()->fetionDoNotDisturb;
            break;
        case P_MEETING:
            status = FetionProtocol::protocol()->fetionMeeting;
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

