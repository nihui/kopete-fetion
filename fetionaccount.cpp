#include "fetionaccount.h"

#include "fetioncontact.h"
#include "fetionprotocol.h"
#include "fetionsession.h"
#include "fetionsipnotifier.h"
#include "fetionvcodedialog.h"

#include <kopeteavatarmanager.h>
#include <kopetechatsession.h>
#include <kopetecontactlist.h>
#include <kopetegroup.h>
#include <kopetemetacontact.h>
#include <kopetestatusmessage.h>

#include <KDialog>
#include <KLocale>

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
    QObject::connect( m_session, SIGNAL(sendClientMessageSuccessed(const QString&)),
                      this, SLOT(slotSendClientMessageSuccessed(const QString&)) );
    QObject::connect( m_session, SIGNAL(gotBuddyDetail(const QString&,const QDomNamedNodeMap&)),
                      this, SLOT(slotGotBuddyDetail(const QString&,const QDomNamedNodeMap&)) );
    QObject::connect( m_session, SIGNAL(buddyPortraitUpdated(const QString&,const QImage&)),
                      this, SLOT(slotBuddyPortraitUpdated(const QString&,const QImage&)) );
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
        m_session->requestBuddyPortrait( id );
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
        qWarning() << "slotBuddyInfoUpdated unknown contact" << id;
        return;
    }
    contact->setNickName( buddyInfo.nick.isEmpty() ? id : buddyInfo.nick );
    contact->setStatusMessage( buddyInfo.smsg );
    contact->metaContact()->setDisplayName( contact->nickName() );
}

void FetionAccount::slotGotMessage( const QString& id, const QString& message )
{
    FetionContact* contact = qobject_cast<FetionContact*>(contacts().value( id ));
    if ( !contact ) {
        qWarning() << "slotGotMessage unknown sender" << id << message;
        return;
    }
    contact->slotMessageReceived( message );
}

void FetionAccount::slotSendClientMessageSuccessed( const QString& id )
{
    FetionContact* contact = qobject_cast<FetionContact*>(contacts().value( id ));
    if ( !contact ) {
        qWarning() << "slotSendClientMessageSuccessed unknown sender" << id;
        return;
    }
    contact->manager()->messageSucceeded();
}

void FetionAccount::slotGotBuddyDetail( const QString& id, const QDomNamedNodeMap& detailMap )
{
    FetionContact* contact = qobject_cast<FetionContact*>(contacts().value( id ));
    if ( !contact ) {
        qWarning() << "slotGotBuddyDetail unknown sender" << id;
        return;
    }
    contact->showUserInfo( detailMap );
}

void FetionAccount::slotBuddyPortraitUpdated( const QString& id, const QImage& portrait )
{
    FetionContact* contact = qobject_cast<FetionContact*>(contacts().value( id ));
    if ( !contact ) {
        qWarning() << "slotBuddyPortraitUpdated unknown sender" << id;
        return;
    }

    Kopete::AvatarManager::AvatarEntry newEntry;
    newEntry.name = "kopete-fetion" + id;
    newEntry.image = portrait;
    newEntry.contact = contact;
    newEntry.category = Kopete::AvatarManager::Contact;

    newEntry = Kopete::AvatarManager::self()->add( newEntry );
    contact->removeProperty( Kopete::Global::Properties::self()->photo() );
    contact->setProperty( Kopete::Global::Properties::self()->photo(), newEntry.path );
}
