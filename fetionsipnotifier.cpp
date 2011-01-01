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
    qWarning() << "FetionSipNotifier::connectToHost" << hostAddress << port;
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
        FetionSipEvent newEvent( siptypeStr, sipheaderStr );

        switch ( newEvent.sipType() ) {
            case FetionSipEvent::SipInvitation: {
                int contentBegin = datastr.indexOf( "s=", headerEnd );
                int contentEnd = datastr.size();/// FIXME: i'm just lazy here...  ;)  --- nihui
                QString contentStr = datastr.mid( contentBegin, contentEnd - contentBegin );
                newEvent.setContent( contentStr );
                index = contentEnd;
                emit sipEventReceived( newEvent );
                break;
            }
            case FetionSipEvent::SipMessage: {
                int contentBegin = datastr.indexOf( "<Font", headerEnd );
                int contentEnd = datastr.indexOf( "</Font>", contentBegin ) + 7;/// length of </Font>
                QString contentStr = datastr.mid( contentBegin, contentEnd - contentBegin );
                newEvent.setContent( contentStr );
                index = contentEnd;
                emit sipEventReceived( newEvent );
                break;
            }
            case FetionSipEvent::SipIncoming: {
                emit sipEventReceived( newEvent );
                break;
            }
            case FetionSipEvent::SipNotification: {
                QString sid = newEvent.typeAddition().section( ' ', 0, 0, QString::SectionSkipEmpty );
                QString notificationType = newEvent.getFirstValue( "N" );
                int contentBegin = datastr.indexOf( "<events>", headerEnd );
                int contentEnd = datastr.indexOf( "</events>", contentBegin ) + 9;/// length of </events>
                QString contentStr = datastr.mid( contentBegin, contentEnd - contentBegin );
                newEvent.setContent( contentStr );
                index = contentEnd;
                emit sipEventReceived( newEvent );
                break;
            }
            case FetionSipEvent::SipOption: {
                emit sipEventReceived( newEvent );
                break;
            }
            case FetionSipEvent::SipSipc_4_0: {
                QString callid = newEvent.getFirstValue( "I" );
                if ( !newEvent.getFirstValue( "L" ).isEmpty() ) {
                    int contentBegin = datastr.indexOf( "<results>", headerEnd );
                    int contentEnd = datastr.indexOf( "</results>", contentBegin ) + 10;/// length of </results>
                    QString contentStr = datastr.mid( contentBegin, contentEnd - contentBegin );
                    newEvent.setContent( contentStr );
                    index = contentEnd;
                }
                else {
                    index = headerEnd;
                }
                emit sipEventReceived( newEvent );
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
