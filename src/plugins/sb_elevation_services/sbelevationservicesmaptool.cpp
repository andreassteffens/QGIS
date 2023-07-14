/***************************************************************************
    sbelevationservicesmaptool.cpp  -  map tool for reverse address search
    ---------------------
    begin                : December 2018
    copyright            : (C) 2018 by Andreas Steffens
    email                : a dot steffens at gds dash team dot de
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QSettings>
#include <QCursor>
#include <QPixmap>

#include "sbelevationservicesmaptool.h"
#include "qgsmapcanvas.h"
#include "qgsmaptopixel.h"
#include "qgsrubberband.h"
#include "qgscoordinatereferencesystem.h"
#include "qgsmapmouseevent.h"
#include "qgsapplication.h"


sbElevationServicesMapTool::sbElevationServicesMapTool( QgsMapCanvas *thepCanvas ) : QgsMapTool( thepCanvas )
{
  mpMapCanvas = thepCanvas;

  setCursor( QgsApplication::getThemeCursor( QgsApplication::Cursor::CrossHair ) );
}

void sbElevationServicesMapTool::canvasMoveEvent( QgsMapMouseEvent *thepEvent )
{
  Q_UNUSED( thepEvent );
}

void sbElevationServicesMapTool::canvasPressEvent( QgsMapMouseEvent *thepEvent )
{
  Q_UNUSED( thepEvent );
}

void sbElevationServicesMapTool::canvasReleaseEvent( QgsMapMouseEvent *thepEvent )
{
  QgsPointXY myOriginalPoint = mCanvas->getCoordinateTransform()->toMapCoordinates( thepEvent->x(), thepEvent->y() );

  emit mouseClicked( myOriginalPoint );
}

void sbElevationServicesMapTool::deactivate()
{
  QgsMapTool::deactivate();
}
