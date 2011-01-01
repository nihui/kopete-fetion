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

#include <QFile>

#include <openssl/crypto.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include <ctime>

static QByteArray myhash( QByteArray userId, QByteArray password )
{
    QByteArray domain( "fetion.com.cn:" );
    QByteArray data = domain + password;
    QByteArray digest = QCryptographicHash::hash( data, QCryptographicHash::Sha1 );
//     qWarning() << "hexdigest" << hexdigest;
    if ( userId.isEmpty() )
        return digest;

    int id = userId.toInt();
    char* bid = (char*)&id;
    QByteArray iddata = QByteArray::fromRawData( bid, sizeof(int) );
    QByteArray data2 = iddata + digest;
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

static QByteArray generateAesKey()
{
    QFile randfile( "/dev/urandom" );
    randfile.open( QIODevice::ReadOnly );
    QByteArray key = randfile.read( 64 );
    randfile.close();
    return key;
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
    url.addQueryItem( "v4digest", myhash( QByteArray() , m_password.toAscii() ).toHex() );

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
        type = verificationElem.attribute( "type" );
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
        m_userId = userElem.attribute( "user-id" );

        QString sId = sipuri.section( ':', 1, -1, QString::SectionSkipEmpty ).section( '@', 0, 0, QString::SectionSkipEmpty );
        m_from = sId;

        /// connect to sipc server
        QString sipcAddressIp = m_sipcProxyAddress.section( ':', 0, 0, QString::SectionSkipEmpty );
        int sipcAddressPort = m_sipcProxyAddress.section( ':', -1, -1, QString::SectionSkipEmpty ).toInt();
        notifier = new FetionSipNotifier( this );
        connect( notifier, SIGNAL(sipEventReceived(const FetionSipEvent&)),
                 this, SLOT(handleSipEvent(const FetionSipEvent&)) );
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
//     qWarning() << sipEvent.toString();
    switch ( sipEvent.sipType() ) {
        case FetionSipEvent::SipInvitation: {
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
            if ( sipEvent.typeAddition() == "401 Unauthoried" ) {
                ///
                QString wstr = sipEvent.getFirstValue( "W" );
                QString nonce = wstr.section( '\"', 3, 3, QString::SectionSkipEmpty );
                QString key = wstr.section( '\"', 5, 5, QString::SectionSkipEmpty );
                QString signature = wstr.section( '\"', 7, 7, QString::SectionSkipEmpty );

                QByteArray aeskey = generateAesKey();
                QByteArray hidden = nonce.toAscii() + myhash( m_userId.toAscii(), m_password.toAscii() ) + aeskey;

                const char* publickey = key.toAscii().constData();
                const unsigned char* fromdata = (const unsigned char*)hidden.constData();
                char modulus[256];
                char exponent[6];
                memcpy( modulus , publickey , 256 );
                memcpy( exponent , publickey + 256 , 6 );
                BIGNUM* bnn = BN_new();
                BIGNUM* bne = BN_new();
                RSA* r = RSA_new();
                BN_hex2bn( &bnn, modulus );
                BN_hex2bn( &bne, exponent );
                r->n = bnn;
                r->e = bne;
                r->d = NULL;
                int flen = RSA_size( r );
                unsigned char* outdata = (unsigned char*)malloc( flen );
                int ret = RSA_public_encrypt( hidden.size(), fromdata, outdata, r, RSA_PKCS1_PADDING );
                qWarning() << "rsa public encrypt" << ret;
                RSA_free( r );

                QByteArray response( (char*)outdata );
                free( outdata );

                FetionSipEvent sipcAuthActionEvent( FetionSipEvent::SipRegister, FetionSipEvent::EventUnknown );
                sipcAuthActionEvent.addHeader( "F", m_from );
                sipcAuthActionEvent.addHeader( "I", QString::number( FetionSipEvent::nextCallid() ) );
                sipcAuthActionEvent.addHeader( "Q", "2 R" );
                QString Astr;
                Astr += "Digest response=\"";
                Astr += response.toHex();
                Astr += "\",algorithm=\"SHA1-sess-v4\"";
                sipcAuthActionEvent.addHeader( "A", Astr );
                sipcAuthActionEvent.addHeader( "AK", "ak-value" );
                QString ackaStr( "Verify response=\"%1\",algorithm=\"%2\",type=\"%3\",chid=\"%4\"" );
                ackaStr = ackaStr.arg(vcode).arg(algorithm).arg(type).arg(picid);
                sipcAuthActionEvent.addHeader( "A", ackaStr );

                QString authContent = "<args><device machine-code=\"001676C0E351\"/><caps value=\"1ff\"/><events value=\"7f\"/>";
                authContent += "<user-info mobile-no=\"";
                authContent += m_accountId;
                authContent += "\" user-id=\"";
                authContent += m_userId;
                authContent += "\">";
                authContent += "<personal version=\"\" attributes=\"v4default\"/>";
                authContent += "<custom-config version=\"\"/>";
                authContent += "<contact-list version=\"\" buddy-attributes=\"v4default\"/></user-info>";
                authContent += "<credentials domains=\"fetion.com.cn\"/>";
                authContent += "<presence><basic value=\"0\" desc=\"\"/></presence></args>";
                sipcAuthActionEvent.setContent( authContent );

                notifier->sendSipEvent( sipcAuthActionEvent );
            }
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
