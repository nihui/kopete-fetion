#include "fetioneditaccountwidget.h"

#include "fetionaccount.h"
#include "fetionprotocol.h"

#include "ui_fetioneditaccount.h"

#include <kopeteuiglobal.h>

#include <KLocale>
#include <KMessageBox>
#include <KToolInvocation>

FetionEditAccountWidget::FetionEditAccountWidget( QWidget* parent, Kopete::Account* account )
: QWidget(parent),
KopeteEditAccountWidget(account)
{
    m_widget = new Ui::FetionEditAccount;
    m_widget->setupUi( this );

    if ( account ) {
        m_widget->m_loginid->setText( account->accountId() );
        m_widget->m_loginid->setReadOnly( true );
        m_widget->m_password->load( &static_cast<FetionAccount*>(account)->password() );
        m_widget->m_autologin->setChecked( account->excludeConnect() );
    }

    connect( m_widget->buttonRegister, SIGNAL(clicked()), this, SLOT(slotOpenRegister()) );
}

FetionEditAccountWidget::~FetionEditAccountWidget()
{
    delete m_widget;
}

Kopete::Account* FetionEditAccountWidget::apply()
{
    if ( !account() )
        setAccount( new FetionAccount( FetionProtocol::protocol(), m_widget->m_loginid->text().trimmed() ) );

    account()->setExcludeConnect( m_widget->m_autologin->isChecked() );
    m_widget->m_password->save( &static_cast<FetionAccount*>(account())->password() );

    /// TODO: save account information into config file

    return account();
}

bool FetionEditAccountWidget::validateData()
{
    if ( FetionProtocol::validContactId( m_widget->m_loginid->text().trimmed() ) )
        return true;

    KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry,
                                   i18n( "<qt>You must enter a valid Fetion account ID.</qt>" ), i18n( "Fetion Plugin" ) );
    return false;
}

void FetionEditAccountWidget::slotOpenRegister()
{
    KToolInvocation::invokeBrowser( "https://feixin.10086.cn/account/register/" );
}
