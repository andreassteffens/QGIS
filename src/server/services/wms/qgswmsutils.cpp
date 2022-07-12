/***************************************************************************
                              qgswmsutils.cpp
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
 *   (at your option) any later version.                                  *
 *                                                                         *
 ***************************************************************************/

#include <QRegularExpression>

#include "qgsmodule.h"
#include "qgswmsutils.h"
#include "qgsmediancut.h"
#include "qgsserverprojectutils.h"
#include "qgswmsserviceexception.h"
#include "qgsproject.h"

namespace QgsWms
{
  QUrl serviceUrl( const QgsServerRequest &request, const QgsProject *project, const QgsServerSettings &settings )
  {
    QUrl href;
    href.setUrl( QgsServerProjectUtils::wmsServiceUrl( project ? *project : *QgsProject::instance(), request, settings ) );

    // Build default url
    if ( href.isEmpty() )
    {
      static const QSet<QString> sFilter
      {
        QStringLiteral( "REQUEST" ),
        QStringLiteral( "VERSION" ),
        QStringLiteral( "SERVICE" ),
        QStringLiteral( "LAYERS" ),
        QStringLiteral( "STYLES" ),
        QStringLiteral( "SLD_VERSION" ),
        QStringLiteral( "_DC" )
      };

      href = request.originalUrl();
      QUrlQuery q( href );

      const QList<QPair<QString, QString> > queryItems = q.queryItems();
      for ( const QPair<QString, QString> &param : queryItems )
      {
        if ( sFilter.contains( param.first.toUpper() ) )
          q.removeAllQueryItems( param.first );
      }

      href.setQuery( q );
    }

    return  href;
  }

  QString sbGetLayerMetadataUrl(const QgsServerRequest &request, const QgsProject *project, const QString &layer)
  {
	  static QSet<QString> sFilter
	  {
		  QStringLiteral("REQUEST"),
		  QStringLiteral("VERSION"),
		  QStringLiteral("SERVICE"),
		  QStringLiteral("LAYERS"),
		  QStringLiteral("STYLES"),
		  QStringLiteral("SLD_VERSION"),
		  QStringLiteral("_DC")
	  };

	  QUrl href = request.url();
	  QUrlQuery q(href);

	  for (auto param : q.queryItems())
	  {
		  if (sFilter.contains(param.first.toUpper()))
			  q.removeAllQueryItems(param.first);
	  }

	  q.addQueryItem("SERVICE", "sb");
	  q.addQueryItem("REQUEST", "GetLayerMetadata");
	  q.addQueryItem("LAYER", layer);

	  href.setQuery(q);

	  return href.toString();
  }

  void sbProfilerOutput(const char *pszOut)
  {
	  QgsMessageLog::logMessage(pszOut, "Server", Qgis::Warning);
  }

  ImageOutputFormat parseImageFormat( const QString &format )
  {
    if ( format.compare( QLatin1String( "png" ), Qt::CaseInsensitive ) == 0 ||
         format.compare( QLatin1String( "image/png" ), Qt::CaseInsensitive ) == 0 )
    {
      return PNG;
    }
    else if ( format.compare( QLatin1String( "jpg " ), Qt::CaseInsensitive ) == 0  ||
              format.compare( QLatin1String( "image/jpeg" ), Qt::CaseInsensitive ) == 0 )
    {
      return JPEG;
    }
    else if ( format.compare( QLatin1String( "webp" ), Qt::CaseInsensitive ) == 0  ||
              format.compare( QLatin1String( "image/webp" ), Qt::CaseInsensitive ) == 0 )
    {
      return WEBP;
    }
    else
    {
      // lookup for png with mode
      const QRegularExpression modeExpr = QRegularExpression( QStringLiteral( "image/png\\s*;\\s*mode=([^;]+)" ),
                                          QRegularExpression::CaseInsensitiveOption );

      const QRegularExpressionMatch match = modeExpr.match( format );
      const QString mode = match.captured( 1 );
      if ( mode.compare( QLatin1String( "16bit" ), Qt::CaseInsensitive ) == 0 )
        return PNG16;
      if ( mode.compare( QLatin1String( "8bit" ), Qt::CaseInsensitive ) == 0 )
        return PNG8;
      if ( mode.compare( QLatin1String( "1bit" ), Qt::CaseInsensitive ) == 0 )
        return PNG1;
    }

    return UNKN;
  }

  // Write image response
  void writeImage( QgsServerResponse &response, QImage &img, const QString &formatStr,
                   int imageQuality )
  {
    const ImageOutputFormat outputFormat = parseImageFormat( formatStr );
    QImage  result;
    QString saveFormat;
    QString contentType;
    switch ( outputFormat )
    {
      case PNG:
        result = img;
        contentType = "image/png";
        saveFormat = "PNG";
        break;
      case PNG8:
      {
        QVector<QRgb> colorTable;

        // Rendering is made with the format QImage::Format_ARGB32_Premultiplied
        // So we need to convert it in QImage::Format_ARGB32 in order to properly build
        // the color table.
        const QImage img256 = img.convertToFormat( QImage::Format_ARGB32 );
        medianCut( colorTable, 256, img256 );
        result = img256.convertToFormat( QImage::Format_Indexed8, colorTable,
                                         Qt::ColorOnly | Qt::ThresholdDither |
                                         Qt::ThresholdAlphaDither | Qt::NoOpaqueDetection );
      }
      contentType = "image/png";
      saveFormat = "PNG";
      break;
      case PNG16:
        result = img.convertToFormat( QImage::Format_ARGB4444_Premultiplied );
        contentType = "image/png";
        saveFormat = "PNG";
        break;
      case PNG1:
        result = img.convertToFormat( QImage::Format_Mono,
                                      Qt::MonoOnly | Qt::ThresholdDither |
                                      Qt::ThresholdAlphaDither | Qt::NoOpaqueDetection );
        contentType = "image/png";
        saveFormat = "PNG";
        break;
      case JPEG:
        result = img;
        contentType = "image/jpeg";
        saveFormat = "JPEG";
        break;
      case WEBP:
        result = img;
        contentType = QStringLiteral( "image/webp" );
        saveFormat = QStringLiteral( "WEBP" );
        break;
      default:
        QgsMessageLog::logMessage( QString( "Unsupported format string %1" ).arg( formatStr ) );
        saveFormat = UNKN;
        break;
    }

    // Preserve DPI, some conversions, in particular the one for 8bit will drop this information
    result.setDotsPerMeterX( img.dotsPerMeterX() );
    result.setDotsPerMeterY( img.dotsPerMeterY() );

    if ( outputFormat != UNKN )
    {
      response.setHeader( "Content-Type", contentType );
      if ( saveFormat == QLatin1String( "JPEG" ) || saveFormat == QLatin1String( "WEBP" ) )
      {
        result.save( response.io(), qPrintable( saveFormat ), imageQuality );
      }
      else
      {
        result.save( response.io(), qPrintable( saveFormat ) );
      }
    }
    else
    {
      QgsWmsParameter parameter( QgsWmsParameter::FORMAT );
      parameter.mValue = formatStr;
      throw QgsBadRequestException( QgsServiceException::OGC_InvalidFormat,
                                    parameter );
    }
  }
  QString extractUrlBase(const QString &url)
  {
	  if (url.indexOf('?') == -1)
		return url;
	  
	  QString qstrUrl = url.left(url.indexOf('?'));
	  return qstrUrl;
  }

  QUrlQuery buildUrlQuery(const QString &url)
  {
	  QUrlQuery ret;

	  if (url.indexOf('?') == -1)
		  return ret;

	  QString qstrUrl = url.left(url.indexOf('?') + 1);
	  //ret = QUrlQuery(qstrUrl);
	  
	  QString tmp = url.right(url.length() - url.indexOf('?') - 1);
	  QStringList paramlist = tmp.split('&');

	  for (int i = 0; i < paramlist.count(); i++)
	  {
			QStringList paramarg = paramlist[i].split('=');
			if(paramarg.count() == 2)
				ret.addQueryItem(paramarg[0], paramarg[1]);
	  }

	  return ret;
  }
  
} // namespace QgsWms
