#include "fetionsession.h"

#include "fetioncontact.h"
#include "fetionprotocol.h"
#include "fetionsipevent.h"
#include "fetionsipnotifier.h"
#include "fetionsiputils.h"
#include "fetionvcodedialog.h"

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
    return QByteArray::fromHex( "4A026855890197CFDF768597D07200B346F3D676411C6F87368B5C2276DCEDD2" );
}

FetionSession::FetionSession( QObject* parent ) : QObject(parent)
{
    m_isConnecting = false;
    m_isConnected = false;
    m_notifier = new FetionSipNotifier( this );
    manager = new QNetworkAccessManager( this );
    /// one minute timer for keep alive heart beep
    m_hearter = new QTimer( this );
    m_hearter->setInterval( 60 * 1000 );
    connect( m_hearter, SIGNAL(timeout()), this, SLOT(sendKeepAlive()) );
}

FetionSession::~FetionSession()
{
    qWarning() << "FetionSession::~FetionSession()";
//     delete m_notifier;
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
    m_isConnecting = true;
    /// post xml system config data
    QString xml = "<config><user mobile-no=\"%1\"/>"
                  "<client type=\"PC\" version=\""PROTO_VERSION"\" platform=\"W5.1\"/>"
                  "<servers version=\"0\"/><parameters version=\"0\"/>"
                  "<hints version=\"0\"/></config>";
    xml = xml.arg( m_accountId );

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

    m_getUri = serversElem.firstChildElement( "get-uri" ).text();
    m_getPortraitUri = m_getUri.replace( "geturi", "getportrait" );

    QDomElement parametersElem = docElem.firstChildElement( "parameters" );
    m_portraitFileTypes = parametersElem.firstChildElement( "portrait-file-type" ).text();

    /// ssi auth
    QNetworkRequest request;
    request.setSslConfiguration( QSslConfiguration::defaultConfiguration() );

    QUrl url;
    url.setUrl( m_ssiAppSignInV2Uri );
    url.setPort( 443 );
    url.addEncodedQueryItem( "mobileno", m_accountId.toAscii() );
    url.addEncodedQueryItem( "domains", "fetion.com.cn" );
    url.addEncodedQueryItem( "v4digest-type", "1" );
    url.addEncodedQueryItem( "v4digest", myhash( QByteArray() , m_password.toAscii() ).toHex() );

    if ( !picid.isEmpty() && !vcode.isEmpty() && !algorithm.isEmpty() ) {
        url.addEncodedQueryItem( "pid", picid.toAscii() );
        url.addEncodedQueryItem( "pic", vcode.toAscii() );
        url.addEncodedQueryItem( "algorithm", algorithm.toAscii() );
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
    QByteArray cookieHeader = reply->rawHeader( "Set-Cookie" );
    reply->deleteLater();

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
        url.addEncodedQueryItem( "algorithm", algorithm.toAscii() );
        request.setUrl( url );

        request.setRawHeader( "User-Agent", "IIC2.0/PC "PROTO_VERSION );
        request.setRawHeader( "Host", "nav.fetion.com.cn" );
        request.setRawHeader( "Connection", "Close" );

        QNetworkReply* r = manager->get( request );
        connect( r, SIGNAL(finished()), this, SLOT(getCodePicFinished()) );
        return;
    }
    else if ( statusCode == "200" ) {
        int cookieBegin = cookieHeader.indexOf( "ssic=", 0 ) + 5;/// length of ssic=
        int cookieEnd = cookieHeader.indexOf( ";", cookieBegin );
        m_ssicCookie = cookieHeader.mid( cookieBegin, cookieEnd - cookieBegin );

        QDomElement userElem = docElem.firstChildElement( "user" );
        m_sipUri = userElem.attribute( "uri" );
        QString mobileno = userElem.attribute( "mobile-no" );
        QString userstatus = userElem.attribute( "user-status" );
        m_userId = userElem.attribute( "user-id" );

        m_from = FetionSipUtils::SipUriToSid( m_sipUri );

        /// connect to sipc server
        QString sipcAddressIp = m_sipcProxyAddress.section( ':', 0, 0, QString::SectionSkipEmpty );
        int sipcAddressPort = m_sipcProxyAddress.section( ':', -1, -1, QString::SectionSkipEmpty ).toInt();
        connect( m_notifier, SIGNAL(sipEventReceived(const FetionSipEvent&)),
                 this, SLOT(handleSipcRegisterReplyEvent(const FetionSipEvent&)) );
        m_notifier->connectToHost( sipcAddressIp, sipcAddressPort );

        /// sipc register
        m_nouce = generateNouce();
        FetionSipEvent registerEvent( FetionSipEvent::SipRegister );
        registerEvent.addHeader( "F", m_from );
        registerEvent.addHeader( "I", QString::number( FetionSipEvent::nextCallid() ) );
        registerEvent.addHeader( "Q", "2 R" );
        registerEvent.addHeader( "CN", m_nouce );
        registerEvent.addHeader( "CL", "type=\"pc\" ,version=\""PROTO_VERSION"\"" );

        m_notifier->sendSipEvent( registerEvent );
    }
}

void FetionSession::getCodePicFinished()
{
    QNetworkReply* reply = (QNetworkReply*)sender();
    QString data = QString::fromUtf8( reply->readAll() );
    reply->deleteLater();

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

void FetionSession::requestBuddyPortraitFinished()
{
    QNetworkReply* reply = (QNetworkReply*)sender();
    QByteArray data = reply->readAll();
    QImage img = QImage::fromData( data );
    emit buddyPortraitUpdated( m_portraitReplyId.value( reply ), img );
    m_portraitReplyId.remove( reply );
    reply->deleteLater();
}

void FetionSession::handleSipcRegisterReplyEvent( const FetionSipEvent& sipEvent )
{
    switch ( sipEvent.sipType() ) {
        case FetionSipEvent::SipSipc_4_0: {
            if ( sipEvent.typeAddition() == "401 Unauthoried" ) {
                ///
                QString wstr = sipEvent.getFirstValue( "W" );
                QString nonce = wstr.section( '\"', 3, 3, QString::SectionSkipEmpty );
                QString key = wstr.section( '\"', 5, 5, QString::SectionSkipEmpty );
                QString signature = wstr.section( '\"', 7, 7, QString::SectionSkipEmpty );

                QByteArray aeskey = generateAesKey();
                QByteArray passwordhash = myhash( m_userId.toAscii(), m_password.toAscii() );
                QByteArray hidden = nonce.toAscii() + passwordhash + aeskey;
                const char* publickey = key.toAscii().constData();
                const unsigned char* fromdata = reinterpret_cast<const unsigned char*>(hidden.constData());
                char modulus[256];
                char exponent[6];
                memcpy( modulus, publickey, 256 );
                memcpy( exponent, publickey + 256, 6 );
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
                RSA_free( r );

                QByteArray response = QByteArray::fromRawData( (char*)outdata, flen );
                response = response.toHex();
                free( outdata );
                qWarning() << "rsa public encrypt" << ret << response;

                FetionSipEvent sipcAuthActionEvent( FetionSipEvent::SipRegister );
                sipcAuthActionEvent.addHeader( "F", m_from );
                sipcAuthActionEvent.addHeader( "I", QString::number( FetionSipEvent::nextCallid() ) );
                sipcAuthActionEvent.addHeader( "Q", "2 R" );
                QString Astr( "Digest response=\"%1\",algorithm=\"SHA1-sess-v4\"" );
                Astr = Astr.arg( QLatin1String( response ) );
                sipcAuthActionEvent.addHeader( "A", Astr );
                sipcAuthActionEvent.addHeader( "AK", "ak-value" );
                QString ackaStr( "Verify response=\"%1\",algorithm=\"%2\",type=\"%3\",chid=\"%4\"" );
                ackaStr = ackaStr.arg( vcode ).arg( algorithm ).arg( type ).arg( picid );
                sipcAuthActionEvent.addHeader( "A", ackaStr );

                QString authContent = "<args><device machine-code=\"001676C0E351\"/>"
                                      "<caps value=\"1ff\"/><events value=\"7f\"/>"
                                      "<user-info mobile-no=\"%1\" user-id=\"%2\">"
                                      "<personal version=\"\" attributes=\"v4default\"/>"
                                      "<custom-config version=\"\"/>"
                                      "<contact-list version=\"\" buddy-attributes=\"v4default\"/></user-info>"
                                      "<credentials domains=\"fetion.com.cn\"/>"
                                      "<presence><basic value=\"0\" desc=\"\"/></presence></args>";
                authContent = authContent.arg( m_accountId ).arg( m_userId );
                sipcAuthActionEvent.addHeader( "L", QString::number( authContent.toUtf8().size() ) );
                sipcAuthActionEvent.setContent( authContent );

                m_notifier->sendSipEvent( sipcAuthActionEvent );
            }
            else if ( sipEvent.typeAddition() == "200 OK" ) {
                /// sipc register success
                /// login success
                m_isConnecting = false;
                m_isConnected = true;
                /// emit success signal
                qWarning() << "fetion login success   :)";
                emit loginSuccessed();

                disconnect( m_notifier, SIGNAL(sipEventReceived(const FetionSipEvent&)),
                            this, SLOT(handleSipcRegisterReplyEvent(const FetionSipEvent&)) );
                connect( m_notifier, SIGNAL(sipEventReceived(const FetionSipEvent&)),
                         this, SLOT(handleSipEvent(const FetionSipEvent&)) );
                /// extract buddy lists and buddies
                QDomDocument doc;
                doc.setContent( sipEvent.getContent() );
                QDomElement docElem = doc.documentElement();
                QDomElement resultsElem = doc.firstChildElement( "results" );
                QDomElement userinfoElem = resultsElem.firstChildElement( "user-info" );
                QDomElement contactlistElem = userinfoElem.firstChildElement( "contact-list" );
                QDomElement buddylistsElem = contactlistElem.firstChildElement( "buddy-lists" );
                QDomElement buddylistElem = buddylistsElem.firstChildElement( "buddy-list" );
                while ( !buddylistElem.isNull() ) {
                    int buddyListId = buddylistElem.attribute( "id" ).toInt();
                    QString buddyListName = buddylistElem.attribute( "name" );
                    m_buddyListIdNames[ buddyListId ] = buddyListName;
                    emit gotBuddyList( buddyListName );
                    buddylistElem = buddylistElem.nextSiblingElement( "buddy-list" );
                }
                QDomElement buddiesElem = contactlistElem.firstChildElement( "buddies" );
                QDomElement buddyElem = buddiesElem.firstChildElement( "b" );
                while ( !buddyElem.isNull() ) {
                    QString buddyId = buddyElem.attribute( "i" );
                    QString sipuri = buddyElem.attribute( "u" );
                    QString list = buddyElem.attribute( "l" );
                    m_buddyIdSipUri[ buddyId ] = sipuri;
                    emit gotBuddy( buddyId, m_buddyListIdNames.value( list.toInt() ) );
                    buddyElem = buddyElem.nextSiblingElement( "b" );
                }

                /// send contact subscribe
                FetionSipEvent subscribeEvent( FetionSipEvent::SipSubScribe );
                subscribeEvent.addHeader( "F", m_from );
                subscribeEvent.addHeader( "I", QString::number( FetionSipEvent::nextCallid() ) );
                subscribeEvent.addHeader( "Q", "2 SUB" );
                subscribeEvent.addHeader( "N", "PresenceV4" );

                QString subscribeBody = "<args><subscription self=\"v4default;mail-count\" buddy=\"v4default\" version=\"0\"/></args>";
                subscribeEvent.addHeader( "L", QString::number( subscribeBody.toUtf8().size() ) );
                subscribeEvent.setContent( subscribeBody );
                qWarning() << subscribeEvent.toString();
                m_notifier->sendSipEvent( subscribeEvent );

                /// start sending keep alive to server every minute
                m_hearter->start();
            }
            break;
        }
        default: {
            qWarning() << "unexpected sipc register reply." << sipEvent.toString();
            return;
        }
    }
}

void FetionSession::handleSipEvent( const FetionSipEvent& sipEvent )
{
    FetionSipNotifier* notifier = static_cast<FetionSipNotifier*>(sender());
//     qWarning() << sipEvent.toString();
    switch ( sipEvent.sipType() ) {
        case FetionSipEvent::SipInvite: {
            QString from = sipEvent.getFirstValue( "F" );
            QString callid = sipEvent.getFirstValue( "I" );
            QString sequence = sipEvent.getFirstValue( "Q" );
            QString auth = sipEvent.getFirstValue( "A" );
            QString addressList = auth.section( '\"', 1, 1, QString::SectionSkipEmpty );
            QString credential = auth.section( '\"', 3, 3, QString::SectionSkipEmpty );
            QString firstAddressPort = addressList.section( ';', 0, 0, QString::SectionSkipEmpty );
            QString firstAddress = firstAddressPort.section( ':', 0, 0, QString::SectionSkipEmpty );
            QString firstPort = firstAddressPort.section( ':', 1, 1, QString::SectionSkipEmpty );

            qWarning() << "Received a conversation invitation";
            FetionSipNotifier* chatNotifier = new FetionSipNotifier( this );
            connect( chatNotifier, SIGNAL(sipEventReceived(const FetionSipEvent&)),
                     this, SLOT(handleSipEvent(const FetionSipEvent&)) );
            chatNotifier->connectToHost( firstAddress, firstPort.toInt() );
            emit chatChannelAccepted( m_buddyIdSipUri.key( from ), chatNotifier );

            /// reply to the invitation
            FetionSipEvent returnEvent( FetionSipEvent::SipSipc_4_0 );
            returnEvent.setTypeAddition( "200 OK" );
            returnEvent.addHeader( "F", from );
            returnEvent.addHeader( "I", callid );
            returnEvent.addHeader( "Q", sequence );
            notifier->sendSipEvent( returnEvent );

            /// register to conversation sip server
            FetionSipEvent registerEvent( FetionSipEvent::SipRegister );
            registerEvent.addHeader( "F", m_from );
            registerEvent.addHeader( "I", QString::number( FetionSipEvent::nextCallid() ) );
            registerEvent.addHeader( "Q", "2 R" );
            registerEvent.addHeader( "A", "TICKS auth=\"" + credential + "\"" );
            registerEvent.addHeader( "K", "text/plain" );
            registerEvent.addHeader( "K", "text/html-fragment" );
            registerEvent.addHeader( "K", "multiparty" );
            registerEvent.addHeader( "K", "nudge" );
            chatNotifier->sendSipEvent( registerEvent );
            break;
        }
        case FetionSipEvent::SipMessage: {
            QString from = sipEvent.getFirstValue( "F" );
            QString callid = sipEvent.getFirstValue( "I" );
            QString sequence = sipEvent.getFirstValue( "Q" );
            QString sendtime = sipEvent.getFirstValue( "D" );

            emit gotMessage( m_buddyIdSipUri.key( from ), sipEvent.getContent() );

            /// reply to the message
            FetionSipEvent returnEvent( FetionSipEvent::SipSipc_4_0 );
            returnEvent.setTypeAddition( "200 OK" );
            returnEvent.addHeader( "F", from );
            returnEvent.addHeader( "I", callid );
            returnEvent.addHeader( "Q", sequence );
            notifier->sendSipEvent( returnEvent );
            break;
        }
        case FetionSipEvent::SipInfo: {
            qWarning() << "got sip info event";
            QString callid = sipEvent.getFirstValue( "I" );
            QString sequence = sipEvent.getFirstValue( "Q" );
            QString from = sipEvent.getFirstValue( "F" );
            QString id = m_buddyIdSipUri.key( from );
            QString infoContent = sipEvent.getContent();
            QDomDocument doc;
            doc.setContent( infoContent );
            QDomElement docElem = doc.documentElement();
            QDomElement stateElem = docElem.firstChildElement( "state" );
//             qWarning() << stateElem.isNull() << stateElem.text();
            if ( stateElem.text() == "nudge" ) {
                emit gotNudge( id );
                /// reply to the nudge
                FetionSipEvent returnEvent( FetionSipEvent::SipSipc_4_0 );
                returnEvent.setTypeAddition( "200 OK" );
                returnEvent.addHeader( "I", callid );
                returnEvent.addHeader( "Q", sequence );
                returnEvent.addHeader( "F", from );
                notifier->sendSipEvent( returnEvent );
            }
            break;
        }
        case FetionSipEvent::SipBENotify: {
            QString notifyType = sipEvent.getFirstValue( "N" );
            if ( notifyType == "PresenceV4" ) {
                ///
                QDomDocument doc;
                doc.setContent( sipEvent.getContent() );
                QDomElement docElem = doc.documentElement();
                QDomElement eventElem = docElem.firstChildElement( "event" );
                QDomElement contactsElem = eventElem.firstChildElement( "contacts" );
                QDomElement contactElem = contactsElem.firstChildElement( "c" );
                while ( !contactElem.isNull() ) {
                    QString id = contactElem.attribute( "id" );
                    QDomElement pElem = contactElem.firstChildElement( "p" );
                    if ( !pElem.isNull() ) {
                        FetionBuddyInfo buddyInfo;
                        buddyInfo.sid = pElem.attribute( "sid" );
                        buddyInfo.sipuri = pElem.attribute( "su" );
                        buddyInfo.mobileno = pElem.attribute( "m" );
                        buddyInfo.nick = pElem.attribute( "n" );
                        buddyInfo.smsg = pElem.attribute( "i" );
                        emit buddyInfoUpdated( id, buddyInfo );
                    }
                    QDomElement prElem = contactElem.firstChildElement( "pr" );
                    if ( !prElem.isNull() ) {
                        QString statusId = prElem.attribute( "b" );
                        emit buddyStatusUpdated( id, statusId );
                    }
                    contactElem = contactElem.nextSiblingElement( "c" );
                }
            }
            else if ( notifyType == "Conversation" ) {
                ///
                QDomDocument doc;
                doc.setContent( sipEvent.getContent() );
                QDomElement docElem = doc.documentElement();
                QDomElement eventElem = docElem.firstChildElement( "event" );
                while ( !eventElem.isNull() ) {
                    QString eventType = eventElem.attribute( "type" );
                    if ( eventType == "Support" ) {
                        /// TODO indicate contact can support the following capability
                        QString supportTypes = eventElem.text();
                    }
                    else if ( eventType == "UserEntered" ) {
                        /// TODO a member enter the conversation
                        QDomElement memberElem = eventElem.firstChildElement( "member" );
                        QString memberSipUri = memberElem.attribute( "uri" );
                        qWarning() << memberSipUri << "enter the conversation";
                    }
                    else if ( eventType == "UserLeft" ) {
                        /// TODO a member leave the conversation
                        QDomElement memberElem = eventElem.firstChildElement( "member" );
                        QString memberSipUri = memberElem.attribute( "uri" );
                        qWarning() << memberSipUri << "leave the conversation";
                    }
                    eventElem = eventElem.nextSiblingElement( "event" );
                }
            }
            else if ( notifyType == "contact" ) {
                ///
            }
            else if ( notifyType == "registration" ) {
                ///
            }
            else if ( notifyType == "SyncUserInfoV4" ) {
                ///
            }
            else if ( notifyType == "PGGroup" ) {
                ///
            }
            break;
        }
        case FetionSipEvent::SipNotify: {
            break;
        }
        case FetionSipEvent::SipOption: {
            break;
        }
        case FetionSipEvent::SipSipc_4_0: {
            int callid = sipEvent.getFirstValue( "I" ).toInt();
            if ( m_callidCallback.contains( callid ) ) {
                FetionSipEventCallbackData cbData = m_callidCallback.value( callid );
                (this->*cbData.cb)( sipEvent.typeAddition() == "200 OK", sipEvent, cbData.data );
                m_callidCallback.remove( callid );
            }
            break;
        }
        case FetionSipEvent::SipUnknown: {
            qWarning() << "unknown sip message";
            return;
        }
    }
}

void FetionSession::sendKeepAlive()
{
    int callid = FetionSipEvent::nextCallid();
    FetionSipEvent sendEvent( FetionSipEvent::SipRegister );
    sendEvent.addHeader( "F", m_from );
    sendEvent.addHeader( "I", QString::number( callid ) );
    sendEvent.addHeader( "Q", "2 R" );
    sendEvent.addHeader( "T", m_sipUri );
    sendEvent.addHeader( "N", "KeepAlive" );

    QString registerBody = "<args><credentials domains=\"fetion.com.cn\" /></args>";
    sendEvent.addHeader( "L", QString::number( registerBody.toUtf8().size() ) );
    sendEvent.setContent( registerBody );

    FetionSipEventCallbackData cbData = { &FetionSession::sendKeepAliveCB, QVariant() };
    m_callidCallback[ callid ] = cbData;
    m_notifier->sendSipEvent( sendEvent );
}

void FetionSession::logout()
{
    FetionSipEvent sendEvent( FetionSipEvent::SipBye );
    sendEvent.addHeader( "F", m_from );
    sendEvent.addHeader( "I", QString::number( FetionSipEvent::nextCallid() ) );
    sendEvent.addHeader( "Q", "2 B" );
    sendEvent.addHeader( "T", m_sipUri );

    m_notifier->sendSipEvent( sendEvent );

    m_hearter->stop();
    m_notifier->close();

    m_isConnected = false;
    emit logoutSuccessed();
}

QString FetionSession::accountId() const
{
    return m_accountId;
}

bool FetionSession::isConnecting() const
{
    return m_isConnecting;
}

bool FetionSession::isConnected() const
{
    return m_isConnected;
}

void FetionSession::setVisibility( bool isVisible )
{
}

void FetionSession::setStatusId( const QString& statusId )
{
    FetionSipEvent sendEvent( FetionSipEvent::SipService );
    sendEvent.addHeader( "F", m_from );
    sendEvent.addHeader( "I", QString::number( FetionSipEvent::nextCallid() ) );
    sendEvent.addHeader( "Q", "2 S" );
    sendEvent.addHeader( "T", m_sipUri );
    sendEvent.addHeader( "N", "SetPresenceV4" );

    QString serviceBody;
    serviceBody = "<args><presence>"
                  "<basic value=\"%1\" />"
                  "</presence></args>";
    serviceBody = serviceBody.arg( statusId );
    sendEvent.addHeader( "L", QString::number( serviceBody.toUtf8().size() ) );
    sendEvent.setContent( serviceBody );

    m_notifier->sendSipEvent( sendEvent );
}

void FetionSession::setStatusMessage( const QString& statusMessage )
{
    FetionSipEvent sendEvent( FetionSipEvent::SipService );
    sendEvent.addHeader( "F", m_from );
    sendEvent.addHeader( "I", QString::number( FetionSipEvent::nextCallid() ) );
    sendEvent.addHeader( "Q", "2 S" );
    sendEvent.addHeader( "T", m_sipUri );
    sendEvent.addHeader( "N", "SetUserInfoV4" );

    QString serviceBody;
    serviceBody = "<args><userinfo>"
                  "<personal impresa=\"%1\" version=\"%2\" />"
                  "<custom-config type=\"PC\" version=\"%3\" />"
                  "</userinfo></args>";
    serviceBody = serviceBody.arg( statusMessage ).arg("0").arg("0");///FIXME
    sendEvent.addHeader( "L", QString::number( serviceBody.toUtf8().size() ) );
    sendEvent.setContent( serviceBody );

    m_notifier->sendSipEvent( sendEvent );
}

void FetionSession::requestChatChannel()
{
    int callid = FetionSipEvent::nextCallid();
    FetionSipEvent sendEvent( FetionSipEvent::SipService );
    sendEvent.addHeader( "F", m_from );
    sendEvent.addHeader( "I", QString::number( callid ) );
    sendEvent.addHeader( "Q", "2 S" );
    sendEvent.addHeader( "N", "StartChat" );

    FetionSipEventCallbackData cbData = { &FetionSession::requestChatChannelCB, QVariant() };
    m_callidCallback[ callid ] = cbData;
    m_notifier->sendSipEvent( sendEvent );
}

void FetionSession::sendClientMessage( const QString& id, const QString& message, FetionSipNotifier* chatChannel )
{
    QString to = m_buddyIdSipUri.value( id );
    if ( to.isEmpty() ) {
        qWarning() << "no such sipuri for contact id" << id;
        return;
    }

    int callid = FetionSipEvent::nextCallid();
    FetionSipEvent sendEvent( FetionSipEvent::SipMessage );
    sendEvent.addHeader( "F", m_from );
    sendEvent.addHeader( "I", QString::number( callid ) );
    sendEvent.addHeader( "Q", "2 M" );
    sendEvent.addHeader( "T", to );
    sendEvent.addHeader( "C", "text/plain" );
    sendEvent.addHeader( "K", "SaveHistory" );
    sendEvent.addHeader( "N", "CatMsg" );
    sendEvent.addHeader( "L", QString::number( message.toUtf8().size() ) );

    sendEvent.setContent( message );

    FetionSipEventCallbackData cbData = { &FetionSession::sendClientMessageCB, id };
    m_callidCallback[ callid ] = cbData;
    if ( chatChannel )
        chatChannel->sendSipEvent( sendEvent );
    else
        m_notifier->sendSipEvent( sendEvent );
}

void FetionSession::sendClientNudge( FetionSipNotifier* chatChannel )
{
    if ( !chatChannel ) {
        qWarning() << "send client nudge without a chat channel is not allowed!";
        return;
    }

    int callid = FetionSipEvent::nextCallid();
    FetionSipEvent sendEvent( FetionSipEvent::SipInfo );
    sendEvent.addHeader( "F", m_from );
    sendEvent.addHeader( "I", QString::number( callid ) );
    sendEvent.addHeader( "Q", "2 IN" );

    QString nudgeBody = "<is-composing><state>nudge</state></is-composing>";
    sendEvent.addHeader( "L", QString::number( nudgeBody.toUtf8().size() ) );

    sendEvent.setContent( nudgeBody );

//     FetionSipEventCallbackData cbData = { &FetionSession::sendClientMessageCB, id };
//     m_callidCallback[ callid ] = cbData;
    chatChannel->sendSipEvent( sendEvent );
}

void FetionSession::sendMobilePhoneMessage( const QString& id, const QString& message )
{
    QString to = m_buddyIdSipUri.value( id );
    if ( to.isEmpty() ) {
        qWarning() << "no such sipuri for contact id" << id;
        return;
    }

    FetionSipEvent sendEvent( FetionSipEvent::SipMessage );
    sendEvent.addHeader( "F", m_from );
    sendEvent.addHeader( "I", QString::number( FetionSipEvent::nextCallid() ) );
    sendEvent.addHeader( "Q", "2 M" );
    sendEvent.addHeader( "T", to );
    sendEvent.addHeader( "N", "SendCatSMS" );
    sendEvent.addHeader( "L", QString::number( message.toUtf8().size() ) );

    sendEvent.setContent( message );

    m_notifier->sendSipEvent( sendEvent );
}

void FetionSession::sendMobilePhoneMessageToMyself( const QString& message )
{
    FetionSipEvent sendEvent( FetionSipEvent::SipMessage );
    sendEvent.addHeader( "F", m_from );
    sendEvent.addHeader( "I", QString::number( FetionSipEvent::nextCallid() ) );
    sendEvent.addHeader( "Q", "2 M" );
    sendEvent.addHeader( "T", m_sipUri );
    sendEvent.addHeader( "N", "SendCatSMS" );
    sendEvent.addHeader( "L", QString::number( message.toUtf8().size() ) );

    sendEvent.setContent( message );

    m_notifier->sendSipEvent( sendEvent );
}

void FetionSession::requestBuddyDetail( const QString& id )
{
    QString to = m_buddyIdSipUri.value( id );
    if ( to.isEmpty() ) {
        qWarning() << "no such sipuri for contact id" << id;
        return;
    }

    int callid = FetionSipEvent::nextCallid();
    FetionSipEvent sendEvent( FetionSipEvent::SipService );
    sendEvent.addHeader( "F", m_from );
    sendEvent.addHeader( "I", QString::number( callid ) );
    sendEvent.addHeader( "Q", "2 S" );
    sendEvent.addHeader( "N", "GetContactInfoV4" );

    QString serviceBody;
    serviceBody = "<args><contact user-id=\"%1\" /></args>";
    serviceBody = serviceBody.arg( id );
    sendEvent.addHeader( "L", QString::number( serviceBody.toUtf8().size() ) );
    sendEvent.setContent( serviceBody );

    FetionSipEventCallbackData cbData = { &FetionSession::requestBuddyDetailCB, id };
    m_callidCallback[ callid ] = cbData;
    m_notifier->sendSipEvent( sendEvent );
}

void FetionSession::requestBuddyPortrait( const QString& id )
{
    QString to = m_buddyIdSipUri.value( id );
    if ( to.isEmpty() ) {
        qWarning() << "no such sipuri for contact id" << id;
        return;
    }

    QNetworkRequest request;
    QUrl url;
    url.setUrl( m_getPortraitUri );
    url.setPort( 80 );
    url.addEncodedQueryItem( "Uri", QUrl::toPercentEncoding( to ) );
    url.addEncodedQueryItem( "Size", "120" );
    url.addEncodedQueryItem( "c", QUrl::toPercentEncoding( m_ssicCookie ) );
    request.setUrl( url );
    request.setRawHeader( "User-Agent", "IIC2.0/PC "PROTO_VERSION );
    request.setRawHeader( "Accept", m_portraitFileTypes.toAscii() );
    request.setRawHeader( "Host", "hdss1fta.fetion.com.cn" );
    request.setRawHeader( "Connection", "Keep-Alive" );
    QNetworkReply* r = manager->get( request );
    m_portraitReplyId[ r ] = id;
    connect( r, SIGNAL(finished()), this, SLOT(requestBuddyPortraitFinished()) );
}

void FetionSession::sendKeepAliveCB( bool isSuccessed, const FetionSipEvent& callbackEvent, const QVariant& data )
{
    Q_UNUSED(callbackEvent)
    Q_UNUSED(data)
    qWarning() << "sendKeepAliveCB" << isSuccessed;
}

void FetionSession::requestChatChannelCB( bool isSuccessed, const FetionSipEvent& callbackEvent, const QVariant& data )
{
    qWarning() << "requestChatChannelCB" << isSuccessed << callbackEvent.toString();
    QString callid = callbackEvent.getFirstValue( "I" );
    QString sequence = callbackEvent.getFirstValue( "Q" );
    QString auth = callbackEvent.getFirstValue( "A" );
    QString addressList = auth.section( '\"', 1, 1, QString::SectionSkipEmpty );
    QString credential = auth.section( '\"', 3, 3, QString::SectionSkipEmpty );
    QString firstAddressPort = addressList.section( ';', 0, 0, QString::SectionSkipEmpty );
    QString firstAddress = firstAddressPort.section( ':', 0, 0, QString::SectionSkipEmpty );
    QString firstPort = firstAddressPort.section( ':', 1, 1, QString::SectionSkipEmpty );

    qWarning() << "Establish a chat channel" << firstAddressPort;
    FetionSipNotifier* chatNotifier = new FetionSipNotifier( this );
    connect( chatNotifier, SIGNAL(sipEventReceived(const FetionSipEvent&)),
                this, SLOT(handleSipEvent(const FetionSipEvent&)) );
    chatNotifier->connectToHost( firstAddress, firstPort.toInt() );
    emit chatChannelEstablished( chatNotifier );
}

void FetionSession::sendClientMessageCB( bool isSuccessed, const FetionSipEvent& callbackEvent, const QVariant& data )
{
    Q_UNUSED(callbackEvent)
    qWarning() << "sendClientMessageCB" << isSuccessed;
    QString id = data.toString();
    emit sendClientMessageSuccessed( id );
}

void FetionSession::requestBuddyDetailCB( bool isSuccessed, const FetionSipEvent& callbackEvent, const QVariant& data )
{
    qWarning() << "requestBuddyDetailCB" << isSuccessed;
    QString id = data.toString();
    QDomDocument doc;
    doc.setContent( callbackEvent.getContent() );
    QDomElement rootElem = doc.documentElement();
    QDomElement contactElem = rootElem.firstChildElement( "contact" );
    emit gotBuddyDetail( id, contactElem.attributes() );
}

