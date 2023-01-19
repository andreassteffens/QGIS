/***************************************************************************
    sbjoinedtoggleutils.h
    ---------------------
    begin                : January 2023
    copyright            : (C) 2023 by Andreas Steffens
    email                : a dot steffens at gds dash team dot de
 ***************************************************************************/

#ifndef SBJOINEDTOGGLEUTILS_H
#define SBJOINEDTOGGLEUTILS_H

class QgsProject;
class QgsMapLayer;

#include <qlist.h>
#include "qgis_core.h"
#include "qgis_sip.h"

typedef struct _sbJoinedToggleLayerSettings
{
  QString layerId;
  QString referencedLayerId;
  bool activateWithReference;
  bool deactivateWithReference;
  bool invertBehavior;

  _sbJoinedToggleLayerSettings()
  {
    layerId = referencedLayerId = "";
    activateWithReference = deactivateWithReference = invertBehavior = false;
  }

  _sbJoinedToggleLayerSettings( const _sbJoinedToggleLayerSettings& s )
        : layerId( s.layerId )
        , referencedLayerId( s.referencedLayerId )
        , activateWithReference( s.activateWithReference )
        , deactivateWithReference( s.deactivateWithReference )
        , invertBehavior( s.invertBehavior )
  {
    // nothing to be done here for now
  }
} sbJoinedToggleLayerSettings;

/**
 * \ingroup core
 */
class CORE_EXPORT sbJoinedToggleUtils
{
  public:

    /**
     *
     */
    static void addJoinedToggleLayer( QgsMapLayer *pReferencedLayer, QgsMapLayer *pJoinedLayer, bool bActivateWithReference, bool bDeactivateWithReference, bool bInvertBehavior );

    /**
     *
     */
    static void removeJoinedToggleLayer ( QgsMapLayer *pJoinedLayer );

    /**
     *
     */
    static QList< sbJoinedToggleLayerSettings > getJoinedToggleLayers( QgsMapLayer* pReferencedLayer );

    /**
     *
     */
    static sbJoinedToggleLayerSettings getReferencedLayer( QgsMapLayer* pLayer );

  private:

    /**
     *
     */
    static void sanitizeJoinedToggleLayers( QgsProject* pProject );

    /**
     *
     */
    static void removeJoinedToggleLayer( QgsMapLayer* pReferencedLayer, QgsMapLayer* pJoinedLayer );

    /**
     *
     */
    static void removeJoinedToggleLayer( QgsMapLayer* pReferencedLayer, QString strJoinedLayerId );
};

#endif SBJOINEDTOGGLEUTILS_H
