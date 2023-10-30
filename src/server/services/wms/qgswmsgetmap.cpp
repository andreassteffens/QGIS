/***************************************************************************
                              qgswmsgetmap.cpp
                              -------------------------
  begin                : December 20 , 2016
  copyright            : (C) 2007 by Marco Hugentobler  (original code)
                         (C) 2014 by Alessandro Pasotti (original code)
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
#include "qgswmsutils.h"
#include "qgswmsgetmap.h"
#include "qgswmsrenderer.h"
#include "qgswmsserviceexception.h"

#include "libProfiler.h"

#include <QImage>

namespace QgsWms
{

  void writeGetMap( QgsServerInterface *serverIface, const QgsProject *project,
                    const QgsWmsRequest &request,
                    QgsServerResponse &response )
  {
    if ( request.serverParameters().version().isEmpty() )
    {
      throw QgsServiceException( QgsServiceException::OGC_OperationNotSupported,
                                 QStringLiteral( "Please add the value of the VERSION parameter" ), 501 );
    }

    const QString strFormat = request.parameters().value( QStringLiteral( "FORMAT" ), QStringLiteral( "PNG" ) );
    QString strContentType;
    QString strSaveFormat;
    QString strCacheMaxAge;
    bool bTiled = false;

    QString strTiled = request.parameter( "TILED" );
    if ( !strTiled.isEmpty() )
      bTiled = strTiled.compare( "TRUE", Qt::CaseInsensitive ) == 0;

    QgsAccessControl *accessControl = serverIface->accessControls();
    QgsServerCacheManager *cacheManager = serverIface->cacheManager();
    if ( bTiled && cacheManager )
    {
      QStringList qlistMetadata = project->metadata().keywords( "sb:USE_WMS_TILED_CACHE" );
      if ( qlistMetadata.count() > 0 )
        bTiled = qlistMetadata[0].compare( "TRUE", Qt::CaseInsensitive ) == 0;
      else
        bTiled = false;

      if ( bTiled )
      {
        qlistMetadata = project->metadata().keywords( "sb:CACHE_MAX_AGE" );
        if ( qlistMetadata.count() > 0 )
          strCacheMaxAge = qlistMetadata[0];

        QString strWidth = request.parameter( "WIDTH" );
        int iWidth = strWidth.toInt();

        QString strHeight = request.parameter( "HEIGHT" );
        int iHeight = strHeight.toInt();

        std::unique_ptr<QImage> image;

        ImageOutputFormat fmt = parseImageFormat( strFormat );
        switch ( fmt )
        {
          case ImageOutputFormat::JPEG:
            strContentType = QStringLiteral( "image/jpeg" );
            strSaveFormat = QStringLiteral( "JPEG" );
            image = std::make_unique<QImage>( iWidth, iHeight, QImage::Format_RGB32 );
            break;
          case ImageOutputFormat::PNG:
            strContentType = QStringLiteral( "image/png" );
            strSaveFormat = QStringLiteral( "PNG" );
            image = std::make_unique<QImage>( iWidth, iHeight, QImage::Format_ARGB32_Premultiplied );
            break;
          case ImageOutputFormat::WEBP:
            strContentType = QStringLiteral( "image/webp" );
            strSaveFormat = QStringLiteral( "WEBP" );
            image = std::make_unique<QImage>( iWidth, iHeight, QImage::Format_ARGB32_Premultiplied );
            break;
          default:
            bTiled = false;
            break;
        }

        if ( bTiled )
        {
          QByteArray content = cacheManager->getCachedImage( project, request, accessControl );
          if ( !content.isEmpty() )
          {
            if ( image->loadFromData( content ) )
            {
              response.setHeader( QStringLiteral( "Content-Type" ), strContentType );
              image->save( response.io(), qPrintable( strSaveFormat ) );

              if ( !strCacheMaxAge.isEmpty() )
                response.setHeader( QStringLiteral( "Cache-Control" ), QStringLiteral( "public, max-age=%1" ).arg( strCacheMaxAge ) );

              response.setHeader( QStringLiteral( "X-QGIS-FROM-CACHE" ), QStringLiteral( "true" ) );

              QString cacheId = cacheManager->sbGetProjectCacheId( project );
              if ( !cacheId.isEmpty() )
                response.setHeader( QStringLiteral( "X-QGIS-CACHE-ID" ), cacheId );

              return;
            }
          }
        }
      }
    }


    // prepare render context
    QgsWmsRenderContext context( project, serverIface );
    context.setFlag( QgsWmsRenderContext::UpdateExtent );
    context.setFlag( QgsWmsRenderContext::UseOpacity );
    context.setFlag( QgsWmsRenderContext::UseFilter );
    context.setFlag( QgsWmsRenderContext::UseSelection );
    context.setFlag( QgsWmsRenderContext::AddHighlightLayers );
    context.setFlag( QgsWmsRenderContext::AddExternalLayers );
    context.setFlag( QgsWmsRenderContext::SetAccessControl );
    context.setFlag( QgsWmsRenderContext::UseTileBuffer );
    context.setParameters( request.wmsParameters() );

    // rendering

    PROFILER_START( writeGetMap_setup );

    QgsRenderer renderer( context );
    PROFILER_END();

    PROFILER_START( writeGetMap_getMap );
    std::unique_ptr<QImage> result( renderer.getMap() );
    PROFILER_END();

    if ( result )
    {
      PROFILER_START( writeGetMap_writeImage );
      const QString format = request.parameters().value( QStringLiteral( "FORMAT" ), QStringLiteral( "PNG" ) );
      writeImage( response, *result, format, context.imageQuality() );
      PROFILER_END();

      if ( bTiled && cacheManager )
      {
        QByteArray content = response.data();
        if ( !content.isEmpty() )
          cacheManager->setCachedImage( &content, project, request, accessControl );

        if ( !strCacheMaxAge.isEmpty() )
          response.setHeader( QStringLiteral( "Cache-Control" ), QStringLiteral( "public, max-age=%1" ).arg( strCacheMaxAge ) );

        response.setHeader( QStringLiteral( "X-QGIS-FROM-CACHE" ), QStringLiteral( "false" ) );

        QString cacheId = cacheManager->sbGetProjectCacheId( project );
        if ( !cacheId.isEmpty() )
          response.setHeader( QStringLiteral( "X-QGIS-CACHE-ID" ), cacheId );
      }
    }
    else
    {
      throw QgsException( QStringLiteral( "Failed to compute GetMap image" ) );
    }
  }
} // namespace QgsWms
