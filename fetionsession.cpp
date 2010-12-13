#include "fetionsession.h"

#include "fetioncontact.h"
#include "fetionprotocol.h"
#include "fetionsipnotifier.h"
#include "fetionvcodedialog.h"

#include <kopetechatsession.h>
#include <kopetecontactlist.h>
#include <kopetemessage.h>
#include <kopetemetacontact.h>
#include <kopeteonlinestatus.h>

#include <KDebug>

FetionSession::FetionSession( const QString& accountId )
{
    m_isConnected = false;
    m_accountId = accountId;
    notifier = 0;
    me = 0;
    config = fetion_config_new();
    ul = fetion_user_list_load( config );
}

FetionSession::~FetionSession()
{
    disconnect();
}

bool FetionSession::isConnected() const
{
    return true;
}

Conversation* FetionSession::getConversation( const QString& sId )
{
    qWarning() << sId;
    if ( contactConvHash.contains( sId ) )
        return contactConvHash[ sId ];
    Contact* contact = fetion_contact_list_find_by_userid( me->contactList, sId.toAscii() );
    if ( !contact )
        exit(0);
    Conversation* conv = fetion_conversation_new( me, contact->sipuri, NULL );
    contactConvHash[ sId ] = conv;
    qWarning() << sId << conv;
    return conv;
}

void FetionSession::connect( const QString& password )
{
    kDebug() << "FetionAccount::connectWithPassword";
    /// TODO: connecting stuff here
    me = fetion_user_new( m_accountId.toAscii(), password.toAscii() );

    /**  init */
    struct userlist* newul;
    struct userlist* ul_cur;
    ///
    fetion_user_set_config( me, config );
LOGIN:
    char* pos = ssi_auth_action( me );
    parse_ssi_auth_response( pos , me );
    free( pos );

    if ( me->loginStatus == 421 || me->loginStatus == 420 ) {
        generate_pic_code( me );
        QImage s;
        s.load( QString( me->config->globalPath ) + "/code.gif", "JPEG" );
        QString codetext = FetionVCodeDialog::getInputCode( s );
        fetion_user_set_verification_code( me, codetext.toAscii() );
        goto LOGIN;
    }
    fetion_config_initialize( config, me->userId );

    /* set user list to be stored in local file  */
    newul = fetion_user_list_find_by_no( ul , m_accountId.toAscii() );
    if ( !newul ) {
        newul = fetion_user_list_new( m_accountId.toAscii(), NULL, me->userId, me->sId, P_ONLINE , 1);
        foreach_userlist(ul , ul_cur)
            ul_cur->islastuser = 0;
        fetion_user_list_append(ul , newul);
    }
    else {
        memset(newul->password, 0, sizeof(newul->password));
        newul->laststate = P_ONLINE;
        foreach_userlist(ul , ul_cur)
            ul_cur->islastuser = 0;
        newul->islastuser = 1;
    }
    fetion_user_list_save(config , ul);
    fetion_user_list_free(ul);

    /* download xml configuration file from the server */
    fetion_config_load( me );
    fetion_config_download_configuration( me );
    fetion_config_save( me );
    fetion_user_set_st( me , P_ONLINE );

    /*load local data*/
    int local_group_count;
    int local_buddy_count;
    fetion_user_load( me );
    fetion_contact_load( me, &local_group_count, &local_buddy_count );
    qWarning() << "g & c" << local_group_count << local_group_count;

    /* start a new tcp connection for registering to sipc server */
    FetionConnection* conn = tcp_connection_new();
    tcp_connection_connect( conn, config->sipcProxyIP, config->sipcProxyPort );
    /* add the connection object into the connection list */
    ///fx_conn_append( conn );
    /* initialize a sip object */
    FetionSip* sip = fetion_sip_new( conn , me->sId );
    fetion_user_set_sip( me , sip );

    pos = sipc_reg_action( me );
    char* nonce;
    char* key;
    parse_sipc_reg_response( pos , &nonce , &key );
    free( pos );
    char* aeskey = generate_aes_key();
    char* response = generate_response( nonce, me->userId, me->password, key, aeskey );
    free(nonce);
    free(key);
    free(aeskey);
    /* start sipc authentication using the response created just now */
    int new_group_count;
    int new_buddy_count;
AUTH:
    pos = sipc_aut_action( me , response );
    parse_sipc_auth_response( pos , me, &new_group_count, &new_buddy_count );
    free( pos );
    if ( me->loginStatus == 421 || me->loginStatus == 420 ) {
        generate_pic_code( me );
        QImage s;
        s.load( QString( me->config->globalPath ) + "/code.gif", "JPEG" );
        QString codetext = FetionVCodeDialog::getInputCode( s );
        fetion_user_set_verification_code( me , codetext.toAscii() );
        goto AUTH;
    }

    qWarning() << "auth finished.";
    /* update buddy list */
    if ( me->groupCount == 0 ) {
        qWarning() << "g c";
        me->groupCount = local_group_count;
    }
    else if ( me->groupCount != local_group_count ) {
        qWarning() << local_group_count;
        Group* g_cur;
        Group* g_tmp;
        for ( g_cur = me->groupList->next; g_cur != me->groupList; ) {
            g_tmp = g_cur;
            g_cur = g_cur->next;
            if( !g_tmp->dirty ) {
                fetion_group_list_remove( g_tmp );
                free( g_tmp );
            }
        }
    }
    /* update buddy count */
    if ( me->contactCount == 0 ) {
        qWarning() << "c c";
        me->contactCount = local_buddy_count;
    }
    else if ( me->contactCount != local_buddy_count ) {
        qWarning() << local_buddy_count;
        Contact* c_cur;
        Contact* c_tmp;
        for ( c_cur = me->contactList->next; c_cur != me->contactList; ) {
            c_tmp = c_cur;
            c_cur = c_cur->next;
            if ( !c_tmp->dirty ) {
                fetion_contact_list_remove( c_tmp );
                free( c_tmp );
            }
        }
    }

    /* send get group request */
    pg_group_get_list( me );

    /* if there is not a buddylist name "Ungrouped" or "Strangers", create one */
    Group* group = NULL;
    if ( fetion_group_list_find_by_id(me->groupList,
            BUDDY_LIST_NOT_GROUPED) == NULL &&
            fetion_contact_has_ungrouped(me->contactList) ) {
        group = fetion_group_new();
        group->groupid = BUDDY_LIST_NOT_GROUPED;
        strcpy(group->groupname , "Ungrouped");
        fetion_group_list_append( me->groupList, group );
    }
    if ( fetion_group_list_find_by_id(me->groupList,
            BUDDY_LIST_STRANGER) == NULL &&
            fetion_contact_has_strangers(me->contactList) ) {
        group = fetion_group_new();
        group->groupid = BUDDY_LIST_STRANGER;
        strcpy(group->groupname , "Strangers");
        fetion_group_list_prepend( me->groupList, group );
    }

    qWarning() << "login finished.";


    /// NOTE: debug cases
    Group* grp = me->groupList;
    Contact* contact = me->contactList;
//     QHash<int, Kopete::Group*> groupHash;
    char* sid = NULL;
    foreach_grouplist(me->groupList , grp){
        qWarning() << QString::fromUtf8( grp->groupname ) << grp->groupid;
        emit gotGroup( grp );
//         Kopete::Group* group = Kopete::ContactList::self()->findGroup( QString::fromUtf8( grp->groupname ) );
//         groupHash[grp->groupid] = group;
    }
    foreach_contactlist(me->contactList,contact) {
        if ( strlen(contact->sId) == 0 )
            sid = fetion_sip_get_sid_by_sipuri(contact->sipuri);
        if ( sid ) {
            free( sid );
            sid = NULL;
        }
        qWarning() << "sId" << contact->mobileno << contact->sId
        << QString::fromUtf8( contact->nickname ) << contact->state;
        emit gotContact( contact );
//         addContact( contact->sId, QString::fromUtf8( contact->nickname ), groupHash[contact->groupid] );
//         Kopete::MetaContact* metaContact = new Kopete::MetaContact;
//         FetionContact* c = new FetionContact( this, contact->sId, metaContact );
//         c->setNickName( QString::fromUtf8( contact->nickname ) );
//         c->setPhoto( QString::fromUtf8( config->iconPath ) + '/' + contact->sId + ".jpg" );
//         c->setOnlineStatus( FetionProtocol::protocol()->fetionOnline );
//         metaContact->addToGroup( groupHash[contact->groupid] );
//         Kopete::ContactList::self()->addMetaContact( metaContact );
    }


    /// if not offline login
    fetion_contact_subscribe_only( me );

    /// TODO: create listening thread here !!!
    notifier = new FetionSipNotifier( me->sip, me );
    QObject::connect( notifier, SIGNAL(newThreadEntered(FetionSip*,User*)),
                      this, SLOT(slotNewThreadEntered(FetionSip*,User*)) );
    QObject::connect( notifier, SIGNAL(messageReceived(const QString&,const QString&)),
                      this, SLOT(slotMessageReceived(const QString&,const QString&,const QString&)) );
    QObject::connect( notifier, SIGNAL(presenceChanged(const QString&,StateType)),
                      this, SLOT(slotPresenceChanged(const QString&,StateType)) );
    notifier->start();
    /// fx_main_listen_thread_func();
}

void FetionSession::disconnect()
{
    if ( notifier ) {
        delete notifier;
        notifier = 0;
    }
    if ( me ) {
        fetion_user_free( me );
        me = 0;
    }
}

void FetionSession::setStatus( const Kopete::OnlineStatus& status )
{
}

void FetionSession::slotMessageReceived( const QString& sId, const QString& msgContent, const QString& qsipuri )
{
    if ( !contactConvHash.contains( sId ) ) {
        Conversation* conv = fetion_conversation_new( me, qsipuri.toAscii(), NULL );
        contactConvHash[ sId ] = conv;
        qWarning() << sId << conv;
    }
    emit gotMessage( sId, msgContent );
}

void FetionSession::slotNewThreadEntered( FetionSip* sip, User* user )
{
    FetionSipNotifier* thread = new FetionSipNotifier( sip, user );
//     QObject::connect( thread, SIGNAL(newThreadEntered(FetionSipNotifier*)),
//                       this, SLOT(slotNewThreadEntered(FetionSipNotifier*)) );
    QObject::connect( thread, SIGNAL(messageReceived(const QString&,const QString&,const QString&)),
                      this, SLOT(slotMessageReceived(const QString&,const QString&,const QString&)) );
    QObject::connect( thread, SIGNAL(presenceChanged(const QString&,StateType)),
                      this, SLOT(slotPresenceChanged(const QString&,StateType)) );
    thread->start();
}

void FetionSession::slotPresenceChanged( const QString& sId, StateType state )
{
    Kopete::OnlineStatus status;
    switch ( state ) {
        case P_ONLINE:
            status = Kopete::OnlineStatus::Online;
            break;
        case P_RIGHTBACK:
            status = Kopete::OnlineStatus::Away;
            break;
        case P_AWAY:
            status = Kopete::OnlineStatus::Away;
            break;
        case P_BUSY:
            status = Kopete::OnlineStatus::Busy;
            break;
        case P_OUTFORLUNCH:
        case P_ONTHEPHONE:
        case P_MEETING:
        case P_DONOTDISTURB:
        case P_HIDDEN:
            status = Kopete::OnlineStatus::Away;
            break;
        case P_OFFLINE:
            status = Kopete::OnlineStatus::Offline;
            break;
        default:
            break;
    }
    emit contactStatusChanged( sId, status );
}
