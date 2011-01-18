#include "fetioncontact.h"

#include "fetionaccount.h"
#include "fetionprotocol.h"
#include "fetionsession.h"

#include <kopeteaccount.h>
#include <kopetechatsessionmanager.h>

#include <KDialog>
#include <QDebug>

FetionContact::FetionContact( Kopete::Account* account, const QString &id, Kopete::MetaContact* parent )
: Kopete::Contact( account, id, parent ),
m_manager(0L)
{
    setOnlineStatus( FetionProtocol::protocol()->fetionOffline );
}

FetionContact::~FetionContact()
{
}

bool FetionContact::isReachable()
{
    return true;
}

// QList<KAction*>* FetionContact::customContextMenuActions()
// {
//     QList<KAction*>* actions = new QList<KAction*>;
//    /// TODO: add custom actions here
//     return actions;
// }

Kopete::ChatSession* FetionContact::manager( Kopete::Contact::CanCreateFlags canCreate )
{
    if( !m_manager && canCreate ) {
        Kopete::ContactPtrList chatmembers;
        chatmembers.append( this );
        m_manager = Kopete::ChatSessionManager::self()->create( account()->myself(), chatmembers, protocol() );
        connect( m_manager, SIGNAL(messageSent(Kopete::Message&,Kopete::ChatSession*)),
                 this, SLOT(slotMessageSent(Kopete::Message&,Kopete::ChatSession*)) );
        connect( m_manager, SIGNAL(destroyed()), this, SLOT(slotChatSessionDestroyed()) );
    }
    return m_manager;
}

void FetionContact::serialize( QMap<QString, QString>& serializedData,
                               QMap<QString, QString>& addressBookData )
{
    Kopete::Contact::serialize( serializedData, addressBookData );
}

void FetionContact::slotUserInfo()
{
    /// TODO retrieve contact info and display in a dialog
    FetionSession* accountSession = static_cast<FetionAccount*>(account())->m_session;
    accountSession->requestBuddyDetail( contactId() );
}

void FetionContact::slotChatSessionDestroyed()
{
    m_manager = 0;
}

void FetionContact::slotMessageReceived( const QString& message )
{
    Kopete::Message m = Kopete::Message( this, account()->myself() );
    m.setPlainBody( message );
    m.setDirection( Kopete::Message::Inbound );
    manager( Kopete::Contact::CanCreate )->appendMessage( m );
}

void FetionContact::slotMessageSent( Kopete::Message& message, Kopete::ChatSession* chatSession )
{
    chatSession->appendMessage( message );
    chatSession->messageSucceeded();
    FetionSession* accountSession = static_cast<FetionAccount*>(account())->m_session;
    accountSession->sendClientMessage( message.to().first()->contactId(), message.plainBody() );
}
