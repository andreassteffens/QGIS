/***************************************************************************
                              qgssbsetpreloadprojects.cpp
                              --------------------------
  begin                : 2020-01-29
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
#include "sbutils.h"

#include "qgssbsetpreloadprojects.h"
#include "qgsserverprojectutils.h"
#include "qgsconfigcache.h"

#include "qgsexception.h"
#include "qgsexpressionnodeimpl.h"


namespace QgsSb
{
  void writeSetPreloadProjects( QgsServerInterface *serverIface, const QgsProject *project,
                                const QString &version, const QgsServerRequest &request,
                                QgsServerResponse &response, const QString &projects )
  {
    Q_UNUSED( version );

    QString strPreloadConfig = QDir( serverIface->serverSettings()->cacheDirectory() ).filePath( "preload_" + serverIface->sbTenant() );

    QFile fileConfig( strPreloadConfig );
    if ( !fileConfig.open( QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate ) )
      return;

    try
    {
      QStringList listProjects = projects.split( "," );

      QTextStream streamOut( &fileConfig );
      for ( QStringList::const_iterator it = listProjects.constBegin(); it != listProjects.constEnd(); it++ )
      {
        QString strPath = *it;

        if ( !strPath.isNull() && !strPath.isEmpty() )
        {
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
      QgsMessageLog::logMessage( QStringLiteral( "writeSetPreloadProjects - Exception: %1" ).arg( QString( ex.what() ) ), QStringLiteral( "Server" ), Qgis::Critical );
    }
    catch ( ... )
    {
      QgsMessageLog::logMessage( QStringLiteral( "writeSetPreloadProjects - Unknown exception" ), QStringLiteral( "Server" ), Qgis::Critical );
    }

    fileConfig.close();

    response.setStatusCode( 200 );
    response.setHeader( QStringLiteral( "Content-Type" ), QStringLiteral( "text/plain; charset=utf-8" ) );
    response.write( "ok" );
  }
} // namespace QgsSb
