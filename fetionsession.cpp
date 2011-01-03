#include "fetionsession.h"

#include "fetioncontact.h"
#include "fetionprotocol.h"
#include "fetionsipevent.h"
#include "fetionsipnotifier.h"
#include "fetionsiputils.h"
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
        m_sipUri = userElem.attribute( "uri" );
        QString mobileno = userElem.attribute( "mobile-no" );
        QString userstatus = userElem.attribute( "user-status" );
        m_userId = userElem.attribute( "user-id" );

        m_from = FetionSipUtils::SipUriToSid( m_sipUri );

        /// connect to sipc server
        QString sipcAddressIp = m_sipcProxyAddress.section( ':', 0, 0, QString::SectionSkipEmpty );
        int sipcAddressPort = m_sipcProxyAddress.section( ':', -1, -1, QString::SectionSkipEmpty ).toInt();
        notifier = new FetionSipNotifier( this );
        connect( notifier, SIGNAL(sipEventReceived(const FetionSipEvent&)),
                 this, SLOT(handleSipcRegisterReplyEvent(const FetionSipEvent&)) );
        notifier->connectToHost( sipcAddressIp, sipcAddressPort );

        /// sipc register
        QString m_nouce = generateNouce();
        FetionSipEvent registerEvent( FetionSipEvent::SipRegister );
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

                qWarning() << sipcAuthActionEvent.toString();

                notifier->sendSipEvent( sipcAuthActionEvent );
            }
            else if ( sipEvent.typeAddition() == "200 OK" ) {
                /// sipc register success
                disconnect( notifier, SIGNAL(sipEventReceived(const FetionSipEvent&)),
                            this, SLOT(handleSipcRegisterReplyEvent(const FetionSipEvent&)) );
                connect( notifier, SIGNAL(sipEventReceived(const FetionSipEvent&)),
                         this, SLOT(handleSipEvent(const FetionSipEvent&)) );
                /// extract buddy lists and buddies
                QDomDocument doc;
                doc.setContent( sipEvent.getContent() );
                QDomElement docElem = doc.documentElement();
                QDomElement userinfoElem = doc.firstChildElement( "user-info" );
                QDomElement contactlistElem = userinfoElem.firstChildElement( "contact-list" );
                QDomElement buddylistsElem = contactlistElem.firstChildElement( "buddy-lists" );
                QDomElement buddylistElem = buddylistsElem.firstChildElement( "buddy-list" );
                while ( !buddylistElem.isNull() ) {
                    gotBuddyList( buddylistElem.attribute( "name" ) );
                    buddylistElem = buddylistElem.nextSiblingElement( "buddy-list" );
                }
                QDomElement buddiesElem = contactlistElem.firstChildElement( "buddies" );
                QDomElement buddyElem = contactlistElem.firstChildElement( "b" );
                while ( !buddyElem.isNull() ) {
                    gotBuddy( buddyElem.attribute( "i" ) );
                    buddyElem = buddyElem.nextSiblingElement( "b" );
                }

                /// send contact subscribe
                FetionSipEvent subscribeEvent( FetionSipEvent::SipSubScribe );
                subscribeEvent.addHeader( "F", m_from );
                subscribeEvent.addHeader( "I", QString::number( FetionSipEvent::nextCallid() ) );
                subscribeEvent.addHeader( "Q", "2 SUB" );
                subscribeEvent.addHeader( "N", "PresenceV4" );

                QString subscribeBody = "<args><subscription self=\"v4default;mail-count\" buddy=\"v4default\" version=\"0\"/></args>";
                subscribeEvent.setContent( subscribeBody );
                qWarning() << subscribeEvent.toString();
                notifier->sendSipEvent( subscribeEvent );

                /// NOTE WARNING FIXME debug...
                sendMobilePhoneMessageToMyself( "This is the first message from kopete-fetion plugin!!! --- nihui" );
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
    qWarning() << sipEvent.toString();
    switch ( sipEvent.sipType() ) {
        case FetionSipEvent::SipInvite: {
            QString from = sipEvent.getFirstValue( "F" );
            QString auth = sipEvent.getFirstValue( "A" );
            QString addressList = auth.section( '\"', 1, 1, QString::SectionSkipEmpty );
            QString credential = auth.section( '\"', 3, 3, QString::SectionSkipEmpty );
            QString firstAddressPort = addressList.section( ';', 0, 0, QString::SectionSkipEmpty );
            QString firstAddress = firstAddressPort.section( ':', 0, 0, QString::SectionSkipEmpty );
            QString firstPort = firstAddressPort.section( ':', 1, 1, QString::SectionSkipEmpty );

            qWarning() << "Received a conversation invitation";
            FetionSipNotifier* conversionNotifier = new FetionSipNotifier( this );
            connect( conversionNotifier, SIGNAL(sipEventReceived(const FetionSipEvent&)),
                     this, SLOT(handleSipEvent(const FetionSipEvent&)) );
            conversionNotifier->connectToHost( firstAddress, firstPort.toInt() );

            FetionSipEvent returnEvent( FetionSipEvent::SipSipc_4_0 );
            returnEvent.setTypeAddition( "200 OK" );
            returnEvent.addHeader( "F", from );
            returnEvent.addHeader( "I", "-61" );
            returnEvent.addHeader( "Q", "200002 I" );

            conversionNotifier->sendSipEvent( returnEvent );

            FetionSipEvent registerEvent( FetionSipEvent::SipRegister );
            registerEvent.addHeader( "A", "TICKS auth=\"" + credential + "\"" );
            registerEvent.addHeader( "K", "text/html-fragment" );
            registerEvent.addHeader( "K", "multiparty" );
            registerEvent.addHeader( "K", "nudge" );

            qWarning() << "Register to conversation server" << firstAddressPort;
            conversionNotifier->sendSipEvent( registerEvent );
            break;
        }
        case FetionSipEvent::SipMessage: {
            QString from = sipEvent.getFirstValue( "F" );
            QString callid = sipEvent.getFirstValue( "I" );
            QString sequence = sipEvent.getFirstValue( "Q" );
            QString sendtime = sipEvent.getFirstValue( "D" );

            FetionSipEvent returnEvent( FetionSipEvent::SipSipc_4_0 );
            returnEvent.setTypeAddition( "200 OK" );
            returnEvent.addHeader( "F", from );
            returnEvent.addHeader( "I", callid );
            returnEvent.addHeader( "Q", sequence );
            notifier->sendSipEvent( returnEvent );
            break;
        }
        case FetionSipEvent::SipInfo: {
            break;
        }
        case FetionSipEvent::SipBENotify: {
            QString notifyType = sipEvent.getFirstValue( "N" );
            QDomDocument doc;
            doc.setContent( sipEvent.getContent() );
            QDomElement docElem = doc.documentElement();
            if ( notifyType == "PresenceV4" ) {
                ///
            }
            else if ( notifyType == "Conversation" ) {
                ///
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
            if ( sipEvent.typeAddition() == "401 Unauthoried" ) {
            }
            else if ( sipEvent.typeAddition() == "200 OK" ) {
            }
            break;
        }
        case FetionSipEvent::SipUnknown: {
            qWarning() << "unknown sip message";
            return;
        }
    }
}

void FetionSession::logout()
{
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

void FetionSession::sendClientMessage( const QString& sipUri, const QString& message )
{
    FetionSipEvent sendEvent( FetionSipEvent::SipMessage );
    sendEvent.addHeader( "F", m_from );
    sendEvent.addHeader( "I", QString::number( FetionSipEvent::nextCallid() ) );
    sendEvent.addHeader( "Q", "2 M" );
    sendEvent.addHeader( "T", sipUri );
    sendEvent.addHeader( "C", "text/plain" );
    sendEvent.addHeader( "K", "SaveHistory" );
    sendEvent.addHeader( "N", "CatMsg" );

    sendEvent.setContent( message );

    notifier->sendSipEvent( sendEvent );
}

void FetionSession::sendMobilePhoneMessage( const QString& sipUri, const QString& message )
{
    FetionSipEvent sendEvent( FetionSipEvent::SipMessage );
    sendEvent.addHeader( "F", m_from );
    sendEvent.addHeader( "I", QString::number( FetionSipEvent::nextCallid() ) );
    sendEvent.addHeader( "Q", "2 M" );
    sendEvent.addHeader( "T", sipUri );
    QString Astr( "Verify algorithm=\"picc\",chid=\"%1\",response=\"%2\"" );
    Astr = Astr.arg( picid ).arg( vcode );
    sendEvent.addHeader( "A", Astr );
    sendEvent.addHeader( "N", "SendCatSMS" );

    sendEvent.setContent( message );

    notifier->sendSipEvent( sendEvent );
}

void FetionSession::sendMobilePhoneMessageToMyself( const QString& message )
{
    FetionSipEvent sendEvent( FetionSipEvent::SipMessage );
    sendEvent.addHeader( "F", m_from );
    sendEvent.addHeader( "I", QString::number( FetionSipEvent::nextCallid() ) );
    sendEvent.addHeader( "Q", "2 M" );
    sendEvent.addHeader( "T", m_sipUri );
    sendEvent.addHeader( "N", "SendCatSMS" );

    sendEvent.setContent( message );

    notifier->sendSipEvent( sendEvent );
}

