#include "fetionsession.h"

#include "fetioncontact.h"
#include "fetionprotocol.h"
#include "fetionsipevent.h"
#include "fetionsipnotifier.h"
#include "fetionvcodedialog.h"

#include <kopetechatsession.h>
#include <kopetecontactlist.h>
#include <kopetemessage.h>
#include <kopetemetacontact.h>
#include <kopeteonlinestatus.h>

#include <KDebug>

#include <QCryptographicHash>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QSslConfiguration>

#include <ctime>

static QByteArray myhash( int userId, QByteArray password )
{
    QByteArray domain( "fetion.com.cn:" );
    QByteArray data = domain + password;
    QByteArray digest = QCryptographicHash::hash( data, QCryptographicHash::Sha1 );
//     qWarning() << "digest" << digest;
    QByteArray hexdigest = digest.toHex();
//     qWarning() << "hexdigest" << hexdigest;
    if ( userId == 0 )
        return hexdigest;

    QByteArray hexpassword = password.toHex();
    char b[4];
    int i = userId;
    memcpy( b, &i, 4 );
    QByteArray userid( b );
    QByteArray data2 = userid + hexpassword;
    QByteArray digest2 = QCryptographicHash::hash( data2, QCryptographicHash::Sha1 );
    return digest2;
}

static QString generateNouce()
{
    qsrand( time( NULL ) );
    QString nouce;
    nouce.sprintf( "%04X%04X%04X%04X%04X%04X%04X%04X",
                   qrand() & 0xFFFF , qrand() & 0xFFFF,
                   qrand() & 0xFFFF , qrand() & 0xFFFF,
                   qrand() & 0xFFFF , qrand() & 0xFFFF,
                   qrand() & 0xFFFF , qrand() & 0xFFFF );
    return nouce;
}

FetionSession::FetionSession( QObject* parent ) : QObject(parent)
{
    notifier = 0;
    manager = new QNetworkAccessManager( this );
}

FetionSession::~FetionSession()
{
    qWarning() << "FetionSession::~FetionSession()";
//     delete notifier;
//     disconnect();
}

void FetionSession::setLoginInformation( const QString& accountId, const QString& password )
{
    m_accountId = accountId;
    m_password = password;
}

#define PROTO_VERSION "4.0.2510"
void FetionSession::login()
{
    /// post xml system config data
    /// we do not use qdomdocument here since the data xml node attributes are order-sensitive
    QString xml;
    xml += "<config>";
    xml += "<user mobile-no=\"";
    xml += m_accountId;
    xml += "\"/>";
    xml += "<client type=\"PC\" version=\""PROTO_VERSION"\" platform=\"W5.1\"/>";
    xml += "<servers version=\"0\"/><parameters version=\"0\"/><hints version=\"0\"/>";
//         xml += "<client-config veersion=\"0\"/><services veersion=\"0\"/>";
    xml += "</config>";

    /// get system configuration
    QNetworkRequest request;

    QUrl url;
    url.setUrl( "http://nav.fetion.com.cn/nav/getsystemconfig.aspx" );
    url.setPort( 80 );
    request.setUrl( url );

    request.setRawHeader( "User-Agent", "IIC2.0/PC "PROTO_VERSION );
    request.setRawHeader( "Host", "nav.fetion.com.cn" );
    request.setRawHeader( "Connection", "Close" );
    request.setRawHeader( "Content-Length", QString::number( xml.toAscii().size() ).toAscii() );
    qWarning() << "@@@@@@@@@" << QString::number( xml.toAscii().size() );

    QNetworkReply* r = manager->post( request, xml.toAscii() );
    connect( r, SIGNAL(finished()), this, SLOT(getSystemConfigFinished()) );
}

void FetionSession::getSystemConfigFinished()
{
    QNetworkReply* reply = (QNetworkReply*)sender();
    QString data = QString::fromUtf8( reply->readAll() );
    reply->deleteLater();

    QDomDocument doc;
    doc.setContent( data );
    QDomElement docElem = doc.documentElement();
    QDomElement serversElem = docElem.firstChildElement( "servers" );
    m_ssiAppSignInV2Uri = serversElem.firstChildElement( "ssi-app-sign-in-v2" ).text();
    m_getPicCodeUri = serversElem.firstChildElement( "get-pic-code" ).text();
    m_sipcProxyAddress = serversElem.firstChildElement( "sipc-proxy" ).text();
    m_sipcSslProxyAddress = serversElem.firstChildElement( "sipc-ssl-proxy" ).text();
    m_httpTunnelAddress = serversElem.firstChildElement( "http-tunnel" ).text();

    /// ssi auth
    QNetworkRequest request;
    request.setSslConfiguration( QSslConfiguration::defaultConfiguration() );

    QUrl url;
    url.setUrl( m_ssiAppSignInV2Uri );
    url.setPort( 443 );
    url.addQueryItem( "mobileno", m_accountId );
    url.addQueryItem( "domains", "fetion.com.cn" );
    url.addQueryItem( "v4digest-type", "1" );
    url.addQueryItem( "v4digest", myhash( 0 , m_password.toAscii() ) );

    if ( !picid.isEmpty() && !vcode.isEmpty() && !algorithm.isEmpty() ) {
        url.addQueryItem( "pid", picid );
        url.addQueryItem( "pic", vcode );
        url.addQueryItem( "algorithm", algorithm );
    }

    request.setUrl( url );

    request.setRawHeader( "User-Agent", "IIC2.0/pc "PROTO_VERSION );
    request.setRawHeader( "Host", "uid.fetion.com.cn" );
    request.setRawHeader( "Cache-Control", "private" );
    request.setRawHeader( "Connection", "Keep-Alive" );

    QNetworkReply* r = manager->get( request );
    connect( r, SIGNAL(finished()), this, SLOT(ssiAuthFinished()) );
}

void FetionSession::ssiAuthFinished()
{
    QNetworkReply* reply = (QNetworkReply*)sender();
    QString data = QString::fromUtf8( reply->readAll() );
    reply->deleteLater();
    qWarning() << data;

    QDomDocument doc;
    doc.setContent( data );
    QDomElement docElem = doc.documentElement();
    QString statusCode = docElem.attribute( "status-code" );

    if ( statusCode == "421" || statusCode == "420" ) {
        QDomElement verificationElem = docElem.firstChildElement( "verification" );
        algorithm = verificationElem.attribute( "algorithm" );
        QString type = verificationElem.attribute( "type" );
        QString text = verificationElem.attribute( "text" );
        QString tips = verificationElem.attribute( "tips" );

        /// generate pic code
        QNetworkRequest request;

        QUrl url;
        url.setUrl( m_getPicCodeUri );
        url.setPort( 80 );
        url.addQueryItem( "algorithm", algorithm );// + " HTTP/1.1" );
        request.setUrl( url );

        request.setRawHeader( "User-Agent", "IIC2.0/PC "PROTO_VERSION );
        request.setRawHeader( "Host", "nav.fetion.com.cn" );
        request.setRawHeader( "Connection", "Close" );

        QNetworkReply* r = manager->get( request );
        connect( r, SIGNAL(finished()), this, SLOT(getCodePicFinished()) );
        return;
    }
    else if ( statusCode == "200" ) {
        /// login success
        qWarning() << "fetion login success   :)";
        /// emit success signal
        QDomElement userElem = docElem.firstChildElement( "user" );
        QString sipuri = userElem.attribute( "uri" );
        QString mobileno = userElem.attribute( "mobile-no" );
        QString userstatus = userElem.attribute( "user-status" );
        QString userid = userElem.attribute( "user-id" );

        QString sId = sipuri.section( ':', 1, -1, QString::SectionSkipEmpty ).section( '@', 0, 0, QString::SectionSkipEmpty );
        m_from = sId;

        /// connect to sipc server
        QString sipcAddressIp = m_sipcProxyAddress.section( ':', 0, 0, QString::SectionSkipEmpty );
        int sipcAddressPort = m_sipcProxyAddress.section( ':', -1, -1, QString::SectionSkipEmpty ).toInt();
        notifier = new FetionSipNotifier( this );
        notifier->connectToHost( sipcAddressIp, sipcAddressPort );

        /// sipc register
        QString m_nouce = generateNouce();
        FetionSipEvent registerEvent( FetionSipEvent::SipRegister, FetionSipEvent::EventUnknown );
        registerEvent.addHeader( "F", m_from );
        registerEvent.addHeader( "I", QString::number( FetionSipEvent::nextCallid() ) );
        registerEvent.addHeader( "Q", "2 R" );
        registerEvent.addHeader( "CN", m_nouce );
        registerEvent.addHeader( "CL", "type=\"pc\" ,version=\""PROTO_VERSION"\"" );
        qWarning() << registerEvent.toString();
        notifier->sendSipEvent( registerEvent );
    }
}

void FetionSession::getCodePicFinished()
{
    QNetworkReply* reply = (QNetworkReply*)sender();
    QString data = QString::fromUtf8( reply->readAll() );
    reply->deleteLater();
    qWarning() << data;

    QDomDocument doc;
    doc.setContent( data );
    QDomElement docElem = doc.documentElement();
    QDomElement picElem = docElem.firstChildElement( "pic-certificate" );
    picid = picElem.attribute( "id" );
    QString pic = picElem.attribute( "pic" );
    QByteArray picdata = QByteArray::fromBase64( pic.toAscii() );
    QImage img = QImage::fromData( picdata, "JPEG" );
    QString codetext = FetionVCodeDialog::getInputCode( img );
    qWarning() << codetext;

    vcode = codetext;

    login();
}

void FetionSession::handleSipEvent( const FetionSipEvent& sipEvent )
{
    switch ( sipEvent.sipType() ) {
        case FetionSipEvent::SipInvitation: {
//                 qWarning() << contentStr;

//                 QString from = newEvent.getFirstValue( "F" );
//                 QString auth = newEvent.getFirstValue( "A" );
//                 QString addressList = auth.section( '\"', 1, 1, QString::SectionSkipEmpty );
//                 QString credential = auth.section( '\"', 3, 3, QString::SectionSkipEmpty );
//                 QString firstAddressPort = addressList.section( ';', 0, 0, QString::SectionSkipEmpty );
//                 QString firstAddress = firstAddressPort.section( ':', 0, 0, QString::SectionSkipEmpty );
//                 QString firstPort = firstAddressPort.section( ':', 1, 1, QString::SectionSkipEmpty );

//                 FetionSipNotifier conversionNotifier;
//                 qWarning() << "Received a conversation invitation";
//
//                 conversionNotifier.connectToHost( firstAddress, firstPort.toInt() );
//
//                 FetionSipEvent returnEvent( FetionSipEvent::SipSipc_4_0, FetionSipEvent::EventUnknown );
//                 returnEvent.setTypeAddition( "200 OK" );
//                 returnEvent.addHeader( "F", from );
//                 returnEvent.addHeader( "I", "-61" );
//                 returnEvent.addHeader( "Q", "200002 I" );
//
//                 conversionNotifier.sendSipEvent( returnEvent );
//
//                 FetionSipEvent registerEvent( FetionSipEvent::SipRegister, FetionSipEvent::EventUnknown );
//                 registerEvent.addHeader( "A", "TICKS auth=\"" + credential + "\"" );
//                 registerEvent.addHeader( "K", "text/html-fragment" );
//                 registerEvent.addHeader( "K", "multiparty" );
//                 registerEvent.addHeader( "K", "nudge" );
//
//                 qWarning() << "Register to conversation server" << firstAddressPort;
//                 conversionNotifier.sendSipEvent( registerEvent );

            ///memset( buf, '\0', sizeof(buf)*sizeof(char) );
            ///int port = tcp_connection_recv( conn , buf , sizeof(buf)*sizeof(char) );

            ///memset( conversionSip->sipuri, 0, sizeof(conversionSip->sipuri)*sizeof(char) );
            ///strcpy( conversionSip->sipuri, from.toAscii().constData() );

//                 emit newThreadEntered( conversionSip, user );
            break;
        }
        case FetionSipEvent::SipMessage: {
//                 qWarning() << contentStr;

//                 QString from = newEvent.getFirstValue( "F" );
//                 QString callid = newEvent.getFirstValue( "I" );
//                 QString sequence = newEvent.getFirstValue( "Q" );
//                 QString sendtime = newEvent.getFirstValue( "D" );
//
//                 FetionSipEvent returnEvent( FetionSipEvent::SipSipc_4_0, FetionSipEvent::EventUnknown );
//                 returnEvent.setTypeAddition( "200 OK" );
//                 returnEvent.addHeader( "F", from );
//                 returnEvent.addHeader( "I", callid );
//                 returnEvent.addHeader( "Q", sequence );
//                 sendSipEvent( returnEvent );

            ///char* sid = fetion_sip_get_sid_by_sipuri( from.toAscii().constData() );
            ///emit messageReceived( QString( sid ), contentStr, from );
            break;
        }
        case FetionSipEvent::SipIncoming: {
            break;
        }
        case FetionSipEvent::SipNotification: {
//                 if ( notificationType == QLatin1String( "PresenceV4" ) ) {
//                     /// NOTIFICATION_TYPE_PRESENCE
//                 }
//                 else if ( notificationType == QLatin1String( "Conversation" ) ) {
//                     /// NOTIFICATION_TYPE_CONVERSATION
//                 }
//                 else if ( notificationType == QLatin1String( "contact" ) ) {
//                     /// NOTIFICATION_TYPE_CONTACT
//                 }
//                 else if ( notificationType == QLatin1String( "registration" ) ) {
//                     /// NOTIFICATION_TYPE_REGISTRATION
//                 }
//                 else if ( notificationType == QLatin1String( "SyncUserInfoV4" ) ) {
//                     /// NOTIFICATION_TYPE_SYNCUSERINFO
//                 }
//                 else if ( notificationType == QLatin1String( "PGGroup" ) ) {
//                     /// NOTIFICATION_TYPE_PGGROUP
//                 }
//                 else {
//                     /// NOTIFICATION_TYPE_UNKNOWN
//                 }
    //         QDomDocument doc;
    //         doc.setContent( list.last() );
            break;
        }
        case FetionSipEvent::SipOption: {
            break;
        }
        case FetionSipEvent::SipSipc_4_0: {
//                 if ( callid == "5" ) {
//                     /// contact information return
//                 }
            break;
        }
        case FetionSipEvent::SipUnknown: {
            qWarning() << "unknown sip message";
            return;
        }
    }
}

// {
//     m_loginSocket.connectToHostEncrypted( "uid.fetion.com.cn", 443 );
//     qWarning() << "Start ssi login";
//     QByteArray s;
//     s += "GET /ssiportal/SSIAppSignInV4.aspx?";
//     s += "mobileno=" + m_accountId.toAscii();
//     QByteArray verifyUri;
// //     if ( !vcode.isEmpty() ) {
// //         verifyUri += "&pic=" + vcode;
// //         vcode.clear();
// //     }
// //     verifyUri += "&pid=" + user->verification->guid;
//     verifyUri += "&pic=" + vcode;
//     verifyUri += "&algorithm=" + algorithm;
//     QByteArray digestType = "1";
//     QByteArray digest = myhash( 0/*user->userId*/ , m_password.toAscii() );
//     s += "&domains=fetion.com.cn" + verifyUri + "&v4digest-type=" + digestType + "&v4digest=" + digest + "\r\n"
//                     "User-Agent: IIC2.0/pc "PROTO_VERSION"\r\n"
//                     "Host: uid.fetion.com.cn\r\n"
//                     "Cache-Control: private\r\n"
//                     "Connection: Keep-Alive\r\n\r\n";
//     m_loginSocket.write( s );
//     connect( &m_loginSocket, SIGNAL(readyRead()), this, SLOT(gotLoginSocketData()) );
// }
//     parse_ssi_auth_response( pos , me );
//     free( pos );
// 
//     if ( me->loginStatus == 421 || me->loginStatus == 420 ) {
//         generate_pic_code( me );
//         QImage s;
//         s.load( QString( me->config->globalPath ) + "/code.gif", "JPEG" );
//         QString codetext = FetionVCodeDialog::getInputCode( s );
//         fetion_user_set_verification_code( me, codetext.toAscii() );
//         goto LOGIN;
//     }
//     fetion_config_initialize( config, me->userId );
// 
//     /* set user list to be stored in local file  */
//     newul = fetion_user_list_find_by_no( ul , m_accountId.toAscii() );
//     if ( !newul ) {
//         newul = fetion_user_list_new( m_accountId.toAscii(), NULL, me->userId, me->sId, P_ONLINE , 1);
//         foreach_userlist(ul , ul_cur)
//             ul_cur->islastuser = 0;
//         fetion_user_list_append(ul , newul);
//     }
//     else {
//         memset(newul->password, 0, sizeof(newul->password));
//         newul->laststate = P_ONLINE;
//         foreach_userlist(ul , ul_cur)
//             ul_cur->islastuser = 0;
//         newul->islastuser = 1;
//     }
//     fetion_user_list_save(config , ul);
//     fetion_user_list_free(ul);
// 
//     /* download xml configuration file from the server */
//     fetion_config_load( me );
//     fetion_config_download_configuration( me );
//     fetion_config_save( me );
//     fetion_user_set_st( me , P_ONLINE );
// 
//     /*load local data*/
//     int local_group_count;
//     int local_buddy_count;
//     fetion_user_load( me );
//     fetion_contact_load( me, &local_group_count, &local_buddy_count );
//     qWarning() << "g & c" << local_group_count << local_group_count;
// 
//     /* start a new tcp connection for registering to sipc server */
//     FetionConnection* conn = tcp_connection_new();
//     tcp_connection_connect( conn, config->sipcProxyIP, config->sipcProxyPort );
//     /* add the connection object into the connection list */
//     ///fx_conn_append( conn );
//     /* initialize a sip object */
//     FetionSip* sip = fetion_sip_new( conn , me->sId );
//     fetion_user_set_sip( me , sip );
// 
//     pos = sipc_reg_action( me );
//     char* nonce;
//     char* key;
//     parse_sipc_reg_response( pos , &nonce , &key );
//     free( pos );
//     char* aeskey = generate_aes_key();
//     char* response = generate_response( nonce, me->userId, me->password, key, aeskey );
//     free(nonce);
//     free(key);
//     free(aeskey);
//     /* start sipc authentication using the response created just now */
//     int new_group_count;
//     int new_buddy_count;
// AUTH:
//     pos = sipc_aut_action( me , response );
//     parse_sipc_auth_response( pos , me, &new_group_count, &new_buddy_count );
//     free( pos );
//     if ( me->loginStatus == 421 || me->loginStatus == 420 ) {
//         generate_pic_code( me );
//         QImage s;
//         s.load( QString( me->config->globalPath ) + "/code.gif", "JPEG" );
//         QString codetext = FetionVCodeDialog::getInputCode( s );
//         fetion_user_set_verification_code( me , codetext.toAscii() );
//         goto AUTH;
//     }
// 
//     qWarning() << "auth finished.";
//     /* update buddy list */
//     if ( me->groupCount == 0 ) {
//         qWarning() << "g c";
//         me->groupCount = local_group_count;
//     }
//     else if ( me->groupCount != local_group_count ) {
//         qWarning() << local_group_count;
//         Group* g_cur;
//         Group* g_tmp;
//         for ( g_cur = me->groupList->next; g_cur != me->groupList; ) {
//             g_tmp = g_cur;
//             g_cur = g_cur->next;
//             if( !g_tmp->dirty ) {
//                 fetion_group_list_remove( g_tmp );
//                 free( g_tmp );
//             }
//         }
//     }
//     /* update buddy count */
//     if ( me->contactCount == 0 ) {
//         qWarning() << "c c";
//         me->contactCount = local_buddy_count;
//     }
//     else if ( me->contactCount != local_buddy_count ) {
//         qWarning() << local_buddy_count;
//         Contact* c_cur;
//         Contact* c_tmp;
//         for ( c_cur = me->contactList->next; c_cur != me->contactList; ) {
//             c_tmp = c_cur;
//             c_cur = c_cur->next;
//             if ( !c_tmp->dirty ) {
//                 fetion_contact_list_remove( c_tmp );
//                 free( c_tmp );
//             }
//         }
//     }
// 
//     /* send get group request */
//     pg_group_get_list( me );
// 
//     /* if there is not a buddylist name "Ungrouped" or "Strangers", create one */
//     Group* group = NULL;
//     if ( fetion_group_list_find_by_id(me->groupList,
//             BUDDY_LIST_NOT_GROUPED) == NULL &&
//             fetion_contact_has_ungrouped(me->contactList) ) {
//         group = fetion_group_new();
//         group->groupid = BUDDY_LIST_NOT_GROUPED;
//         strcpy(group->groupname , "Ungrouped");
//         fetion_group_list_append( me->groupList, group );
//     }
//     if ( fetion_group_list_find_by_id(me->groupList,
//             BUDDY_LIST_STRANGER) == NULL &&
//             fetion_contact_has_strangers(me->contactList) ) {
//         group = fetion_group_new();
//         group->groupid = BUDDY_LIST_STRANGER;
//         strcpy(group->groupname , "Strangers");
//         fetion_group_list_prepend( me->groupList, group );
//     }
// 
//     qWarning() << "login finished.";
// 
//     /// NOTE: debug cases
//     Group* grp = me->groupList;
//     Contact* contact = me->contactList;
//     char* sid = NULL;
//     foreach_grouplist(me->groupList , grp){
//         qWarning() << QString::fromUtf8( grp->groupname ) << grp->groupid;
//         emit gotGroup( grp->groupid, QString::fromUtf8( grp->groupname ) );
//     }
//     foreach_contactlist(me->contactList,contact) {
//         if ( strlen(contact->sId) == 0 )
//             sid = fetion_sip_get_sid_by_sipuri(contact->sipuri);
//         if ( sid ) {
//             free( sid );
//             sid = NULL;
//         }
//         qWarning() << "sId" << contact->mobileno << contact->sId
//         << QString::fromUtf8( contact->nickname ) << contact->state;
//         emit gotContact( QString::fromLatin1( contact->sId ), QString::fromUtf8( contact->nickname ), contact->groupid );
//     }
// 
// 
//     /// if not offline login
//     fetion_contact_subscribe_only( me );
// 
//     /// TODO: create listening thread here !!!
//     notifier = new FetionSipNotifier( /*me->sip, */me, this );
//     QObject::connect( notifier, SIGNAL(newThreadEntered(FetionSip*,User*)),
//                       this, SLOT(slotNewThreadEntered(FetionSip*,User*)) );
//     QObject::connect( notifier, SIGNAL(messageReceived(const QString&,const QString&,const QString&)),
//                       this, SLOT(slotMessageReceived(const QString&,const QString&,const QString&)) );
//     QObject::connect( notifier, SIGNAL(presenceChanged(const QString&,StateType)),
//                       this, SLOT(slotPresenceChanged(const QString&,StateType)) );
// //     notifier->start();
// 
// }

void FetionSession::logout()
{
}

bool FetionSession::isLoggedIn() const
{
    return true;
}

QString FetionSession::accountId() const
{
    return m_accountId;
}

void FetionSession::setVisibility( bool isVisible )
{
}

void FetionSession::setStatusMessage( const QString& status )
{
}

void FetionSession::sendClientMessage( const QString& sId, const QString& message )
{
//     Contact* contact = fetion_contact_get_contact_info_by_no( me, sId.toAscii().constData(), FETION_NO );
//     if ( !contact ) {
//         qWarning() << "get contact information failed" << sId;
//         return;
//     }

    /* find the sipuri of the target user */
//     Contact* contact_cur;
//     Contact* target_contact = NULL;
//     foreach_contactlist( me->contactList, contact_cur ) {
//         if(strcmp(contact_cur->userId, contact->userId) == 0) {
//             target_contact = contact_cur;
//             break;
//         }
//     }

//     if ( !target_contact ) {
//         qWarning() << "not in your contact list" << sId;
//         return;
//     }

//     Conversation* conv = fetion_conversation_new( me, target_contact->sipuri, NULL );

//     fetion_conversation_send_sms( conv, message.toUtf8() );
}

void FetionSession::sendMobilePhoneMessage( const QString& sId, const QString& message )
{
//     Contact* contact = fetion_contact_get_contact_info_by_no( me, sId.toAscii(), FETION_NO );
//     if ( !contact ) {
//         qWarning() << "get contact information failed" << sId;
//         return;
//     }
// 
//     /* find the sipuri of the target user */
//     Contact* contact_cur;
//     Contact* target_contact = NULL;
//     foreach_contactlist( me->contactList, contact_cur ) {
//         if(strcmp(contact_cur->userId, contact->userId) == 0) {
//             target_contact = contact_cur;
//             break;
//         }
//     }
// 
//     if ( !target_contact ) {
//         qWarning() << "not in your contact list" << sId;
//         return;
//     }
// 
//     Conversation* conv = fetion_conversation_new( me, target_contact->sipuri, NULL );
// 
//     int daycount;
//     int monthcount;
//     if ( fetion_conversation_send_sms_to_phone_with_reply( conv, message.toUtf8(), &daycount, &monthcount ) == -1 ) {
//         qWarning() << "send sms failed";
//         return;
//     }
}

// bool FetionSession::isConnected() const
// {
//     return true;
// }

// void FetionSession::disconnect()
// {
//     if ( notifier ) {
//         notifier->exit();
//         delete notifier;
//         notifier = 0;
//     }
// }

// void FetionSession::setStatus( const Kopete::OnlineStatus& status )
// {
// }

// void FetionSession::slotMessageReceived( const QString& sId, const QString& msgContent, const QString& qsipuri )
// {
//     emit gotMessage( sId, msgContent );
// }

// void FetionSession::slotNewThreadEntered( FetionSip* sip, User* user )
// {
//     FetionSipNotifier* thread = new FetionSipNotifier( /*sip, */user, this );
//     QObject::connect( thread, SIGNAL(newThreadEntered(FetionSipNotifier*)),
//                       this, SLOT(slotNewThreadEntered(FetionSipNotifier*)) );
//     QObject::connect( thread, SIGNAL(messageReceived(const QString&,const QString&,const QString&)),
//                       this, SLOT(slotMessageReceived(const QString&,const QString&,const QString&)) );
//     QObject::connect( thread, SIGNAL(presenceChanged(const QString&,StateType)),
//                       this, SLOT(slotPresenceChanged(const QString&,StateType)) );
// //     thread->start();
// }

// void FetionSession::slotPresenceChanged( const QString& sId, StateType state )
// {
//     Kopete::OnlineStatus status;
//     switch ( state ) {
//         case P_ONLINE:
//             status = Kopete::OnlineStatus::Online;
//             break;
//         case P_RIGHTBACK:
//             status = Kopete::OnlineStatus::Away;
//             break;
//         case P_AWAY:
//             status = Kopete::OnlineStatus::Away;
//             break;
//         case P_BUSY:
//             status = Kopete::OnlineStatus::Busy;
//             break;
//         case P_OUTFORLUNCH:
//         case P_ONTHEPHONE:
//         case P_MEETING:
//         case P_DONOTDISTURB:
//         case P_HIDDEN:
//             status = Kopete::OnlineStatus::Away;
//             break;
//         case P_OFFLINE:
//             status = Kopete::OnlineStatus::Offline;
//             break;
//         default:
//             break;
//     }
//     emit contactStatusChanged( sId, status );
// }
