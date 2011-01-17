#ifndef FETIONCONTACT_H
#define FETIONCONTACT_H

#include <kopetecontact.h>
#include <kopetemessage.h>
#include <QList>

class FetionContact : public Kopete::Contact
{
    Q_OBJECT
    public:
        explicit FetionContact( Kopete::Account* account, const QString& id, Kopete::MetaContact* parent );
        virtual ~FetionContact();
        virtual bool isReachable();
//         virtual QList<KAction*>* customContextMenuActions();
        virtual Kopete::ChatSession* manager( Kopete::Contact::CanCreateFlags canCreate = Kopete::Contact::CannotCreate );
        virtual void serialize( QMap<QString, QString>& serializedData,
                                QMap<QString, QString>& addressBookData );
    private Q_SLOTS:
        void slotMessageSent( Kopete::Message& message, Kopete::ChatSession* chatSession );
    private:
        Kopete::ChatSession* m_manager;
};

#endif // FETIONCONTACT_H
