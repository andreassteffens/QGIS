/***************************************************************************
                              qgswmslayerinfos.cpp

  Layer's information
  ------------------------------------
  begin                : September 26 , 2022
  copyright            : (C) 2022 by René-Luc D'Hont and David Marteau
  email                : rldhont at 3liz doc com
         dmarteau at 3liz dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsserverprojectutils.h"
#include "qgscoordinatetransform.h"
#include "qgsproject.h"
#include "qgsvectorlayer.h"
#include "qgswmslayerinfos.h"
#include "qgsmessagelog.h"
#include "qgsserverinterface.h"
#include "qgsmaplayerstylemanager.h"
#include "qgswmsprovider.h"

#include <algorithm>

QgsRectangle QgsWmsLayerInfos::transformExtent(
  const QgsRectangle &extent,
  const QgsCoordinateReferenceSystem &source,
  const QgsCoordinateReferenceSystem &destination,
  const QgsCoordinateTransformContext &context,
  const bool &ballparkTransformsAreAppropriate )
{
  QgsCoordinateTransform transformer { source, destination, context };
  transformer.setBallparkTransformsAreAppropriate( ballparkTransformsAreAppropriate );
  // Transform extent and do not catch exception
  return transformer.transformBoundingBox( extent );
}

QMap< QString, QgsRectangle > QgsWmsLayerInfos::transformExtentToCrsList(
  const QgsRectangle &extent,
  const QgsCoordinateReferenceSystem &source,
  const QList<QgsCoordinateReferenceSystem> &destinations,
  const QgsCoordinateTransformContext &context )
{
  QMap< QString, QgsRectangle > crsExtents;
  if ( extent.isEmpty() )
  {
    return crsExtents;
  }
  for ( const QgsCoordinateReferenceSystem &destination : std::as_const( destinations ) )
  {
    // Transform extent and do not catch exception
    QgsCoordinateTransform crsTransform { source, destination, context };
    crsExtents[ destination.authid() ] = crsTransform.transformBoundingBox( extent );
  }
  return crsExtents;
}


bool setBoundingRect(
  const QgsProject *project,
  QgsWmsLayerInfos &pLayer,
  QgsMapLayer *ml,
  const QgsRectangle &wmsExtent,
  const QgsCoordinateReferenceSystem &wgs84,
  const QList<QgsCoordinateReferenceSystem> &outputCrsList )
{
  QgsRectangle layerExtent = ml->extent();

  QgsLayerMetadata meta = ml->metadata();
  if ( meta.extent().spatialExtents().count() > 0 )
  {
    QgsLayerMetadata::SpatialExtent spext = meta.extent().spatialExtents().first();
    if ( spext.extentCrs.isValid() )
    {
      if ( spext.bounds.width() > 0 && spext.bounds.height() > 0 )
      {
        QgsRectangle extent = QgsRectangle( spext.bounds.xMinimum(), spext.bounds.yMinimum(), spext.bounds.xMaximum(), spext.bounds.yMaximum() );

        if ( ml->crs() == spext.extentCrs )
          layerExtent = extent;
        else
        {
          QgsCoordinateTransform trans( spext.extentCrs, ml->crs(), project );
          layerExtent = trans.transformBoundingBox( extent );
        }
      }
    }
  }

  if ( layerExtent.isEmpty() )
  {
    // if the extent is empty (not only Null), use the wms extent
    // defined in the project...
    if ( wmsExtent.isNull() )
    {
      // or the CRS extent otherwise
      layerExtent = ml->crs().bounds();
    }
    else
    {
      layerExtent = QgsRectangle( wmsExtent );
      if ( ml->crs() != project->crs() )
      {
        // If CRS is different transform it to layer's CRS
        try
        {
          layerExtent = QgsWmsLayerInfos::transformExtent( wmsExtent, project->crs(), ml->crs(), project->transformContext() );
        }
        catch ( QgsCsException &cse )
        {
          QgsMessageLog::logMessage( QStringLiteral( "Error transforming extent for layer %1: %2" ).arg( ml->name() ).arg( cse.what() ), QStringLiteral( "Server" ), Qgis::MessageLevel::Warning );
          return false;
        }
      }
    }

    if ( qgsDoubleNear( layerExtent.xMinimum(), layerExtent.xMaximum() ) || qgsDoubleNear( layerExtent.yMinimum(), layerExtent.yMaximum() ) )
    {
      //layer bbox cannot be empty
      layerExtent.grow( 0.000001 );
    }

    // Now we have a layer Extent we need the WGS84 bounding rectangle
    try
    {
      pLayer.wgs84BoundingRect = QgsWmsLayerInfos::transformExtent( layerExtent, ml->crs(), wgs84, project->transformContext(), true );
    }
    catch ( const QgsCsException &cse )
    {
      QgsMessageLog::logMessage( QStringLiteral( "Error transforming extent for layer %1: %2" ).arg( ml->name() ).arg( cse.what() ), QStringLiteral( "Server" ), Qgis::MessageLevel::Warning );
      return false;
    }

    if ( qgsDoubleNear( pLayer.wgs84BoundingRect.xMinimum(), pLayer.wgs84BoundingRect.xMaximum() ) || qgsDoubleNear( pLayer.wgs84BoundingRect.yMinimum(), pLayer.wgs84BoundingRect.yMaximum() ) )
    {
      //layer bbox cannot be empty
      pLayer.wgs84BoundingRect.grow( 0.000001 );
    }
  }
  else
  {
    pLayer.wgs84BoundingRect = ml->wgs84Extent();
  }

  try
  {
    pLayer.crsExtents = QgsWmsLayerInfos::transformExtentToCrsList(
                          layerExtent, ml->crs(), outputCrsList, project->transformContext()
                        );
  }
  catch ( QgsCsException &cse )
  {
    QgsMessageLog::logMessage( QStringLiteral( "Error transforming extent for layer %1: %2" ).arg( ml->name() ).arg( cse.what() ), QStringLiteral( "Server" ), Qgis::MessageLevel::Warning );
    return false;
  }

  return true;
}

// ===================================
// Get wms layer infos
// ===================================
QMap< QString, QgsWmsLayerInfos > QgsWmsLayerInfos::buildWmsLayerInfos(
  QgsServerInterface *serverIface,
  const QgsProject *project,
  const QList<QgsCoordinateReferenceSystem> &outputCrsList )
{
  QMap< QString, QgsWmsLayerInfos > wmsLayers;
#ifdef HAVE_SERVER_PYTHON_PLUGINS
  QgsAccessControl *accessControl = serverIface->accessControls();
#else
  ( void )serverIface;
#endif

  bool useLayerIds = QgsServerProjectUtils::wmsUseLayerIds( *project );
  const QStringList restrictedLayers = QgsServerProjectUtils::wmsRestrictedLayers( *project );
  const QgsRectangle wmsExtent = QgsServerProjectUtils::wmsExtent( *project );
  const QgsCoordinateReferenceSystem wgs84 = QgsCoordinateReferenceSystem::fromOgcWmsCrs( geoEpsgCrsAuthId() );

  for ( QgsMapLayer *ml : project->mapLayers() )
  {
    if ( !ml || restrictedLayers.contains( ml->name() ) ) //unpublished layer
    {
      continue;
    }

#ifdef HAVE_SERVER_PYTHON_PLUGINS
    if ( accessControl && !accessControl->layerReadPermission( ml ) )
    {
      continue;
    }
#endif

    QgsWmsLayerInfos pLayer;
    pLayer.id = ml->id();

    // Calculate layer extents for the WMS output CRSes list
    // First define if the layer has an extent
    // Vector layer with No Geometry has no extent the other has one
    bool hasExtent = true;
    if ( ml->type() == Qgis::LayerType::Vector )
    {
      QgsVectorLayer *vLayer = qobject_cast<QgsVectorLayer *>( ml );
      if ( !vLayer || vLayer->wkbType() == Qgis::WkbType::NoGeometry )
      {
        hasExtent = false;
      }
    }

    // If the layer has an extent and we cannot get CRS bounding boxes, do not keep the layer
    if ( hasExtent && !setBoundingRect( project, pLayer, ml, wmsExtent, wgs84, outputCrsList ) )
      continue;

    // layer type
    pLayer.type = ml->type();
    // layer wms name
    pLayer.name = ml->name();
    if ( useLayerIds )
    {
      pLayer.name = ml->id();
    }
    else if ( !ml->shortName().isEmpty() )
    {
      pLayer.name = ml->shortName();
    }
    // layer title
    pLayer.title = ml->title();
    if ( pLayer.title.isEmpty() )
    {
      pLayer.title = ml->name();
    }
    // layer abstract
    pLayer.abstract = ml->abstract();
    // layer is queryable
    pLayer.queryable = ml->flags().testFlag( QgsMapLayer::Identifiable );
    // layer keywords
    if ( !ml->keywordList().isEmpty() )
    {
      pLayer.keywords = ml->keywordList().split( ',' );
    }
    // layer styles
    pLayer.styles = ml->styleManager()->styles();
    // layer legend URL
    pLayer.legendUrl = ml->legendUrl();
    // layer legend URL format
    pLayer.legendUrlFormat = ml->legendUrlFormat();
    // layer min/max scales
    if ( ml->hasScaleBasedVisibility() )
    {
      pLayer.hasScaleBasedVisibility = ml->hasScaleBasedVisibility();
      pLayer.maxScale = ml->maximumScale();
      pLayer.minScale = ml->minimumScale();
    }
    else
    {
      if ( ml->dataProvider() != NULL )
      {
        if ( ml->dataProvider()->name() == "wms" )
        {
          QgsWmsProvider *wmsProvider = ( QgsWmsProvider * )ml->dataProvider();

          double minScale, maxScale;
          if ( wmsProvider->getMinMaxScale( maxScale, minScale ) )
          {
            pLayer.hasScaleBasedVisibility = true;
            pLayer.maxScale = maxScale;
            pLayer.minScale = minScale;
          }
        }
      }
    }
    // layer data URL
    pLayer.dataUrl = ml->dataUrl();
    // layer attribution
    pLayer.attribution = ml->attribution();
    pLayer.attributionUrl = ml->attributionUrl();
    // layer metadata URLs
    pLayer.metadataUrls = ml->serverProperties()->metadataUrls();

    wmsLayers[pLayer.id] = pLayer;
  }

  return wmsLayers;
}

