#ifndef FETIONEDITACCOUNTWIDGET_H
#define FETIONEDITACCOUNTWIDGET_H

#include <ui/editaccountwidget.h>
#include <QWidget>

class KLineEdit;

class FetionEditAccountWidget : public QWidget, public KopeteEditAccountWidget
{
    Q_OBJECT
    public:
        explicit FetionEditAccountWidget( QWidget* parent, Kopete::Account* account );
        virtual ~FetionEditAccountWidget();
        virtual Kopete::Account* apply();
        virtual bool validateData();
    private:
        KLineEdit* m_loginEdit;
        KLineEdit* m_passwdEdit;
};

#endif // FETIONEDITACCOUNTWIDGET_H
