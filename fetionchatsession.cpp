#include "fetionchatsession.h"
#include "fetionaccount.h"
#include "fetionsession.h"

#include <kopetechatsessionmanager.h>
#include <kopetecontact.h>
#include <kopetemessage.h>
#include <kopeteprotocol.h>

#include <KAction>
#include <KActionCollection>
#include <KIcon>
#include <KLocale>

FetionChatSession::FetionChatSession( const Kopete::Contact* user,
                                      Kopete::ContactPtrList others,
                                      Kopete::Protocol* protocol,
                                      Kopete::ChatSession::Form form )
: Kopete::ChatSession(user,others,protocol,form)
{
    Kopete::ChatSessionManager::self()->registerChatSession( this );
    setComponentData( protocol->componentData() );

    QObject::connect( this, SIGNAL(messageSent(Kopete::Message&,Kopete::ChatSession*)),
                      this, SLOT(slotMessageSent(Kopete::Message&,Kopete::ChatSession*)) );

    m_sendNudge = new KAction( this );
    m_sendNudge->setIcon( KIcon( "preferences-desktop-notification-bell" ) );
    m_sendNudge->setText( i18n( "send nudge" ) );
    actionCollection()->addAction( "sendNudge", m_sendNudge );
    m_sendSMS = new KAction( this );
    m_sendSMS->setIcon( KIcon( "konsole" ) );/// FIXME  :P
    m_sendSMS->setText( i18n( "send direct sms" ) );
    actionCollection()->addAction( "sendSMS", m_sendSMS );

    setXMLFile( "fetionchatui.rc" );
}

FetionChatSession::~FetionChatSession()
{
}

void FetionChatSession::slotMessageSent( Kopete::Message& message, Kopete::ChatSession* chatSession )
{
    appendMessage( message );
    FetionSession* accountSession = static_cast<FetionAccount*>(account())->session();
    accountSession->sendClientMessage( message.to().first()->contactId(), message.plainBody() );
}
