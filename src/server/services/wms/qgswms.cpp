/***************************************************************************
                              qgswms.cpp
                              -------------------------
  begin                : December 20 , 2016
  copyright            : (C) 2007 by Marco Hugentobler  ( parts from qgswmshandler)
                         (C) 2014 by Alessandro Pasotti ( parts from qgswmshandler)
                         (C) 2016 by David Marteau
  email                : marco dot hugentobler at karto dot baug dot ethz dot ch
                         a dot pasotti at itopen dot it
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

#include "sbconstants.h"
#include "qgsmodule.h"
#include "qgsdxfwriter.h"
#include "qgswmsserviceexception.h"
#include "qgswmsgetcapabilities.h"
#include "qgswmsgetmap.h"
#include "qgswmsgetstyles.h"
#include "qgswmsgetcontext.h"
#include "qgswmsgetschemaextension.h"
#include "qgswmsgetprint.h"
#include "qgswmsgetfeatureinfo.h"
#include "qgswmsdescribelayer.h"
#include "qgswmsgetlegendgraphics.h"
#include "qgswmsparameters.h"
#include "qgswmsrequest.h"
#include "qgswmsutils.h"

#include "libProfiler.h"

#define QSTR_COMPARE( str, lit )\
  (str.compare( QLatin1String( lit ), Qt::CaseInsensitive ) == 0)

namespace QgsWms
{

  /**
   * \ingroup server
   * \class QgsWms::Service
   * \brief OGC web service specialized for WMS
   * \since QGIS 3.0
   */
  class Service: public QgsService
  {
    public:

      /**
       * Constructor for WMS service.
       * \param version Version of the WMS service.
       * \param serverIface Interface for plugins.
       */
      Service( const QString &version, QgsServerInterface *serverIface )
        : mVersion( version )
        , mServerIface( serverIface )
      {}

      QString name()    const override { return QStringLiteral( "WMS" ); }
      QString version() const override { return mVersion; }

      void executeRequest( const QgsServerRequest &request, QgsServerResponse &response,
                           const QgsProject *project ) override
      {
        // Get the request
        const QgsWmsRequest wmsRequest( request );
        const QString req = wmsRequest.wmsParameters().request();

        if ( req.isEmpty() )
        {
          throw QgsServiceException( QgsServiceException::OGC_OperationNotSupported,
                                     QStringLiteral( "Please add or check the value of the REQUEST parameter" ), 501 );
        }

        if ( QSTR_COMPARE( req, "GetCapabilities" ) )
        {
          try
          {
            writeGetCapabilities( mServerIface, project, wmsRequest, response );
          }
          catch (QgsServerException &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetCapabilities - QgsServerException: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
              throw ex;
          }
          catch (QgsException &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetCapabilities - QgsException: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (std::runtime_error &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetCapabilities - RuntimeError: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (std::exception &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetCapabilities - Exception: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
              throw ex;
          }
          catch (...)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetCapabilities - Unknown exception: %1").arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
          }
        }
        else if ( QSTR_COMPARE( req, "GetProjectSettings" ) )
        {
          try
          {
            writeGetCapabilities( mServerIface, project, request, response, true );
          }
          catch (QgsServerException &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetProjectSettings - QgsServerException: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (QgsException &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetProjectSettings - QgsException: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (std::runtime_error &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetProjectSettings - RuntimeError: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (std::exception &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetProjectSettings - Exception: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (...)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetProjectSettings - Unknown exception: %1").arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
          }
          
        }
        else if ( QSTR_COMPARE( req, "GetMap" ) )
        {
          try
          {
            if QSTR_COMPARE( wmsRequest.wmsParameters().formatAsString(), "application/dxf" )
            {
              writeAsDxf( mServerIface, project, request, response );
            }
            else
            {
              writeGetMap( mServerIface, project, request, response );
            }
          }
          catch (QgsServerException &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetMap - QgsServerException: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (QgsException &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetMap - QgsException: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (std::runtime_error &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetMap - RuntimeError: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (std::exception &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetMap - Exception: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (...)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetMap - Unknown exception: %1").arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
          }
        }
        else if ( QSTR_COMPARE( req, "GetFeatureInfo" ) )
        {
          try
          {
            writeGetFeatureInfo( mServerIface, project, request, response );
          }
          catch (QgsServerException &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetFeatureInfo - QgsServerException: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (QgsException &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetFeatureInfo - QgsException: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (std::runtime_error &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetFeatureInfo - RuntimeError: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (std::exception &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetFeatureInfo - Exception: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (...)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetFeatureInfo - Unknown exception: %1").arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
          }
        }
        else if ( QSTR_COMPARE( req, "GetContext" ) )
        {
          try
          {
            writeGetContext( mServerIface, project, request, response );
          }
          catch (QgsServerException &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetContext - QgsServerException: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (QgsException &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetContext - QgsException: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (std::runtime_error &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetContext - RuntimeError: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (std::exception &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetContext - Exception: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (...)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetContext - Unknown exception: %1").arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
          }
        }
        else if ( QSTR_COMPARE( req, "GetSchemaExtension" ) )
        {
          try
          {
              writeGetSchemaExtension( response );
          }
          catch (QgsServerException &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetSchemaExtension - QgsServerException: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (QgsException &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetSchemaExtension - QgsException: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (std::runtime_error &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetSchemaExtension - RuntimeError: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (std::exception &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetSchemaExtension - Exception: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (...)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetSchemaExtension - Unknown exception: %1").arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
          }
        }
        else if ( QSTR_COMPARE( req, "GetStyle" ) || QSTR_COMPARE( req, "GetStyles" ) )
        {
          try
          {
            writeGetStyles( mServerIface, project, request, response );
          }
          catch (QgsServerException &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetStyle - QgsServerException: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (QgsException &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetStyle - QgsException: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (std::runtime_error &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetStyle - RuntimeError: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (std::exception &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetStyle - Exception: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (...)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetStyle - Unknown exception: %1").arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
          }
        }
        else if ( QSTR_COMPARE( req, "DescribeLayer" ) )
        {
          try
          {
              writeDescribeLayer( mServerIface, project, request, response );
          }
          catch (QgsServerException &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("DescribeLayer - QgsServerException: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (QgsException &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("DescribeLayer - QgsException: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (std::runtime_error &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("DescribeLayer - RuntimeError: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (std::exception &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("DescribeLayer - Exception: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (...)
          {
            QgsMessageLog::logMessage(QStringLiteral("DescribeLayer - Unknown exception: %1").arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
          }
        }
        else if ( QSTR_COMPARE( req, "GetLegendGraphic" ) || QSTR_COMPARE( req, "GetLegendGraphics" ) )
        {
          try
          {
            writeGetLegendGraphics( mServerIface, project, request, response );
          }
          catch (QgsServerException &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetLegendGraphic - QgsServerException: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (QgsException &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetLegendGraphic - QgsException: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (std::runtime_error &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetLegendGraphic - RuntimeError: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (std::exception &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetLegendGraphic - Exception: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (...)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetLegendGraphic - Unknown exception: %1").arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
          }
        }
        else if ( QSTR_COMPARE( req, "GetPrint" ) )
        {
          if ( mServerIface->serverSettings() && mServerIface->serverSettings()->getPrintDisabled() )
          {
            // GetPrint has been disabled
            QgsDebugMsg( QStringLiteral( "WMS GetPrint request called, but it has been disabled." ) );
            throw QgsServiceException( QgsServiceException::OGC_OperationNotSupported,
                                       QStringLiteral( "Request %1 is not supported" ).arg( req ), 501 );
          }

          try
          {
            writeGetPrint( mServerIface, project, request, response );
          }
          catch (QgsServerException &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetPrint - QgsServerException: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (QgsException &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetPrint - QgsException: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (std::runtime_error &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetPrint - RuntimeError: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (std::exception &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetPrint - Exception: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (...)
          {
            QgsMessageLog::logMessage(QStringLiteral("GetPrint - Unknown exception: %1").arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
          }
        }
        else if (QSTR_COMPARE(req, "LogProfiler"))
        {
          try
          {
            LogProfiler();
          }
          catch (QgsServerException &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("LogProfiler - QgsServerException: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (QgsException &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("LogProfiler - QgsException: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (std::runtime_error &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("LogProfiler - RuntimeError: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (std::exception &ex)
          {
            QgsMessageLog::logMessage(QStringLiteral("LogProfiler - Exception: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
            throw ex;
          }
          catch (...)
          {
            QgsMessageLog::logMessage(QStringLiteral("LogProfiler - Unknown exception: %1").arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
          }
        }
        else
        {
          // Operation not supported
          throw QgsServiceException( QgsServiceException::OGC_OperationNotSupported,
                                     QStringLiteral( "Request %1 is not supported" ).arg( req ), 501 );
        }
      }

    private:
      QString mVersion;
      QgsServerInterface *mServerIface = nullptr;
  };
} // namespace QgsWms

/**
 * \ingroup server
 * \class QgsWmsModule
 * \brief Module specialized for WMS service
 * \since QGIS 3.0
 */
class QgsWmsModule: public QgsServiceModule
{
  public:
    void registerSelf( QgsServiceRegistry &registry, QgsServerInterface *serverIface ) override
    {
      QgsDebugMsg( QStringLiteral( "WMSModule::registerSelf called" ) );
      registry.registerService( new  QgsWms::Service( QgsWms::implementationVersion(), serverIface ) ); // 1.3.0 default version
      registry.registerService( new  QgsWms::Service( QStringLiteral( "1.1.1" ), serverIface ) ); // second supported version
    }
};


// Entry points
QGISEXTERN QgsServiceModule *QGS_ServiceModule_Init()
{
  static QgsWmsModule sModule;
  
  PROFILER_ENABLE;
  
  return &sModule;
}
QGISEXTERN void QGS_ServiceModule_Exit( QgsServiceModule * )
{
  LogProfiler();

  PROFILER_DISABLE;
}
