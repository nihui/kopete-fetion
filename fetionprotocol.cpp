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
fetionOnline( Kopete::OnlineStatus::Online, 25, this, 1,
              QStringList(), i18n( "Online" ), i18n( "O&nline" ),
              Kopete::OnlineStatusManager::Online ),
fetionAway( Kopete::OnlineStatus::Away, 25, this, 2,
            QStringList( "fetion_away" ), i18n( "Away" ), i18n( "&Away" ),
            Kopete::OnlineStatusManager::Away ),
fetionBusy( Kopete::OnlineStatus::Busy, 25, this, 3,
            QStringList( "fetion_busy" ), i18n( "Busy" ), i18n( "&Busy" ),
            Kopete::OnlineStatusManager::Busy ),
fetionInvisible( Kopete::OnlineStatus::Invisible, 25, this, 4,
                 QStringList( "fetion_invisible" ), i18n( "Invisible" ), i18n( "&Invisible" ),
                 Kopete::OnlineStatusManager::Invisible ),
fetionOffline( Kopete::OnlineStatus::Offline, 25, this, 5,
               QStringList(), i18n( "Offline" ), i18n( "O&ffline" ),
               Kopete::OnlineStatusManager::Offline, Kopete::OnlineStatusManager::DisabledIfOffline )
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

FetionProtocol *FetionProtocol::protocol()
{
    return s_protocol;
}

