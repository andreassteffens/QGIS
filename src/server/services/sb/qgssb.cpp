/***************************************************************************
						   qgssb.cpp

  sb service implementation
  -----------------------------
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

#include "sbconstants.h"
#include "qgsmodule.h"
#include "qgssbserviceexception.h"
#include "qgssbgetserverinfo.h"
#include "qgssbconvertproject.h"
#include "qgssbgetlayermetadata.h"
#include "qgssbgetencryptedpath.h"
#include "qgssbpurgecache.h"
#include "qgssbsetunloadprojects.h"
#include "qgssbsetpreloadprojects.h"
#include "qgssbgetserverprocessid.h"

#define QSTR_COMPARE( str, lit )\
  (str.compare( QStringLiteral( lit ), Qt::CaseInsensitive ) == 0)

namespace QgsSb
{
	class sbService : public QgsService
	{
		private:
			QString mVersion;
			QgsServerInterface *mServerIface = nullptr;

		public:
			QString name()    const override { return "SB"; }
			QString version() const override { return mVersion; }

			sbService(const QString &version, QgsServerInterface *serverIface)
				: mVersion(version)
				, mServerIface(serverIface)
			{}

			void executeRequest(const QgsServerRequest &request, QgsServerResponse &response,
				const QgsProject *project) override
			{
				QgsServerRequest::Parameters params = request.parameters();
				QString versionString = params.value("VERSION");

				// Set the default version
				const bool valid = versionString.compare("1.0.0") == 0;
				if (versionString.isEmpty() || !valid)
				{
					versionString = mVersion;
				}

				// Get the request
				QString req = params.value(QStringLiteral("REQUEST"));
				if (req.isEmpty())
				{
					throw QgsServiceException(QStringLiteral("OperationNotSupported"),
						QStringLiteral("Please check the value of the REQUEST parameter"));
				}

				if ( QSTR_COMPARE( req, "GetServerInfo" ) )
				{
					QString key = params.value(QStringLiteral("SBKEY"));
					if(key.isEmpty())
					  throw QgsServerException(QStringLiteral("Request is missing required parameter 'SBKEY'"));

					if (!QSTR_COMPARE(key, SB_SERVICE_KEY))
					  throw QgsServerException(QStringLiteral("Invalid service key"));

					try
					{
						writeGetServerInfo(mServerIface, project, versionString, request, response);
					}
					catch (QgsServerException &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("GetServerInfo - QgsServerException: %1").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (QgsException &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("GetServerInfo - QgsException: %1").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (std::runtime_error &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("GetServerInfo - RuntimeError: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (std::exception &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("GetServerInfo - Exception: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (...)
					{
						QgsMessageLog::logMessage(QStringLiteral("GetServerInfo - Unknown exception: %1").arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
					}
				}
				else if ( QSTR_COMPARE( req, "ConvertProject" ) )
				{
				  if(!project)
					throw QgsServerException(QStringLiteral("Project file error"));

				  QString outputFilename = params.value( QStringLiteral( "OUTPUT_FILENAME" ) );
				  if(outputFilename.isEmpty())
					throw QgsServerException(QStringLiteral("Request is missing required parameter 'OUTPUT_FILENAME'"));

				  QString key = params.value(QStringLiteral("SBKEY"));
				  if (key.isEmpty())
					  throw QgsServerException(QStringLiteral("Request is missing required parameter 'SBKEY'"));

				  if (!QSTR_COMPARE(key, SB_SERVICE_KEY))
					  throw QgsServerException(QStringLiteral("Invalid service key"));

				  try
				  {
					  writeConvertProject(mServerIface, project, versionString, request, response, outputFilename);
				  }
				  catch (QgsServerException &ex)
				  {
					  QgsMessageLog::logMessage(QStringLiteral("ConvertProject - QgsServerException: %1").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
					  throw ex;
				  }
				  catch (QgsException &ex)
				  {
					  QgsMessageLog::logMessage(QStringLiteral("ConvertProject - QgsException: %1").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
					  throw ex;
				  }
				  catch (std::runtime_error &ex)
				  {
					  QgsMessageLog::logMessage(QStringLiteral("ConvertProject - RuntimeError: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
					  throw ex;
				  }
				  catch (std::exception &ex)
				  {
					  QgsMessageLog::logMessage(QStringLiteral("ConvertProject - Exception: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
					  throw ex;
				  }
				  catch (...)
				  {
					  QgsMessageLog::logMessage(QStringLiteral("ConvertProject - Unknown exception: %1").arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
				  }
				}
				else if (QSTR_COMPARE(req, "GetLayerMetadata"))
				{
					if (!project)
						throw QgsServerException(QStringLiteral("Project file error"));

					QString layerName = params.value(QStringLiteral("LAYER"));
					if (layerName.isEmpty())
						throw QgsServerException(QStringLiteral("Request is missing required parameter 'LAYER'"));

					try
					{
						writeGetLayerMetadata(mServerIface, project, versionString, request, response, layerName);
					}
					catch (QgsServerException &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("GetLayerMetadata - QgsServerException: %1").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (QgsException &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("GetLayerMetadata - QgsException: %1").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (std::runtime_error &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("GetLayerMetadata - RuntimeError: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (std::exception &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("GetLayerMetadata - Exception: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (...)
					{
						QgsMessageLog::logMessage(QStringLiteral("GetLayerMetadata - Unknown exception: %1").arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
					}
				}
				else if (QSTR_COMPARE(req, "PurgeCache"))
				{
					if (!project)
						throw QgsServerException(QStringLiteral("Project file error"));

					try
					{
						writePurgeCache(mServerIface, project, versionString, request, response);
					}
					catch (QgsServerException &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("PurgeCache - QgsServerException: %1").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (QgsException &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("PurgeCache - QgsException: %1").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (std::runtime_error &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("PurgeCache - RuntimeError: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (std::exception &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("PurgeCache - Exception: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (...)
					{
						QgsMessageLog::logMessage(QStringLiteral("PurgeCache - Unknown exception: %1").arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
					}
				}
				else if (QSTR_COMPARE(req, "GetEncryptedPath"))
				{
					QString path = params.value(QStringLiteral("PATH"));
					if (path.isEmpty())
						throw QgsServerException(QStringLiteral("Request is missing required parameter 'PATH'"));

					QString key = params.value(QStringLiteral("SBKEY"));
					if (key.isEmpty())
						throw QgsServerException(QStringLiteral("Request is missing required parameter 'SBKEY'"));
					
					if (!QSTR_COMPARE(key, SB_SERVICE_KEY))
						throw QgsServerException(QStringLiteral("Invalid service key"));
					
					try
					{
						writeGetEncryptedPath(mServerIface, project, versionString, request, response, path);
					}
					catch (QgsServerException &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("GetEncryptedPath - QgsServerException: %1").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (QgsException &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("GetEncryptedPath - QgsException: %1").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (std::runtime_error &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("GetEncryptedPath - RuntimeError: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (std::exception &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("GetEncryptedPath - Exception: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (...)
					{
						QgsMessageLog::logMessage(QStringLiteral("GetEncryptedPath - Unknown exception: %1").arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
					}
				}
				else if (QSTR_COMPARE(req, "SetUnloadProjects"))
				{
					QString projects = params.value(QStringLiteral("PROJECTS"));
					
					QString key = params.value(QStringLiteral("SBKEY"));
					if (key.isEmpty())
						throw QgsServerException(QStringLiteral("Request is missing required parameter 'SBKEY'"));

					if (!QSTR_COMPARE(key, SB_SERVICE_KEY))
						throw QgsServerException(QStringLiteral("Invalid service key"));

					try
					{
						writeSetUnloadProjects(mServerIface, versionString, request, response, projects);
					}
					catch (QgsServerException &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("SetUnloadProjects - QgsServerException: %1").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (QgsException &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("SetUnloadProjects - QgsException: %1").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (std::runtime_error &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("SetUnloadProjects - RuntimeError: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (std::exception &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("SetUnloadProjects - Exception: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (...)
					{
						QgsMessageLog::logMessage(QStringLiteral("SetUnloadProjects - Unknown exception: %1").arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
					}
				}
				else if (QSTR_COMPARE(req, "SetPreloadProjects"))
				{
					QString projects = params.value(QStringLiteral("PROJECTS"));
					if (projects.isEmpty())
						throw QgsServerException(QStringLiteral("Request is missing required parameter 'PROJECTS'"));
					
					QString key = params.value(QStringLiteral("SBKEY"));
					if (key.isEmpty())
						throw QgsServerException(QStringLiteral("Request is missing required parameter 'SBKEY'"));

					if (!QSTR_COMPARE(key, SB_SERVICE_KEY))
						throw QgsServerException(QStringLiteral("Invalid service key"));

					try
					{
						writeSetPreloadProjects(mServerIface, project, versionString, request, response, projects);
					}
					catch (QgsServerException &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("SetPreloadProjects - QgsServerException: %1").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (QgsException &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("SetPreloadProjects - QgsException: %1").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (std::runtime_error &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("SetPreloadProjects - RuntimeError: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (std::exception &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("SetPreloadProjects - Exception: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (...)
					{
						QgsMessageLog::logMessage(QStringLiteral("SetPreloadProjects - Unknown exception: %1").arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
					}
				}
				else if (QSTR_COMPARE(req, "GetServerProcessId"))
				{
					QString key = params.value(QStringLiteral("SBKEY"));
					if (key.isEmpty())
						throw QgsServerException(QStringLiteral("Request is missing required parameter 'SBKEY'"));

					if (!QSTR_COMPARE(key, SB_SERVICE_KEY))
						throw QgsServerException(QStringLiteral("Invalid service key"));

					try
					{
						writeGetServerProcessId(mServerIface, versionString, request, response);
					}
					catch (QgsServerException &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("GetServerProcessId - QgsServerException: %1").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (QgsException &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("GetServerProcessId - QgsException: %1").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (std::runtime_error &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("GetServerProcessId - RuntimeError: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (std::exception &ex)
					{
						QgsMessageLog::logMessage(QStringLiteral("GetServerProcessId - Exception: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						throw ex;
					}
					catch (...)
					{
						QgsMessageLog::logMessage(QStringLiteral("GetServerProcessId - Unknown exception: %1").arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
					}
				}
				else
				{
				  // Operation not supported
				  throw QgsServiceException( QStringLiteral( "OperationNotSupported" ), QString( "Request %1 is not supported" ).arg( req ) );
				}
			}
	};
}

// Module
class QgsSbModule: public QgsServiceModule
{
  public:
	void registerSelf( QgsServiceRegistry &registry, QgsServerInterface *serverIface ) override
	{
	  Q_UNUSED( serverIface );
	  QgsDebugMsg( "sbModule::registerSelf called" );
	  registry.registerService( new QgsSb::sbService("1.0.0", serverIface) );
	}
};

// Entry points
QGISEXTERN QgsServiceModule *QGS_ServiceModule_Init()
{
  static QgsSbModule sModule;
  return &sModule;
}
QGISEXTERN void QGS_ServiceModule_Exit( QgsServiceModule * )
{
  // Nothing to do
}
