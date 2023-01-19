/***************************************************************************
    sbminpixelsizefilterutils.cpp
    -----------------------------
    begin                : January 2023
    copyright            : (C) 2023 by Andreas Steffens
    email                : a dot steffens at gds dash team dot de
 ***************************************************************************/
 
#include "sbminpixelsizefilterutils.h"
#include "qgsmaplayer.h"

#define MIN_PIXEL_SIZE_FILTER_ENABLED_KEY "sb:MIN_PIXEL_SIZE_FILTER_ENABLED"
#define MIN_PIXEL_SIZE_FILTER_SIZE_KEY "sb:MIN_PIXEL_SIZE_FILTER_SIZE"
#define MIN_PIXEL_SIZE_FILTER_MAX_SCALE_KEY "sb:MIN_PIXEL_SIZE_FILTER_MAX_SCALE"
#define MIN_PIXEL_SIZE_FILTER_DEBUG_KEY "sb:MIN_PIXEL_SIZE_FILTER_DEBUG_MODE"

void sbMinPixelSizeFilterUtils::setFilterProperties( QgsMapLayer *pLayer, bool bEnabled, double dMinPixelSize, double dMaxScale, bool bDebug )
{
  pLayer->setCustomProperty( MIN_PIXEL_SIZE_FILTER_ENABLED_KEY, QVariant( bEnabled ) );
  pLayer->setCustomProperty( MIN_PIXEL_SIZE_FILTER_SIZE_KEY, QVariant( dMinPixelSize ) );
  pLayer->setCustomProperty( MIN_PIXEL_SIZE_FILTER_MAX_SCALE_KEY, QVariant( dMaxScale ) );
  pLayer->setCustomProperty( MIN_PIXEL_SIZE_FILTER_DEBUG_KEY, QVariant( bDebug ) );
}

void sbMinPixelSizeFilterUtils::getFilterProperties( QgsMapLayer *pLayer, bool* pbEnabled, double* pdMinPixelSize, double *pdMaxScale, bool* pbDebug )
{
  *pbEnabled = false;
  *pdMinPixelSize = 0;
  *pdMaxScale = 0;
  *pbDebug = false;

  QVariant varEnabled = pLayer->customProperty( MIN_PIXEL_SIZE_FILTER_ENABLED_KEY, QVariant( false ) );
  if ( varEnabled.isValid() && varEnabled.type() == QVariant::Type::Bool )
    *pbEnabled = varEnabled.toBool();

  QVariant varMinPixelSize = pLayer->customProperty( MIN_PIXEL_SIZE_FILTER_SIZE_KEY, QVariant( (double)2 ) );
  if ( varMinPixelSize.isValid() && varMinPixelSize.type() == QVariant::Type::Double )
    *pdMinPixelSize = varMinPixelSize.toDouble();

  QVariant varMaxScale = pLayer->customProperty( MIN_PIXEL_SIZE_FILTER_MAX_SCALE_KEY, QVariant( (double)5000 ) );
  if ( varMaxScale.isValid() && varMaxScale.type() == QVariant::Type::Double )
    *pdMaxScale = varMaxScale.toDouble();

  QVariant varDebug = pLayer->customProperty( MIN_PIXEL_SIZE_FILTER_DEBUG_KEY, QVariant( false ) );
  if ( varDebug.isValid() && varDebug.type() == QVariant::Type::Bool )
    *pbDebug = varDebug.toBool();
}
