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
    if ( !fileConfig.open( QIODevice::WriteOnly | QIODevice::Text ) )
      return;

    QStringList listProjects = projects.split( "," );

    QTextStream streamOut( &fileConfig );
    for ( int i = 0; i < listProjects.count(); i++ )
      streamOut << listProjects[i] + "\n";

    response.setStatusCode( 200 );
    response.setHeader( QStringLiteral( "Content-Type" ), QStringLiteral( "text/plain; charset=utf-8" ) );
    response.write( "ok" );
  }
} // namespace QgsSb
