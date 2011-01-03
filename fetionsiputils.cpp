#include "fetionsiputils.h"

QString FetionSipUtils::SipUriToSid( const QString& sipUri )
{
    return sipUri.section( ':', 1, -1, QString::SectionSkipEmpty ).section( '@', 0, 0, QString::SectionSkipEmpty );
}

QString FetionSipUtils::SIdToSipUri( const QString& sId, const QString& mobileNo )
{
    QString pTag = QString::number( mobileNo.left( 6 ).toInt() - 134099 );
    return QString( "sip:" ) + sId + "@fetion.com.cn;p=" + pTag;
}
