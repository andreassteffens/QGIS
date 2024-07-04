/***************************************************************************
                              qgswcs.cpp
                              -------------------------
  begin                : January 16 , 2017
  copyright            : (C) 2013 by René-Luc D'Hont  ( parts from qgswcsserver )
                         (C) 2017 by David Marteau
  email                : rldhont at 3liz dot com
                         david dot marteau at 3liz dot com
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
#include "qgswcsutils.h"
#include "qgswcsgetcapabilities.h"
#include "qgswcsdescribecoverage.h"
#include "qgswcsgetcoverage.h"

#define QSTR_COMPARE( str, lit )\
  (str.compare( QLatin1String( lit ), Qt::CaseInsensitive ) == 0)

namespace QgsWcs
{

  /**
   * \ingroup server
   * \class QgsWcs::Service
   * \brief OGC web service specialized for WCS
   * \since QGIS 3.0
   */
  class Service: public QgsService
  {
    public:

      /**
       * Constructor for WCS service.
       * \param serverIface Interface for plugins.
       */
      Service( QgsServerInterface *serverIface )
        : mServerIface( serverIface )
      {}

      QString name()    const override { return QStringLiteral( "WCS" ); }
      QString version() const override { return implementationVersion(); }

      void executeRequest( const QgsServerRequest &request, QgsServerResponse &response,
                           const QgsProject *project, bool sbJustLoaded ) override
      {
        Q_UNUSED( project )

        const QgsServerRequest::Parameters params = request.parameters();
        QString versionString = params.value( "VERSION" );

        // Set the default version
        if ( versionString.isEmpty() )
        {
          versionString = version();
        }

        // Get the request
        const QString req = params.value( QStringLiteral( "REQUEST" ) );
        if ( req.isEmpty() )
        {
          throw QgsServiceException( QStringLiteral( "OperationNotSupported" ),
                                     QStringLiteral( "Please add or check the value of the REQUEST parameter" ), 501 );
        }

        if ( QSTR_COMPARE( req, "GetCapabilities" ) )
        {
          try
          {
            writeGetCapabilities( mServerIface, project, versionString, request, response );
          }
          catch ( QgsServerException &ex )
          {
            QgsMessageLog::logMessage( QStringLiteral( "GetCapabilities - QgsServerException: %1" ).arg( QString( ex.what() ) ).arg( request.url().toString() ), QStringLiteral( "Server" ), Qgis::Critical );
            throw ex;
          }
          catch ( QgsException &ex )
          {
            QgsMessageLog::logMessage( QStringLiteral( "GetCapabilities - QgsException: %1" ).arg( QString( ex.what() ) ).arg( request.url().toString() ), QStringLiteral( "Server" ), Qgis::Critical );
            throw ex;
          }
          catch ( std::runtime_error &ex )
          {
            QgsMessageLog::logMessage( QStringLiteral( "GetCapabilities - RuntimeError: %1 | %2" ).arg( QString( ex.what() ) ).arg( request.url().toString() ), QStringLiteral( "Server" ), Qgis::Critical );
            throw ex;
          }
          catch ( std::exception &ex )
          {
            QgsMessageLog::logMessage( QStringLiteral( "GetCapabilities - Exception: %1 | %2" ).arg( QString( ex.what() ) ).arg( request.url().toString() ), QStringLiteral( "Server" ), Qgis::Critical );
            throw ex;
          }
          catch ( ... )
          {
            QgsMessageLog::logMessage( QStringLiteral( "GetCapabilities - Unknown exception: %1" ).arg( request.url().toString() ), QStringLiteral( "Server" ), Qgis::Critical );
            throw;
          }
        }
        else if ( QSTR_COMPARE( req, "DescribeCoverage" ) )
        {
          try
          {
            writeDescribeCoverage( mServerIface, project, versionString, request, response );
          }
          catch ( QgsServerException &ex )
          {
            QgsMessageLog::logMessage( QStringLiteral( "DescribeCoverage - QgsServerException: %1" ).arg( QString( ex.what() ) ).arg( request.url().toString() ), QStringLiteral( "Server" ), Qgis::Critical );
            throw ex;
          }
          catch ( QgsException &ex )
          {
            QgsMessageLog::logMessage( QStringLiteral( "DescribeCoverage - QgsException: %1" ).arg( QString( ex.what() ) ).arg( request.url().toString() ), QStringLiteral( "Server" ), Qgis::Critical );
            throw ex;
          }
          catch ( std::runtime_error &ex )
          {
            QgsMessageLog::logMessage( QStringLiteral( "DescribeCoverage - RuntimeError: %1 | %2" ).arg( QString( ex.what() ) ).arg( request.url().toString() ), QStringLiteral( "Server" ), Qgis::Critical );
            throw ex;
          }
          catch ( std::exception &ex )
          {
            QgsMessageLog::logMessage( QStringLiteral( "DescribeCoverage - Exception: %1 | %2" ).arg( QString( ex.what() ) ).arg( request.url().toString() ), QStringLiteral( "Server" ), Qgis::Critical );
            throw ex;
          }
          catch ( ... )
          {
            QgsMessageLog::logMessage( QStringLiteral( "DescribeCoverage - Unknown exception: %1" ).arg( request.url().toString() ), QStringLiteral( "Server" ), Qgis::Critical );
            throw;
          }
        }
        else if ( QSTR_COMPARE( req, "GetCoverage" ) )
        {
          try
          {
            writeGetCoverage( mServerIface, project, versionString, request, response );
          }
          catch ( QgsServerException &ex )
          {
            QgsMessageLog::logMessage( QStringLiteral( "GetCoverage - QgsServerException: %1" ).arg( QString( ex.what() ) ).arg( request.url().toString() ), QStringLiteral( "Server" ), Qgis::Critical );
            throw ex;
          }
          catch ( QgsException &ex )
          {
            QgsMessageLog::logMessage( QStringLiteral( "GetCoverage - QgsException: %1" ).arg( QString( ex.what() ) ).arg( request.url().toString() ), QStringLiteral( "Server" ), Qgis::Critical );
            throw ex;
          }
          catch ( std::runtime_error &ex )
          {
            QgsMessageLog::logMessage( QStringLiteral( "GetCoverage - RuntimeError: %1 | %2" ).arg( QString( ex.what() ) ).arg( request.url().toString() ), QStringLiteral( "Server" ), Qgis::Critical );
            throw ex;
          }
          catch ( std::exception &ex )
          {
            QgsMessageLog::logMessage( QStringLiteral( "GetCoverage - Exception: %1 | %2" ).arg( QString( ex.what() ) ).arg( request.url().toString() ), QStringLiteral( "Server" ), Qgis::Critical );
            throw ex;
          }
          catch ( ... )
          {
            QgsMessageLog::logMessage( QStringLiteral( "GetCoverage - Unknown exception: %1" ).arg( request.url().toString() ), QStringLiteral( "Server" ), Qgis::Critical );
            throw;
          }
        }
        else
        {
          // Operation not supported
          throw QgsServiceException( QStringLiteral( "OperationNotSupported" ),
                                     QStringLiteral( "Request %1 is not supported" ).arg( req ), 501 );
        }
      }

    private:
      QgsServerInterface *mServerIface = nullptr;
  };


} // namespace QgsWcs

/**
 * \ingroup server
 * \class QgsWcsModule
 * \brief Service module specialized for WCS
 * \since QGIS 3.0
 */
class QgsWcsModule: public QgsServiceModule
{
  public:
    void registerSelf( QgsServiceRegistry &registry, QgsServerInterface *serverIface ) override
    {
      QgsDebugMsgLevel( QStringLiteral( "WCSModule::registerSelf called" ), 2 );
      registry.registerService( new  QgsWcs::Service( serverIface ) );
    }
};


// Entry points
QGISEXTERN QgsServiceModule *QGS_ServiceModule_Init()
{
  static QgsWcsModule module;
  return &module;
}
QGISEXTERN void QGS_ServiceModule_Exit( QgsServiceModule * )
{
  // Nothing to do
}
