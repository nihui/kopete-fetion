#ifndef FETIONPROTOCOL_H
#define FETIONPROTOCOL_H

#include <kopeteprotocol.h>
#include <QVariantList>

#define P_OFFLINE       -1
#define P_HIDDEN        0
#define P_AWAY          100
#define P_OUTFORLAUNCH  150
#define P_RIGHTBACK     300
#define P_ONLINE        400
#define P_ONTHEPHONE    500
#define P_BUSY          600
#define P_DONOTDISTURB  800
#define P_MEETING       850

class FetionProtocol : public Kopete::Protocol
{
    Q_OBJECT
    public:
        explicit FetionProtocol( QObject* parent, const QVariantList& args );
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

        const Kopete::OnlineStatus fetionOffline;
        const Kopete::OnlineStatus fetionInvisible;
        const Kopete::OnlineStatus fetionAway;
        const Kopete::OnlineStatus fetionOutForLaunch;
        const Kopete::OnlineStatus fetionRightBack;
        const Kopete::OnlineStatus fetionOnline;
        const Kopete::OnlineStatus fetionOnThePhone;
        const Kopete::OnlineStatus fetionBusy;
        const Kopete::OnlineStatus fetionDoNotDisturb;
        const Kopete::OnlineStatus fetionMeeting;
    protected:
        static FetionProtocol* s_protocol;
};

#endif // FETIONPROTOCOL_H
