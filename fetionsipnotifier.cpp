#include "fetionsipnotifier.h"

#include <QStringList>
#include <KDebug>
// #include <sys/select.h>

class FetionSipHeader
{
    public:
        explicit FetionSipHeader( const QString& headerStr );
        SipType type() const;
        QString typeAddition() const;
        bool hasKey( const QString& key ) const;
        QList<QString> value( const QString& key ) const;
    private:
        QHash<QString, QList<QString> > m_headerContents;
        SipType m_type;
        QString m_typeAddition;
};

FetionSipHeader::FetionSipHeader( const QString& headerStr )
{
    QStringList list = headerStr.split( "\r\n", QString::SkipEmptyParts );
    QStringList::ConstIterator it = list.constBegin();
    QStringList::ConstIterator end = list.constEnd();

    QString headerTypeStr = *it;
    QString typeId = headerTypeStr.section( QLatin1Char( ' ' ), 0, 0, QString::SectionSkipEmpty );
    QString m_typeAddition = headerTypeStr.section( QLatin1Char( ' ' ), 1, -1, QString::SectionSkipEmpty );
    if ( typeId == QLatin1String( "I" ) )
        m_type = SIP_INVITATION;
    else if ( typeId == QLatin1String( "M" ) )
        m_type = SIP_MESSAGE;
    else if ( typeId == QLatin1String( "IN" ) )
        m_type = SIP_INCOMING;
    else if ( typeId == QLatin1String( "BN" ) )
        m_type = SIP_NOTIFICATION;
    else if ( typeId == QLatin1String( "O" ) )
        m_type = SIP_OPTION;
    else if ( typeId == QLatin1String( "SIP-C/4.0" ) || typeId == QLatin1String( "SIP-C/2.0" ) )
        m_type = SIP_SIPC_4_0;
    else
        m_type = SIP_UNKNOWN;

    ++it;/// skip the first header type line

    while ( it != end ) {
        QStringList keyvalue = (*it).split( ": ", QString::SkipEmptyParts );
        m_headerContents[ keyvalue.at( 0 ) ] << keyvalue.at( 1 );
        ++it;
    }
}

SipType FetionSipHeader::type() const
{
    return m_type;
}

QString FetionSipHeader::typeAddition() const
{
    return m_typeAddition;
}

bool FetionSipHeader::hasKey( const QString& key ) const
{
    return m_headerContents.contains( key );
}

QList<QString> FetionSipHeader::value( const QString& key ) const
{
    return m_headerContents.value( key );
}

/** ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

FetionSipNotifier::FetionSipNotifier( FetionSip* _sip, User* _user, QObject* parent )
: QObject(parent)
{
    sip = _sip;
    user = _user;
    m_socket.setSocketDescriptor( sip->tcp->socketfd );
    connect( &m_socket, SIGNAL(readyRead()), this, SLOT(slotReadyRead()) );
}

FetionSipNotifier::~FetionSipNotifier()
{
    m_socket.close();
}

void FetionSipNotifier::slotReadyRead()
{
    QByteArray data = m_socket.readAll();
    if ( data.isEmpty() )
        return;

    QString datastr = QString::fromUtf8( data );
    qWarning() << datastr;

    int index = 0;
    int len = datastr.size();

    int headerBegin;
    int headerEnd;

    while ( index < len ) {
        headerBegin = index;
        headerEnd = datastr.indexOf( "\r\n\r\n", headerBegin ) + 4;/// length of \r\n\r\n
        QString headerStr = datastr.mid( headerBegin, headerEnd - headerBegin );

        FetionSipHeader header( headerStr );

        switch ( header.type() ) {
            case SIP_INVITATION: {
                int contentBegin = datastr.indexOf( "s=", headerEnd );
                int contentEnd = datastr.size();/// FIXME: i'm just lazy here...  ;)  --- nihui
                QString contentStr = datastr.mid( contentBegin, contentEnd - contentBegin );
                qWarning() << contentStr;

                QString from = header.value( "F" ).at( 0 );
                QString auth = header.value( "A" ).at( 0 );
                QString addressList = auth.section( '\"', 1, 1, QString::SectionSkipEmpty );
                QString credential = auth.section( '\"', 3, 3, QString::SectionSkipEmpty );
                QString firstAddressPort = addressList.section( ';', 0, 0, QString::SectionSkipEmpty );
                QString firstAddress = firstAddressPort.section( ':', 0, 0, QString::SectionSkipEmpty );
                QString firstPort = firstAddressPort.section( ':', 1, 1, QString::SectionSkipEmpty );

                FetionConnection* conn = tcp_connection_new();
                tcp_connection_connect( conn, firstAddress.toAscii().constData(), firstPort.toInt() );/// TODO: proxy here
                FetionSip* conversionSip = fetion_sip_clone( sip );
                fetion_sip_set_connection( conversionSip , conn );
                qWarning() << "Received a conversation invitation";

                char buf[1024] = { '\0' };
                sprintf( buf , "SIP-C/4.0 200 OK\r\nF: %s\r\nI: -61\r\nQ: 200002 I\r\n\r\n", from.toAscii().constData() );
                tcp_connection_send( sip->tcp , buf , strlen( buf ) );

                fetion_sip_set_type( sip , SIP_REGISTER );
                SipHeader* aheader = fetion_sip_credential_header_new( credential.toAscii() );
                SipHeader* theader = fetion_sip_header_new( "K" , "text/html-fragment" );
                SipHeader* mheader = fetion_sip_header_new( "K" , "multiparty" );
                SipHeader* nheader = fetion_sip_header_new( "K" , "nudge" );
                fetion_sip_add_header( sip , aheader );
                fetion_sip_add_header( sip , theader );
                fetion_sip_add_header( sip , mheader );
                fetion_sip_add_header( sip , nheader );
                char* sipres = fetion_sip_to_string( sip , NULL );
                qWarning() << "Register to conversation server" << firstAddressPort;
                tcp_connection_send( conn , sipres , strlen( sipres ) );
                free( sipres );
                memset( buf, '\0', sizeof(buf)*sizeof(char) );
                int port = tcp_connection_recv( conn , buf , sizeof(buf)*sizeof(char) );

                memset( conversionSip->sipuri, 0, sizeof(conversionSip->sipuri)*sizeof(char) );
                strcpy( conversionSip->sipuri, from.toAscii().constData() );

                emit newThreadEntered( conversionSip, user );

                index = contentEnd;
                break;
            }
            case SIP_MESSAGE: {
                int contentBegin = datastr.indexOf( "<Font", headerEnd );
                int contentEnd = datastr.indexOf( "</Font>", contentBegin ) + 7;/// length of </Font>
                QString contentStr = datastr.mid( contentBegin, contentEnd - contentBegin );
                qWarning() << contentStr;

//                 Message* msg = fetion_message_new();
//                 fetion_message_free( msg );

                QString from = header.value( "F" ).at( 0 );
                QString callid = header.value( "I" ).at( 0 );
                QString sequence = header.value( "Q" ).at( 0 );
                QString sendtime = header.value( "D" ).at( 0 );

                char rep[256] = { '\0' };
                sprintf( rep, "SIP-C/4.0 200 OK\r\nF: %s\r\nI: %s\r\nQ: %s\r\n\r\n",
                         from.toAscii().constData(), callid.toAscii().constData(), sequence.toAscii().constData() );
                tcp_connection_send( sip->tcp, rep, strlen( rep ) );

                index = contentEnd;
                break;
            }
            case SIP_INCOMING: {
                break;
            }
            case SIP_NOTIFICATION: {
                QString sid = header.typeAddition().section( ' ', 0, 0, QString::SectionSkipEmpty );
                QString notificationType = header.value( "N" ).at( 0 );

                int contentBegin = datastr.indexOf( "<events>", headerEnd );
                int contentEnd = datastr.indexOf( "</events>", contentBegin ) + 9;/// length of </events>
                QString contentStr = datastr.mid( contentBegin, contentEnd - contentBegin );
                qWarning() << contentStr;

                index = contentEnd;

                if ( notificationType == QLatin1String( "PresenceV4" ) ) {
                    /// NOTIFICATION_TYPE_PRESENCE
                }
                else if ( notificationType == QLatin1String( "Conversation" ) ) {
                    /// NOTIFICATION_TYPE_CONVERSATION
                }
                else if ( notificationType == QLatin1String( "contact" ) ) {
                    /// NOTIFICATION_TYPE_CONTACT
                }
                else if ( notificationType == QLatin1String( "registration" ) ) {
                    /// NOTIFICATION_TYPE_REGISTRATION
                }
                else if ( notificationType == QLatin1String( "SyncUserInfoV4" ) ) {
                    /// NOTIFICATION_TYPE_SYNCUSERINFO
                }
                else if ( notificationType == QLatin1String( "PGGroup" ) ) {
                    /// NOTIFICATION_TYPE_PGGROUP
                }
                else {
                    /// NOTIFICATION_TYPE_UNKNOWN
                }
        //         QDomDocument doc;
        //         doc.setContent( list.last() );
                break;
            }
            case SIP_OPTION: {
                break;
            }
            case SIP_SIPC_4_0: {
                if ( header.hasKey( "L" ) ) {
                    int contentBegin = datastr.indexOf( "<results>", headerEnd );
                    int contentEnd = datastr.indexOf( "</results>", contentBegin ) + 10;/// length of </results>
                    index = contentEnd;
                }
                else {
                    index = headerEnd;
                }
                break;
            }
            case SIP_UNKNOWN: {
                qWarning() << "unknown sip message" << datastr;
                return;
            }
        }

        qWarning() << "#############################";
    }
}

void FetionSipNotifier::handleSipMessage( SipMsg* sipmsg )
{
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
//                     QThread::usleep(1);
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

}
