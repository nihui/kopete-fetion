#include "fetionaccount.h"

#include "fetioncontact.h"
#include "fetionprotocol.h"
#include "fetionsession.h"
#include "fetionsipnotifier.h"
#include "fetionvcodedialog.h"

#include <kopetechatsession.h>
#include <kopetecontactlist.h>

#include <kopetemetacontact.h>

#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include <KDebug>

FetionAccount::FetionAccount( FetionProtocol* parent, const QString& accountID )
: Kopete::PasswordedAccount( parent, accountID )
{
    m_session = 0;
    setMyself( new FetionContact( this, accountID, Kopete::ContactList::self()->myself() ) );
}

FetionAccount::~FetionAccount()
{
    delete m_session;
}

void FetionAccount::connectWithPassword( const QString& password )
{
    m_session = new FetionSession( accountId() );
    QObject::connect( m_session, SIGNAL(gotContact(const Contact*)), this, SLOT(slotGotContact(const Contact*)) );
    QObject::connect( m_session, SIGNAL(gotGroup(const Group*)), this, SLOT(slotGotGroup(const Group*)) );
    QObject::connect( m_session, SIGNAL(gotMessage(const QString&,const QString&)),
                      this, SLOT(slotGotMessage(const QString&,const QString&)) );
    QObject::connect( m_session, SIGNAL(contactStatusChanged(const QString&,const Kopete::OnlineStatus&)),
                      this, SLOT(slotContactStatusChanged(const QString&,const Kopete::OnlineStatus&)) );
    m_session->connect( password );
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
    m_session->setStatus( status );
}

void FetionAccount::setStatusMessage( const Kopete::StatusMessage& statusMessage )
{
    /// TODO: personal signature
}

void FetionAccount::slotSentMessage( const QString& sId, const QString& msgContent )
{
    qWarning() << "slotSentMessage" << sId << msgContent;
    Conversation* conv = m_session->getConversation( sId );
//     Q_ASSERT(conv);
    fetion_conversation_send_sms( conv, msgContent.toUtf8() );
}

bool FetionAccount::createContact( const QString& contactId, Kopete::MetaContact* parentContact )
{
    /// TODO: add contact procedure
    FetionContact* newContact = new FetionContact( this, contactId, parentContact );
    if ( newContact == NULL )
        return false;
    return true;
}

void FetionAccount::slotGotContact( const Contact* contact )
{
    Kopete::Contact* c = contacts().value( contact->sId );
    if ( !c ) {
        Kopete::MetaContact* mc = addContact( contact->sId, QString::fromUtf8( contact->nickname ), groupHash[contact->groupid] );
//         FetionContact* c = new FetionContact( this, contact->sId, mc );
//         c->setNickName( QString::fromUtf8( contact->nickname ) );
//         c->setPhoto( QString::fromUtf8( config->iconPath ) + '/' + contact->sId + ".jpg" );
//         c->setOnlineStatus( FetionProtocol::protocol()->fetionOnline );
    }
}

void FetionAccount::slotGotGroup( const Group* group )
{
    Kopete::Group* g = Kopete::ContactList::self()->findGroup( QString::fromUtf8( group->groupname ) );
    groupHash[group->groupid] = g;
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

