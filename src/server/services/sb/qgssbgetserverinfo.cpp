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

    QDomElement serverInfoElem = doc.createElement( QStringLiteral( "sbServerInfo" ) );
    serverInfoElem.setAttribute( QStringLiteral( "xmlns" ), QStringLiteral( "http://www.atapa.de/sbServerInfo" ) );
    serverInfoElem.setAttribute( QStringLiteral( "version" ), QStringLiteral( "1.0.0" ) );
    doc.appendChild( serverInfoElem );

    QDomElement versionElem = doc.createElement( QStringLiteral( "Version" ) );
    QDomText versionText = doc.createTextNode( Qgis::version() );
    versionElem.appendChild( versionText );
    serverInfoElem.appendChild( versionElem );

    QDomElement devVersionElem = doc.createElement( QStringLiteral( "Version" ) );
    QDomText devVersionText = doc.createTextNode( Qgis::devVersion() );
    devVersionElem.appendChild( devVersionText );
    serverInfoElem.appendChild( devVersionElem );

    QDomElement releaseElem = doc.createElement( QStringLiteral( "Release" ) );
    QDomText releaseText = doc.createTextNode( Qgis::releaseName() );
    releaseElem.appendChild( releaseText );
    serverInfoElem.appendChild( releaseElem );

    QgsSettings settings;
    QString strUserTranslation = settings.value( QStringLiteral( "locale/userLocale" ), "" ).toString();
    QString strGlobalTranslation = settings.value( QStringLiteral( "locale/globalLocale" ), "" ).toString();
    QDomElement localeElem = doc.createElement( QStringLiteral( "Locale" ) );
    QDomText localeText = doc.createTextNode( strGlobalTranslation + " | " + strUserTranslation );
    localeElem.appendChild( localeText );
    serverInfoElem.appendChild( localeElem );

    QDomElement translationElem = doc.createElement( QStringLiteral( "Translation" ) );
    QDomText translationText = doc.createTextNode( QgsApplication::instance()->translation() );
    translationElem.appendChild( translationText );
    serverInfoElem.appendChild( translationElem );

    QDomElement pathsElem = doc.createElement( QStringLiteral( "Paths" ) );
    QDomText pathsText = doc.createTextNode( QgsApplication::showSettings() );
    pathsElem.appendChild( pathsText );
    serverInfoElem.appendChild( pathsElem );

    QFontDatabase db;
    QStringList listFamilies = db.families();
    QDomElement fontsElem = doc.createElement( QStringLiteral( "Fonts" ) );
    QDomText fontsText = doc.createTextNode( listFamilies.join( "," ) );
    fontsElem.appendChild( fontsText );
    serverInfoElem.appendChild( fontsElem );


    QDomElement serverConfigurationElem = doc.createElement( QStringLiteral( "ServerConfiguration" ) );
    serverInfoElem.appendChild( serverConfigurationElem );

    QStringList qstrlstSettings = serverIface->serverSettings()->getSettings();
    for ( int iSetting = 0; iSetting < qstrlstSettings.count(); iSetting++ )
    {
      QStringList qstrlstParts = qstrlstSettings[iSetting].split( "=" );
      if ( qstrlstParts.count() == 2 )
      {
        QDomElement configElem = doc.createElement( QStringLiteral( "ConfigParam" ) );
        configElem.setAttribute( "name", qstrlstParts[0] );
        QDomText configText = doc.createTextNode( qstrlstParts[1] );
        configElem.appendChild( configText );
        serverConfigurationElem.appendChild( configElem );
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


    QDomElement gdalConfigurationElem = doc.createElement( QStringLiteral( "GdalConfiguration" ) );
    serverInfoElem.appendChild( gdalConfigurationElem );

    char **ppOptions = CPLGetConfigOptions();
    for ( char *pOption = *ppOptions; pOption; pOption = *++ppOptions )
    {
      QString qstrOption = pOption;
      QStringList qstrlstParts = qstrOption.split( "=" );
      if ( qstrlstParts.count() == 2 )
      {
        QDomElement configElem = doc.createElement( QStringLiteral( "ConfigParam" ) );
        configElem.setAttribute( "name", qstrlstParts[0] );
        QDomText configText = doc.createTextNode( qstrlstParts[1] );
        configElem.appendChild( configText );
        gdalConfigurationElem.appendChild( configElem );
      }
    }


    QDomElement qtConfigurationElem = doc.createElement( QStringLiteral( "QtConfiguration" ) );
    serverInfoElem.appendChild( qtConfigurationElem );

    QProcessEnvironment qEnv = QProcessEnvironment::systemEnvironment();
    QStringList qlistQtEnv = qEnv.toStringList();
    for ( int i = 0; i < qlistQtEnv.count(); i++ )
    {
      QStringList qstrlstParts = qlistQtEnv[i].split( "=" );
      if ( qstrlstParts.count() == 2 )
      {
        QDomElement configElem = doc.createElement( QStringLiteral( "ConfigParam" ) );
        configElem.setAttribute( "name", qstrlstParts[0] );
        QDomText configText = doc.createTextNode( qstrlstParts[1] );
        configElem.appendChild( configText );
        qtConfigurationElem.appendChild( configElem );
      }
    }

    QDomElement loadedProjectsElem = doc.createElement( QStringLiteral( "LoadedProjects" ) );
    serverInfoElem.appendChild( loadedProjectsElem );

    QStringList listProjects = serverIface->sbLoadedProjects();
    for ( int i = 0; i < listProjects.count(); i++ )
    {
      QDomElement configElem = doc.createElement( QStringLiteral( "Path" ) );
      QDomText configText = doc.createTextNode( listProjects[i] );
      configElem.appendChild( configText );
      loadedProjectsElem.appendChild( configElem );
    }

    QDomElement blockedProjectsElem = doc.createElement( QStringLiteral( "UnloadedProjects" ) );
    serverInfoElem.appendChild( blockedProjectsElem );

    QString strUnloadConfig = QDir( serverIface->serverSettings()->cacheDirectory() ).filePath( "unload_" + serverIface->sbTenant() );
    QFile unloadFile( strUnloadConfig );
    if ( unloadFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
      QTextStream streamIn( &unloadFile );
      QString strLine;
      while ( streamIn.readLineInto( &strLine ) )
      {
        if ( strLine.isNull() || strLine.isEmpty() )
          continue;

        QDomElement configElem = doc.createElement( QStringLiteral( "Path" ) );
        QDomText configText = doc.createTextNode( strLine );
        configElem.appendChild( configText );
        blockedProjectsElem.appendChild( configElem );
      }

      unloadFile.close();
    }

    response.setHeader( QStringLiteral( "Content-Type" ), QStringLiteral( "text/xml; charset=utf-8" ) );
    response.write( doc.toByteArray() );
  }
} // namespace QgsSb
