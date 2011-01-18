#include "fetionprotocol.h"

#include "fetionaccount.h"
#include "fetionaddcontactpage.h"
#include "fetioncontact.h"
#include "fetioneditaccountwidget.h"

#include <kopeteaccountmanager.h>

#include <kgenericfactory.h>
#include <KDebug>
#include <QList>

K_PLUGIN_FACTORY( FetionProtocolFactory, registerPlugin<FetionProtocol>(); )
K_EXPORT_PLUGIN( FetionProtocolFactory( "kopete_fetion" ) )

FetionProtocol* FetionProtocol::s_protocol = 0L;

FetionProtocol::FetionProtocol( QObject* parent, const QVariantList& args )
: Kopete::Protocol( FetionProtocolFactory::componentData(), parent ),
fetionOffline( Kopete::OnlineStatus::Offline, 25, this, 1,
               QStringList(), i18n( "Offline" ), i18n( "O&ffline" ),
               Kopete::OnlineStatusManager::Offline, Kopete::OnlineStatusManager::DisabledIfOffline ),
fetionInvisible( Kopete::OnlineStatus::Invisible, 25, this, 2,
                 QStringList( "fetion_invisible" ), i18n( "Invisible" ), i18n( "&Invisible" ),
                 Kopete::OnlineStatusManager::Invisible ),
fetionAway( Kopete::OnlineStatus::Away, 25, this, 3,
            QStringList( "fetion_away" ), i18n( "Away" ), i18n( "&Away" ),
            Kopete::OnlineStatusManager::Away ),
fetionOutForLaunch( Kopete::OnlineStatus::Away, 25, this, 4,
                    QStringList( "fetion_outforlaunch" ), i18n( "Out for launch" ), i18n( "O&ut for launch" ),
                    Kopete::OnlineStatusManager::Away ),
fetionRightBack( Kopete::OnlineStatus::Away, 25, this, 5,
                 QStringList( "fetion_rightback" ), i18n( "Right back" ), i18n( "&Right back" ),
                 Kopete::OnlineStatusManager::Away ),
fetionOnline( Kopete::OnlineStatus::Online, 25, this, 6,
              QStringList(), i18n( "Online" ), i18n( "O&nline" ),
              Kopete::OnlineStatusManager::Online ),
fetionOnThePhone( Kopete::OnlineStatus::Busy, 25, this, 7,
                  QStringList( "fetion_onthephone" ), i18n( "On the phone" ), i18n( "On &the phone" ),
                  Kopete::OnlineStatusManager::Busy ),
fetionBusy( Kopete::OnlineStatus::Busy, 25, this, 8,
            QStringList( "fetion_busy" ), i18n( "Busy" ), i18n( "&Busy" ),
            Kopete::OnlineStatusManager::Busy ),
fetionDoNotDisturb( Kopete::OnlineStatus::Busy, 25, this, 9,
                    QStringList( "fetion_donotdisturb" ), i18n( "Do not disturb" ), i18n( "&Do not disturb" ),
                    Kopete::OnlineStatusManager::Busy ),
fetionMeeting( Kopete::OnlineStatus::Busy, 25, this, 10,
               QStringList( "fetion_meeting" ), i18n( "Meeting" ), i18n( "&Meeting" ),
               Kopete::OnlineStatusManager::Busy )
{
    Q_UNUSED(args)
    s_protocol = this;
}

FetionProtocol::~FetionProtocol()
{
}

AddContactPage* FetionProtocol::createAddContactWidget( QWidget* parent, Kopete::Account* account )
{
    Q_UNUSED(account)
    return new FetionAddContactPage( parent );
}

KopeteEditAccountWidget* FetionProtocol::createEditAccountWidget( Kopete::Account* account, QWidget* parent )
{
    return new FetionEditAccountWidget( parent, account );
}

Kopete::Account *FetionProtocol::createNewAccount( const QString &accountId )
{
    return new FetionAccount( this, accountId );
}

Kopete::Contact* FetionProtocol::deserializeContact( Kopete::MetaContact* metaContact,
                                                     const QMap<QString, QString>& serializedData,
                                                     const QMap<QString, QString>& addressBookData )
{
    QString contactId = serializedData[ "contactId" ];
    QString accountId = serializedData[ "accountId" ];
    QString displayName = serializedData[ "displayName" ];

    QList<Kopete::Account*> accounts = Kopete::AccountManager::self()->accounts( this );
    Kopete::Account* account = 0;
    foreach (Kopete::Account * acct, accounts) {
        if ( acct->accountId () == accountId )
            account = acct;
    }
    if ( !account ) {
        kDebug() << "Account doesn't exist, skipping";
        return 0;
    }

    return new FetionContact( account, contactId, metaContact );
}

bool FetionProtocol::validContactId( const QString& userId )
{
    QRegExp re("[1-9][0-9]*");
    return re.exactMatch( userId );
}

FetionProtocol* FetionProtocol::protocol()
{
    return s_protocol;
}

