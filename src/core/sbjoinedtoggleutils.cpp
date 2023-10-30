/***************************************************************************
    sbjoinedtoggleutils.cpp
    -----------------------
    begin                : January 2023
    copyright            : (C) 2023 by Andreas Steffens
    email                : a dot steffens at gds dash team dot de
 ***************************************************************************/

#include "sbjoinedtoggleutils.h"
#include "qgsproject.h"
#include "qgsmaplayer.h"

#define JOINED_TOGGLE_LAYERS_KEY "sb:JOINED_TOGGLE_LAYERS"
#define JOINED_TOGGLE_ACTIVATE_BY_REFERENCE_KEY "sb:JOINED_TOGGLE_ACTIVATE_BY_REFERENCE"
#define JOINED_TOGGLE_DEACTIVATE_BY_REFERENCE_KEY "sb:JOINED_TOGGLE_DEACTIVATE_BY_REFERENCE"
#define JOINED_TOGGLE_INVERT_BEHAVIOR_KEY "sb:JOINED_TOGGLE_INVERT_BEHAVIOR"

void sbJoinedToggleUtils::addJoinedToggleLayer( QgsMapLayer *pReferencedLayer, QgsMapLayer *pJoinedLayer, bool bActivateWithReference, bool bDeactivateWithReference, bool bInvertBehavior )
{
  sanitizeJoinedToggleLayers( pReferencedLayer->project() );

  QString strJoinedLayerId = pJoinedLayer->id();

  QVariantMap varMapLayers;
  QVariant varLayers = pReferencedLayer->customProperty( JOINED_TOGGLE_LAYERS_KEY );
  if ( varLayers.isValid() && varLayers.type() == QVariant::Type::Map )
    varMapLayers = varLayers.toMap();
  varMapLayers.insert( strJoinedLayerId, strJoinedLayerId );
  pReferencedLayer->setCustomProperty( JOINED_TOGGLE_LAYERS_KEY, varMapLayers );

  QVariantMap varMapActivate;
  QVariant varActivate = pReferencedLayer->customProperty( JOINED_TOGGLE_ACTIVATE_BY_REFERENCE_KEY );
  if ( varActivate.isValid() && varActivate.type() == QVariant::Type::Map )
    varMapActivate = varActivate.toMap();
  varMapActivate.insert( strJoinedLayerId, bActivateWithReference );
  pReferencedLayer->setCustomProperty( JOINED_TOGGLE_ACTIVATE_BY_REFERENCE_KEY, varMapActivate );

  QVariantMap varMapDeactivate;
  QVariant varDeactivate = pReferencedLayer->customProperty( JOINED_TOGGLE_DEACTIVATE_BY_REFERENCE_KEY );
  if ( varDeactivate.isValid() && varDeactivate.type() == QVariant::Type::Map )
    varMapDeactivate = varDeactivate.toMap();
  varMapDeactivate.insert( strJoinedLayerId, bDeactivateWithReference );
  pReferencedLayer->setCustomProperty( JOINED_TOGGLE_DEACTIVATE_BY_REFERENCE_KEY, varMapDeactivate );

  QVariantMap varMapInvert;
  QVariant varInvert = pReferencedLayer->customProperty( JOINED_TOGGLE_INVERT_BEHAVIOR_KEY );
  if ( varInvert.isValid() && varInvert.type() == QVariant::Type::Map )
    varMapInvert = varInvert.toMap();
  varMapInvert.insert( strJoinedLayerId, bInvertBehavior );
  pReferencedLayer->setCustomProperty( JOINED_TOGGLE_DEACTIVATE_BY_REFERENCE_KEY, varMapInvert );
}

void sbJoinedToggleUtils::removeJoinedToggleLayer( QgsMapLayer *pJoinedLayer )
{
  QgsProject *pProject = pJoinedLayer->project();
  if ( !pProject )
    return;

  QMap< QString, QgsMapLayer * > mapLayers = pProject->mapLayers();
  for ( QMap< QString, QgsMapLayer * >::const_iterator iter = mapLayers.constBegin(); iter != mapLayers.constEnd(); iter++ )
    removeJoinedToggleLayer( iter.value(), pJoinedLayer );

  sanitizeJoinedToggleLayers( pProject );
}

void sbJoinedToggleUtils::removeJoinedToggleLayer( QgsMapLayer *pReferencedLayer, QgsMapLayer *pJoinedLayer )
{
  removeJoinedToggleLayer( pReferencedLayer, pJoinedLayer->id() );
}

void sbJoinedToggleUtils::removeJoinedToggleLayer( QgsMapLayer *pReferencedLayer, QString strJoinedLayerId )
{
  QVariant varLayers = pReferencedLayer->customProperty( JOINED_TOGGLE_LAYERS_KEY );
  if ( varLayers.isValid() && varLayers.type() == QVariant::Type::Map )
  {
    QVariantMap varMapLayers = varLayers.toMap();
    if ( varMapLayers.contains( strJoinedLayerId ) )
    {
      varMapLayers.remove( strJoinedLayerId );
      pReferencedLayer->setCustomProperty( JOINED_TOGGLE_LAYERS_KEY, varMapLayers );
    }
  }

  QVariant varActivate = pReferencedLayer->customProperty( JOINED_TOGGLE_ACTIVATE_BY_REFERENCE_KEY );
  if ( varActivate.isValid() && varActivate.type() == QVariant::Type::Map )
  {
    QVariantMap varMapActivate = varActivate.toMap();
    if ( varMapActivate.contains( strJoinedLayerId ) )
    {
      varMapActivate.remove( strJoinedLayerId );
      pReferencedLayer->setCustomProperty( JOINED_TOGGLE_ACTIVATE_BY_REFERENCE_KEY, varMapActivate );
    }
  }

  QVariant varDeactivate = pReferencedLayer->customProperty( JOINED_TOGGLE_DEACTIVATE_BY_REFERENCE_KEY );
  if ( varDeactivate.isValid() && varDeactivate.type() == QVariant::Type::Map )
  {
    QVariantMap varMapDeactivate = varDeactivate.toMap();
    if ( varMapDeactivate.contains( strJoinedLayerId ) )
    {
      varMapDeactivate.remove( strJoinedLayerId );
      pReferencedLayer->setCustomProperty( JOINED_TOGGLE_DEACTIVATE_BY_REFERENCE_KEY, varMapDeactivate );
    }
  }

  QVariant varInvert = pReferencedLayer->customProperty( JOINED_TOGGLE_INVERT_BEHAVIOR_KEY );
  if ( varInvert.isValid() && varInvert.type() == QVariant::Type::Map )
  {
    QVariantMap varMapInvert = varInvert.toMap();
    if ( varMapInvert.contains( strJoinedLayerId ) )
    {
      varMapInvert.remove( strJoinedLayerId );
      pReferencedLayer->setCustomProperty( JOINED_TOGGLE_DEACTIVATE_BY_REFERENCE_KEY, varMapInvert );
    }
  }
}

QList< sbJoinedToggleLayerSettings > sbJoinedToggleUtils::getJoinedToggleLayers( QgsMapLayer *pReferencedLayer )
{
  QList< sbJoinedToggleLayerSettings > listSettings;

  QVariant varLayers = pReferencedLayer->customProperty( JOINED_TOGGLE_LAYERS_KEY );
  if ( varLayers.isValid() && varLayers.type() == QVariant::Type::Map )
  {
    QVariantMap varMapLayers = varLayers.toMap();
    for ( QVariantMap::const_iterator iter = varMapLayers.constBegin(); iter != varMapLayers.constEnd(); iter++ )
    {
      sbJoinedToggleLayerSettings settings;

      settings.referencedLayerId = pReferencedLayer->id();
      settings.layerId = iter.key();

      QVariant varActivate = pReferencedLayer->customProperty( JOINED_TOGGLE_ACTIVATE_BY_REFERENCE_KEY );
      if ( varActivate.isValid() && varActivate.type() == QVariant::Type::Map )
      {
        QVariantMap varMapActivate = varActivate.toMap();
        settings.activateWithReference = varMapActivate.value( settings.layerId, QVariant( false ) ).toBool();
      }

      QVariant varDeactivate = pReferencedLayer->customProperty( JOINED_TOGGLE_DEACTIVATE_BY_REFERENCE_KEY );
      if ( varDeactivate.isValid() && varDeactivate.type() == QVariant::Type::Map )
      {
        QVariantMap varMapDeactivate = varDeactivate.toMap();
        settings.deactivateWithReference = varMapDeactivate.value( settings.layerId, QVariant( false ) ).toBool();
      }

      QVariant varInvert = pReferencedLayer->customProperty( JOINED_TOGGLE_INVERT_BEHAVIOR_KEY );
      if ( varInvert.isValid() && varInvert.type() == QVariant::Type::Map )
      {
        QVariantMap varMapInvert = varInvert.toMap();
        settings.invertBehavior = varMapInvert.value( settings.layerId, QVariant( false ) ).toBool();
      }

      listSettings.append( settings );
    }
  }

  return listSettings;
}

sbJoinedToggleLayerSettings sbJoinedToggleUtils::getReferencedLayer( QgsMapLayer *pLayer )
{
  sbJoinedToggleLayerSettings settings;

  QgsProject *pProject = pLayer->project();
  if ( !pProject )
    return settings;

  QMap< QString, QgsMapLayer * > mapLayers = pProject->mapLayers();
  for ( QMap< QString, QgsMapLayer * >::const_iterator iter = mapLayers.constBegin(); iter != mapLayers.constEnd(); iter++ )
  {
    QList< sbJoinedToggleLayerSettings > listSettings = getJoinedToggleLayers( iter.value() );
    for ( QList< sbJoinedToggleLayerSettings >::const_iterator iterSettings = listSettings.constBegin(); iterSettings != listSettings.constEnd(); iterSettings++ )
    {
      if ( iterSettings->layerId.compare( pLayer->id() ) == 0 )
        return *iterSettings;
    }
  }

  return settings;
}

void sbJoinedToggleUtils::sanitizeJoinedToggleLayers( QgsProject *pProject )
{
  QMap< QString, QgsMapLayer * > mapLayers = pProject->mapLayers();
  for ( QMap< QString, QgsMapLayer * >::const_iterator iter = mapLayers.constBegin(); iter != mapLayers.constEnd(); iter++ )
  {
    QList< sbJoinedToggleLayerSettings > listSettings = getJoinedToggleLayers( iter.value() );
    for ( QList< sbJoinedToggleLayerSettings >::const_iterator iterSettings = listSettings.constBegin(); iterSettings != listSettings.constEnd(); iterSettings++ )
    {
      if ( !mapLayers.contains( iterSettings->layerId ) )
        removeJoinedToggleLayer( iter.value(), iterSettings->layerId );
    }
  }
}
