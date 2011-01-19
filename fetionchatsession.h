#ifndef FETIONCHATSESSION_H
#define FETIONCHATSESSION_H

#include <kopetechatsession.h>
class KAction;

class FetionChatSession : public Kopete::ChatSession
{
    Q_OBJECT
    public:
        explicit FetionChatSession( const Kopete::Contact* user,
                                     Kopete::ContactPtrList others,
                                     Kopete::Protocol* protocol,
                                     Kopete::ChatSession::Form form = Small );
        virtual ~FetionChatSession();
    private Q_SLOTS:
        void slotMessageSent( Kopete::Message& message, Kopete::ChatSession* chatSession );
    private:
        KAction* m_sendNudge;
        KAction* m_sendSMS;
};

#endif // FETIONCHATSESSION_H
