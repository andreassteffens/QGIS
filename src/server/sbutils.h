/***************************************************************************
                           sbutils.h

  sb service implementation
  -----------------------------
  begin                : 2021-01-21
  copyright            : (C) 2021 by Andreas Steffens
  email                : a dot steffens at gds dash team dot de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SBUTILS_H
#define SBUTILS_H

#define SIP_NO_FILE


#include <qstring.h>


inline QString sbGetStandardizedPath( const QString &strPath )
{
  QString strStandardizedPath = strPath;
  strStandardizedPath = strStandardizedPath.replace( QStringLiteral( "\\" ), QStringLiteral( "/" ) );

#ifdef Q_OS_WIN
  strStandardizedPath = strStandardizedPath.toLower();
#endif

  return strStandardizedPath;
}

#endif // SBUTILS_H
