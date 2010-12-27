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
//     delete m_session;
}

void FetionAccount::connectWithPassword( const QString& password )
{
//     m_session = new FetionSession( accountId() );
    QObject::connect( m_session, SIGNAL(gotContact(const QString&,const QString&,int)),
                      this, SLOT(slotGotContact(const QString&,const QString&,int)) );
    QObject::connect( m_session, SIGNAL(gotGroup(int,const QString&)),
                      this, SLOT(slotGotGroup(int,const QString&)) );
    QObject::connect( m_session, SIGNAL(gotMessage(const QString&,const QString&)),
                      this, SLOT(slotGotMessage(const QString&,const QString&)) );
    QObject::connect( m_session, SIGNAL(contactStatusChanged(const QString&,const Kopete::OnlineStatus&)),
                      this, SLOT(slotContactStatusChanged(const QString&,const Kopete::OnlineStatus&)) );
    m_session->setLoginInformation( accountId(), password );
    m_session->login();
//     m_session->connect( password );
    /// TODO: connecting stuff end
    myself()->setOnlineStatus( FetionProtocol::protocol()->fetionOnline );
}

void FetionAccount::disconnect()
{
    /// TODO: disconnecting stuff here
    m_session->disconnect();
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
    /// TODO: personal signature
}

void FetionAccount::slotSentMessage( const QString& sId, const QString& msgContent )
{
    qWarning() << "slotSentMessage" << sId << msgContent;
    m_session->sendClientMessage( sId, msgContent );
}

bool FetionAccount::createContact( const QString& contactId, Kopete::MetaContact* parentContact )
{
    if ( !contacts().value( contactId ) ) {
        FetionContact* newContact = new FetionContact( this, contactId, parentContact );
        return newContact != NULL;
    }
    return false;
}

void FetionAccount::slotGotContact( const QString& contactId, const QString& contactName, int groupId )
{
    Kopete::Contact* contact = contacts().value( contactId );
    if ( !contact ) {
        Kopete::MetaContact* mc = addContact( contactId, contactName, groupHash[groupId] );
//         FetionContact* c = new FetionContact( this, contact->sId, mc );
//         c->setNickName( QString::fromUtf8( contact->nickname ) );
//         c->setPhoto( QString::fromUtf8( config->iconPath ) + '/' + contact->sId + ".jpg" );
//         c->setOnlineStatus( FetionProtocol::protocol()->fetionOnline );
    }
}

void FetionAccount::slotGotGroup( int groupId, const QString& groupName )
{
    Kopete::Group* group = Kopete::ContactList::self()->findGroup( groupName );
    if ( !group ) {
        group = new Kopete::Group( groupName );
        Kopete::ContactList::self()->addGroup( group );
    }
    groupHash[groupId] = group;
}

void FetionAccount::slotContactStatusChanged( const QString& sId, const Kopete::OnlineStatus& status )
{
    Kopete::Contact* c = contacts().value( sId );
    if ( c ) {
        c->setOnlineStatus( status );
    }
}

void FetionAccount::slotGotMessage( const QString& sId, const QString& msgContent )
{
    qWarning() << "slotGotMessage";
//     char* charsid = fetion_sip_get_sid_by_sipuri( msg->sipuri );
//     QString sId( charsid );
//     free( charsid );

//     qWarning() << "sId" << sId;

    Kopete::Contact* contact = contacts().value( sId );
    if ( !contact ) {
        qWarning() << "unknown sender" << sId << msgContent;
        return;
        //c->setOnlineStatus( status );
    }

    Kopete::Message message = Kopete::Message( contact, myself() );
    message.setPlainBody( msgContent );
    message.setDirection( Kopete::Message::Inbound );
    contact->manager( Kopete::Contact::CanCreate )->appendMessage( message );

    qWarning() << sId << "say:" << msgContent;

//     fetion_message_free( msg );
}

