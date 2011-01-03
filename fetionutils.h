#ifndef FETIONUTILS_H
#define FETIONUTILS_H

#include <QString>

namespace FetionUtils
{
    QString SipUriToSid( const QString& sipUri );
    QString SIdToSipUri( const QString& sId, const QString& mobileNo );
}

#endif // FETIONUTILS_H
