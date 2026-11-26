/***************************************************************************
                              qgssbsetunloadprojects.cpp
                              -------------------------
  begin                : 2020-04-22
  copyright            : (C) 2020 by Andreas Steffens
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

#include "qgsmodule.h"
#include "qgssbserviceexception.h"
#include "qgsapplication.h"

#include "qgssbsetunloadprojects.h"
#include "qgsserverprojectutils.h"
#include "qgsconfigcache.h"
#include "qgssettings.h"
#include "sbutils.h"

#include "qgsexception.h"
#include "qgsexpressionnodeimpl.h"

#include "qgsexception.h"

namespace QgsSb
{
  void writeSetUnloadProjects( QgsServerInterface *serverIface, const QString &version, const QgsServerRequest &request,
                               QgsServerResponse &response, const QString &projects )
  {
    QString strUnloadConfig = QDir( serverIface->serverSettings()->cacheDirectory() ).filePath( "unload_" + serverIface->sbTenant() );

    QFile fileConfig( strUnloadConfig );
    if ( !fileConfig.open( QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate ) )
      return;

    try
    {
      if ( !projects.isNull() && !projects.isEmpty() )
      {
        QStringList listProjects = projects.split( "," );

        QTextStream streamOut( &fileConfig );
        for ( QStringList::const_iterator it = listProjects.constBegin(); it != listProjects.constEnd(); it++ )
        {
          QString strPath = *it;
          bool bClearName = strPath.contains( ".qgs", Qt::CaseInsensitive ) || strPath.contains( ".qgz", Qt::CaseInsensitive );
          if ( !bClearName )
            strPath = SimpleCrypt::sbDecrypt( strPath );

          strPath = sbGetStandardizedPath( strPath );

          streamOut << strPath << endl;
        }
      }
    }
    catch ( std::exception &ex )
    {
      QgsMessageLog::logMessage( QStringLiteral( "writeSetUnloadProjects - Exception: %1" ).arg( QString( ex.what() ) ), QStringLiteral( "Server" ), Qgis::Critical );
    }
    catch ( ... )
    {
      QgsMessageLog::logMessage( QStringLiteral( "writeSetUnloadProjects - Unknown exception" ), QStringLiteral( "Server" ), Qgis::Critical );
    }

    fileConfig.close();

    response.setStatusCode( 200 );
    response.setHeader( QStringLiteral( "Content-Type" ), QStringLiteral( "text/plain; charset=utf-8" ) );
    response.write( "ok" );
  }
} // namespace QgsWms
