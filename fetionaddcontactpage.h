#ifndef FETIONADDCONTACTPAGE_H
#define FETIONADDCONTACTPAGE_H

#include <ui/addcontactpage.h>

class KLineEdit;

class FetionAddContactPage : public AddContactPage
{
    Q_OBJECT
    public:
        explicit FetionAddContactPage( QWidget* parent );
        virtual ~FetionAddContactPage();
        virtual bool apply( Kopete::Account* account, Kopete::MetaContact* metaContact );
        virtual bool validateData();
    private:
        KLineEdit* m_contactEdit;
};

#endif // FETIONADDCONTACTPAGE_H
