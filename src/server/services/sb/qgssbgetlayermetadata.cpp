/***************************************************************************
                              qgssbgetlayermetadata.cpp
                              -------------------------
  begin                : 2018-10-11
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

#include "qgssbgetlayermetadata.h"
#include "qgsserverprojectutils.h"
#include "qgsconfigcache.h"
#include "qgsvectorlayer.h"

#include "qgsexception.h"
#include "qgsexpressionnodeimpl.h"

namespace QgsSb
{
  void writeGetLayerMetadata( QgsServerInterface *serverIface, const QgsProject *project,
                             const QString &version, const QgsServerRequest &request,
                             QgsServerResponse &response, const QString &layerName )
  {
	  Q_UNUSED(version);

	  QString qstrMetadata;

	  // Use layer ids
	  bool useLayerIds = QgsServerProjectUtils::wmsUseLayerIds(*project);

	  Q_FOREACH(QgsMapLayer *layer, project->mapLayers())
	  {
		  QString name = layer->name();
		  if (useLayerIds)
			  name = layer->id();
		  else if (!layer->shortName().isEmpty())
			  name = layer->shortName();

		  if (layerName != name)
			  continue;

		  qstrMetadata = layer->htmlMetadata();

		  break;
	  }

	  if(qstrMetadata.isEmpty()) 
		  throw QgsServerException(QStringLiteral("Layer not found"));

	  response.setHeader(QStringLiteral("Content-Type"), QStringLiteral("text/html; charset=utf-8"));
	  response.write(qstrMetadata);
  }
} // namespace QgsSb