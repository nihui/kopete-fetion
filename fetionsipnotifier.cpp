#include "fetionsipnotifier.h"

#include <KDebug>
#include <sys/select.h>

FetionSipNotifier::FetionSipNotifier( FetionSip* _sip, User* _user )
{
    sip = _sip;
    user = _user;
}

FetionSipNotifier::~FetionSipNotifier()
{
}

void FetionSipNotifier::run()
{
    fd_set fd_read;
    struct timeval tv;
    for ( ; ; ) {
        FD_ZERO(&fd_read);
        FD_SET(sip->tcp->socketfd, &fd_read);
        tv.tv_sec = 13;
        tv.tv_usec = 0;

        int ret = select(sip->tcp->socketfd+1, &fd_read, NULL, NULL, &tv);

        if (ret == 0)
            continue;
        if (ret == -1) {
//             qWarning ("Error.. to read socket, exit thread");
            if(sip != user->sip){
//                 qWarning("Error.. thread sip freed\n");
                free(sip);
            }
//             QThread::setTerminationEnabled(true);
//             quit();
            return;
        }

        if (!FD_ISSET(sip->tcp->socketfd, &fd_read)) {
            QThread::usleep(100);
            continue;
        }

        int error;
        SipMsg* sipmsg = fetion_sip_listen( sip, &error );
        if ( !sipmsg && error ) {
            if ( sip == user->sip ) {
    //             gdk_threads_enter();
//                 qWarning("\n\nError ...... break out...\n\n");
                ///fx_conn_offline(fxmain);
    //             gdk_threads_leave();
//                 QThread::setTerminationEnabled(true);
//                 quit();
                return;
            }
            else{
//                 qWarning("\n\n Error ... user listen thread break out\n\n");
                ///chat_listen_thread_end( fxmain, sip->sipuri );
                tcp_connection_free( sip->tcp );
//                 QThread::setTerminationEnabled(true);
//                 quit();
                return;
            }
        }

        /* handle the message */
        SipMsg* pos = sipmsg;
        int type;
        while ( pos ) {
            type = fetion_sip_get_type( pos->message );
            switch ( type ) {
                case SIP_NOTIFICATION: {
                    qWarning() << "SIP_NOTIFICATION";
                    int event, notification_type;
                    char* xml;
                    fetion_sip_parse_notification( pos->message , &notification_type , &event , &xml );
                    QThread::usleep(1);
                    switch ( notification_type ) {
                        case NOTIFICATION_TYPE_PRESENCE:
                            switch (event) {
                                case NOTIFICATION_EVENT_PRESENCECHANGED: {
                                    qWarning() << "NOTIFICATION_EVENT_PRESENCECHANGED";
                                    Contact* contactlist = fetion_user_parse_presence_body( xml , user );
                                    Contact* contact = contactlist;
                                    foreach_contactlist(contactlist , contact){
                                        qWarning() << QString::fromUtf8( contact->nickname ) << contact->state;
                                        emit presenceChanged( contact->sId, contact->state );
                                    }
                                    ///fx_main_process_presence(fxmain , xml);
                                    break;
                                }
                                default:
                                    break;
                            }
                        case NOTIFICATION_TYPE_CONVERSATION:
                            if(event == NOTIFICATION_EVENT_USERLEFT){
                                qWarning() << "NOTIFICATION_EVENT_USERLEFT";
                                ///fx_main_process_user_left(fxmain , sipmsg);
                                break;
                            }
                            break;
                        case NOTIFICATION_TYPE_REGISTRATION:
                            if(event == NOTIFICATION_EVENT_DEREGISTRATION){
                                qWarning() << "NOTIFICATION_EVENT_DEREGISTRATION";
                                ///fx_main_process_deregistration(fxmain);
                                break;
                            }
                            break;
                        case NOTIFICATION_TYPE_SYNCUSERINFO:
                            if(event == NOTIFICATION_EVENT_SYNCUSERINFO){
                                qWarning() << "NOTIFICATION_EVENT_SYNCUSERINFO";
                                ///fx_main_process_syncuserinfo(fxmain , xml);
                                break;
                            }
                            break;
                        case NOTIFICATION_TYPE_CONTACT:
                            if(event == NOTIFICATION_EVENT_ADDBUDDYAPPLICATION){
                                qWarning() << "NOTIFICATION_EVENT_ADDBUDDYAPPLICATION";
                                ///fx_main_process_addbuddyapplication(fxmain , sipmsg);
                                break;
                            }
                            break;
                        case NOTIFICATION_TYPE_PGGROUP:
                            if(event == NOTIFICATION_EVENT_PGGETGROUPINFO){
                                qWarning() << "NOTIFICATION_EVENT_PGGETGROUPINFO";
                                ///fx_main_process_pggetgroupinfo(fxmain , sipmsg);
                                break;
                            }
                            else if(event == NOTIFICATION_EVENT_PRESENCECHANGED){
                                qWarning() << "NOTIFICATION_EVENT_PRESENCECHANGED";
                                ///fx_main_process_pgpresencechanged(fxmain , sipmsg);
                                break;
                            }
                            break;
                        default:
                            break;
                    }
                    free( xml );
                    ///fx_main_process_notification(fxmain , pos->message);
                    break;
                }
                case SIP_MESSAGE: {
                    qWarning() << "SIP_MESSAGE";
                    Message* msg;
                    fetion_sip_parse_message( sip , pos->message , &msg );
                    /* group message */
                    if ( msg->pguri ) {
                        ///process_group_message(fxmain , msg);
                    }
                    else {
                        char* sid = fetion_sip_get_sid_by_sipuri( msg->sipuri );
                        if(strlen(sid) < 5 || strcmp(sid , "10000") == 0){
                            /* system message */
                            ///process_system_message( pos->message );
                        }
                        else {
                            /// msg->message, msg->sendtime, msg->sysback
                            QString sId( sid );
                            QString msgContent = QString::fromUtf8( msg->message );
                            QString qsipuri( msg->sipuri );
                            qWarning() << sId << msgContent << qsipuri;
                            emit messageReceived( sId, msgContent, qsipuri );
                        }
                        free( sid );
                    }
                    fetion_message_free( msg );

                    ///fx_main_process_message(fxmain , sip , pos->message);
                    break;
                }
                case SIP_INVITATION: {
                    qWarning() << "SIP_INVITATION";
                    FetionSip* osip;
                    char* sipuri;
                    char event[16];
                    memset( event, 0, sizeof( event ) * sizeof( char ) );
                    if ( fetion_sip_get_attr( pos->message , "N" , event ) != -1 )
                        break;
                    fetion_sip_parse_invitation( sip, user->config->proxy, pos->message, &osip, &sipuri );

                    /* create a thread to listen in this channel */
                    emit newThreadEntered( osip, user );
//                     FetionSipNotifier* thread = new FetionSipNotifier( osip, user );
//                     emit newThreadEntered( thread );
//                     QObject::connect( thread, SIGNAL(messageReceived(const Message*)),
//                                       this, SIGNAL(messageReceived(const Message*)) );
//                     QObject::connect( thread, SIGNAL(presenceChanged(const QString&,StateType)),
//                                       this, SIGNAL(presenceChanged(const QString&,StateType)) );
//                     thread->start();
                    /// send keep alive through this channel

                    ///fx_main_process_invitation(fxmain , pos->message);
                    break;
                }
                case SIP_INCOMING:
                    qWarning() << "SIP_INCOMING";
                    IncomingType type;
                    IncomingActionType action;
                    char* sipuri;
                    fetion_sip_parse_incoming( sip , pos->message , &sipuri , &type , &action );
                    switch (type) {
                        case INCOMING_NUDGE: {
                            qWarning() << "INCOMING_NUDGE";
//                             gdk_threads_enter();
//                             fxchat = fx_main_create_chat_window(fxmain , sipuri);
//                             gdk_threads_leave();

//                             fx_chat_nudge_in_thread(fxchat);

//                             gdk_threads_enter();
//                             fx_chat_add_information(fxchat , _("Receive a window jitter"));
//                             gdk_threads_leave();
//                             debug_info("Received a nudge from %s" , sipuri);
                            break;
                        }
                        case INCOMING_SHARE_CONTENT: {
                            switch (action) {
                                case INCOMING_ACTION_ACCEPT:
                                    qWarning() << "INCOMING_ACTION_ACCEPT";
                                    //process_share_action_accept(fxmain , sip , sipmsg , sipuri);
                                    break;
                                case INCOMING_ACTION_CANCEL :
                                    qWarning() << "INCOMING_ACTION_CANCEL";
                                    //process_share_action_cancel(fxmain , sip , sipmsg , sipuri);
                                    break;
                                default:
                                    break;
                            }
                            break;
                        }
                        default:
                            break;
                    }
                    ///fx_main_process_incoming(fxmain , sip , pos->message);
                    break;
                case SIP_SIPC_4_0: {
                    qWarning() << "SIP_SIPC_4_0";
                    int callid;
                    char* xml;
                    int code = fetion_sip_parse_sipc( pos->message , &callid , &xml );
                    free( xml );
                    ///fx_main_process_sipc(fxmain , pos->message);
                    break;
                }
                default:
                    qWarning() << "UNKNOWN SIP MESSAGE";
                    //printf("%s\n" , pos->message);
                    break;
            }
            pos = pos->next;
        }

        fetion_sip_message_free( sipmsg );
    }
}
