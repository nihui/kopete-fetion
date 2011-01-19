#ifndef FETIONEDITACCOUNTWIDGET_H
#define FETIONEDITACCOUNTWIDGET_H

#include <editaccountwidget.h>
#include <QWidget>

namespace Ui { class FetionEditAccount; };
class FetionEditAccountWidget : public QWidget, public KopeteEditAccountWidget
{
    Q_OBJECT
    public:
        explicit FetionEditAccountWidget( QWidget* parent, Kopete::Account* account );
        virtual ~FetionEditAccountWidget();
        virtual Kopete::Account* apply();
        virtual bool validateData();
    private Q_SLOTS:
        void slotOpenRegister();
    private:
        Ui::FetionEditAccount* m_widget;
};

#endif // FETIONEDITACCOUNTWIDGET_H
