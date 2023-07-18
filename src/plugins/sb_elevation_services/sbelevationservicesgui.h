/***************************************************************************
 *   Copyright (C) 2018 by Andreas Steffens                                *
 *   a dot steffens at gds dash team dot de                                *
 *                                                                         *
 *   This is a plugin generated from the QGIS plugin template              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef SBADDRESSSERVICESGUI_H
#define SBADDRESSSERVICESGUI_H

#include "ui_sbelevationservicesguibase.h"

#include <QDockWidget>
#include <qnetworkaccessmanager.h>
#include <qnetworkreply.h>
#include "sbelevationservicesplugin.h"
#include "qgspoint.h"

class sbElevationServicesGui : public QWidget, private Ui::sbElevationServicesGuiBase
{
    Q_OBJECT

  public:
    sbElevationServicesGui( QgisInterface *pQgisIface, QWidget *parent = nullptr, Qt::WindowFlags fl = nullptr );
    ~sbElevationServicesGui();

  public slots:
    void onDestinationCrsChanged();

    void onMapToolMouseClicked( const QgsPointXY &point );

    void onSettingsTextChanged( const QString &text );
    void onClearResultsBtnPressed();

    void onActivateInfoBtnToggled( bool checked );

    void onMapToolSet( QgsMapTool *newTool, QgsMapTool *oldTool );

    void onNetworkReplyFinished();

    void onClearedProject();

  private:
    QgisInterface            *mpQgisIface      = nullptr;

    QNetworkAccessManager        mNetworkManager;
    QPointer<QNetworkReply>        mpNetworkReply;

    QPointer<QgsRubberBand>        mpRubberBand    = nullptr;
    QPointer<sbElevationServicesMapTool>  mpMapTool      = nullptr;
    QgsCoordinateTransform        mTransform;

    void doInfo( const QgsPointXY &point );
    void doGoogleInfo( const QgsPointXY &point );
    bool processGoogleInfoReply( const QString &strReply );
};

#endif
