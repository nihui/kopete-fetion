#include "fetionsipnotifier.h"

#include <QStringList>
#include <KDebug>

#include "fetionsipevent.h"

/** ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

FetionSipNotifier::FetionSipNotifier( QObject* parent )
: QObject(parent)
{
    connect( &m_socket, SIGNAL(readyRead()), this, SLOT(slotReadyRead()) );
}

FetionSipNotifier::~FetionSipNotifier()
{
    m_socket.close();
}

void FetionSipNotifier::connectToHost( const QString& hostAddress, int port )
{
    m_socket.connectToHost( hostAddress, port );
}

void FetionSipNotifier::sendSipEvent( const FetionSipEvent& sipEvent )
{
    m_socket.write( sipEvent.toString().toAscii() );
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

        QString siptypeStr = headerStr.section( "\r\n", 0, 0, QString::SectionSkipEmpty );
        QString sipheaderStr = headerStr.section( "\r\n", 1, -1, QString::SectionSkipEmpty );
        FetionSipEvent header( siptypeStr, sipheaderStr );

        switch ( header.sipType() ) {
            case FetionSipEvent::SipInvitation: {
                int contentBegin = datastr.indexOf( "s=", headerEnd );
                int contentEnd = datastr.size();/// FIXME: i'm just lazy here...  ;)  --- nihui
                QString contentStr = datastr.mid( contentBegin, contentEnd - contentBegin );
                qWarning() << contentStr;

                QString from = header.getFirstValue( "F" );
                QString auth = header.getFirstValue( "A" );
                QString addressList = auth.section( '\"', 1, 1, QString::SectionSkipEmpty );
                QString credential = auth.section( '\"', 3, 3, QString::SectionSkipEmpty );
                QString firstAddressPort = addressList.section( ';', 0, 0, QString::SectionSkipEmpty );
                QString firstAddress = firstAddressPort.section( ':', 0, 0, QString::SectionSkipEmpty );
                QString firstPort = firstAddressPort.section( ':', 1, 1, QString::SectionSkipEmpty );

                FetionSipNotifier conversionNotifier;
                qWarning() << "Received a conversation invitation";

                conversionNotifier.connectToHost( firstAddress, firstPort.toInt() );

                FetionSipEvent returnEvent( FetionSipEvent::SipSipc_4_0, FetionSipEvent::EventUnknown );
                returnEvent.setTypeAddition( "200 OK" );
                returnEvent.addHeader( "F", from );
                returnEvent.addHeader( "I", "-61" );
                returnEvent.addHeader( "Q", "200002 I" );

                conversionNotifier.sendSipEvent( returnEvent );

                FetionSipEvent registerEvent( FetionSipEvent::SipRegister, FetionSipEvent::EventUnknown );
                registerEvent.addHeader( "A", "TICKS auth=\"" + credential + "\"" );
                registerEvent.addHeader( "K", "text/html-fragment" );
                registerEvent.addHeader( "K", "multiparty" );
                registerEvent.addHeader( "K", "nudge" );

                qWarning() << "Register to conversation server" << firstAddressPort;
                conversionNotifier.sendSipEvent( registerEvent );

                ///memset( buf, '\0', sizeof(buf)*sizeof(char) );
                ///int port = tcp_connection_recv( conn , buf , sizeof(buf)*sizeof(char) );

                ///memset( conversionSip->sipuri, 0, sizeof(conversionSip->sipuri)*sizeof(char) );
                ///strcpy( conversionSip->sipuri, from.toAscii().constData() );

//                 emit newThreadEntered( conversionSip, user );

                index = contentEnd;
                break;
            }
            case FetionSipEvent::SipMessage: {
                int contentBegin = datastr.indexOf( "<Font", headerEnd );
                int contentEnd = datastr.indexOf( "</Font>", contentBegin ) + 7;/// length of </Font>
                QString contentStr = datastr.mid( contentBegin, contentEnd - contentBegin );
                qWarning() << contentStr;

                QString from = header.getFirstValue( "F" );
                QString callid = header.getFirstValue( "I" );
                QString sequence = header.getFirstValue( "Q" );
                QString sendtime = header.getFirstValue( "D" );

                FetionSipEvent returnEvent( FetionSipEvent::SipSipc_4_0, FetionSipEvent::EventUnknown );
                returnEvent.setTypeAddition( "200 OK" );
                returnEvent.addHeader( "F", from );
                returnEvent.addHeader( "I", callid );
                returnEvent.addHeader( "Q", sequence );
                sendSipEvent( returnEvent );

                ///char* sid = fetion_sip_get_sid_by_sipuri( from.toAscii().constData() );
                ///emit messageReceived( QString( sid ), contentStr, from );

                index = contentEnd;
                break;
            }
            case FetionSipEvent::SipIncoming: {
                break;
            }
            case FetionSipEvent::SipNotification: {
                QString sid = header.typeAddition().section( ' ', 0, 0, QString::SectionSkipEmpty );
                QString notificationType = header.getFirstValue( "N" );

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
            case FetionSipEvent::SipOption: {
                break;
            }
            case FetionSipEvent::SipSipc_4_0: {
                QString callid = header.getFirstValue( "I" );

                if ( !header.getFirstValue( "L" ).isEmpty() ) {
                    int contentBegin = datastr.indexOf( "<results>", headerEnd );
                    int contentEnd = datastr.indexOf( "</results>", contentBegin ) + 10;/// length of </results>
                    index = contentEnd;
                }
                else {
                    index = headerEnd;
                }

                if ( callid == "5" ) {
                    /// contact information return
                }

                break;
            }
            case FetionSipEvent::SipUnknown: {
                qWarning() << "unknown sip message" << datastr;
                return;
            }
        }

        qWarning() << "#############################";
    }
}
