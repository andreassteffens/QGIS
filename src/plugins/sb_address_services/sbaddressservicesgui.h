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

#include "ui_sbaddressservicesguibase.h"

#include <QDockWidget>
#include <qnetworkaccessmanager.h>
#include <qnetworkreply.h>
#include "sbaddressservicesplugin.h"

class sbAddressServicesGui : public QWidget, private Ui::sbAddressServicesGuiBase
{
    Q_OBJECT

  public:
    sbAddressServicesGui(QgisInterface *pQgisIface, QWidget *parent = nullptr, Qt::WindowFlags fl = nullptr);
    ~sbAddressServicesGui();

  public slots:
    void onDestinationCrsChanged();

    void onMapToolMouseClicked(const QgsPointXY &point);

    void onSettingsTextChanged(const QString &text);
    void onClearResultsBtnPressed();

    void onSearchTextReturnPressed();
    void onSearchBtnPressed();
    void onSearchCheckBoxStateChanged(int state);
    void onActivateInfoBtnToggled(bool checked);
    void onFunnyPlacesComboIndexChanged(int index);
    void onResultsComboIndexChanged(int index);
    void onNavigateToResultBtnPressed();

    void onMapToolSet(QgsMapTool* newTool, QgsMapTool* oldTool);

    void onNetworkReplyFinished();

    void onClearedProject();
    
  private:
    QgisInterface*            mpQgisIface      = nullptr;
    
    QNetworkAccessManager        mNetworkManager;
    QPointer<QNetworkReply>        mpNetworkReply;

    QPointer<QgsRubberBand>        mpRubberBand    = nullptr;
    QPointer<sbAddressServicesMapTool>  mpMapTool      = nullptr;
    QgsCoordinateTransform        mTransform;

    void doFunnySearch();
    void doSearch(const QString& strText);
    void doGoogleSearch(const QString& strText);
    void doOsmSearch(const QString& strText);
    bool processGoogleSearchReply(const QString& strReply);
    bool processOsmSearchReply(const QString& strReply);

    void doInfo(const QgsPointXY& point);
    void doGoogleInfo(const QgsPointXY& point);
    void doOsmInfo(const QgsPointXY& point);
    bool processGoogleInfoReply(const QString& strReply);
    bool processOsmInfoReply(const QString& strReply);

  public:
    class AddressDetails
    {
      private:
        QString mstrName;
        QString mstrExtras;
        QgsRectangle mrcBounds;

      public:
        AddressDetails(const QString &strName, const QgsRectangle &rcBounds, const QString &strExtras);

        const QString& getName();
        const QString& getExtras();
        const QgsRectangle& getBounds();

        const QString toJson();

        static AddressDetails fromJson(const QString &strJson);
    };
};

#endif
