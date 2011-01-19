#include "fetioncontact.h"

#include "fetionaccount.h"
#include "fetionprotocol.h"
#include "fetionsession.h"
#include "ui_fetioncontactinfo.h"

#include <kopeteaccount.h>
#include <kopetechatsessionmanager.h>

#include <KDialog>
#include <KLocale>
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
//     KAction* a = new KAction( this );
//     a->setText( i18n( "&Reload avatar" ) );
//     connect( a, SIGNAL(triggered()), this, SLOT(reloadAvatar()) );
//     actions->append( a );
//     return actions;
// }
//
// QList<KAction*>* FetionContact::customContextMenuActions( Kopete::ChatSession* chatSession )
// {
//     QList<KAction*>* actions = new QList<KAction*>;
//     KAction* a = new KAction( this );
//     a->setText( i18n( "&Reload avatar" ) );
//     connect( a, SIGNAL(triggered()), this, SLOT(reloadAvatar()) );
//     actions->append( a );
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
    serializedData["avatar"] = property( Kopete::Global::Properties::self()->photo() ).value().toString();
}

void FetionContact::showUserInfo( const QDomNamedNodeMap& detailMap )
{
    KDialog* infoDialog = new KDialog;
    infoDialog->setButtons( KDialog::Close );
    infoDialog->setDefaultButton( KDialog::Close );
    Ui::FetionContactInfo info;
    info.setupUi( infoDialog->mainWidget() );

    info.m_id->setText( detailMap.namedItem( "user-id" ).nodeValue() );
    info.m_mobileNo->setText( detailMap.namedItem( "mobile-no" ).nodeValue() );
    info.m_carrier->setText( detailMap.namedItem( "carrier" ).nodeValue() );
    info.m_nickname->setText( detailMap.namedItem( "nickname" ).nodeValue() );
    info.m_gender->setText( detailMap.namedItem( "gender" ).nodeValue() );
    info.m_birthdate->setText( detailMap.namedItem( "birth-date" ).nodeValue() );
    info.m_personalEmail->setText( detailMap.namedItem( "personal-email" ).nodeValue() );
    info.m_workEmail->setText( detailMap.namedItem( "work-email" ).nodeValue() );
    info.m_otherEmail->setText( detailMap.namedItem( "other-email" ).nodeValue() );
    info.m_lunarAnimal->setText( detailMap.namedItem( "lunar-animal" ).nodeValue() );
    info.m_horoscope->setText( detailMap.namedItem( "horoscope" ).nodeValue() );
    info.m_profile->setText( detailMap.namedItem( "profile" ).nodeValue() );
    info.m_bloodtype->setText( detailMap.namedItem( "bloodtype" ).nodeValue() );
    info.m_occupation->setText( detailMap.namedItem( "occupation" ).nodeValue() );
    info.m_hobby->setText( detailMap.namedItem( "hobby" ).nodeValue() );
    info.m_scoreLevel->setText( detailMap.namedItem( "score-level" ).nodeValue() );

    infoDialog->setCaption( i18n( "Fetion contact" ) );
    infoDialog->exec();
    delete infoDialog;
}

void FetionContact::slotUserInfo()
{
    /// TODO retrieve contact info and display in a dialog
    FetionSession* accountSession = static_cast<FetionAccount*>(account())->m_session;
    accountSession->requestBuddyDetail( contactId() );
}

void FetionContact::reloadAvatar()
{
    FetionSession* accountSession = static_cast<FetionAccount*>(account())->m_session;
    accountSession->requestBuddyPortrait( contactId() );
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
    FetionSession* accountSession = static_cast<FetionAccount*>(account())->m_session;
    accountSession->sendClientMessage( message.to().first()->contactId(), message.plainBody() );
}
