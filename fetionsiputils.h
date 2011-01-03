#ifndef FETIONSIPUTILS_H
#define FETIONSIPUTILS_H

#include <QString>

namespace FetionSipUtils
{
    QString SipUriToSid( const QString& sipUri );
    QString SIdToSipUri( const QString& sId, const QString& mobileNo );
}

#endif // FETIONSIPUTILS_H
