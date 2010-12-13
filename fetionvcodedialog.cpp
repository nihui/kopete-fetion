#include "fetionvcodedialog.h"

#include <KLocale>
#include <KLineEdit>

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

FetionVCodeDialog::FetionVCodeDialog( const QImage& image, QWidget* parent )
: QDialog(parent)
{
    QVBoxLayout* layout = new QVBoxLayout;
    setLayout( layout );

    m_imagelabel = new QLabel;
    m_edit = new KLineEdit;
    m_confirm = new QPushButton( i18n( "OK" ) );

    m_imagelabel->setPixmap( QPixmap::fromImage( image ) );
    m_imagelabel->adjustSize();

    layout->addWidget( m_imagelabel );
    layout->addWidget( m_edit );
    layout->addWidget( m_confirm );

    connect( m_confirm, SIGNAL(clicked()), this, SLOT(accept()) );
}

FetionVCodeDialog::~FetionVCodeDialog()
{
}

QString FetionVCodeDialog::textValue() const
{
    return m_edit->text();
}

QString FetionVCodeDialog::getInputCode( const QImage& image, QWidget* parent )
{
    FetionVCodeDialog dialog( image, parent );
    dialog.setWindowTitle( i18n( "Input code" ) );

    int ret = dialog.exec();
    if ( ret )
        return dialog.textValue();
    else
        return QString();
}
