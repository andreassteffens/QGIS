/***************************************************************************
                              qgswmsrestorer.cpp
                              ------------------
  begin                : April 24, 2017
  copyright            : (C) 2017 by Paul Blottiere
  email                : paul.blottiere@oslandia.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgswmsrestorer.h"
#include "qgsmessagelog.h"
#include "qgsmaplayer.h"
#include "qgsvectorlayer.h"
#include "qgsrasterlayer.h"
#include "qgsrasterrenderer.h"
#include "qgsmaplayerstylemanager.h"
#include "qgslegendsymbolitem.h"
#include "qgsrenderer.h"
#include "qgsmessagelog.h"
#include "qgsreadwritecontext.h"
#include "qgswmsrendercontext.h"

QgsLayerRestorer::QgsLayerRestorer( const QgsWmsRenderContext &context )
{
  QMultiMap<QString, QgsWmsParametersRules> mapRules = context.parameters().sbAllLayerRules();
  QMultiMap<QString, bool> mapLabels = context.parameters().sbAllLayerLabels();

  for ( QgsMapLayer *layer : context.layers() )
  {
    if (layer->name().isEmpty())
      continue;

    mLayerSettings.emplace( layer, QgsLayerSettings() );
    QgsLayerSettings &settings = mLayerSettings[layer ];

    settings.name = layer->name();
    settings.mOpacity = 0;
    settings.mSetLabelVisibility = false;
    settings.mLabelVisibility = false;
    settings.mSetScaleBasedVisibility = false;
    settings.mScaleBasedVisibility = false;

    QgsMapLayerStyleManager *styleManager = layer->styleManager();

    if (styleManager)
      settings.mNamedStyle = styleManager->currentStyle();

    switch ( layer->type() )
    {
      case Qgis::LayerType::Vector:
      {
        QgsVectorLayer *vLayer = qobject_cast<QgsVectorLayer *>( layer );

        if ( vLayer )
        {
          settings.mOpacity = vLayer->opacity();
          settings.mSelectedFeatureIds = vLayer->selectedFeatureIds();
          settings.mFilter = vLayer->subsetString();
          // Labeling opacity
          if ( vLayer->labelsEnabled() && vLayer->labeling() )
          {
            settings.mLabeling.reset( vLayer->labeling()->clone() );
          }

          settings.mSetLegendItemStates = false;
          settings.mSetLabelVisibility = false;

          QMultiMap<QString, QgsWmsParametersRules>::const_iterator iterRules = mapRules.find(context.layerNickname(*layer));
          if (iterRules != mapRules.end())
          {
            QgsFeatureRenderer *renderer = vLayer->renderer();
            if (renderer)
            {
              settings.mSetLegendItemStates = true;

              for (const QgsLegendSymbolItem &legendItem : renderer->legendSymbolItems())
              {
                if (!legendItem.isCheckable())
                  continue;

                QString strRule = legendItem.ruleKey();
                bool bState = renderer->legendSymbolItemChecked(strRule);
                settings.mLegendItemStates.insert(strRule, bState);
              }
            }
          }

          QMultiMap<QString, bool>::const_iterator iterLabels = mapLabels.find(context.layerNickname(*layer));
          if (iterLabels != mapLabels.end())
          {
            settings.mSetLabelVisibility = true;
            settings.mLabelVisibility = vLayer->labelsEnabled();
          }
        }
        break;
      }
      case Qgis::LayerType::Raster:
      {
        QgsRasterLayer *rLayer = qobject_cast<QgsRasterLayer *>( layer );

        if ( rLayer )
        {
          settings.mOpacity = rLayer->renderer()->opacity();
        }
        break;
      }

      case Qgis::LayerType::Mesh:
      case Qgis::LayerType::VectorTile:
      case Qgis::LayerType::Plugin:
      case Qgis::LayerType::Annotation:
      case Qgis::LayerType::PointCloud:
      case Qgis::LayerType::Group:
        break;
    }

  }
}

void QgsLayerRestorer::sbUpdateScaleBasedVisibility(QgsWmsRenderContext &context, double dScale)
{
  QMultiMap<QString, QString> mapSelectionLayers;
  if (context.parameters().sbAlwaysRenderSelection())
  {
    QStringList listSelections = context.parameters().selections();
    for (int iSelection = 0; iSelection < listSelections.count(); iSelection++)
    {
      QStringList listParts = listSelections[iSelection].split(':');
      if (!mapSelectionLayers.contains(listParts[0]))
        mapSelectionLayers.insert(listParts[0], listParts[0]);
    }
  }

  QMap<QgsMapLayer *, QgsLayerSettings>::iterator iterSettings;
  for (iterSettings = mLayerSettings.begin(); iterSettings != mLayerSettings.end(); iterSettings++)
  {
    QgsMapLayer* pLayer = iterSettings.key();
    switch (pLayer->type())
    {
      case QgsMapLayerType::VectorLayer:
      {
        QgsVectorLayer* pVectorLayer = qobject_cast<QgsVectorLayer *>(pLayer);
        if (context.parameters().sbAlwaysRenderSelection() && pVectorLayer->hasScaleBasedVisibility() && dScale > -1)
        {
          if (mapSelectionLayers.contains(pVectorLayer->shortName()))
          {
            if (dScale > pVectorLayer->minimumScale() || dScale < pVectorLayer->maximumScale())
            {
              iterSettings->mScaleBasedVisibility = true;
              iterSettings->mSetScaleBasedVisibility = true;

              pVectorLayer->setScaleBasedVisibility(false);

              context.sbAddRenderSelectionOnlyLayer(pVectorLayer->shortName());
            }
          }
        }
      }
      break;
    }
  }
}

QgsLayerRestorer::~QgsLayerRestorer()
{
  for ( auto it = mLayerSettings.begin(); it != mLayerSettings.end(); it++ )
  {
    if (layer->name().isEmpty())
      continue;

    QgsLayerSettings settings = mLayerSettings[layer];

    QgsMapLayerStyleManager *styleManager = layer->styleManager();
    if (styleManager)
    QgsMapLayer *layer = it->first;

    // Firstly check if a SLD file has been loaded for rendering and removed it
    const QString sldStyleName { layer->customProperty( "sldStyleName", "" ).toString() };
    if ( !sldStyleName.isEmpty() )
    {
      styleManager->setCurrentStyle(settings.mNamedStyle);

      // if a SLD file has been loaded for rendering, we restore the previous style
      const QString sldStyleName{ layer->customProperty("sldStyleName", "").toString() };
      if (!sldStyleName.isEmpty())
      {
        styleManager->removeStyle(sldStyleName);
        layer->removeCustomProperty("sldStyleName");
      }

      styleManager->setCurrentStyle( settings.mNamedStyle ); 
    }

    // Then restore the previous style
    QgsLayerSettings &settings = it->second;
    layer->styleManager()->setCurrentStyle( settings.mNamedStyle );
    layer->setName( settings.name );

    switch ( layer->type() )
    {
      case Qgis::LayerType::Vector:
      {
        QgsVectorLayer *vLayer = qobject_cast<QgsVectorLayer *>( layer );

        if ( vLayer )
        {
          vLayer->setOpacity( settings.mOpacity );
          vLayer->selectByIds( settings.mSelectedFeatureIds );
          vLayer->setSubsetString( settings.mFilter );
          if ( settings.mLabeling )
          {
            vLayer->setLabeling( settings.mLabeling.release() );
          }

          if (settings.mSetLegendItemStates)
          {
            QgsFeatureRenderer *renderer = vLayer->renderer();
            if (renderer)
            {
              for (const QgsLegendSymbolItem &legendItem : renderer->legendSymbolItems())
              {
                if (!legendItem.isCheckable())
                  continue;

                QString strRule = legendItem.ruleKey();
                QMultiMap<QString, bool>::iterator it = settings.mLegendItemStates.find(strRule);
                if (it != settings.mLegendItemStates.end())
                {
                  bool bState = it.value();
                  if (bState != renderer->legendSymbolItemChecked(strRule))
                    renderer->checkLegendSymbolItem(strRule, bState);
                }
              }
            }
          }

          if (settings.mSetLabelVisibility)
            vLayer->setLabelsEnabled(settings.mLabelVisibility);

          if (settings.mSetScaleBasedVisibility)
            vLayer->setScaleBasedVisibility(settings.mScaleBasedVisibility);
        }
        break;
      }
      case Qgis::LayerType::Raster:
      {
        QgsRasterLayer *rLayer = qobject_cast<QgsRasterLayer *>( layer );

        if ( rLayer )
        {
          rLayer->renderer()->setOpacity( settings.mOpacity );
        }
        break;
      }

      case Qgis::LayerType::Mesh:
      case Qgis::LayerType::VectorTile:
      case Qgis::LayerType::Plugin:
      case Qgis::LayerType::Annotation:
      case Qgis::LayerType::PointCloud:
      case Qgis::LayerType::Group:
        break;
    }
  }
}

namespace QgsWms
{
  QgsWmsRestorer::QgsWmsRestorer( const QgsWmsRenderContext &context )
    : mLayerRestorer( context )
  {
  }
}
