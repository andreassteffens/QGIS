/***************************************************************************
    sbelevationservicesmaptool.h  -  map tool for reverse address search
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

#ifndef SBADDRESSSERVICESMAPTOOL_H
#define SBADDRESSSERVICESMAPTOOL_H

#include "qgsmaptool.h"
#include "qgspointxy.h"

#include <QObject>
#include <QPointer>

class QgsRubberBand;

/**
*  \brief Map tool for capturing mouse clicks to clipboard
*/
class sbElevationServicesMapTool : public QgsMapTool
{
    Q_OBJECT

  public:
    explicit sbElevationServicesMapTool( QgsMapCanvas *thepCanvas );

    //! Overridden mouse move event
    void canvasMoveEvent( QgsMapMouseEvent *e ) override;

    //! Overridden mouse press event
    void canvasPressEvent( QgsMapMouseEvent *e ) override;

    //! Overridden mouse release event
    void canvasReleaseEvent( QgsMapMouseEvent *e ) override;

    //! called when map tool is being deactivated
    void deactivate() override;

  signals:
    void mouseClicked( const QgsPointXY & );

  private:
    QPointer<QgsMapCanvas> mpMapCanvas;
};

#endif
