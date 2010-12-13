#include "fetionchatsession.h"

#include <kopetechatsessionmanager.h>
#include <kopetecontact.h>
#include <kopeteprotocol.h>

#include <KComponentData>

#include <openfetion.h>

#include <KDebug>

FetionChatSession::FetionChatSession( const Kopete::Contact* user,
                                      Kopete::ContactPtrList others,
                                      Kopete::Protocol* protocol )
: Kopete::ChatSession( user, others, protocol )
{
    setComponentData( protocol->componentData() );
    Kopete::ChatSessionManager::self()->registerChatSession( this );

//     connect( this, SIGNAL(messageSent(Kopete::Message&,Kopete::ChatSession*)),
//              this, SLOT(slotMessageSent(Kopete::Message&,Kopete::ChatSession*)) );

//     conv = fetion_conversation_new( user->account(), const char* sipuri , NULL );
    qWarning() << "new charsession here";
}

FetionChatSession::~FetionChatSession()
{
//     free( conv );
}

// void FetionChatSession::slotMessageSent( Kopete::Message& message, Kopete::ChatSession* chatSession )
// {
//    /// TODO: sent the message
// }
