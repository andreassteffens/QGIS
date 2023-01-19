/***************************************************************************
    sbminpixelsizefilterutils.h
    ---------------------------
    begin                : January 2023
    copyright            : (C) 2023 by Andreas Steffens
    email                : a dot steffens at gds dash team dot de
 ***************************************************************************/

#ifndef SBMINPIXELSIZEFILTERUTILS_H
#define SBMINPIXELSIZEFILTERUTILS_H

class QgsProject;
class QgsMapLayer;

#include "qgis_core.h"
#include "qgis_sip.h"

/**
 * \ingroup core
 */
class CORE_EXPORT sbMinPixelSizeFilterUtils
{
  public:

    /**
     *
     */
    static void setFilterProperties( QgsMapLayer *pLayer, bool bEnabled, double dMinPixelSize, double dMaxScale, bool bDebug );

    /**
     *
     */
    static void getFilterProperties( QgsMapLayer *pLayer, bool* pbEnabled, double* pdMinPixelSize, double *pdMaxScale, bool* pbDebug );
};

#endif
