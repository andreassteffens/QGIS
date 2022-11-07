/***************************************************************************
                              qgswmtsgettile.cpp
                            -------------------------
  begin                : July 23 , 2017
  copyright            : (C) 2018 by René-Luc D'Hont
  email                : rldhont at 3liz dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "qgswmtsutils.h"
#include "qgswmtsparameters.h"
#include "qgswmtsgettile.h"
#include "qgsproject.h"
#include "qgsserverprojectutils.h"
#include "qgsmessagelog.h"
#include "qgsserver.h"

#include <QImage>

namespace QgsWmts
{

  void writeGetTile( QgsServerInterface *serverIface, const QgsProject *project,
                     const QString &version, const QgsServerRequest &request,
                     QgsServerResponse &response )
  {
    Q_UNUSED( version )
    const QgsWmtsParameters params( QUrlQuery( request.url() ) );

    // WMS query
    const QUrlQuery query = translateWmtsParamToWmsQueryItem( QStringLiteral( "GetMap" ), params, project, serverIface );

    QString strCacheMaxAge;
    if (!params.layer().isEmpty())
    {
      QString strLayer = params.layer();
      if(!strLayer.isEmpty())
      {
        // Use layer ids
        bool useLayerIds = QgsServerProjectUtils::wmsUseLayerIds(*project);

        for (QgsMapLayer *layer : project->mapLayers(true))
        {
          QString name = layer->name();
          if (useLayerIds)
            name = layer->id();
          else if (!layer->shortName().isEmpty())
            name = layer->shortName();

          if (strLayer.compare(name, Qt::CaseInsensitive) != 0)
            continue;

          QList<QgsLayerMetadata::Constraint> qlistConstraints = layer->metadata().constraints();
          for (int iMeta = 0; iMeta < qlistConstraints.length(); iMeta++)
          {
            if ( qlistConstraints[iMeta].type.compare( "sb:CACHE_MAX_AGE" , Qt::CaseInsensitive ) == 0 )
            {
              strCacheMaxAge = qlistConstraints[iMeta].constraint;
              break;
            }
          }

          break;
        }
      }

      if (strCacheMaxAge.isEmpty())
      {
        QStringList qlistMetadata = project->metadata().keywords( "sb:CACHE_MAX_AGE" );
        if (qlistMetadata.count() > 0)
          strCacheMaxAge = qlistMetadata[0];
      }
    }

    // Get cached image
#ifdef HAVE_SERVER_PYTHON_PLUGINS
    QgsAccessControl *accessControl = serverIface->accessControls();
    QgsServerCacheManager *cacheManager = serverIface->cacheManager();
    if ( cacheManager )
    {
      const QgsWmtsParameters::Format f = params.format();
      QString contentType;
      QString saveFormat;
      std::unique_ptr<QImage> image;
      if ( f == QgsWmtsParameters::Format::JPG )
      {
        contentType = QStringLiteral( "image/jpeg" );
        saveFormat = QStringLiteral( "JPEG" );
        image = std::make_unique<QImage>( 256, 256, QImage::Format_RGB32 );
      }
      else
      {
        contentType = QStringLiteral( "image/png" );
        saveFormat = QStringLiteral( "PNG" );
        image = std::make_unique<QImage>( 256, 256, QImage::Format_ARGB32_Premultiplied );
      }

      const QByteArray content = cacheManager->getCachedImage( project, request, accessControl );
      if ( !content.isEmpty() && image->loadFromData( content ) )
      {
        response.setHeader( QStringLiteral( "Content-Type" ), contentType );
        image->save( response.io(), qPrintable( saveFormat ) );

        if (!strCacheMaxAge.isEmpty())
          response.setHeader( QStringLiteral( "Cache-Control" ), QStringLiteral( "public, max-age=%1" ).arg( strCacheMaxAge ) );

        response.setHeader( QStringLiteral( "X-QGIS-FROM-CACHE" ), QStringLiteral( "true" ) );

        return;
      }
    }
#endif

    const QgsServerParameters wmsParams( query );
    const QgsServerRequest wmsRequest( "?" + query.query( QUrl::FullyDecoded ) );

    QgsService *service = serverIface->serviceRegistry()->getService( wmsParams.service(), wmsParams.version() );
    service->executeRequest( wmsRequest, response, project );
#ifdef HAVE_SERVER_PYTHON_PLUGINS
    if ( cacheManager )
    {
      const QByteArray content = response.data();
      if ( !content.isEmpty() )
        cacheManager->setCachedImage( &content, project, request, accessControl );
    }
#endif

    if (!strCacheMaxAge.isEmpty())
      response.setHeader( QStringLiteral( "Cache-Control" ), QStringLiteral( "public, max-age=%1" ).arg( strCacheMaxAge ) );

    response.setHeader( QStringLiteral( "X-QGIS-FROM-CACHE" ), QStringLiteral( "false" ) );
  }

} // namespace QgsWmts




