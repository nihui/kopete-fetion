#ifndef FETIONCHATSESSION_H
#define FETIONCHATSESSION_H

#include <kopetechatsession.h>

class FetionChatSession : public Kopete::ChatSession
{
    Q_OBJECT
    public:
        virtual ~FetionChatSession();
    protected:
        FetionChatSession( const Kopete::Contact* user,
                           Kopete::ContactPtrList others,
                           Kopete::Protocol* protocol );
//     private Q_SLOTS:
//         void slotMessageSent( Kopete::Message& message, Kopete::ChatSession* chatSession );
    private:
//         Conversation* conv;
};

#endif // FETIONCHATSESSION_H
