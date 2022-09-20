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
#include <SimpleCrypt.h>
#include "sbaddressservicesplugin.h"

class sbAddressServicesGui : public QWidget, private Ui::sbAddressServicesGuiBase
{
    Q_OBJECT

  public:
    sbAddressServicesGui(QgisInterface *pQgisIface, const QString& strPluginName, QWidget *parent = nullptr, Qt::WindowFlags fl = nullptr);
    ~sbAddressServicesGui();

  public slots:
    void onDestinationCrsChanged();

    void onMapToolMouseClicked(const QgsPointXY &point, double dScale);

    void onCurrentTabChanged(int index);

    void onSettingsTextChanged(const QString &text);
    void onClearResultsBtnPressed();

    void onSearchTextReturnPressed();
    void onSearchBtnPressed();
    void onSearchCheckBoxStateChanged(int state);
    void onQueryLevelComboIndexChanged(int index);
    void onActivateInfoBtnToggled(bool checked);
    void onFunnyPlacesComboIndexChanged(int index);
    void onResultsComboIndexChanged(int index);
    void onNavigateToResultBtnPressed();

    void onMapToolSet(QgsMapTool* newTool, QgsMapTool* oldTool);

    void onSslErrors(QNetworkReply* reply, const QList<QSslError>& errors);
    void onNetworkReplyFinished();

    void onClearedProject();
    
  private:
    static SimpleCrypt sCrypto;

    QgisInterface* mpQgisIface = nullptr;
    const QString& mstrPluginName;
    
    QNetworkAccessManager mNetworkManager;
    QPointer<QNetworkReply> mpNetworkReply;

    QPointer<QgsRubberBand> mpRubberBand = nullptr;
    QPointer<sbAddressServicesMapTool> mpMapTool = nullptr;
    QgsCoordinateTransform mTransform;

    void setGuiState(bool bEnabled);

    void doFunnySearch();
    void doSearch(const QString& strText, bool bBypassRegionRestriction);
    void doGoogleSearch(const QString& strText, bool bBypassRegionRestriction, QVariantList& vlSearches);
    void doOsmSearch(const QString& strText, bool bBypassRegionRestriction, QVariantList& vlSearches);
    void doPhotonSearch(const QString& strText, bool bBypassRegionRestriction, QVariantList& vlSearches);
    void doPeliasSearch(const QString& strText, bool bBypassRegionRestriction, QVariantList& vlSearches);
    bool processGoogleSearchReply(const QString& strReply, const QString& strBounds);
    bool processOsmSearchReply(const QString& strReply);
    bool processPhotonSearchReply(const QString& strReply);
    bool processPeliasSearchReply(const QString& strReply);
    void nextSearch(const QString& strText, bool bBypassRegionRestriction, QVariantList& vlSearches);

    void doInfo(const QgsPointXY& point, double dScale);
    void doGoogleInfo(const QgsPointXY& point, double dScale, QVariantList& vlInfos);
    void doOsmInfo(const QgsPointXY& point, double dScale, QVariantList& vlInfos);
    void doPhotonInfo(const QgsPointXY& point, double dScale, QVariantList& vlInfos);
    void doPeliasInfo(const QgsPointXY& point, double dScale, QVariantList& vlInfos);
    bool processGoogleInfoReply(const QString& strReply);
    bool processOsmInfoReply(const QString& strReply);
    bool processPhotonInfoReply(const QString& strReply);
    bool processPeliasInfoReply(const QString& strReply);
    void nextInfo(const QgsPointXY& point, double dScale, QVariantList& vlInfos);

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
