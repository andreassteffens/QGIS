/***************************************************************************
                              qgsserver.cpp
 A server application supporting WMS / WFS / WCS
                              -------------------
  begin                : July 04, 2006
  copyright            : (C) 2006 by Marco Hugentobler & Ionut Iosifescu Enescu
                       : (C) 2015 by Alessandro Pasotti
  email                : marco dot hugentobler at karto dot baug dot ethz dot ch
                       : elpaso at itopen dot it
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

//for CMAKE_INSTALL_PREFIX
#include "qgsconfig.h"
#include "qgsversion.h"
#include "qgsserver.h"
#include "qgsauthmanager.h"
#include "qgscapabilitiescache.h"
#include "qgsfontutils.h"
#include "qgsrequesthandler.h"
#include "qgsproject.h"
#include "qgsproviderregistry.h"
#include "qgslogger.h"
#include "qgsmapserviceexception.h"
#include "qgsnetworkaccessmanager.h"
#include "qgsserverlogger.h"
#include "qgsserverrequest.h"
#include "qgsfilterresponsedecorator.h"
#include "qgsservice.h"
#include "qgsserverapi.h"
#include "qgsserverapicontext.h"
#include "qgsserverparameters.h"
#include "qgsapplication.h"
#include "qgsruntimeprofiler.h"
#include "qgscoordinatetransform.h"
#include "qgssettings.h"
#include "sbservercachefilter.h"
#include "sbutils.h"

#include <QDomDocument>
#include <QNetworkDiskCache>
#include <QSettings>
#include <QElapsedTimer>
#include <QFont>
#include <QFontDatabase>

// TODO: remove, it's only needed by a single debug message
#include <fcgi_stdio.h>
#include <cstdlib>

#include <cpl_conv.h>

// Server status static initializers.
// Default values are for C++, SIP bindings will override their
// options in in init()

QString *QgsServer::sConfigFilePath = nullptr;
QgsCapabilitiesCache *QgsServer::sCapabilitiesCache = nullptr;
QgsServerInterfaceImpl *QgsServer::sServerInterface = nullptr;
sbServerCacheFilter *QgsServer::sSbServerCacheFilter = nullptr;

// Initialization must run once for all servers
bool QgsServer::sInitialized = false;

SimpleCrypt QgsServer::sCrypto;

QgsServiceRegistry *QgsServer::sServiceRegistry = nullptr;

Q_GLOBAL_STATIC( QgsServerSettings, sSettings );

QgsServer::QgsServer(const QString& strTenant)
{
  mSbTenant = strTenant;

  // QgsApplication must exist
  if (qobject_cast<QgsApplication *>(qApp) == nullptr)
  {
    qFatal("A QgsApplication must exist before a QgsServer instance can be created.");
    abort();
  }
  init(strTenant);
  
  QString strUnloadConfig = QDir(sSettings()->cacheDirectory()).filePath("unload_" + strTenant);
  mSbUnloadWatcher.setWatchedPath(strUnloadConfig);
  mSbUnloadWatcher.setTimeout(sSettings()->sbUnloadWatcherInterval());
  mSbUnloadWatcher.start();
}

QgsServer::QgsServer()
{
  // QgsApplication must exist
  if ( qobject_cast<QgsApplication *>( qApp ) == nullptr )
  {
    qFatal( "A QgsApplication must exist before a QgsServer instance can be created." );
    abort();
  }
  init("");
}

QgsServer::~QgsServer()
{
  mSbUnloadWatcher.exit();
  if (!mSbUnloadWatcher.wait(3000))
    mSbUnloadWatcher.terminate();
}

QString &QgsServer::serverName()
{
  static QString *name = new QString(QStringLiteral("qgis_server"));
  return *name;
}

QFileInfo QgsServer::defaultAdminSLD()
{
  return QFileInfo( QStringLiteral( "admin.sld" ) );
}

void QgsServer::setupNetworkAccessManager()
{
  const QSettings settings;
  QgsNetworkAccessManager *nam = QgsNetworkAccessManager::instance();
  QNetworkDiskCache *cache = new QNetworkDiskCache( nullptr );
  const qint64 cacheSize = sSettings()->cacheSize();
  const QString cacheDirectory = sSettings()->cacheDirectory();
  cache->setCacheDirectory( cacheDirectory );
  cache->setMaximumCacheSize( cacheSize );
  QgsMessageLog::logMessage( QStringLiteral( "cacheDirectory: %1" ).arg( cache->cacheDirectory() ), QStringLiteral( "Server" ), Qgis::MessageLevel::Info );
  QgsMessageLog::logMessage( QStringLiteral( "maximumCacheSize: %1" ).arg( cache->maximumCacheSize() ), QStringLiteral( "Server" ), Qgis::MessageLevel::Info );
  nam->setCache( cache );
}

QFileInfo QgsServer::defaultProjectFile()
{
  const QDir currentDir;
  fprintf( FCGI_stderr, "current directory: %s\n", currentDir.absolutePath().toUtf8().constData() );
  QStringList nameFilterList;
  nameFilterList << QStringLiteral( "*.qgs" )
                 << QStringLiteral( "*.qgz" );
  const QFileInfoList projectFiles = currentDir.entryInfoList( nameFilterList, QDir::Files, QDir::Name );
  for ( int x = 0; x < projectFiles.size(); x++ )
  {
    QgsMessageLog::logMessage( projectFiles.at( x ).absoluteFilePath(), QStringLiteral( "Server" ), Qgis::MessageLevel::Info );
  }
  if ( projectFiles.isEmpty() )
  {
    return QFileInfo();
  }
  return projectFiles.at( 0 );
}

void QgsServer::printRequestParameters( const QMap< QString, QString> &parameterMap, Qgis::MessageLevel logLevel )
{
  if ( logLevel > Qgis::MessageLevel::Info )
  {
    return;
  }

  QMap< QString, QString>::const_iterator pIt = parameterMap.constBegin();
  for ( ; pIt != parameterMap.constEnd(); ++pIt )
  {
    QgsMessageLog::logMessage( pIt.key() + ":" + pIt.value(), QStringLiteral( "Server" ), Qgis::MessageLevel::Info );
  }
}

QString QgsServer::configPath( const QString &defaultConfigPath, const QString &configPath )
{
  QString cfPath( defaultConfigPath );
  const QString projectFile = sSettings()->projectFile();
  if ( !projectFile.isEmpty() )
  {
    cfPath = projectFile;
    QgsDebugMsg( QStringLiteral( "QGIS_PROJECT_FILE:%1" ).arg( cfPath ) );
  }
  else
  {
    if ( configPath.isEmpty() )
    {
      // Read it from the environment, because a rewrite rule may have rewritten it
      if ( getenv( "QGIS_PROJECT_FILE" ) )
      {
        cfPath = getenv( "QGIS_PROJECT_FILE" );
        QgsMessageLog::logMessage( QStringLiteral( "Using configuration file path from environment: %1" ).arg( cfPath ), QStringLiteral( "Server" ), Qgis::MessageLevel::Info );
      }
      else  if ( ! defaultConfigPath.isEmpty() )
      {
        QgsMessageLog::logMessage( QStringLiteral( "Using default configuration file path: %1" ).arg( defaultConfigPath ), QStringLiteral( "Server" ), Qgis::MessageLevel::Info );
      }
    }
    else
    {
      cfPath = sbGetDecryptedProjectPath(configPath);
    }
  }
  return cfPath;
}

const QString& QgsServer::sbTenant()
{
  return mSbTenant;
}

int QgsServer::sbPreloadProjects()
{
  int iRet = 0;

  QString strTenant = sbTenant();

  if (strTenant.isEmpty())
    strTenant = "default";

  QString strPreloadConfig = QDir(sSettings->cacheDirectory()).filePath("preload_" + strTenant);
  QgsMessageLog::logMessage(QStringLiteral("Attempting to preload projects from '%1' ...").arg(strPreloadConfig), QStringLiteral("Server"), Qgis::Warning);
  if (QFile::exists(strPreloadConfig))
  {
    QFile fileConfig(strPreloadConfig);
    if (!fileConfig.open(QIODevice::ReadOnly | QIODevice::Text))
      return iRet;

    QTextStream streamIn(&fileConfig);
    QString strLine;
    while (streamIn.readLineInto(&strLine))
    {
      try
      {
        if (mSbUnloadWatcher.isUnloaded(strLine))
          continue;

        QgsMessageLog::logMessage(QStringLiteral("Preloading project '%1' ...").arg(strLine), QStringLiteral("Server"), Qgis::Warning);
        const QgsProject* pProject = QgsConfigCache::instance()->project(strLine);
        if (pProject != NULL)
        {
          QgsMessageLog::logMessage(QStringLiteral("Preloading of project '%1' SUCCEEDED").arg(strLine), QStringLiteral("Server"), Qgis::Warning);
          iRet++;
        }
        else
        {
          QgsMessageLog::logMessage(QStringLiteral("Preloading of project '%1' FAILED").arg(strLine), QStringLiteral("Server"), Qgis::Warning);
          iRet--;
        }
      }
      catch (QgsServerException &ex)
      {
        QgsMessageLog::logMessage(QStringLiteral("Preloading project '%1' failed: %2").arg(strLine).arg(QString(ex.what())), QStringLiteral("Server"), Qgis::Critical);
        throw ex;
      }
      catch (QgsException &ex)
      {
        QgsMessageLog::logMessage(QStringLiteral("Preloading project '%1' failed: %2").arg(strLine).arg(QString(ex.what())), QStringLiteral("Server"), Qgis::Critical);
        throw ex;
      }
      catch (std::runtime_error &ex)
      {
        QgsMessageLog::logMessage(QStringLiteral("Preloading project '%1' failed: %2").arg(strLine).arg(QString(ex.what())), QStringLiteral("Server"), Qgis::Critical);
        throw ex;
      }
      catch (std::exception &ex)
      {
        QgsMessageLog::logMessage(QStringLiteral("Preloading project '%1' failed %2").arg(strLine).arg(QString(ex.what())), QStringLiteral("Server"), Qgis::Critical);
        throw ex;
      }
      catch (...)
      {
        QgsMessageLog::logMessage(QStringLiteral("Preloading project '%1' failed").arg(strLine), QStringLiteral("Server"), Qgis::Critical);
      }
    }
  }

  return iRet;
}

QString QgsServer::sbGetDecryptedProjectPath(QString strPath)
{
  QString strDecryptedPath = strPath;

  bool bClearName = strPath.contains(".qgs", Qt::CaseInsensitive) || strPath.contains(".qgz", Qt::CaseInsensitive);
  if (!bClearName)
  {
    strDecryptedPath = sCrypto.sbDecryptFromBase64String(strPath);

    if (!QFileInfo(strDecryptedPath).exists())
      strDecryptedPath = strPath;
  }

  return strDecryptedPath;
}

void QgsServer::initLocale()
{
  // System locale override
  if ( ! sSettings()->overrideSystemLocale().isEmpty() )
  {
    QLocale::setDefault( QLocale( sSettings()->overrideSystemLocale() ) );
  }
  // Number group separator settings
  QLocale currentLocale;
  if ( sSettings()->showGroupSeparator() )
  {
    currentLocale.setNumberOptions( currentLocale.numberOptions() &= ~QLocale::NumberOption::OmitGroupSeparator );
  }
  else
  {
    currentLocale.setNumberOptions( currentLocale.numberOptions() |= QLocale::NumberOption::OmitGroupSeparator );
  }
  QLocale::setDefault( currentLocale );
}

bool QgsServer::init(const QString& strTenant)
{
  if ( sInitialized )
  {
    return false;
  }

  QCoreApplication::setOrganizationName( QgsApplication::QGIS_ORGANIZATION_NAME );
  QCoreApplication::setOrganizationDomain( QgsApplication::QGIS_ORGANIZATION_DOMAIN );
  QCoreApplication::setApplicationName( QgsApplication::QGIS_APPLICATION_NAME );

  // TODO: remove QGIS_OPTIONS_PATH from settings and rely on QgsApplication's env var QGIS_CUSTOM_CONFIG_PATH
  //       Note that QGIS_CUSTOM_CONFIG_PATH gives /tmp/qt_temp-rUpsId/profiles/default/QGIS/QGIS3.ini
  //       while     QGIS_OPTIONS_PATH gives       /tmp/qt_temp-rUpsId/QGIS/QGIS3.ini
  QgsApplication::init( qgetenv( "QGIS_OPTIONS_PATH" ) );

#if defined(SERVER_SKIP_ECW)
  QgsMessageLog::logMessage( "Skipping GDAL ECW drivers in server.", "Server", Qgis::MessageLevel::Info );
  QgsApplication::skipGdalDriver( "ECW" );
  QgsApplication::skipGdalDriver( "JP2ECW" );
#endif

  // reload settings to take into account QCoreApplication and QgsApplication
  // configuration
  sSettings()->load();

  // init and configure logger
  QgsServerLogger::instance();
  QgsServerLogger::instance()->setLogLevel( sSettings()->logLevel() );
  if ( ! sSettings()->logFile().isEmpty() )
  {
    QgsServerLogger::instance()->setLogFile( sSettings()->logFile() );
  }
  else if ( sSettings()->logStderr() )
  {
    QgsServerLogger::instance()->setLogStderr();
  }

  // Logging handlers for CRS grid issues
  QgsCoordinateTransform::setCustomMissingRequiredGridHandler( [ = ]( const QgsCoordinateReferenceSystem & sourceCrs,
      const QgsCoordinateReferenceSystem & destinationCrs,
      const QgsDatumTransform::GridDetails & grid )
  {
    QgsServerLogger::instance()->logMessage( QStringLiteral( "Cannot use project transform between %1 and %2 - missing grid %3" )
        .arg( sourceCrs.userFriendlyIdentifier( QgsCoordinateReferenceSystem::ShortString ),
              destinationCrs.userFriendlyIdentifier( QgsCoordinateReferenceSystem::ShortString ),
              grid.shortName ),
        QStringLiteral( "QGIS Server" ), Qgis::MessageLevel::Warning );
  } );


  QgsCoordinateTransform::setCustomMissingGridUsedByContextHandler( [ = ]( const QgsCoordinateReferenceSystem & sourceCrs,
      const QgsCoordinateReferenceSystem & destinationCrs,
      const QgsDatumTransform::TransformDetails & details )
  {
    QString gridMessage;
    for ( const QgsDatumTransform::GridDetails &grid : details.grids )
    {
      if ( !grid.isAvailable )
      {
        gridMessage.append( QStringLiteral( "This transformation requires the grid file '%1', which is not available for use on the system.\n" ).arg( grid.shortName ) );
      }
    }
    QgsServerLogger::instance()->logMessage( QStringLiteral( "Cannot use project transform between %1 and %2 - %3.\n%4" )
        .arg( sourceCrs.userFriendlyIdentifier( QgsCoordinateReferenceSystem::ShortString ),
              destinationCrs.userFriendlyIdentifier( QgsCoordinateReferenceSystem::ShortString ),
              details.name,
              gridMessage ),
        QStringLiteral( "QGIS Server" ), Qgis::MessageLevel::Warning );
  } );


  QgsCoordinateTransform::setCustomMissingPreferredGridHandler( [ = ]( const QgsCoordinateReferenceSystem & sourceCrs,
      const QgsCoordinateReferenceSystem & destinationCrs,
      const QgsDatumTransform::TransformDetails & preferredOperation,
      const QgsDatumTransform::TransformDetails & availableOperation )
  {

    QString gridMessage;
    for ( const QgsDatumTransform::GridDetails &grid : preferredOperation.grids )
    {
      if ( !grid.isAvailable )
      {
        gridMessage.append( QStringLiteral( "This transformation requires the grid file '%1', which is not available for use on the system.\n" ).arg( grid.shortName ) );
        if ( !grid.url.isEmpty() )
        {
          if ( !grid.packageName.isEmpty() )
          {
            gridMessage.append( QStringLiteral( "This grid is part of the '%1' package, available for download from %2.\n" ).arg( grid.packageName, grid.url ) );
          }
          else
          {
            gridMessage.append( QStringLiteral( "This grid is available for download from %1.\n" ).arg( grid.url ) );
          }
        }
      }
    }

    QString accuracyMessage;
    if ( availableOperation.accuracy >= 0 && preferredOperation.accuracy >= 0 )
      accuracyMessage = QStringLiteral( "Current transform '%1' has an accuracy of %2 meters, while the preferred transformation '%3' has accuracy %4 meters.\n" ).arg( availableOperation.name )
                        .arg( availableOperation.accuracy ).arg( preferredOperation.name ).arg( preferredOperation.accuracy );
    else if ( preferredOperation.accuracy >= 0 )
      accuracyMessage = QStringLiteral( "Current transform '%1' has an unknown accuracy, while the preferred transformation '%2' has accuracy %3 meters.\n" )
                        .arg( availableOperation.name, preferredOperation.name )
                        .arg( preferredOperation.accuracy );

    const QString longMessage = QStringLiteral( "The preferred transform between '%1' and '%2' is not available for use on the system.\n" ).arg( sourceCrs.userFriendlyIdentifier(),
                                destinationCrs.userFriendlyIdentifier() )
                                + gridMessage + accuracyMessage;

    QgsServerLogger::instance()->logMessage( longMessage, QStringLiteral( "QGIS Server" ), Qgis::MessageLevel::Warning );

  } );

  QgsCoordinateTransform::setCustomCoordinateOperationCreationErrorHandler( [ = ]( const QgsCoordinateReferenceSystem & sourceCrs, const QgsCoordinateReferenceSystem & destinationCrs, const QString & error )
  {
    const QString longMessage = QStringLiteral( "No transform is available between %1 and %2: %3" )
                                .arg( sourceCrs.userFriendlyIdentifier(), destinationCrs.userFriendlyIdentifier(), error );
    QgsServerLogger::instance()->logMessage( longMessage, QStringLiteral( "QGIS Server" ), Qgis::MessageLevel::Warning );
  } );

  // Configure locale
  initLocale();

  QgsMessageLog::logMessage( QStringLiteral( "QGIS Server Starting : %1 (%2)" ).arg( _QGIS_VERSION, QGSVERSION ), "Server", Qgis::MessageLevel::Info );

  // log settings currently used
  sSettings()->logSummary();

  setupNetworkAccessManager();
  QDomImplementation::setInvalidDataPolicy( QDomImplementation::DropInvalidChars );

  // Instantiate the plugin directory so that providers are loaded
  QgsProviderRegistry::instance( QgsApplication::pluginPath() );
  QgsMessageLog::logMessage( "Prefix  PATH: " + QgsApplication::prefixPath(), QStringLiteral( "Server" ), Qgis::MessageLevel::Info );
  QgsMessageLog::logMessage( "Plugin  PATH: " + QgsApplication::pluginPath(), QStringLiteral( "Server" ), Qgis::MessageLevel::Info );
  QgsMessageLog::logMessage( "PkgData PATH: " + QgsApplication::pkgDataPath(), QStringLiteral( "Server" ), Qgis::MessageLevel::Info );
  QgsMessageLog::logMessage( "User DB PATH: " + QgsApplication::qgisUserDatabaseFilePath(), QStringLiteral( "Server" ), Qgis::MessageLevel::Info );
  QgsMessageLog::logMessage( "Auth DB PATH: " + QgsApplication::qgisAuthDatabaseFilePath(), QStringLiteral( "Server" ), Qgis::MessageLevel::Info );
  QgsMessageLog::logMessage( "SVG PATHS: " + QgsApplication::svgPaths().join( QDir::listSeparator() ), QStringLiteral( "Server" ), Qgis::MessageLevel::Info );

  QString qstrConfigValue = getenv("GDAL_HTTP_TIMEOUT");
  if (!qstrConfigValue.isEmpty())
    CPLSetConfigOption("GDAL_HTTP_TIMEOUT", qstrConfigValue.toStdString().c_str());

  qstrConfigValue = getenv("GDAL_HTTP_UNSAFESSL");
  if (!qstrConfigValue.isEmpty())
    CPLSetConfigOption("GDAL_HTTP_UNSAFESSL", qstrConfigValue.toStdString().c_str());

  qstrConfigValue = getenv("VSI_CACHE");
  if (!qstrConfigValue.isEmpty())
    CPLSetConfigOption("VSI_CACHE", qstrConfigValue.toStdString().c_str());

  qstrConfigValue = getenv("VSI_CACHE_SIZE");
  if (!qstrConfigValue.isEmpty())
    CPLSetConfigOption("VSI_CACHE_SIZE", qstrConfigValue.toStdString().c_str());

  qstrConfigValue = getenv("GDAL_CACHEMAX");
  if (!qstrConfigValue.isEmpty())
    CPLSetConfigOption("GDAL_CACHEMAX", qstrConfigValue.toStdString().c_str());

  qstrConfigValue = getenv("GDAL_SWATH_SIZE");
  if (!qstrConfigValue.isEmpty())
    CPLSetConfigOption("GDAL_SWATH_SIZE", qstrConfigValue.toStdString().c_str());

  qstrConfigValue = getenv("GDAL_FILENAME_IS_UTF8");
  if (!qstrConfigValue.isEmpty())
    CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", qstrConfigValue.toStdString().c_str());

  qstrConfigValue = getenv("OGR_SQLITE_PRAGMA");
  if (!qstrConfigValue.isEmpty())
    CPLSetConfigOption("OGR_SQLITE_PRAGMA", qstrConfigValue.toStdString().c_str());

  qstrConfigValue = getenv("OGR_SQLITE_JOURNAL");
  if (!qstrConfigValue.isEmpty())
    CPLSetConfigOption("OGR_SQLITE_JOURNAL", qstrConfigValue.toStdString().c_str());

  qstrConfigValue = getenv("OGR_SQLITE_SYNCHRONOUS");
  if (!qstrConfigValue.isEmpty())
    CPLSetConfigOption("OGR_SQLITE_SYNCHRONOUS", qstrConfigValue.toStdString().c_str());

  qstrConfigValue = getenv("CPL_DEBUG");
  if (!qstrConfigValue.isEmpty())
    CPLSetConfigOption("CPL_DEBUG", qstrConfigValue.toStdString().c_str());

  qstrConfigValue = getenv("CPL_LOG_ERRORS");
  if (!qstrConfigValue.isEmpty())
    CPLSetConfigOption("CPL_LOG_ERRORS", qstrConfigValue.toStdString().c_str());

  qstrConfigValue = getenv("CPL_LOG");
  if (!qstrConfigValue.isEmpty())
    CPLSetConfigOption("CPL_LOG", qstrConfigValue.toStdString().c_str());

  QgsApplication::createDatabase(); //init qgis.db (e.g. necessary for user crs)

  // Initialize the authentication system
  //   creates or uses qgis-auth.db in ~/.qgis3/ or directory defined by QGIS_AUTH_DB_DIR_PATH env variable
  //   set the master password as first line of file defined by QGIS_AUTH_PASSWORD_FILE env variable
  //   (QGIS_AUTH_PASSWORD_FILE variable removed from environment after accessing)
  QgsApplication::authManager()->init( QgsApplication::pluginPath(), QgsApplication::qgisAuthDatabaseFilePath() );

  QString defaultConfigFilePath;
  const QFileInfo projectFileInfo = defaultProjectFile(); //try to find a .qgs/.qgz file in the server directory
  if ( projectFileInfo.exists() )
  {
    defaultConfigFilePath = projectFileInfo.absoluteFilePath();
    QgsMessageLog::logMessage( QStringLiteral("Using default project file: %1").arg(defaultConfigFilePath), QStringLiteral( "Server" ), Qgis::MessageLevel::Info );
  }
  else
  {
    const QFileInfo adminSLDFileInfo = defaultAdminSLD();
    if ( adminSLDFileInfo.exists() )
    {
      defaultConfigFilePath = adminSLDFileInfo.absoluteFilePath();
    }
  }
  // Store the config file path
  sConfigFilePath = new QString( defaultConfigFilePath );

  //create cache for capabilities XML
  sCapabilitiesCache = new QgsCapabilitiesCache();

  // Initialize config cache
  QgsConfigCache::initialize(sSettings);

  QgsFontUtils::loadStandardTestFonts( QStringList() << QStringLiteral( "Roman" ) << QStringLiteral( "Bold" ) );

  QgsMessageLog::logMessage("([a]tapa) This service has been modified by the great and powerful Ender. Go ahead and enjoy!", QStringLiteral("Server"), Qgis::Info);

  QString strFontPath = sSettings->fontsDirectory();
  if (!strFontPath.isEmpty())
  {
    QgsMessageLog::logMessage("([a]tapa) Font directory: " + strFontPath, QStringLiteral("Server"), Qgis::Info);

    int iLoaded = QgsFontUtils::loadLocalResourceFonts(strFontPath);

    QgsMessageLog::logMessage("([a]tapa) Loaded additional fonts: " + QString::number(iLoaded), QStringLiteral("Server"), Qgis::Info);
  }

  sServiceRegistry = new QgsServiceRegistry();

  sServerInterface = new QgsServerInterfaceImpl( sCapabilitiesCache, sServiceRegistry, sSettings(), strTenant );
  sCrypto.setKey(Q_UINT64_C(0x0c2ad4a4acb9f023));

  if (!sSettings->cacheDirectory().isEmpty() && sSettings->sbUseCache())
  {
    QString strCacheDirectory = QDir(sSettings->cacheDirectory()).filePath("sb");
    QgsMessageLog::logMessage(QStringLiteral("([a]tapa) Initializing server cache in directory: %1").arg(strCacheDirectory), QStringLiteral("Server"), Qgis::Info);

    sSbServerCacheFilter = new sbServerCacheFilter(sServerInterface, strCacheDirectory);
    sServerInterface->registerServerCache(sSbServerCacheFilter, 1);
  }

  // Load service module
  const QString modulePath = QgsApplication::libexecPath() + "server";
  // qDebug() << QStringLiteral( "Initializing server modules from: %1" ).arg( modulePath );
  sServiceRegistry->init( modulePath,  sServerInterface );

  sInitialized = true;
  QgsMessageLog::logMessage( QStringLiteral( "Server initialized" ), QStringLiteral( "Server" ), Qgis::MessageLevel::Info );
  return true;
}



void QgsServer::putenv( const QString &var, const QString &val )
{
  if ( val.isEmpty() )
  {
    qunsetenv( var.toUtf8().data() );
  }
  else
  {
    qputenv( var.toUtf8().data(), val.toUtf8() );
  }
  sSettings()->load( var );
}

void QgsServer::handleRequest( QgsServerRequest &request, QgsServerResponse &response, const QgsProject *project )
{
  const Qgis::MessageLevel logLevel = QgsServerLogger::instance()->logLevel();
  {

    const QgsScopedRuntimeProfile profiler { QStringLiteral( "handleRequest" ), QStringLiteral( "server" ) };

    qApp->processEvents();

    response.clear();

    // Clean up qgis access control filter's cache to prevent side effects
    // across requests
#ifdef HAVE_SERVER_PYTHON_PLUGINS
    QgsAccessControl *accessControls = sServerInterface->accessControls();
    if ( accessControls )
    {
      accessControls->unresolveFilterFeatures();
    }
#endif

    // Pass the filters to the requestHandler, this is needed for the following reasons:
    // Allow server request to call sendResponse plugin hook if enabled
    QgsFilterResponseDecorator responseDecorator( sServerInterface->filters(), response );

    //Request handler
    QgsRequestHandler requestHandler( request, response );

    try
    {
      // TODO: split parse input into plain parse and processing from specific services
      requestHandler.parseInput();
    }
    catch ( QgsMapServiceException &e )
    {
      QgsMessageLog::logMessage( QStringLiteral("Parse input exception: %1").arg(e.message()), QStringLiteral( "Server" ), Qgis::MessageLevel::Critical );
      requestHandler.setServiceException( e );
    }

    // Set the request handler into the interface for plugins to manipulate it
    sServerInterface->setRequestHandler( &requestHandler );

    // Initialize configfilepath so that is is available
    // before calling plugin methods
    // Note that plugins may still change that value using
    // setConfigFilePath() interface method
    if ( ! project )
    {
      const QString configFilePath = configPath( *sConfigFilePath, request.serverParameters().map() );
      sServerInterface->setConfigFilePath( configFilePath );
    }
    else
    {
      sServerInterface->setConfigFilePath( project->fileName() );
    }

    // Call  requestReady() method (if enabled)
    // This may also throw exceptions if there are errors in python plugins code
    try
    {
      responseDecorator.start();
    }
    catch ( QgsException &ex )
    {
      // Internal server error
      response.sendError( 500, QStringLiteral( "Internal Server Error" ) );
      QgsMessageLog::logMessage( ex.what(), QStringLiteral( "Server" ), Qgis::MessageLevel::Critical );
    }

    // Plugins may have set exceptions
    if ( !requestHandler.exceptionRaised() )
    {
      try
      {
        const QgsServerParameters params = request.serverParameters();
        printRequestParameters( params.toMap(), logLevel );

        // Setup project (config file path)
        if ( !project )
        {
          const QString configFilePath = configPath( *sConfigFilePath, params.map() );

          if (configFilePath.isEmpty())
          {
            bool bError = true;

            QString serviceString = params.service();
            if (!serviceString.isEmpty() && serviceString.compare("sb", Qt::CaseInsensitive) == 0)
            {
              QString requestString = params.request();
              if (!requestString.isEmpty() && 
                 (requestString.compare("GetServerInfo", Qt::CaseInsensitive) == 0 || 
                 requestString.compare("GetServerProcessId", Qt::CaseInsensitive) == 0 || 
                 requestString.compare("SetPreloadProjects", Qt::CaseInsensitive) == 0 ||
                 requestString.compare("SetUnloadProjects", Qt::CaseInsensitive) == 0))
                bError = false;
            }

            if(bError)
              throw QgsServerException(QStringLiteral("Project file path is empty!"));
          }
          else
          {
            if(mSbUnloadWatcher.isUnloaded(configFilePath))
              throw QgsServerException(QStringLiteral("Project has been marked unloaded!"));

            // load the project if needed and not empty
            // Note that  QgsConfigCache::project( ... ) call QgsProject::setInstance(...)
            project = QgsConfigCache::instance()->project( configFilePath, sServerInterface->serverSettings() );
          }
        }

        // Set the current project instance
        QgsProject::setInstance( const_cast<QgsProject *>( project ) );

        if ( project )
        {
          sServerInterface->setConfigFilePath( project->fileName() );
        }
        else
        {
          sServerInterface->setConfigFilePath( QString() );
        }

        // Note that at this point we still might not have set a valid project.
        // There are APIs that work without a project (e.g. the landing page catalog API that
        // lists the available projects metadata).

        // Dispatcher: if SERVICE is set, we assume a OWS service, if not, let's try an API
        // TODO: QGIS 4 fix the OWS services and treat them as APIs
        QgsServerApi *api = nullptr;

        if ( params.service().isEmpty() && ( api = sServiceRegistry->apiForRequest( request ) ) )
        {
          const QgsServerApiContext context { api->rootPath(), &request, &responseDecorator, project, sServerInterface };
          api->executeRequest( context );
        }
        else
        {
          //Service parameter
          QString serviceString = params.service();

          if (!serviceString.isEmpty() && serviceString.compare("sb", Qt::CaseInsensitive) == 0)
          {
            // empty project doesn't matter at this point
          }
          else if ( ! project )
          {
            throw QgsServerException( QStringLiteral( "Project file error. For OWS services: please provide a SERVICE and a MAP parameter pointing to a valid QGIS project file" ) );
          }

          if (serviceString.isEmpty())
          {
            // SERVICE not mandatory for WMS 1.3.0 GetMap & GetFeatureInfo
            QString requestString = params.request();
            requestString = requestString.toLower();
            if (requestString == QLatin1String("getmap") || requestString == QLatin1String("getfeatureinfo"))
            {
              serviceString = QStringLiteral("WMS");
            }
          }

          QString versionString = params.version();

          if ( ! params.fileName().isEmpty() )
          {
            const QString value = QString( "attachment; filename=\"%1\"" ).arg( params.fileName() );
            requestHandler.setResponseHeader( QStringLiteral( "Content-Disposition" ), value );
          }

          // Lookup for service
          QgsService *service = sServiceRegistry->getService(serviceString, versionString);
          if (service == NULL)
            service = sServiceRegistry->getService(serviceString.toUpper(), versionString);
          if (service == NULL)
            service = sServiceRegistry->getService(serviceString.toLower(), versionString);
          if ( service )
          {
            service->executeRequest( request, responseDecorator, project );
          }
          else
          {
            QStringList listServices = sServiceRegistry->sbGetRegisteredServices();
            throw QgsOgcServiceException( QStringLiteral( "Service configuration error" ),
                                          QStringLiteral( "Service unknown or unsupported. Current supported services: %1, or use a WFS3 (OGC API Features) endpoint" ).arg(listServices.join(',')));
          }
        }
      }
      catch ( QgsServerException &ex )
      {
        responseDecorator.write( ex );
        QString format;
        QgsMessageLog::logMessage( ex.formatResponse( format ), QStringLiteral( "Server" ), Qgis::MessageLevel::Critical );
      }
      catch ( QgsException &ex )
      {
        // Internal server error
        QgsMessageLog::logMessage(QStringLiteral("QgsException: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
        response.sendError(500, QString(ex.what()));
      }
      catch (std::runtime_error &ex)
      {
        // Internal server error
        QgsMessageLog::logMessage(QStringLiteral("RuntimeError: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
        response.sendError(500, QString(ex.what()));
      }
      catch (std::exception &ex)
      {
        // Internal server error
        QgsMessageLog::logMessage(QStringLiteral("Exception: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
        response.sendError(500, QString(ex.what()));
      }
      catch (...)
      {
        // Internal server error
        QgsMessageLog::logMessage(QStringLiteral("Unknown exception: %1").arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
        response.sendError(500, QStringLiteral("Unknown exception"));
      }
    }

    // Terminate the response
    // This may also throw exceptions if there are errors in python plugins code
    try
    {
      responseDecorator.finish();
    }
    catch ( QgsException &ex )
    {
      // Internal server error
      response.sendError( 500, QStringLiteral( "Internal Server Error" ) );
      QgsMessageLog::logMessage( ex.what(), QStringLiteral( "Server" ), Qgis::MessageLevel::Critical );
    }

    // We are done using requestHandler in plugins, make sure we don't access
    // to a deleted request handler from Python bindings
    sServerInterface->clearRequestHandler();
  }

  if ( logLevel == Qgis::MessageLevel::Info )
  {
    QgsMessageLog::logMessage( QStringLiteral("Request finished in %1").arg(QString::number( QgsApplication::profiler()->profileTime( QStringLiteral( "handleRequest" ), QStringLiteral( "server" ) ) * 1000.0 ) + " ms"), QStringLiteral( "Server" ), Qgis::MessageLevel::Info );
    if ( sSettings->logProfile() )
    {
      std::function <void( const QModelIndex &, int )> profileFormatter;
      profileFormatter = [ &profileFormatter ]( const QModelIndex & idx, int level )
      {
        QgsMessageLog::logMessage( QStringLiteral( "Profile: %1%2, %3 : %4 ms" )
                                   .arg( level > 0 ? QString().fill( '-', level ) + ' ' : QString() )
                                   .arg( QgsApplication::profiler()->data( idx, QgsRuntimeProfilerNode::Roles::Group ).toString() )
                                   .arg( QgsApplication::profiler()->data( idx, QgsRuntimeProfilerNode::Roles::Name ).toString() )
                                   .arg( QString::number( QgsApplication::profiler()->data( idx, QgsRuntimeProfilerNode::Roles::Elapsed ).toDouble() * 1000.0 ) ), QStringLiteral( "Server" ), Qgis::MessageLevel::Info );

        for ( int subRow = 0; subRow < QgsApplication::profiler()->rowCount( idx ); subRow++ )
        {
          const auto subIdx { QgsApplication::profiler()->index( subRow, 0, idx ) };
          profileFormatter( subIdx, level + 1 );
        }

      };

      for ( int row = 0; row < QgsApplication::profiler()->rowCount( ); row++ )
      {
        const auto idx { QgsApplication::profiler()->index( row, 0 ) };
        profileFormatter( idx, 0 );
      }
    }
  }


  // Clear the profiler server section after each request
  QgsApplication::profiler()->clear( QStringLiteral( "server" ) );

}


#ifdef HAVE_SERVER_PYTHON_PLUGINS
void QgsServer::initPython()
{
  // Init plugins
  if ( ! QgsServerPlugins::initPlugins( sServerInterface ) )
  {
    QgsMessageLog::logMessage( QStringLiteral( "No server python plugins are available" ), QStringLiteral( "Server" ), Qgis::MessageLevel::Info );
  }
  else
  {
    QgsMessageLog::logMessage( QStringLiteral( "Server python plugins loaded" ), QStringLiteral( "Server" ), Qgis::MessageLevel::Info );
  }
}
#endif

