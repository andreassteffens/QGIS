/***************************************************************************
                              qgssbpurgecache.cpp
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

#include "qgssbpurgecache.h"
#include "qgsserverprojectutils.h"
#include "qgsconfigcache.h"
#include "qgssettings.h"

#include "qgsexception.h"
#include "qgsexpressionnodeimpl.h"

#include "qgsexception.h"

namespace QgsSb
{
  void writePurgeCache( QgsServerInterface *serverIface, const QgsProject *project,
                        const QString &version, const QgsServerRequest &request,
                        QgsServerResponse &response )
  {
#ifdef HAVE_SERVER_PYTHON_PLUGINS
	  QgsServerCacheManager *cacheManager = serverIface->cacheManager();
	  if (cacheManager)
	  {
		  QgsServerRequest::Parameters params = request.parameters();
		  QString strDocumentsParam = params.value("documents");
		  QString strImagesParam = params.value("images");

		  if (!strDocumentsParam.isEmpty() || !strImagesParam.isEmpty())
		  {
			  bool bDeleteDocuments = false;
			  bool bDeleteImages = false;

			  if (!strDocumentsParam.isEmpty())
			  {
				  if (strDocumentsParam.compare("true", Qt::CaseInsensitive) == 0)
					  bDeleteDocuments = true;
			  }

			  if (!strImagesParam.isEmpty())
			  {
				  if (strImagesParam.compare("true", Qt::CaseInsensitive) == 0)
					  bDeleteImages = true;
			  }

			  if(bDeleteDocuments)
				  cacheManager->deleteCachedDocuments(project);
			  if(bDeleteImages)
				  cacheManager->deleteCachedImages(project);
		  }
		  else
		  {
			  cacheManager->deleteCachedDocuments(project);
			  cacheManager->deleteCachedImages(project);
		  }
	  }
#endif

	response.setStatusCode(200);
    response.setHeader( QStringLiteral( "Content-Type" ), QStringLiteral( "text/plain; charset=utf-8" ) );
    response.write( "ok" );
  }

} // namespace QgsWms
