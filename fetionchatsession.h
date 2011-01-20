#ifndef FETIONCHATSESSION_H
#define FETIONCHATSESSION_H

#include <kopetechatsession.h>
class KAction;
class KToggleAction;
class FetionSipNotifier;

class FetionChatSession : public Kopete::ChatSession
{
    Q_OBJECT
    public:
        explicit FetionChatSession( const Kopete::Contact* user,
                                     Kopete::ContactPtrList others,
                                     Kopete::Protocol* protocol,
                                     Kopete::ChatSession::Form form = Small );
        virtual ~FetionChatSession();
//         void requestChatChannel();
        void setChatChannel( FetionSipNotifier* chatChannel );
        FetionSipNotifier* chatChannel() const;
    private Q_SLOTS:
        void slotMessageSent( Kopete::Message& message, Kopete::ChatSession* chatSession );
        void slotSendNudge();
//         void slotSendPending();
    private:
        FetionSipNotifier* m_chatChannel;
        KAction* m_sendNudge;
        KToggleAction* m_sendSMS;
};

#endif // FETIONCHATSESSION_H
