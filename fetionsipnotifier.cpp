#include "fetionsipnotifier.h"

#include <QStringList>
#include <KDebug>

#include "fetionsipevent.h"

FetionSipNotifier::FetionSipNotifier( QObject* parent )
: QObject(parent)
{
    connect( &m_socket, SIGNAL(readyRead()), this, SLOT(slotReadyRead()) );
}

FetionSipNotifier::~FetionSipNotifier()
{
    close();
}

void FetionSipNotifier::connectToHost( const QString& hostAddress, int port )
{
    qWarning() << "FetionSipNotifier::connectToHost" << hostAddress << port;
    m_socket.connectToHost( hostAddress, port );
}

void FetionSipNotifier::close()
{
    m_socket.close();
}

void FetionSipNotifier::sendSipEvent( const FetionSipEvent& sipEvent )
{
    m_socket.write( sipEvent.toString().toUtf8() );
}

void FetionSipNotifier::slotReadyRead()
{
    // storing data received from last read
    static QByteArray pendingBuffer;

    QByteArray data = m_socket.readAll();
    if ( data.isEmpty() )
        return;

    QByteArray datastr = pendingBuffer + data;
    qWarning() << datastr;

    int index = 0;
    int len = datastr.size();
    int headerBegin;
    int headerEnd;
    int siptypeStrEnd;
    int sipheaderStrBegin;
    int contentBegin;

    while ( index < len ) {
        headerBegin = index;
        headerEnd = datastr.indexOf( "\r\n\r\n", headerBegin );
        siptypeStrEnd = datastr.indexOf( "\r\n", headerBegin );
        sipheaderStrBegin = siptypeStrEnd + 2;// length of \r\n
        QByteArray siptypeStr = datastr.mid( headerBegin, siptypeStrEnd - headerBegin );
        QByteArray sipheaderStr = datastr.mid( sipheaderStrBegin, headerEnd - sipheaderStrBegin );
        FetionSipEvent newEvent( QString::fromUtf8( siptypeStr ), QString::fromUtf8( sipheaderStr ) );

        QString contentLengthStr = newEvent.getFirstValue( "L" );
        if ( contentLengthStr.isEmpty() ) {
            // content-less events
            index = headerEnd + 4;// length of \r\n\r\n
            emit sipEventReceived( newEvent );
            continue;
        }

        int contentLength = contentLengthStr.toInt();
        if ( len - headerEnd < contentLength ) {
            // data has been split into more than one read, pending it
            pendingBuffer = datastr.right( len - headerBegin );
            return;
        }
        pendingBuffer.clear();

        contentBegin = headerEnd + 4;// length of \r\n\r\n
        QByteArray contentStr = datastr.mid( contentBegin, contentLength );
        newEvent.setContent( QString::fromUtf8( contentStr ) );
        index = contentBegin + contentLength;
        emit sipEventReceived( newEvent );
    }
}
