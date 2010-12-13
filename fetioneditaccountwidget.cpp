#include "fetioneditaccountwidget.h"

#include "fetionaccount.h"
#include "fetionprotocol.h"

#include <KLineEdit>
#include <KLocale>

#include <QFormLayout>

FetionEditAccountWidget::FetionEditAccountWidget( QWidget* parent, Kopete::Account* account )
: QWidget(parent),
KopeteEditAccountWidget(account)
{
    m_loginEdit = new KLineEdit;
    m_passwdEdit = new KLineEdit;

    QFormLayout* layout = new QFormLayout;
    setLayout( layout );
    layout->addRow( i18n( "User ID:" ), m_loginEdit );
    layout->addRow( i18n( "Password:" ), m_passwdEdit );
}

FetionEditAccountWidget::~FetionEditAccountWidget()
{
}

Kopete::Account* FetionEditAccountWidget::apply()
{
    if ( !account() )
        setAccount( new FetionAccount( FetionProtocol::protocol(), m_loginEdit->text() ) );

    /// TODO: save account information into config file

    return account();
}

bool FetionEditAccountWidget::validateData()
{
    if ( FetionProtocol::validContactId( m_loginEdit->text() ) )
        return true;
    return false;
}
