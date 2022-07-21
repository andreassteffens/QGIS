/***************************************************************************
    sbaddressservicesmaptool.cpp  -  map tool for reverse address search
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

#include "sbaddressservicesmaptool.h"
#include "qgsmapcanvas.h"
#include "qgsmaptopixel.h"
#include "qgsrubberband.h"
#include "qgscoordinatereferencesystem.h"
#include "qgsmapmouseevent.h"
#include "qgsapplication.h"


sbAddressServicesMapTool::sbAddressServicesMapTool( QgsMapCanvas *thepCanvas ) : QgsMapTool( thepCanvas )
{
  mpMapCanvas = thepCanvas;

  setCursor( QgsApplication::getThemeCursor( QgsApplication::Cursor::CrossHair ) );
}

void sbAddressServicesMapTool::canvasMoveEvent( QgsMapMouseEvent *thepEvent )
{
  Q_UNUSED(thepEvent);
}

void sbAddressServicesMapTool::canvasPressEvent( QgsMapMouseEvent *thepEvent )
{
  Q_UNUSED( thepEvent );
}

void sbAddressServicesMapTool::canvasReleaseEvent( QgsMapMouseEvent *thepEvent )
{
  QgsPointXY myOriginalPoint = mCanvas->getCoordinateTransform()->toMapCoordinates( thepEvent->x(), thepEvent->y() );

  emit mouseClicked( myOriginalPoint, mCanvas->scale() );
}

void sbAddressServicesMapTool::deactivate()
{
  QgsMapTool::deactivate();
}
