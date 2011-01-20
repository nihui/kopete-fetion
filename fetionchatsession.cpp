#include "fetionchatsession.h"
#include "fetionaccount.h"
#include "fetionsession.h"
#include "fetionsipnotifier.h"

#include <kopetechatsessionmanager.h>
#include <kopetecontact.h>
#include <kopetemessage.h>
#include <kopeteprotocol.h>

#include <KAction>
#include <KActionCollection>
#include <KIcon>
#include <KLocale>
#include <KToggleAction>

FetionChatSession::FetionChatSession( const Kopete::Contact* user,
                                      Kopete::ContactPtrList others,
                                      Kopete::Protocol* protocol,
                                      Kopete::ChatSession::Form form )
: Kopete::ChatSession(user,others,protocol,form),
m_chatChannel(0)
{
    Kopete::ChatSessionManager::self()->registerChatSession( this );
    setComponentData( protocol->componentData() );

    QObject::connect( this, SIGNAL(messageSent(Kopete::Message&,Kopete::ChatSession*)),
                      this, SLOT(slotMessageSent(Kopete::Message&,Kopete::ChatSession*)) );

    m_sendNudge = new KAction( this );
    m_sendNudge->setIcon( KIcon( "preferences-desktop-notification-bell" ) );
    m_sendNudge->setText( i18n( "send nudge" ) );
    actionCollection()->addAction( "sendNudge", m_sendNudge );
    connect( m_sendNudge, SIGNAL(triggered()), this, SLOT(slotSendNudge()) );
    m_sendSMS = new KToggleAction( this );
    m_sendSMS->setIcon( KIcon( "start-here-kde" ) );/// FIXME  :P
    m_sendSMS->setText( i18n( "send direct sms" ) );
    actionCollection()->addAction( "sendSMS", m_sendSMS );

    setXMLFile( "fetionchatui.rc" );
}

FetionChatSession::~FetionChatSession()
{
    qWarning() << "chat channel destroying" << m_chatChannel;
    setChatChannel( 0 );
}

void FetionChatSession::setChatChannel( FetionSipNotifier* chatChannel )
{
    if ( m_chatChannel ) {
        m_chatChannel->close();
        delete m_chatChannel;
    }
    m_chatChannel = chatChannel;
}

FetionSipNotifier* FetionChatSession::chatChannel() const
{
    return m_chatChannel;
}

void FetionChatSession::slotMessageSent( Kopete::Message& message, Kopete::ChatSession* chatSession )
{
    appendMessage( message );
    FetionSession* accountSession = static_cast<FetionAccount*>(account())->session();
    if ( m_sendSMS->isChecked() )
        accountSession->sendMobilePhoneMessage( message.to().first()->contactId(), message.plainBody() );
    else
        accountSession->sendClientMessage( message.to().first()->contactId(), message.plainBody(), chatChannel() );
}

void FetionChatSession::slotSendNudge()
{
    FetionSession* accountSession = static_cast<FetionAccount*>(account())->session();
    accountSession->sendClientNudge( chatChannel() );
}
