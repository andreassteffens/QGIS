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
#include <qdir.h>

inline bool sbIsChildPath( const QString &parentPath, const QString &childPath )
{
  QDir parent( QDir::cleanPath( parentPath ) );

  QString relative = parent.relativeFilePath( QDir::cleanPath( childPath ) );

  return !relative.startsWith( ".." ) && relative != ".." &&  !QDir::isAbsolutePath( relative );
}

inline QString sbGetStandardizedPath( const QString &path )
{
  QString standardizedPath = path;
  standardizedPath = standardizedPath.replace( QStringLiteral( "\\" ), QStringLiteral( "/" ) );

#ifdef Q_OS_WIN
  standardizedPath = standardizedPath.toLower();
#endif

  return standardizedPath;
}

#endif // SBUTILS_H
