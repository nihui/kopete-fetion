#ifndef FETIONPROTOCOL_H
#define FETIONPROTOCOL_H

#include <kopeteprotocol.h>
#include <QVariantList>

class FetionProtocol : public Kopete::Protocol
{
    Q_OBJECT
    public:
        FetionProtocol( QObject* parent, const QVariantList& args );
        virtual ~FetionProtocol();
        /** Generate the widget needed to add FetionAccount */
        virtual AddContactPage* createAddContactWidget( QWidget* parent, Kopete::Account* account );
        /** Generate the widget needed to add/edit accounts for this protocol */
        virtual KopeteEditAccountWidget* createEditAccountWidget( Kopete::Account* account, QWidget* parent);
        /** Generate a FetionAccount */
        virtual Kopete::Account* createNewAccount( const QString& accountId );
        /** Convert the serialized data back into a FetionAccount and add this to its Kopete::MetaContact */
        virtual Kopete::Contact* deserializeContact( Kopete::MetaContact* metaContact,
                                                     const QMap<QString, QString>& serializedData,
                                                     const QMap<QString, QString>& addressBookData );
        /** Access the instance of this protocol */
        static FetionProtocol* protocol();
        /** Validate whether userId is a legal Fetion account */
        static bool validContactId( const QString& userId );

        const Kopete::OnlineStatus fetionOnline;
        const Kopete::OnlineStatus fetionAway;
        const Kopete::OnlineStatus fetionBusy;
        const Kopete::OnlineStatus fetionInvisible;
        const Kopete::OnlineStatus fetionOffline;
    protected:
        static FetionProtocol* s_protocol;
};

#endif // FETIONPROTOCOL_H
