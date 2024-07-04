/***************************************************************************
                              qgssbgetserverinfo.h
                              -------------------------
  begin                : 2018-10-05
  copyright            : (C) 2018 by Andreas Steffens
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

#include "qgssbgetserverinfo.h"
#include "qgsserverprojectutils.h"
#include "qgsconfigcache.h"
#include "qgssettings.h"

#include "qgsexception.h"
#include "qgsexpressionnodeimpl.h"

#include <qfontdatabase.h>
#include <qprocess.h>

#include <cpl_conv.h>


namespace QgsSb
{
  void writeGetServerInfo( QgsServerInterface *serverIface, const QgsProject *project,
                           const QString &version, const QgsServerRequest &request,
                           QgsServerResponse &response )
  {
    Q_UNUSED( version );

    QDomDocument doc;
    QDomProcessingInstruction xmlDeclaration = doc.createProcessingInstruction( QStringLiteral( "xml" ),
        QStringLiteral( "version=\"1.0\" encoding=\"utf-8\"" ) );

    doc.appendChild( xmlDeclaration );


    serverIface->sbRequestLogMessage( QStringLiteral( "writeGetServerInfo: Collecting application info..." ) );

    QDomElement serverInfoElem = doc.createElement( QStringLiteral( "sbServerInfo" ) );
    serverInfoElem.setAttribute( QStringLiteral( "xmlns" ), QStringLiteral( "http://www.atapa.de/sbServerInfo" ) );
    serverInfoElem.setAttribute( QStringLiteral( "version" ), QStringLiteral( "1.0.0" ) );
    doc.appendChild( serverInfoElem );

    QDomElement versionElem = doc.createElement( QStringLiteral( "Version" ) );
    QDomText versionText = doc.createTextNode( Qgis::version() );
    versionElem.appendChild( versionText );
    serverInfoElem.appendChild( versionElem );

    QDomElement devVersionElem = doc.createElement( QStringLiteral( "DevVersion" ) );
    QDomText devVersionText = doc.createTextNode( Qgis::devVersion() );
    devVersionElem.appendChild( devVersionText );
    serverInfoElem.appendChild( devVersionElem );

    QDomElement releaseElem = doc.createElement( QStringLiteral( "Release" ) );
    QDomText releaseText = doc.createTextNode( Qgis::releaseName() );
    releaseElem.appendChild( releaseText );
    serverInfoElem.appendChild( releaseElem );


    serverIface->sbRequestLogMessage( QStringLiteral( "writeGetServerInfo: Collecting localization info..." ) );

    QgsSettings settings;
    QString strUserLocale = settings.value( QStringLiteral( "locale/userLocale" ), "N/A" ).toString();
    QString strGlobalLocale = settings.value( QStringLiteral( "locale/globalLocale" ), "N/A" ).toString();
    QDomElement localeElem = doc.createElement( QStringLiteral( "Locale" ) );
    QDomText localeText = doc.createTextNode( strGlobalLocale + " | " + strUserLocale );
    localeElem.appendChild( localeText );
    serverInfoElem.appendChild( localeElem );


    serverIface->sbRequestLogMessage( QStringLiteral( "writeGetServerInfo: Collecting translation info..." ) );

    QString strTranslation = QgsApplication::instance()->translation();
    if ( !strTranslation.isNull() && !strTranslation.isEmpty() )
    {
      QDomElement translationElem = doc.createElement( QStringLiteral( "Translation" ) );
      QDomText translationText = doc.createTextNode( strTranslation );
      translationElem.appendChild( translationText );
      serverInfoElem.appendChild( translationElem );
    }


    serverIface->sbRequestLogMessage( QStringLiteral( "writeGetServerInfo: Collecting paths info..." ) );

    QDomElement pathsElem = doc.createElement( QStringLiteral( "Paths" ) );
    QDomText pathsText = doc.createTextNode( QgsApplication::showSettings() );
    pathsElem.appendChild( pathsText );
    serverInfoElem.appendChild( pathsElem );


    serverIface->sbRequestLogMessage( QStringLiteral( "writeGetServerInfo: Collecting fonts info..." ) );

    QFontDatabase db;
    QStringList listFamilies = db.families();
    QDomElement fontsElem = doc.createElement( QStringLiteral( "Fonts" ) );
    serverInfoElem.appendChild(fontsElem);

    for ( QStringList::const_iterator it = listFamilies.constBegin(); it != listFamilies.constEnd(); ++it )
    {
      QDomElement fontElem = doc.createElement( QStringLiteral( "Font" ) );
      QDomText fontText = doc.createTextNode ( *it );
      fontElem.appendChild( fontText );
      fontsElem.appendChild( fontElem );
    }


    serverIface->sbRequestLogMessage( QStringLiteral( "writeGetServerInfo: Collecting configuration parameters..." ) );

    QDomElement serverConfigurationElem = doc.createElement( QStringLiteral( "ServerConfiguration" ) );
    serverInfoElem.appendChild( serverConfigurationElem );

    QStringList listSettings = serverIface->serverSettings()->getSettings();
    for ( QStringList::const_iterator it = listSettings.constBegin(); it != listSettings.constEnd(); it++ )
    {
      if ( !it->isNull() && !it->isEmpty() )
      {
        QStringList listParts = it->split( "=" );
        if ( listParts.count() == 2 )
        {
          if ( !listParts[0].isNull() && !listParts[0].isEmpty() && !listParts[1].isNull() && !listParts[1].isEmpty() )
          {
            QDomElement configElem = doc.createElement( QStringLiteral( "ConfigParam" ) );
            configElem.setAttribute( "name", listParts[0] );
            QDomText configText = doc.createTextNode( listParts[1] );
            configElem.appendChild( configText );
            serverConfigurationElem.appendChild( configElem );
          }
        }
      }
    }

    QDomElement prefixPathElem = doc.createElement( QStringLiteral( "ConfigParam" ) );
    prefixPathElem.setAttribute( "name", "Prefix-Path" );
    QDomText prefixPathText = doc.createTextNode( QgsApplication::prefixPath() );
    prefixPathElem.appendChild( prefixPathText );
    serverConfigurationElem.appendChild( prefixPathElem );

    QDomElement pkgPathElem = doc.createElement( QStringLiteral( "ConfigParam" ) );
    pkgPathElem.setAttribute( "name", "PkgData-Path" );
    QDomText pkgPathText = doc.createTextNode( QgsApplication::pkgDataPath() );
    pkgPathElem.appendChild( pkgPathText );
    serverConfigurationElem.appendChild( pkgPathElem );

    QDomElement userDbPathElem = doc.createElement( QStringLiteral( "ConfigParam" ) );
    userDbPathElem.setAttribute( "name", "UserDB-Path" );
    QDomText userDbPathText = doc.createTextNode( QgsApplication::qgisUserDatabaseFilePath() );
    userDbPathElem.appendChild( userDbPathText );
    serverConfigurationElem.appendChild( userDbPathElem );

    QDomElement authDbPathElem = doc.createElement( QStringLiteral( "ConfigParam" ) );
    authDbPathElem.setAttribute( "name", "AuthDB-Path" );
    QDomText authDbPathText = doc.createTextNode( QgsApplication::qgisAuthDatabaseFilePath() );
    authDbPathElem.appendChild( authDbPathText );
    serverConfigurationElem.appendChild( authDbPathElem );

    QDomElement svgPathElem = doc.createElement( QStringLiteral( "ConfigParam" ) );
    svgPathElem.setAttribute( "name", "SVG-Path" );
    QDomText svgPathText = doc.createTextNode( QgsApplication::svgPaths().join( ',' ) );
    svgPathElem.appendChild( svgPathText );
    serverConfigurationElem.appendChild( svgPathElem );


    serverIface->sbRequestLogMessage( QStringLiteral( "writeGetServerInfo: Collecting GEOS info..." ) );

    QDomElement geosVersionElem = doc.createElement( QStringLiteral( "GeosVersion" ) );
    QDomText geosVersionText = doc.createTextNode( Qgis::geosVersion() );
    geosVersionElem.appendChild( geosVersionText );
    serverInfoElem.appendChild( geosVersionElem );


    serverIface->sbRequestLogMessage( QStringLiteral( "writeGetServerInfo: Collecting Qt info..." ) );

    QDomElement qtConfigurationElem = doc.createElement( QStringLiteral( "QtConfiguration" ) );
    serverInfoElem.appendChild( qtConfigurationElem );

    QProcessEnvironment qEnv = QProcessEnvironment::systemEnvironment();
    QStringList listQtEnv = qEnv.toStringList();
    for ( QStringList::const_iterator it = listQtEnv.constBegin(); it != listQtEnv.constEnd(); it++ )
    {
      if ( !it->isNull() && !it->isEmpty() )
      {
        QStringList listParts = it->split( "=" );
        if ( listParts.count() == 2 )
        {
          if ( !listParts[0].isNull() && !listParts[0].isEmpty() && !listParts[1].isNull() && !listParts[1].isEmpty() )
          {
            QDomElement configElem = doc.createElement( QStringLiteral( "ConfigParam" ) );
            configElem.setAttribute( "name", listParts[0] );
            QDomText configText = doc.createTextNode( listParts[1] );
            configElem.appendChild( configText );
            qtConfigurationElem.appendChild( configElem );
          }
        }
      }
    }

    serverIface->sbRequestLogMessage( QStringLiteral( "writeGetServerInfo: Collecting projects info..." ) );

    QDomElement loadedProjectsElem = doc.createElement( QStringLiteral( "LoadedProjects" ) );
    serverInfoElem.appendChild( loadedProjectsElem );

    QStringList listProjects = serverIface->sbLoadedProjects();
    for ( QStringList::const_iterator it = listProjects.constBegin(); it != listProjects.constEnd(); it++ )
    {
      if ( !it->isNull() && !it->isEmpty() )
      {
        QDomElement configElem = doc.createElement( QStringLiteral( "Project" ) );
        QDomText configText = doc.createTextNode( *it );
        configElem.appendChild( configText );
        loadedProjectsElem.appendChild( configElem );
      }
    }

    QDomElement blockedProjectsElem = doc.createElement( QStringLiteral( "UnloadedProjects" ) );
    serverInfoElem.appendChild( blockedProjectsElem );

    QString strUnloadConfig = QDir( serverIface->serverSettings()->cacheDirectory() ).filePath( "unload_" + serverIface->sbTenant() );
    QFile unloadFile( strUnloadConfig );
    if ( unloadFile.exists() )
    {
      if ( unloadFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
      {
        try
        {
          QTextStream streamIn( &unloadFile );
          QString strLine;
          while ( streamIn.readLineInto( &strLine ) )
          {
            if ( strLine.isNull() || strLine.isEmpty() )
              continue;

            QDomElement configElem = doc.createElement( QStringLiteral( "Project" ) );
            QDomText configText = doc.createTextNode( strLine );
            configElem.appendChild( configText );
            blockedProjectsElem.appendChild( configElem );
          }
        }
        catch ( std::exception &ex )
        {
          QgsMessageLog::logMessage( QStringLiteral( "writeGetServerInfo - Exception: %1" ).arg( QString( ex.what() ) ), QStringLiteral( "Server" ), Qgis::Critical );
        }
        catch ( ... )
        {
          QgsMessageLog::logMessage( QStringLiteral( "writeGetServerInfo - Unknown exception" ), QStringLiteral( "Server" ), Qgis::Critical );
        }

        unloadFile.close();
      }
    }

    response.setStatusCode( 200 );
    response.setHeader( QStringLiteral( "Content-Type" ), QStringLiteral( "text/xml; charset=utf-8" ) );
    response.write( doc.toByteArray() );
  }
} // namespace QgsSb
