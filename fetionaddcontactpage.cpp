#include "fetionaddcontactpage.h"

#include "fetionprotocol.h"

#include <KLineEdit>
#include <KLocale>
#include <QFormLayout>

FetionAddContactPage::FetionAddContactPage( QWidget* parent )
: AddContactPage(parent)
{
    m_contactEdit = new KLineEdit;

    QFormLayout* layout = new QFormLayout;
    setLayout( layout );

    layout->addRow( i18n( "Contact:" ), m_contactEdit );
}

FetionAddContactPage::~FetionAddContactPage()
{
}

bool FetionAddContactPage::apply( Kopete::Account* account, Kopete::MetaContact* metaContact )
{
    /// TODO
    return false;
}

bool FetionAddContactPage::validateData()
{
    if ( FetionProtocol::validContactId( m_contactEdit->text() ) )
        return true;
    return false;
}
