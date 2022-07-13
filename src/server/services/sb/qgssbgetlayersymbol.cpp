/***************************************************************************
							  qgssbgetlayermetadata.cpp
							  -------------------------
  begin                : 2019-06-08
  copyright            : (C) 2019 by Andreas Steffens
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

#include "qgssbgetlayersymbol.h"
#include "qgsserverprojectutils.h"
#include "qgsconfigcache.h"
#include "qgsvectorlayer.h"
#include "qgsrenderer.h"
#include "qgssymbol.h"
#include "qgssymbollayer.h"
#include "qgsmarkersymbol.h"

#include "qgsexception.h"
#include "qgsexpressionnodeimpl.h"

namespace QgsSb
{
	//! Supported image output format
	enum ImageOutputFormat
	{
		UNKN,
		PNG,
		PNG16,
		PNG1,
		JPEG
	};

	ImageOutputFormat parseImageFormat(const QString &format)
	{
		if (format.compare(QLatin1String("png"), Qt::CaseInsensitive) == 0 ||
			format.compare(QLatin1String("image/png"), Qt::CaseInsensitive) == 0)
		{
			return PNG;
		}
		else if (format.compare(QLatin1String("jpg "), Qt::CaseInsensitive) == 0 ||
			format.compare(QLatin1String("image/jpeg"), Qt::CaseInsensitive) == 0)
		{
			return JPEG;
		}
		else
		{
			// lookup for png with mode
			QRegularExpression modeExpr = QRegularExpression(QStringLiteral("image/png\\s*;\\s*mode=([^;]+)"),
				QRegularExpression::CaseInsensitiveOption);

			QRegularExpressionMatch match = modeExpr.match(format);
			QString mode = match.captured(1);
			if (mode.compare(QLatin1String("16bit"), Qt::CaseInsensitive) == 0)
				return PNG16;
			if (mode.compare(QLatin1String("1bit"), Qt::CaseInsensitive) == 0)
				return PNG1;
		}

		return UNKN;
	}

	// Write image response
	void writeImage(QgsServerResponse &response, QImage &img, const QString &formatStr, int imageQuality)
	{
		ImageOutputFormat outputFormat = parseImageFormat(formatStr);
		QImage  result;
		QString saveFormat;
		QString contentType;
		switch (outputFormat)
		{
		case PNG:
			result = img;
			contentType = "image/png";
			saveFormat = "PNG";
			break;
		case PNG16:
			result = img.convertToFormat(QImage::Format_ARGB4444_Premultiplied);
			contentType = "image/png";
			saveFormat = "PNG";
			break;
		case PNG1:
			result = img.convertToFormat(QImage::Format_Mono,
				Qt::MonoOnly | Qt::ThresholdDither |
				Qt::ThresholdAlphaDither | Qt::NoOpaqueDetection);
			contentType = "image/png";
			saveFormat = "PNG";
			break;
		case JPEG:
			result = img;
			contentType = "image/jpeg";
			saveFormat = "JPEG";
			break;
		default:
			QgsMessageLog::logMessage(QString("Unsupported format string %1").arg(formatStr));
			saveFormat = UNKN;
			break;
		}

		if (outputFormat != UNKN)
		{
			response.setHeader("Content-Type", contentType);
			if (saveFormat == "JPEG")
			{
				result.save(response.io(), qPrintable(saveFormat), imageQuality);
			}
			else
			{
				result.save(response.io(), qPrintable(saveFormat));
			}
		}
		else
		{
			throw QgsServiceException("InvalidFormat",
				QString("Output format '%1' is not supported in the GetMap request").arg(formatStr));
		}
	}

  void writeGetLayerSymbol( QgsServerInterface *serverIface, const QgsProject *project,
							 const QString &version, const QgsServerRequest &request,
							 QgsServerResponse &response, const QString &layerId, const QString &layerName, const QString &ruleName, int iWidth, int iHeight)
  {
	  Q_UNUSED(version);

	  QgsServerRequest::Parameters params = request.parameters();
	  QString format = params.value(QStringLiteral("FORMAT"), QStringLiteral("PNG"));

	  // Get cached image
	  QgsAccessControl *accessControl = nullptr;
	  QgsServerCacheManager *cacheManager = nullptr;
#ifdef HAVE_SERVER_PYTHON_PLUGINS
	  accessControl = serverIface->accessControls();
	  cacheManager = serverIface->cacheManager();
#endif
	  if (cacheManager)
	  {
		  ImageOutputFormat outputFormat = parseImageFormat(format);
		  QString saveFormat;
		  QString contentType;
		  switch (outputFormat)
		  {
		  case PNG:
		  case PNG16:
		  case PNG1:
			  contentType = "image/png";
			  saveFormat = "PNG";
			  break;
		  case JPEG:
			  contentType = "image/jpeg";
			  saveFormat = "JPEG";
			  break;
		  default:
			  throw QgsServiceException("InvalidFormat",
				  QString("Output format '%1' is not supported in the GetLegendGraphic request").arg(format));
			  break;
		  }
		  
		  QImage image;
		  QByteArray content = cacheManager->getCachedImage(project, request, accessControl);
		  if (!content.isEmpty() && image.loadFromData(content))
		  {
			  response.setHeader(QStringLiteral("Content-Type"), contentType);
			  response.setHeader(QStringLiteral("X-QGIS-FROM-CACHE"), QStringLiteral("true"));
			  image.save(response.io(), qPrintable(saveFormat));
			  return;
		  }
	  }

	  QgsVectorLayer *vl = NULL;

	  if (!layerId.isEmpty())
	  {
		  QgsMapLayer *mapLayer = project->mapLayer(layerId);
		  if (mapLayer->type() == QgsMapLayerType::VectorLayer)
			  vl = qobject_cast<QgsVectorLayer *>(mapLayer);
	  }
	  else
	  {
		  // Use layer ids
		  bool useLayerIds = QgsServerProjectUtils::wmsUseLayerIds(*project);
		  
		  for(QgsMapLayer *layer : project->mapLayers(true))
		  {
			  if (layer->type() != QgsMapLayerType::VectorLayer)
				  continue;

			  QString name = layer->name();
			  if (useLayerIds)
				  name = layer->id();
			  else if (!layer->shortName().isEmpty())
				  name = layer->shortName();

			  if (layerName.compare(name, Qt::CaseInsensitive) != 0)
				  continue;

			  vl = qobject_cast<QgsVectorLayer *>(layer);
			  break;
		  }
	  }

	  QImage imgResult(QSize(iWidth, iHeight), QImage::Format_ARGB32_Premultiplied);
	  imgResult.fill(0);

	  bool bFound = false;
	  if (vl)
	  {
		  QgsFeatureRenderer *renderer = vl->renderer();
		  if(renderer)
		  {
			  QgsLegendSymbolList listSymbols = renderer->legendSymbolItems();
			  for (int i = 0; i < listSymbols.count(); i++)
			  {
				  if (listSymbols[i].ruleKey().compare(ruleName, Qt::CaseInsensitive) == 0)
				  {
					  if (listSymbols[i].symbol()->type() == Qgis::SymbolType::Marker)
					  {
						  QgsMarkerSymbol *pSymbol = (QgsMarkerSymbol*)listSymbols[i].symbol();

						  int iMaxDim = std::max(iWidth, iHeight);
						  
						  QgsUnitTypes::RenderUnit resetUnit = pSymbol->sizeUnit();
						  double dResetSize = pSymbol->size();

						  try
						  {
							  pSymbol->setSizeUnit(QgsUnitTypes::RenderUnit::RenderPixels);
							  pSymbol->setSize(iMaxDim);

							  QImage image = QImage(QSize(iMaxDim, iMaxDim), QImage::Format_ARGB32_Premultiplied);
							  image.fill(0);

							  QPainter p(&image);

							  QgsRenderContext context = QgsRenderContext::fromQPainter(&p);

							  context.setForceVectorOutput(true);

							  imgResult = pSymbol->asImage(QSize(iMaxDim, iMaxDim), &context);
							  imgResult = image;

							  bool bAllAlpha = true;
							  for (int iX = 0; iX < iMaxDim && bAllAlpha; iX++)
							  {
								  for (int iY = 0; iY < iMaxDim && bAllAlpha; iY++)
								  {
									  int iAlpha = qAlpha(imgResult.pixel(iX, iY));
									  bAllAlpha = iAlpha == 0;
								  }
							  }

							  if (bAllAlpha)
							  {
								  pSymbol->setSize(dResetSize);
								  pSymbol->setSizeUnit(resetUnit);

								  imgResult = pSymbol->asImage(QSize(iMaxDim, iMaxDim), &context);
								  imgResult = image;
							  }
							  
							  imgResult = imgResult.scaled(QSize(iWidth, iHeight), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
						  }
						  catch (QgsServerException &ex)
						  {
							  QgsMessageLog::logMessage(QStringLiteral("GetLayerSymbol - QgsServerException: %1").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
							  throw ex;
						  }
						  catch (QgsException &ex)
						  {
							  QgsMessageLog::logMessage(QStringLiteral("GetLayerSymbol - QgsException: %1").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
							  throw ex;
						  }
						  catch (std::runtime_error &ex)
						  {
							  QgsMessageLog::logMessage(QStringLiteral("GetLayerSymbol - RuntimeError: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
							  throw ex;
						  }
						  catch (std::exception &ex)
						  {
							  QgsMessageLog::logMessage(QStringLiteral("GetLayerSymbol - Exception: %1 | %2").arg(QString(ex.what())).arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
							  throw ex;
						  }
						  catch (...)
						  {
							  QgsMessageLog::logMessage(QStringLiteral("GetLayerSymbol - Unknown exception: %1").arg(request.url().toString()), QStringLiteral("Server"), Qgis::Critical);
						  }

						  pSymbol->setSize(dResetSize);
						  pSymbol->setSizeUnit(resetUnit);
					  }
					  else
						  imgResult = listSymbols[i].symbol()->asImage(QSize(iWidth, iHeight));

					  bFound = true;

					  break;
				  }
			  }
		  }
	  }
	  
	  if(!bFound)
		  QgsMessageLog::logMessage(QStringLiteral("Layer or symbol not found!"), QStringLiteral("Server"), Qgis::Critical);

	  response.setHeader(QStringLiteral("X-QGIS-FROM-CACHE"), QStringLiteral("true"));

	  writeImage(response, imgResult, format, 90);
	  if (cacheManager)
	  {
		  QByteArray content = response.data();
		  if (!content.isEmpty())
			  cacheManager->setCachedImage(&content, project, request, accessControl);
	  }
  }
} // namespace QgsSb
