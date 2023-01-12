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

#include "sbaddressservicesgui.h"
#include "qgisinterface.h"
#include "qgsguiutils.h"
#include "qgspoint.h"
#include "qgspolygon.h"
#include "qgsmapcanvas.h"
#include "qgis.h"
#include "qgscoordinatereferencesystem.h"
#include "qgscoordinatetransform.h"
#include "qgslogger.h"
#include "qgsmessagelog.h"
#include "qgsrubberband.h"
#include "qgsproject.h"
#include "qgsrectangle.h"
#include "qgspasswordlineedit.h"
#include <SimpleCrypt.h>
#include <qjsonarray.h>
#include <qnetworkreply.h>

#define GOOGLE_ADDRESS_SEARCH_URL "https://maps.google.com/maps/api/geocode/json?sensor=false&address="
#define GOOGLE_ADDRESS_INFO_URL "https://maps.googleapis.com/maps/api/geocode/json?sensor=false&latlng="

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) { if(p) { delete (p); (p)=NULL; } }
#endif

enum sbAddressServiceRequestType
{
  ASRT_INVALID = -1,

  ASRT_GOOGLE_SEARCH = 0,
  ASRT_OSM_SEARCH = 1,
  ASRT_PHOTON_SEARCH = 2,
  ASRT_PELIAS_SEARCH = 3,

  ASRT_GOOGLE_INFO = 4,
  ASRT_OSM_INFO = 5,
  ASRT_PHOTON_INFO = 6,
  ASRT_PELIAS_INFO = 7
};

static double OSM_ZOOM_SCALES[19] = {
  500000000,
  250000000,
  150000000,
   70000000,
   35000000,
   15000000,
   10000000,
    4000000,
    2000000,
    1000000,
     500000,
     250000,
     150000,
      70000,
      35000,
      15000,
       8000,
       4000,
       2000
};

SimpleCrypt sbAddressServicesGui::sCrypto;

sbAddressServicesGui::sbAddressServicesGui(QgisInterface *pQgisIface, const QString& strPluginName, QWidget *parent, Qt::WindowFlags fl)
  : QWidget(parent, fl)
  , mstrPluginName(strPluginName)
{
  mpQgisIface = pQgisIface;

  sCrypto.setKey(Q_UINT64_C(0x0c2ad4a4acb9f023));

  setupUi(this);

  mComboNominatimQueryLevel->addItem(QApplication::translate("sbAddressServicesPlugin", "dynamic"), QVariant((int)-1));
  mComboNominatimQueryLevel->addItem(QApplication::translate("sbAddressServicesPlugin", "0 - Continent/Sea"), QVariant((int)0));
  mComboNominatimQueryLevel->addItem(QApplication::translate("sbAddressServicesPlugin", "1"), QVariant((int)1));
  mComboNominatimQueryLevel->addItem(QApplication::translate("sbAddressServicesPlugin", "2"), QVariant((int)2));
  mComboNominatimQueryLevel->addItem(QApplication::translate("sbAddressServicesPlugin", "3 - County"), QVariant((int)3));
  mComboNominatimQueryLevel->addItem(QApplication::translate("sbAddressServicesPlugin", "4"), QVariant((int)4));
  mComboNominatimQueryLevel->addItem(QApplication::translate("sbAddressServicesPlugin", "5 - State"), QVariant((int)5));
  mComboNominatimQueryLevel->addItem(QApplication::translate("sbAddressServicesPlugin", "6 - Region"), QVariant((int)6));
  mComboNominatimQueryLevel->addItem(QApplication::translate("sbAddressServicesPlugin", "7"), QVariant((int)7));
  mComboNominatimQueryLevel->addItem(QApplication::translate("sbAddressServicesPlugin", "8 - County"), QVariant((int)8));
  mComboNominatimQueryLevel->addItem(QApplication::translate("sbAddressServicesPlugin", "9"), QVariant((int)9));
  mComboNominatimQueryLevel->addItem(QApplication::translate("sbAddressServicesPlugin", "10 - City"), QVariant((int)10));
  mComboNominatimQueryLevel->addItem(QApplication::translate("sbAddressServicesPlugin", "11"), QVariant((int)11));
  mComboNominatimQueryLevel->addItem(QApplication::translate("sbAddressServicesPlugin", "12 - Town/Village"), QVariant((int)12));
  mComboNominatimQueryLevel->addItem(QApplication::translate("sbAddressServicesPlugin", "14"), QVariant((int)13));
  mComboNominatimQueryLevel->addItem(QApplication::translate("sbAddressServicesPlugin", "14 - Suburb"), QVariant((int)14));
  mComboNominatimQueryLevel->addItem(QApplication::translate("sbAddressServicesPlugin", "15"), QVariant((int)15));
  mComboNominatimQueryLevel->addItem(QApplication::translate("sbAddressServicesPlugin", "16 - Street"), QVariant((int)16));
  mComboNominatimQueryLevel->addItem(QApplication::translate("sbAddressServicesPlugin", "17"), QVariant((int)17));
  mComboNominatimQueryLevel->addItem(QApplication::translate("sbAddressServicesPlugin", "18 - Building"), QVariant((int)18));

  setEnabled(true);

  connect(QgsProject::instance(), &QgsProject::cleared, this, &sbAddressServicesGui::onClearedProject);

  QSettings s;

  // search
  mCheckRestrictSearchBounds->setCheckState((Qt::CheckState)s.value("sbAddressServices/SearchRestrictToViewBounds", (int)Qt::CheckState::Checked).toInt());

  connect(mLeSearch, &QLineEdit::returnPressed, this, &sbAddressServicesGui::onSearchTextReturnPressed);
  connect(mTbtnSearch, &QToolButton::pressed, this, &sbAddressServicesGui::onSearchBtnPressed);
  connect(mCheckRestrictSearchBounds, &QCheckBox::stateChanged, this, &sbAddressServicesGui::onSearchCheckBoxStateChanged);

  // info
  mComboNominatimQueryLevel->setCurrentIndex(s.value("sbAddressServices/OsmNominatimQueryLevel", 0).toInt());

  connect(mpQgisIface->mapCanvas(), &QgsMapCanvas::mapToolSet, this, &sbAddressServicesGui::onMapToolSet);
  connect(mpQgisIface->mapCanvas(), &QgsMapCanvas::destinationCrsChanged, this, &sbAddressServicesGui::onDestinationCrsChanged);
  onDestinationCrsChanged();

  mpMapTool = new sbAddressServicesMapTool(mpQgisIface->mapCanvas());
  connect(mpMapTool, &sbAddressServicesMapTool::mouseClicked, this, &sbAddressServicesGui::onMapToolMouseClicked);

  connect(mPbtnActivateInfo, &QPushButton::toggled, this, &sbAddressServicesGui::onActivateInfoBtnToggled);
  connect(mComboNominatimQueryLevel, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &sbAddressServicesGui::onQueryLevelComboIndexChanged);

  // funny
  connect(mComboFunnyPlaces, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &sbAddressServicesGui::onFunnyPlacesComboIndexChanged);


  // settings
  mCheckGoogleActive->setCheckState((Qt::CheckState)s.value("sbAddressServices/GoogleActive", (int)Qt::CheckState::Unchecked).toInt());
  QString strKey = s.value(QStringLiteral("sbAddressServices/GoogleApiKey"), "").toString();
  if (!strKey.isEmpty())
    strKey = sCrypto.decryptToString(strKey);
  mPleGoogleKey->setText(strKey);

  mLeGoogleQueryOptions->setText(s.value(QStringLiteral("sbAddressServices/GoogleQueryOptions"), "").toString());

  mCheckNominatimActive->setCheckState((Qt::CheckState)s.value("sbAddressServices/NominatimActive", (int)Qt::CheckState::Unchecked).toInt());
  mLeNominatimService->setText(s.value(QStringLiteral("sbAddressServices/OsmNominatimService"), "https://tileserver.gis24.eu/nominatim").toString());
  mLeNominatimQueryOptions->setText(s.value(QStringLiteral("sbAddressServices/OsmNominatimQueryOptions"), "").toString());

  mCheckPhotonActive->setCheckState((Qt::CheckState)s.value("sbAddressServices/PhotonActive", (int)Qt::CheckState::Unchecked).toInt());
  mLePhotonService->setText(s.value(QStringLiteral("sbAddressServices/PhotonService"), "").toString());
  mLePhotonQueryOptions->setText(s.value(QStringLiteral("sbAddressServices/PhotonQueryOptions"), "").toString());

  mCheckPeliasActive->setCheckState((Qt::CheckState)s.value("sbAddressServices/PeliasActive", (int)Qt::CheckState::Unchecked).toInt());
  mLePeliasService->setText(s.value(QStringLiteral("sbAddressServices/PeliasService"), "").toString());
  mLePeliasQueryOptions->setText(s.value(QStringLiteral("sbAddressServices/PeliasQueryOptions"), "").toString());
  mCheckDebugMode->setCheckState((Qt::CheckState)s.value("sbAddressServices/DebugMode", (int)Qt::CheckState::Checked).toInt());

  connect(mCheckGoogleActive, &QCheckBox::stateChanged, this, &sbAddressServicesGui::onSearchCheckBoxStateChanged);
  connect(mPleGoogleKey, &QgsPasswordLineEdit::textChanged, this, &sbAddressServicesGui::onSettingsTextChanged);
  connect(mLeGoogleQueryOptions, &QLineEdit::textChanged, this, &sbAddressServicesGui::onSettingsTextChanged);
  connect(mCheckNominatimActive, &QCheckBox::stateChanged, this, &sbAddressServicesGui::onSearchCheckBoxStateChanged);
  connect(mLeNominatimService, &QLineEdit::textChanged, this, &sbAddressServicesGui::onSettingsTextChanged);
  connect(mLeNominatimQueryOptions, &QLineEdit::textChanged, this, &sbAddressServicesGui::onSettingsTextChanged);
  connect(mCheckPhotonActive, &QCheckBox::stateChanged, this, &sbAddressServicesGui::onSearchCheckBoxStateChanged);
  connect(mLePhotonService, &QLineEdit::textChanged, this, &sbAddressServicesGui::onSettingsTextChanged);
  connect(mLePhotonQueryOptions, &QLineEdit::textChanged, this, &sbAddressServicesGui::onSettingsTextChanged);
  connect(mCheckPeliasActive, &QCheckBox::stateChanged, this, &sbAddressServicesGui::onSearchCheckBoxStateChanged);
  connect(mLePeliasService, &QLineEdit::textChanged, this, &sbAddressServicesGui::onSettingsTextChanged);
  connect(mLePeliasQueryOptions, &QLineEdit::textChanged, this, &sbAddressServicesGui::onSettingsTextChanged);
  connect(mCheckDebugMode, &QCheckBox::stateChanged, this, &sbAddressServicesGui::onSearchCheckBoxStateChanged);


  // general
  mTabsServices->setCurrentIndex(s.value("sbAddressServices/CurrentTabIndex", 0).toInt());

  connect(mPbtnClearResults, &QPushButton::pressed, this, &sbAddressServicesGui::onClearResultsBtnPressed);
  connect(mComboResults, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &sbAddressServicesGui::onResultsComboIndexChanged);
  connect(mPbtnNavigateToResult, &QPushButton::pressed, this, &sbAddressServicesGui::onNavigateToResultBtnPressed);
  connect(mTabsServices, &QTabWidget::currentChanged, this, &sbAddressServicesGui::onCurrentTabChanged);

  mpRubberBand = new QgsRubberBand(mpQgisIface->mapCanvas(), QgsWkbTypes::PolygonGeometry);
  mpRubberBand->setColor(Qt::blue);
  mpRubberBand->setWidth(2);
  mpRubberBand->setFillColor(QColor::fromRgba(qRgba(255, 0, 0, 128)));


  // network manager
  connect(&mNetworkManager, &QNetworkAccessManager::sslErrors, this, &sbAddressServicesGui::onSslErrors);
}

sbAddressServicesGui::~sbAddressServicesGui()
{
  if (mpMapTool)
  {
    mpMapTool->deactivate();
    delete mpMapTool;
  }

  if (mpRubberBand)
    delete mpRubberBand;
  
  mpQgisIface = nullptr;
}

void sbAddressServicesGui::onClearedProject()
{
  onClearResultsBtnPressed();
}

void sbAddressServicesGui::onMapToolSet(QgsMapTool* newTool, QgsMapTool* oldTool)
{
  if (newTool != mpMapTool.data())
    mPbtnActivateInfo->setChecked(false);
}

void sbAddressServicesGui::onMapToolMouseClicked(const QgsPointXY &point, double dScale)
{
  QgsPointXY ptTransformed = mTransform.transform(point);

  if (mpNetworkReply)
  {
    mpNetworkReply->abort();
    mpNetworkReply.clear();
  }

  doInfo(ptTransformed, dScale);
}

void sbAddressServicesGui::onFunnyPlacesComboIndexChanged(int index)
{
  if (index >= 0)
    doFunnySearch();
}

void sbAddressServicesGui::onDestinationCrsChanged()
{
  mTransform.setSourceCrs(mpQgisIface->mapCanvas()->mapSettings().destinationCrs());
  
  QgsCoordinateReferenceSystem crsWgs84 = QgsCoordinateReferenceSystem::fromOgcWmsCrs(geoEpsgCrsAuthId());
  mTransform.setDestinationCrs(crsWgs84);
}

void sbAddressServicesGui::onCurrentTabChanged(int index)
{
  QSettings s;

  s.setValue("sbAddressServices/CurrentTabIndex", QVariant(index));
}

void sbAddressServicesGui::onQueryLevelComboIndexChanged(int index)
{
  QSettings s;

  s.setValue("sbAddressServices/OsmNominatimQueryLevel", QVariant(mComboNominatimQueryLevel->currentIndex()));
}

void sbAddressServicesGui::onSettingsTextChanged(const QString &text)
{
  QSettings s;

  QString strKey = mPleGoogleKey->text();
  if (!strKey.isEmpty())
    strKey = sCrypto.encryptToString(strKey);
  s.setValue("sbAddressServices/GoogleApiKey", QVariant(strKey));

  s.setValue("sbAddressServices/GoogleQueryOptions", QVariant(mLeGoogleQueryOptions->text()));
  s.setValue("sbAddressServices/OsmNominatimService", QVariant(mLeNominatimService->text()));
  s.setValue("sbAddressServices/OsmNominatimQueryOptions", QVariant(mLeNominatimQueryOptions->text()));
  s.setValue("sbAddressServices/PhotonService", QVariant(mLePhotonService->text()));
  s.setValue("sbAddressServices/PhotonQueryOptions", QVariant(mLePhotonQueryOptions->text()));
  s.setValue("sbAddressServices/PeliasService", QVariant(mLePeliasService->text()));
  s.setValue("sbAddressServices/PeliasQueryOptions", QVariant(mLePeliasQueryOptions->text()));
}

void sbAddressServicesGui::onSearchCheckBoxStateChanged(int state)
{
  QSettings s;

  s.setValue("sbAddressServices/GoogleActive", QVariant((int)mCheckGoogleActive->checkState()));
  s.setValue("sbAddressServices/NominatimActive", QVariant((int)mCheckNominatimActive->checkState()));
  s.setValue("sbAddressServices/PhotonActive", QVariant((int)mCheckPhotonActive->checkState()));
  s.setValue("sbAddressServices/PeliasActive", QVariant((int)mCheckPeliasActive->checkState()));
  s.setValue("sbAddressServices/SearchRestrictToViewBounds", QVariant((int)mCheckRestrictSearchBounds->checkState()));
  s.setValue("sbAddressServices/DebugMode", QVariant((int)mCheckDebugMode->checkState()));
}

void sbAddressServicesGui::onClearResultsBtnPressed()
{
  mComboResults->clear();
  mPteResult->clear();

  mpRubberBand->reset(QgsWkbTypes::PolygonGeometry);
}

void sbAddressServicesGui::onSearchTextReturnPressed()
{
  doSearch(mLeSearch->text(), false);
}

void sbAddressServicesGui::onSearchBtnPressed()
{
  doSearch(mLeSearch->text(), false);
}

void sbAddressServicesGui::onResultsComboIndexChanged(int index)
{
  mPbtnNavigateToResult->setEnabled(index >= 0);
  mPbtnClearResults->setEnabled(index >= 0);

  if (index < 0)
  {
    onClearResultsBtnPressed();
    return;
  }
    
  QVariant varData = mComboResults->itemData(index);
  if (varData.type() == QVariant::String)
  {
    AddressDetails addr = AddressDetails::fromJson(varData.toString());
    if (!addr.getExtras().isEmpty())
      mPteResult->setPlainText(addr.getName() + "\n\n" + addr.getExtras());
    else
      mPteResult->setPlainText(addr.getName());

    const QgsRectangle rcBounds = addr.getBounds();
    if (!rcBounds.isEmpty())
    {
      QgsRectangle rcBoundsTransformed = mTransform.transformBoundingBox(rcBounds, Qgis::TransformDirection::Reverse);

      mpRubberBand->reset(QgsWkbTypes::PolygonGeometry);
      mpRubberBand->addGeometry(QgsGeometry::fromRect(rcBoundsTransformed), mTransform.sourceCrs());
      mpRubberBand->show();
    }
  }
}

void sbAddressServicesGui::onNavigateToResultBtnPressed()
{
  int index = mComboResults->currentIndex();
  if (index < 0)
  {
    onClearResultsBtnPressed();
    return;
  }

  QVariant varData = mComboResults->itemData(index);
  if (varData.type() == QVariant::String)
  {
    AddressDetails addr = AddressDetails::fromJson(varData.toString());
    if (!addr.getExtras().isEmpty())
      mPteResult->setPlainText(addr.getName() + "\n\n" + addr.getExtras());
    else
      mPteResult->setPlainText(addr.getName());

    const QgsRectangle rcBounds = addr.getBounds();
    if (!rcBounds.isEmpty())
    {
      QgsRectangle rcBoundsTransformed = mTransform.transformBoundingBox(rcBounds, Qgis::TransformDirection::Reverse);

      mpQgisIface->mapCanvas()->setExtent(rcBoundsTransformed);
      mpQgisIface->mapCanvas()->refresh();
    }
  }
}

void sbAddressServicesGui::onActivateInfoBtnToggled(bool checked)
{
  if (mpMapTool.isNull())
    return;

  if (!checked)
    mpQgisIface->mapCanvas()->unsetMapTool(mpMapTool);
  else
    mpQgisIface->mapCanvas()->setMapTool(mpMapTool);
}

void sbAddressServicesGui::setGuiState(bool bEnabled)
{
  mTabsServices->setEnabled(bEnabled);
  mComboResults->setEnabled(bEnabled);

  //mPbtnNavigateToResult->setEnabled(bEnabled);
  //mPbtnClearResults->setEnabled(bEnabled);
}

void sbAddressServicesGui::doSearch(const QString& strText, bool bBypassRegionRestriction)
{
  if (strText.isEmpty() || strText.isNull())
    return;

  onClearResultsBtnPressed();

  if (mpNetworkReply)
  {
    mpNetworkReply->abort();
    mpNetworkReply.clear();
  }

  QVariantList vlSearches;

  if (mCheckGoogleActive->checkState() == Qt::CheckState::Checked && !mPleGoogleKey->text().isEmpty())
    vlSearches.append(QVariant(ASRT_GOOGLE_SEARCH));

  if (mCheckNominatimActive->checkState() == Qt::CheckState::Checked && !mLeNominatimService->text().isEmpty())
    vlSearches.append(QVariant(ASRT_OSM_SEARCH));

  if (mCheckPhotonActive->checkState() == Qt::CheckState::Checked && !mLePhotonService->text().isEmpty())
    vlSearches.append(QVariant(ASRT_PHOTON_SEARCH));

  if (mCheckPeliasActive->checkState() == Qt::CheckState::Checked && !mLePeliasService->text().isEmpty())
    vlSearches.append(QVariant(ASRT_PELIAS_SEARCH));

  if (vlSearches.isEmpty())
  {
    mPteResult->setPlainText(QApplication::translate("sbAddressServicesPlugin", "No active/configured search provider(s)!"));
    return;
  }

  nextSearch(strText, bBypassRegionRestriction, vlSearches);
}

void sbAddressServicesGui::nextSearch(const QString& strText, bool bBypassRegionRestriction, QVariantList& vlSearches)
{
  if (vlSearches.count() == 0)
  {
    if (mComboResults->count() == 0)
    {
      QString strText = QApplication::translate("sbAddressServicesPlugin", "No results!");
      if (mCheckRestrictSearchBounds->isChecked())
        strText += "\n\n" + QApplication::translate("sbAddressServicesPlugin", "Try to get more results through disabling the region restriction!");

      mPteResult->setPlainText(strText);
    }
    else
      onResultsComboIndexChanged(mComboResults->currentIndex());
  }
  else
  {
    QVariant varSearchId = vlSearches.front();
    vlSearches.pop_front();

    sbAddressServiceRequestType enType = (sbAddressServiceRequestType)varSearchId.toInt();
    switch (enType)
    {
      case ASRT_GOOGLE_SEARCH:
        doGoogleSearch(strText, bBypassRegionRestriction, vlSearches);
        break;
      case ASRT_OSM_SEARCH:
        doOsmSearch(strText, bBypassRegionRestriction, vlSearches);
        break;
      case ASRT_PHOTON_SEARCH:
        doPhotonSearch(strText, bBypassRegionRestriction, vlSearches);
        break;
      case ASRT_PELIAS_SEARCH:
        doPeliasSearch(strText, bBypassRegionRestriction, vlSearches);
        break;
    }
  }
}

void sbAddressServicesGui::doFunnySearch()
{
  if (mComboFunnyPlaces->currentIndex() < 0)
    return;

  doSearch(mComboFunnyPlaces->currentText(), true);
}

void sbAddressServicesGui::doGoogleSearch(const QString& strText, bool bBypassRegionRestriction, QVariantList& vlSearches)
{
  QString currentLocale = QLocale().name();

  QString strUrl = GOOGLE_ADDRESS_SEARCH_URL;
  strUrl += QUrl::toPercentEncoding(strText);
  strUrl += "&language=" + QLocale().name();
  strUrl += "&key=" + mPleGoogleKey->text();

  QString strWktBounds;
  if (!bBypassRegionRestriction)
  {
    if (mCheckRestrictSearchBounds->checkState() == Qt::CheckState::Checked)
    {
      QgsRectangle rcMapExtent = mpQgisIface->mapCanvas()->extent();
      QgsRectangle rcMapExtentTransformed = mTransform.transformBoundingBox(rcMapExtent, Qgis::TransformDirection::Forward);
      strWktBounds = rcMapExtentTransformed.asWktPolygon();

      strUrl += "&bounds=" + QString::number(rcMapExtentTransformed.xMinimum()) + "," + QString::number(rcMapExtentTransformed.yMinimum()) + "|" + QString::number(rcMapExtentTransformed.xMaximum()) + "," + QString::number(rcMapExtentTransformed.yMaximum());
    }
  }
  
  QString strQueryOptions = mLeGoogleQueryOptions->text();
  if (!strQueryOptions.isEmpty() && !strQueryOptions.isNull())
  {
    if (!strQueryOptions.startsWith("&"))
      strUrl += "&";

    strUrl += strQueryOptions;
  }

  if (mCheckDebugMode->checkState() == Qt::CheckState::Checked)
  {
    QgsMessageLog::logMessage( QStringLiteral( "Google Address Search URL: %1" ).arg( strUrl ), QApplication::translate( "sbAddressServicesPlugin", "[a]tapa Address Services" ), Qgis::MessageLevel::Info );
  }

  QNetworkRequest rq;
  rq.setUrl(QUrl(strUrl));
  rq.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, mstrPluginName);
  
  SAFE_DELETE(mpNetworkReply);

  mpNetworkReply = mNetworkManager.get(rq);
  mpNetworkReply->setProperty("REQUEST_TYPE", ASRT_GOOGLE_SEARCH);
  mpNetworkReply->setProperty("QUERY", strText);
  mpNetworkReply->setProperty("SEARCHES", vlSearches);
  if( !strWktBounds.isEmpty() )
    mpNetworkReply->setProperty("BOUNDS", strWktBounds);

  
  connect(mpNetworkReply, &QNetworkReply::finished, this, &sbAddressServicesGui::onNetworkReplyFinished);
  
  mPteResult->setPlainText(QApplication::translate("sbAddressServicesPlugin", "Searching..."));
}

void sbAddressServicesGui::doOsmSearch(const QString& strText, bool bBypassRegionRestriction, QVariantList& vlSearches)
{
  QString strUrl = mLeNominatimService->text();
  if (strUrl.isEmpty() || strUrl.isNull())
    return;

  QString strLocale = QLocale().name();
  if(strLocale.contains("_"))
  {
    QStringList listParts = strLocale.split("_");
    strLocale = listParts[0];
  }

  if (!strUrl.endsWith('/'))
    strUrl += "/";
  strUrl += "search?q=" + QUrl::toPercentEncoding(strText);
  strUrl += "&accept-language=" + strLocale + ",en";
  strUrl += "&format=json&polygon=0&addressdetails=0&extratags=1";

  QString strWktBounds;
  if (!bBypassRegionRestriction)
  {
    if (mCheckRestrictSearchBounds->checkState() == Qt::CheckState::Checked)
    {
      QgsRectangle rcMapExtent = mpQgisIface->mapCanvas()->extent();
      QgsRectangle rcMapExtentTransformed = mTransform.transformBoundingBox(rcMapExtent, Qgis::TransformDirection::Forward);
      strWktBounds = rcMapExtentTransformed.asWktPolygon();

      strUrl += "&viewbox=" + QString::number(rcMapExtentTransformed.xMinimum()) + "," + QString::number(rcMapExtentTransformed.yMinimum()) + "," + QString::number(rcMapExtentTransformed.xMaximum()) + "," + QString::number(rcMapExtentTransformed.yMaximum());
      strUrl += "&bounded=1";
    }
  }
  
  QString strQueryOptions = mLeNominatimQueryOptions->text();
  if (!strQueryOptions.isEmpty() && !strQueryOptions.isNull())
  {
    if (!strQueryOptions.startsWith("&"))
      strUrl += "&";

    strUrl += strQueryOptions;
  }

  if (mCheckDebugMode->checkState() == Qt::CheckState::Checked)
  {
    QgsMessageLog::logMessage( QStringLiteral( "OpenStreetMap Nominatim Search URL: %1" ).arg( strUrl ), QApplication::translate( "sbAddressServicesPlugin", "[a]tapa Address Services" ), Qgis::MessageLevel::Info );
  }

  QNetworkRequest rq;
  rq.setUrl(QUrl(strUrl));
  rq.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, mstrPluginName);

  SAFE_DELETE(mpNetworkReply);

  mpNetworkReply = mNetworkManager.get(rq);
  mpNetworkReply->setProperty("REQUEST_TYPE", ASRT_OSM_SEARCH);
  mpNetworkReply->setProperty("QUERY", strText);
  mpNetworkReply->setProperty("SEARCHES", vlSearches);
  if (!strWktBounds.isEmpty())
    mpNetworkReply->setProperty("BOUNDS", strWktBounds);

  connect(mpNetworkReply, &QNetworkReply::finished, this, &sbAddressServicesGui::onNetworkReplyFinished);

  mPteResult->setPlainText(QApplication::translate("sbAddressServicesPlugin", "Searching..."));
}

void sbAddressServicesGui::doPhotonSearch(const QString& strText, bool bBypassRegionRestriction, QVariantList& vlSearches)
{
  QString strUrl = mLePhotonService->text();
  if (strUrl.isEmpty() || strUrl.isNull())
    return;

  QString strLocale = QLocale().name();
  if (strLocale.contains("_"))
  {
    QStringList listParts = strLocale.split("_");
    strLocale = listParts[0];
  }

  if (!strUrl.endsWith('/'))
    strUrl += "/";
  strUrl += "api?q=" + QUrl::toPercentEncoding(strText);
  strUrl += "&lang=" + strLocale;

  QString strWktBounds;
  if (!bBypassRegionRestriction)
  {
    if (mCheckRestrictSearchBounds->checkState() == Qt::CheckState::Checked)
    {
      QgsRectangle rcMapExtent = mpQgisIface->mapCanvas()->extent();
      QgsRectangle rcMapExtentTransformed = mTransform.transformBoundingBox(rcMapExtent, Qgis::TransformDirection::Forward);
      strWktBounds = rcMapExtentTransformed.asWktPolygon();

      strUrl += "&bbox=" + QString::number(rcMapExtentTransformed.xMinimum()) + "," + QString::number(rcMapExtentTransformed.yMinimum()) + "," + QString::number(rcMapExtentTransformed.xMaximum()) + "," + QString::number(rcMapExtentTransformed.yMaximum());
    }
  }

  QString strQueryOptions = mLePhotonQueryOptions->text();
  if (!strQueryOptions.isEmpty() && !strQueryOptions.isNull())
  {
    if (!strQueryOptions.startsWith("&"))
      strUrl += "&";

    strUrl += strQueryOptions;
  }

  if (mCheckDebugMode->checkState() == Qt::CheckState::Checked)
  {
    QgsMessageLog::logMessage(QStringLiteral("Photon Search URL: %1").arg(strUrl), QApplication::translate("sbAddressServicesPlugin", "[a]tapa Address Services"), Qgis::MessageLevel::Info);
  }

  QNetworkRequest rq;
  rq.setUrl(QUrl(strUrl));
  rq.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, mstrPluginName);

  SAFE_DELETE(mpNetworkReply);

  mpNetworkReply = mNetworkManager.get(rq);
  mpNetworkReply->setProperty("REQUEST_TYPE", ASRT_PHOTON_SEARCH);
  mpNetworkReply->setProperty("QUERY", strText);
  mpNetworkReply->setProperty("SEARCHES", vlSearches);
  if (!strWktBounds.isEmpty())
    mpNetworkReply->setProperty("BOUNDS", strWktBounds);

  connect(mpNetworkReply, &QNetworkReply::finished, this, &sbAddressServicesGui::onNetworkReplyFinished);

  mPteResult->setPlainText(QApplication::translate("sbAddressServicesPlugin", "Searching..."));
}

void sbAddressServicesGui::doPeliasSearch(const QString& strText, bool bBypassRegionRestriction, QVariantList& vlSearches)
{
  QString strUrl = mLePeliasService->text();
  if (strUrl.isEmpty() || strUrl.isNull())
    return;

  QString strLocale = QLocale().name();
  if (strLocale.contains("_"))
  {
    QStringList listParts = strLocale.split("_");
    strLocale = listParts[0];
  }

  if (!strUrl.endsWith('/'))
    strUrl += "/";
  strUrl += "v1/search?text=" + QUrl::toPercentEncoding(strText);
  strUrl += "&lang=" + strLocale;

  QString strWktBounds;
  if (!bBypassRegionRestriction)
  {
    if (mCheckRestrictSearchBounds->checkState() == Qt::CheckState::Checked)
    {
      QgsRectangle rcMapExtent = mpQgisIface->mapCanvas()->extent();
      QgsRectangle rcMapExtentTransformed = mTransform.transformBoundingBox(rcMapExtent, Qgis::TransformDirection::Forward);
      strWktBounds = rcMapExtentTransformed.asWktPolygon();

      strUrl += "&boundary.rect.min_lon=" + QString::number(rcMapExtentTransformed.xMinimum()) + "&boundary.rect.min_lat=" + QString::number(rcMapExtentTransformed.yMinimum()) + "&boundary.rect.max_lon=" + QString::number(rcMapExtentTransformed.xMaximum()) + "&boundary.rect.max_lat=" + QString::number(rcMapExtentTransformed.yMaximum());
    }
  }

  QString strQueryOptions = mLePeliasQueryOptions->text();
  if (!strQueryOptions.isEmpty() && !strQueryOptions.isNull())
  {
    if (!strQueryOptions.startsWith("&"))
      strUrl += "&";

    strUrl += strQueryOptions;
  }

  if (mCheckDebugMode->checkState() == Qt::CheckState::Checked)
  {
    QgsMessageLog::logMessage(QStringLiteral("Pelias Search URL: %1").arg(strUrl), QApplication::translate("sbAddressServicesPlugin", "[a]tapa Address Services"), Qgis::MessageLevel::Info);
  }

  QNetworkRequest rq;
  rq.setUrl(QUrl(strUrl));
  rq.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, mstrPluginName);

  SAFE_DELETE(mpNetworkReply);

  mpNetworkReply = mNetworkManager.get(rq);
  mpNetworkReply->setProperty("REQUEST_TYPE", ASRT_PELIAS_SEARCH);
  mpNetworkReply->setProperty("QUERY", strText);
  mpNetworkReply->setProperty("SEARCHES", vlSearches);
  if (!strWktBounds.isEmpty())
    mpNetworkReply->setProperty("BOUNDS", strWktBounds);

  connect(mpNetworkReply, &QNetworkReply::finished, this, &sbAddressServicesGui::onNetworkReplyFinished);

  mPteResult->setPlainText(QApplication::translate("sbAddressServicesPlugin", "Searching..."));
}

void sbAddressServicesGui::doInfo(const QgsPointXY& point, double dScale)
{
  onClearResultsBtnPressed();

  QVariantList vlInfos;

  if (mCheckGoogleActive->checkState() == Qt::CheckState::Checked && !mPleGoogleKey->text().isEmpty())
    vlInfos.append(QVariant(ASRT_GOOGLE_INFO));

  if (mCheckNominatimActive->checkState() == Qt::CheckState::Checked && !mLeNominatimService->text().isEmpty())
    vlInfos.append(QVariant(ASRT_OSM_INFO));

  if (mCheckPhotonActive->checkState() == Qt::CheckState::Checked && !mLePhotonService->text().isEmpty())
    vlInfos.append(QVariant(ASRT_PHOTON_INFO));

  if (mCheckPeliasActive->checkState() == Qt::CheckState::Checked && !mLePeliasService->text().isEmpty())
    vlInfos.append(QVariant(ASRT_PELIAS_INFO));

  if(vlInfos.isEmpty())
  {
    mPteResult->setPlainText(QApplication::translate("sbAddressServicesPlugin", "No active/configured search provider(s)!"));
    return;
  }

  nextInfo(point, dScale, vlInfos);
}

void sbAddressServicesGui::nextInfo(const QgsPointXY& point, double dScale, QVariantList& vlInfos)
{
  if (vlInfos.count() == 0)
  {
    if (mComboResults->count() > 0)
      onResultsComboIndexChanged(mComboResults->currentIndex());
  }
  else
  {
    QVariant varSearchId = vlInfos.front();
    vlInfos.pop_front();

    sbAddressServiceRequestType enType = (sbAddressServiceRequestType)varSearchId.toInt();
    switch (enType)
    {
      case ASRT_GOOGLE_INFO:
        doGoogleInfo(point, dScale, vlInfos);
        break;
      case ASRT_OSM_INFO:
        doOsmInfo(point, dScale, vlInfos);
        break;
      case ASRT_PHOTON_INFO:
        doPhotonInfo(point, dScale, vlInfos);
        break;
      case ASRT_PELIAS_INFO:
        doPeliasInfo(point, dScale, vlInfos);
        break;
    }
  }
}

void sbAddressServicesGui::doGoogleInfo(const QgsPointXY& point, double dScale, QVariantList& vlInfos)
{
  QString currentLocale = QLocale().name();

  QString strUrl = GOOGLE_ADDRESS_INFO_URL;
  strUrl += QString::number(point.y()) + "," + QString::number(point.x());
  strUrl += "&language=" + currentLocale;
  strUrl += "&key=" + mPleGoogleKey->text();

  QNetworkRequest rq;
  rq.setUrl(QUrl(strUrl));
  rq.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, mstrPluginName);

  if (mCheckDebugMode->checkState() == Qt::CheckState::Checked)
  {
    QgsMessageLog::logMessage( QStringLiteral( "Google Address Search URL: %1" ).arg( strUrl ), QApplication::translate( "sbAddressServicesPlugin", "[a]tapa Address Services" ), Qgis::MessageLevel::Info );
  }

  SAFE_DELETE(mpNetworkReply);

  mpNetworkReply = mNetworkManager.get(rq);
  mpNetworkReply->setProperty("REQUEST_TYPE", ASRT_GOOGLE_INFO);
  mpNetworkReply->setProperty("QUERY", point.asWkt());
  mpNetworkReply->setProperty("SCALE", dScale);
  mpNetworkReply->setProperty("INFOS", vlInfos);

  connect(mpNetworkReply, &QNetworkReply::finished, this, &sbAddressServicesGui::onNetworkReplyFinished);

  mPteResult->setPlainText(QApplication::translate("sbAddressServicesPlugin", "Searching..."));
}

void sbAddressServicesGui::doOsmInfo(const QgsPointXY& point, double dScale, QVariantList& vlInfos)
{
  QString strUrl = mLeNominatimService->text();
  if (strUrl.isEmpty() || strUrl.isNull())
    return;

  QVariant varZoomLevel = mComboNominatimQueryLevel->itemData(mComboNominatimQueryLevel->currentIndex());
  int iZoomLevel = varZoomLevel.toInt();
  if (iZoomLevel == -1)
  {
    for (int iLevel = 0; iLevel < 19; iLevel++)
    {
      double dZoomScale = OSM_ZOOM_SCALES[iLevel];

      iZoomLevel = iLevel;

      if (dScale >= dZoomScale)
        break;
    }
  }

  QString strLocale = QLocale().name();
  if (strLocale.contains("_"))
  {
    QStringList listParts = strLocale.split("_");
    strLocale = listParts[0];
  }

  if (!strUrl.endsWith('/'))
    strUrl += "/";
  strUrl += "reverse?q=";
  strUrl += "&accept-language=" + strLocale + ",en";
  strUrl += "&format=json&zoom=" + QString::number(iZoomLevel) + "&addressdetails=0&extratags=1";
  strUrl += "&lat=" + QString::number(point.y()) + "&lon=" + QString::number(point.x());

  QNetworkRequest rq;
  rq.setUrl(QUrl(strUrl));
  rq.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, mstrPluginName);

  if (mCheckDebugMode->checkState() == Qt::CheckState::Checked)
  {
    QgsMessageLog::logMessage( QStringLiteral( "OpenStreetMap Nominatim Info URL: %1" ).arg( strUrl ), QApplication::translate( "sbAddressServicesPlugin", "[a]tapa Address Services" ), Qgis::MessageLevel::Info );
  }

  SAFE_DELETE(mpNetworkReply);

  mpNetworkReply = mNetworkManager.get(rq);
  mpNetworkReply->setProperty("REQUEST_TYPE", ASRT_OSM_INFO);
  mpNetworkReply->setProperty("QUERY", point.asWkt());
  mpNetworkReply->setProperty("INFOS", vlInfos);

  connect(mpNetworkReply, &QNetworkReply::finished, this, &sbAddressServicesGui::onNetworkReplyFinished);

  mPteResult->setPlainText(QApplication::translate("sbAddressServicesPlugin", "Searching..."));
}

void sbAddressServicesGui::doPhotonInfo(const QgsPointXY& point, double dScale, QVariantList& vlInfos)
{
  QString strUrl = mLePhotonService->text();
  if (strUrl.isEmpty() || strUrl.isNull())
    return;

  QString strLocale = QLocale().name();
  if (strLocale.contains("_"))
  {
    QStringList listParts = strLocale.split("_");
    strLocale = listParts[0];
  }

  if (!strUrl.endsWith('/'))
    strUrl += "/";
  strUrl += "reverse";
  strUrl += "?lang=" + strLocale;
  strUrl += "&lat=" + QString::number(point.y()) + "&lon=" + QString::number(point.x());

  QNetworkRequest rq;
  rq.setUrl(QUrl(strUrl));
  rq.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, mstrPluginName);

  if (mCheckDebugMode->checkState() == Qt::CheckState::Checked)
  {
    QgsMessageLog::logMessage(QStringLiteral("Photon Info URL: %1").arg(strUrl), QApplication::translate("sbAddressServicesPlugin", "[a]tapa Address Services"), Qgis::MessageLevel::Info);
  }

  SAFE_DELETE(mpNetworkReply);

  mpNetworkReply = mNetworkManager.get(rq);
  mpNetworkReply->setProperty("REQUEST_TYPE", ASRT_PHOTON_INFO);
  mpNetworkReply->setProperty("QUERY", point.asWkt());
  mpNetworkReply->setProperty("INFOS", vlInfos);

  connect(mpNetworkReply, &QNetworkReply::finished, this, &sbAddressServicesGui::onNetworkReplyFinished);

  mPteResult->setPlainText(QApplication::translate("sbAddressServicesPlugin", "Searching..."));
}

void sbAddressServicesGui::doPeliasInfo(const QgsPointXY& point, double dScale, QVariantList& vlInfos)
{
  QString strUrl = mLePeliasService->text();
  if (strUrl.isEmpty() || strUrl.isNull())
    return;

  QString strLocale = QLocale().name();
  if (strLocale.contains("_"))
  {
    QStringList listParts = strLocale.split("_");
    strLocale = listParts[0];
  }

  if (!strUrl.endsWith('/'))
    strUrl += "/";
  strUrl += "v1/reverse";
  strUrl += "?lang=" + strLocale;
  strUrl += "&point.lat=" + QString::number(point.y()) + "&point.lon=" + QString::number(point.x());

  QNetworkRequest rq;
  rq.setUrl(QUrl(strUrl));
  rq.setHeader(QNetworkRequest::KnownHeaders::UserAgentHeader, mstrPluginName);

  if (mCheckDebugMode->checkState() == Qt::CheckState::Checked)
  {
    QgsMessageLog::logMessage(QStringLiteral("Pelias Info URL: %1").arg(strUrl), QApplication::translate("sbAddressServicesPlugin", "[a]tapa Address Services"), Qgis::MessageLevel::Info);
  }

  SAFE_DELETE(mpNetworkReply);

  mpNetworkReply = mNetworkManager.get(rq);
  mpNetworkReply->setProperty("REQUEST_TYPE", ASRT_PELIAS_INFO);
  mpNetworkReply->setProperty("QUERY", point.asWkt());
  mpNetworkReply->setProperty("INFOS", vlInfos);

  connect(mpNetworkReply, &QNetworkReply::finished, this, &sbAddressServicesGui::onNetworkReplyFinished);

  mPteResult->setPlainText(QApplication::translate("sbAddressServicesPlugin", "Searching..."));
}

void sbAddressServicesGui::onSslErrors(QNetworkReply* reply, const QList<QSslError>& errors)
{
  reply->ignoreSslErrors(errors);

  if (mCheckDebugMode->checkState() == Qt::CheckState::Checked)
  {
    foreach(const QSslError &error, errors)
    {
      QgsMessageLog::logMessage(QStringLiteral("SSL error for request '%1': %2").arg(reply->request().url().toString()).arg(error.errorString()), QApplication::translate("sbAddressServicesPlugin", "[a]tapa Address Services"), Qgis::MessageLevel::Warning);
    }
  }
}

void sbAddressServicesGui::onNetworkReplyFinished()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
  if (!reply)
    return;

  QVariant varType = reply->property("REQUEST_TYPE");
  if (varType.isNull())
    return;

  sbAddressServiceRequestType enType = (sbAddressServiceRequestType)varType.toInt();;

  QString strType = "";
  switch (enType)
  {
    case ASRT_GOOGLE_SEARCH:
    case ASRT_GOOGLE_INFO:
      strType = "Google";
      break;
    case ASRT_OSM_SEARCH:
    case ASRT_OSM_INFO:
      strType = "OSM";
      break;
    case ASRT_PHOTON_SEARCH:
    case ASRT_PHOTON_INFO:
      strType = "Photon";
      break;
    case ASRT_PELIAS_SEARCH:
    case ASRT_PELIAS_INFO:
      strType = "Pelias";
      break;
  }

  QString strError = "";

  try
  {
    if (reply->error() == QNetworkReply::NoError)
    {
      bool bRes = false;
      
      QString strReply = (QString)reply->readAll();
      if(!strReply.isNull() && !strReply.isEmpty())
      {
        switch (enType)
        {
          case sbAddressServiceRequestType::ASRT_GOOGLE_SEARCH:
          case sbAddressServiceRequestType::ASRT_OSM_SEARCH:
          case sbAddressServiceRequestType::ASRT_PHOTON_SEARCH:
          case sbAddressServiceRequestType::ASRT_PELIAS_SEARCH:
            {
              QString strBounds;
              QVariant varBounds = reply->property("BOUNDS");
              if (!varBounds.isNull())
                strBounds = varBounds.toString();

              QVariant varQuery = reply->property("QUERY");
              QString strQuery = varQuery.toString();

              QVariantList vlSearches = reply->property("SEARCHES").toList();

              switch (enType)
              {
                case sbAddressServiceRequestType::ASRT_GOOGLE_SEARCH:
                  bRes = processGoogleSearchReply(strReply, strBounds);
                  break;
                case sbAddressServiceRequestType::ASRT_OSM_SEARCH:
                  bRes = processOsmSearchReply(strReply);
                  break;
                case sbAddressServiceRequestType::ASRT_PHOTON_SEARCH:
                  bRes = processPhotonSearchReply(strReply);
                  break;
                case sbAddressServiceRequestType::ASRT_PELIAS_SEARCH:
                  bRes = processPeliasSearchReply(strReply);
                  break;
              }

              SAFE_DELETE(reply);
              SAFE_DELETE(mpNetworkReply);

              nextSearch(varQuery.toString(), strBounds.isEmpty(), vlSearches);
            }
            break;
          case sbAddressServiceRequestType::ASRT_GOOGLE_INFO:
          case sbAddressServiceRequestType::ASRT_OSM_INFO:
          case sbAddressServiceRequestType::ASRT_PELIAS_INFO:
          case sbAddressServiceRequestType::ASRT_PHOTON_INFO:
            {
              QVariant varQuery = reply->property("QUERY");
              QString strQuery = varQuery.toString();

              QVariant varScale = reply->property("SCALE");
              double dScale = varScale.toDouble();

              QVariantList vlInfos = reply->property("INFOS").toList();

              switch (enType)
              {
                case sbAddressServiceRequestType::ASRT_GOOGLE_INFO:
                  bRes = processGoogleInfoReply(strReply);
                  break;
                case sbAddressServiceRequestType::ASRT_OSM_INFO:
                  bRes = processOsmInfoReply(strReply);
                  break;
                case sbAddressServiceRequestType::ASRT_PELIAS_INFO:
                  bRes = processPeliasInfoReply(strReply);
                  break;
                case sbAddressServiceRequestType::ASRT_PHOTON_INFO:
                  bRes = processPhotonInfoReply(strReply);
                  break;
              }

              SAFE_DELETE(reply);
              SAFE_DELETE(mpNetworkReply);

              QgsPoint pt;
              if (!pt.fromWkt(strQuery))
                vlInfos.clear();
                  
              nextInfo(QgsPointXY(pt.x(), pt.y()), dScale, vlInfos);
            }
            break;
        }
      }
    }
    else
      strError = reply->errorString();
  }
  catch (const std::runtime_error& re)
  {
    strError = re.what();
  }
  catch (const std::exception& e)
  {
    strError = e.what();
  }
  catch (...)
  {
    strError = "Unknown exception in sbaddressservicesgui.cpp:onNetworkRequestFinished";
  }

  if(!strError.isEmpty())
  {
    sbAddressServicesGui::AddressDetails addr(reply->errorString(), QgsRectangle(), QString());
    mComboResults->addItem("[" + strType + "] " + reply->errorString(), QVariant(addr.toJson()));
  }
  
  SAFE_DELETE(reply);
}

bool sbAddressServicesGui::processGoogleInfoReply(const QString& strReply)
{
  bool bRes = false;

  QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
  QJsonObject jsonObject = jsonResponse.object();

  if (!jsonObject.isEmpty())
  {
    if (!jsonObject["results"].isNull())
    {
      QJsonArray jsonResults = jsonObject["results"].toArray();
      for (int i = 0; i < jsonResults.count(); i++)
      {
        QJsonObject jsonResult = jsonResults[i].toObject();

        QString strDisplayName;
        QString strExtras;
        QgsRectangle rcBounds;

        if (!jsonResult["formatted_address"].isNull())
          strDisplayName = jsonResult["formatted_address"].toString();

        if (!jsonResult["geometry"].isNull())
        {
          if (!jsonResult["geometry"].toObject()["location"].isNull())
          {
            QJsonObject jsonLocation = jsonResult["geometry"].toObject()["location"].toObject();

            double dLat = jsonLocation["lat"].toDouble();
            double dLon = jsonLocation["lng"].toDouble();

            rcBounds.set(dLon - 0.00025, dLat - 0.00015, dLon + 0.00025, dLat + 0.00015);
          }
        }

        if (!strDisplayName.isEmpty())
        {
          sbAddressServicesGui::AddressDetails addr(strDisplayName, rcBounds, strExtras);
          mComboResults->addItem("[Google] " + strDisplayName, QVariant(addr.toJson()));

          bRes = true;
        }
      }
    }
  }

  return bRes;
}

bool sbAddressServicesGui::processOsmInfoReply(const QString& strReply)
{
  QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
  QJsonObject jsonObject = jsonResponse.object();

  QString strDisplayName;
  QString strExtras;
  QgsRectangle rcBounds;
    
  if (!jsonObject["display_name"].isNull())
    strDisplayName = jsonObject["display_name"].toString();

  if (!jsonObject["boundingbox"].isNull())
  {
    if (jsonObject["boundingbox"].isArray())
    {
      QJsonArray jsonBounds = jsonObject["boundingbox"].toArray();
      if (jsonBounds.count() == 4)
        rcBounds.set(jsonBounds[3].toString().toDouble(), jsonBounds[1].toString().toDouble(), jsonBounds[2].toString().toDouble(), jsonBounds[0].toString().toDouble());
    }
  }

  if (!jsonObject["class"].isNull())
    strExtras += "[class] " + jsonObject["class"].toString() + "\n";

  if (!jsonObject["type"].isNull())
    strExtras += "[type] " + jsonObject["type"].toString() + "\n";

  if (!jsonObject["extratags"].isNull())
  {
    QJsonObject jsonObjectTags = jsonObject["extratags"].toObject();
    QStringList listKeys = jsonObjectTags.keys();
    for (int i = 0; i < listKeys.count(); i++)
    {
      QString strKey = listKeys[i];
      QString strValue = jsonObjectTags[strKey].toString();
      strExtras += "[" + strKey + "] " + strValue + "\n";
    }
  }

  if (!strDisplayName.isEmpty())
  {
    sbAddressServicesGui::AddressDetails addr(strDisplayName, rcBounds, strExtras);

    mComboResults->addItem("[OSM] " + strDisplayName, QVariant(addr.toJson()));

    return true;
  }

  return false;
}

bool sbAddressServicesGui::processPhotonInfoReply(const QString& strReply)
{
  bool bRes = false;

  const QStringList listNameAttributeOrder = { "housenumber", "street", "locality", "district", "postcode", "city", "county", "state", "country" };

  QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
  if (jsonResponse.isObject())
  {
    QJsonObject jsonCollection = jsonResponse.object();
    if (!jsonCollection["features"].isNull())
    {
      QJsonArray jsonArray = jsonCollection["features"].toArray();

      if (jsonArray.count() > 0)
      {
        for (int i = 0; i < jsonArray.count(); i++)
        {
          QString strDisplayName;
          QString strExtras;
          QgsRectangle rcBounds;

          QJsonObject jsonObject = jsonArray[i].toObject();
          if (!jsonObject["geometry"].isNull())
          {
            if (jsonObject["geometry"].toObject()["type"].toString().compare("Point", Qt::CaseInsensitive) == 0)
            {
              double dX = jsonObject["geometry"].toObject()["coordinates"].toArray()[0].toDouble();
              double dY = jsonObject["geometry"].toObject()["coordinates"].toArray()[1].toDouble();

              rcBounds.set(dX - 0.0001, dY - 0.00005, dX + 0.0001, dY + 0.00005);
            }
          }

          if (!jsonObject["properties"].isNull())
          {
            QJsonObject jsonProperties = jsonObject["properties"].toObject();

            if (!jsonProperties["name"].isNull())
              strDisplayName = jsonProperties["name"].toString();

            if (strDisplayName.isEmpty())
            {
              QStringList listNameParts;

              foreach(const QString & strKey, listNameAttributeOrder)
              {
                if (!jsonProperties[strKey].isNull())
                {
                  QString strValue = jsonProperties[strKey].toString();
                  if (!strValue.isEmpty())
                    listNameParts.append(strValue);
                }
              }

              strDisplayName = listNameParts.join(", ");
            }

            QStringList listPropertyNames = jsonProperties.keys();
            foreach(const QString & strKey, listPropertyNames)
            {
              if (listNameAttributeOrder.contains(strKey, Qt::CaseInsensitive))
                continue;

              QString strValue;
              if (jsonProperties[strKey].isString())
                strValue = jsonProperties[strKey].toString();
              else if (jsonProperties[strKey].isDouble())
                strValue = QString::number(jsonProperties[strKey].toDouble());

              if (!strValue.isEmpty())
                strExtras += "[" + strKey + "] " + strValue + "\n";
            }
          }

          if (!strDisplayName.isEmpty() && !rcBounds.isEmpty())
          {
            sbAddressServicesGui::AddressDetails addr(strDisplayName, rcBounds, strExtras);
            mComboResults->addItem("[Photon] " + strDisplayName, QVariant(addr.toJson()));

            bRes = true;
          }
        }
      }
    }
  }

  return bRes;
}

bool sbAddressServicesGui::processPeliasInfoReply(const QString& strReply)
{
  bool bRes = false;

  const QStringList listNameAttributeOrder = { "housenumber", "street", "locality", "district", "postalccode", "city", "county", "macrocounty", "country" };

  QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
  if (jsonResponse.isObject())
  {
    QJsonObject jsonCollection = jsonResponse.object();
    if (!jsonCollection["features"].isNull())
    {
      QJsonArray jsonArray = jsonCollection["features"].toArray();

      if (jsonArray.count() > 0)
      {
        for (int i = 0; i < jsonArray.count(); i++)
        {
          QString strDisplayName;
          QString strExtras;
          QgsRectangle rcBounds;

          QJsonObject jsonObject = jsonArray[i].toObject();
          if (!jsonObject["geometry"].isNull())
          {
            if (jsonObject["geometry"].toObject()["type"].toString().compare("Point", Qt::CaseInsensitive) == 0)
            {
              double dX = jsonObject["geometry"].toObject()["coordinates"].toArray()[0].toDouble();
              double dY = jsonObject["geometry"].toObject()["coordinates"].toArray()[1].toDouble();

              rcBounds.set(dX - 0.0001, dY - 0.00005, dX + 0.0001, dY + 0.00005);
            }
          }

          if (!jsonObject["properties"].isNull())
          {
            QJsonObject jsonProperties = jsonObject["properties"].toObject();

            if (!jsonProperties["label"].isNull())
              strDisplayName = jsonProperties["label"].toString();
            else if (!jsonProperties["name"].isNull())
              strDisplayName = jsonProperties["name"].toString();

            if (strDisplayName.isEmpty())
            {
              QStringList listNameParts;

              foreach(const QString & strKey, listNameAttributeOrder)
              {
                if (!jsonProperties[strKey].isNull())
                {
                  QString strValue = jsonProperties[strKey].toString();
                  if (!strValue.isEmpty())
                    listNameParts.append(strValue);
                }
              }

              strDisplayName = listNameParts.join(", ");
            }

            QStringList listPropertyNames = jsonProperties.keys();
            foreach(const QString & strKey, listPropertyNames)
            {
              if (listNameAttributeOrder.contains(strKey, Qt::CaseInsensitive))
                continue;

              QString strValue;
              if (jsonProperties[strKey].isString())
                strValue = jsonProperties[strKey].toString();
              else if (jsonProperties[strKey].isDouble())
                strValue = QString::number(jsonProperties[strKey].toDouble());

              if (!strValue.isEmpty())
                strExtras += "[" + strKey + "] " + strValue + "\n";
            }
          }

          if (!strDisplayName.isEmpty() && !rcBounds.isEmpty())
          {
            sbAddressServicesGui::AddressDetails addr(strDisplayName, rcBounds, strExtras);
            mComboResults->addItem("[Pelias] " + strDisplayName, QVariant(addr.toJson()));

            bRes = true;
          }
        }
      }
    }
  }

  return bRes;
}

bool sbAddressServicesGui::processGoogleSearchReply(const QString& strReply, const QString& strBounds)
{
  bool bRes = false;

  QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
  QJsonObject jsonObject = jsonResponse.object();

  if (!jsonObject.isEmpty())
  {
    if (!jsonObject["results"].isNull())
    {
      QJsonArray jsonResults = jsonObject["results"].toArray();
      for (int i = 0; i < jsonResults.count(); i++)
      {
        QString strDisplayName;
        QString strExtras;
        QgsRectangle rcBounds;
        bool bValid = true;

        QJsonObject jsonResult = jsonResults[i].toObject();

        if (!jsonResult["formatted_address"].isNull())
          strDisplayName = jsonResult["formatted_address"].toString();

        if (!jsonResult["geometry"].isNull())
        {
          if (!jsonResult["geometry"].toObject()["location"].isNull())
          {
            QJsonObject jsonLocation = jsonResult["geometry"].toObject()["location"].toObject();

            double dLat = jsonLocation["lat"].toDouble();
            double dLon = jsonLocation["lng"].toDouble();

            rcBounds.set(dLon - 0.00025, dLat - 0.00015, dLon + 0.00025, dLat + 0.00015);

            if (!strBounds.isEmpty())
            {
              QgsRectangle rcQueryBounds = QgsRectangle::fromWkt(strBounds);
              bValid = rcBounds.intersects(rcQueryBounds);
            }
          }
        }

        if (!strDisplayName.isEmpty() && bValid)
        {
          sbAddressServicesGui::AddressDetails addr(strDisplayName, rcBounds, strExtras);
          mComboResults->addItem("[Google] " + strDisplayName, QVariant(addr.toJson()));

          bRes = true;
        }
      }  
    }
  }

  return bRes;
}

bool sbAddressServicesGui::processOsmSearchReply(const QString& strReply)
{
  bool bRes = false;

  QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
  QJsonArray jsonArray = jsonResponse.array();
  
  if (jsonArray.count() > 0)
  {
    for (int i = 0; i < jsonArray.count(); i++)
    {
      QString strDisplayName;
      QString strExtras;
      QgsRectangle rcBounds;

      QJsonObject jsonObject = jsonArray[i].toObject();
      
      if (!jsonObject["display_name"].isNull())
        strDisplayName = jsonObject["display_name"].toString();

      if (!jsonObject["boundingbox"].isNull())
      {
        if (jsonObject["boundingbox"].isArray())
        {
          QJsonArray jsonBounds = jsonObject["boundingbox"].toArray();
          if (jsonBounds.count() == 4)
          {
            rcBounds.set(jsonBounds[3].toString().toDouble(), jsonBounds[1].toString().toDouble(), jsonBounds[2].toString().toDouble(), jsonBounds[0].toString().toDouble());
          }
        }
      }

      if (!jsonObject["class"].isNull())
        strExtras += "[class] " + jsonObject["class"].toString() + "\n";

      if (!jsonObject["type"].isNull())
        strExtras += "[type] " + jsonObject["type"].toString() + "\n";

      if (!jsonObject["extratags"].isNull())
      {
        QJsonObject jsonObjectTags = jsonObject["extratags"].toObject();
        QStringList listKeys = jsonObjectTags.keys();
        for(int i = 0; i < listKeys.count(); i++)
        {
          QString strKey = listKeys[i];
          QString strValue = jsonObjectTags[strKey].toString();
          strExtras += "[" + strKey + "] " + strValue + "\n";
        }
      }
      
      if (!strDisplayName.isEmpty())
      {
        sbAddressServicesGui::AddressDetails addr(strDisplayName, rcBounds, strExtras);
        mComboResults->addItem("[OSM] " + strDisplayName, QVariant(addr.toJson()));

        bRes = true;
      }
    }
  }

  return bRes;
}

bool sbAddressServicesGui::processPhotonSearchReply(const QString& strReply)
{
  bool bRes = false;

  const QStringList listNameAttributeOrder = { "housenumber", "street", "locality", "district", "postcode", "city", "county", "state", "country" };

  QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
  if (jsonResponse.isObject())
  {
    QJsonObject jsonCollection = jsonResponse.object();
    if (!jsonCollection["features"].isNull())
    {
      QJsonArray jsonArray = jsonCollection["features"].toArray();

      if (jsonArray.count() > 0)
      {
        for (int i = 0; i < jsonArray.count(); i++)
        {
          QString strDisplayName;
          QString strExtras;
          QgsRectangle rcBounds;

          QJsonObject jsonObject = jsonArray[i].toObject();
          if (!jsonObject["geometry"].isNull())
          {
            if (jsonObject["geometry"].toObject()["type"].toString().compare("Point", Qt::CaseInsensitive) == 0)
            {
              double dX = jsonObject["geometry"].toObject()["coordinates"].toArray()[0].toDouble();
              double dY = jsonObject["geometry"].toObject()["coordinates"].toArray()[1].toDouble();

              rcBounds.set(dX - 0.0001, dY - 0.00005, dX + 0.0001, dY + 0.00005);
            }
          }

          if (!jsonObject["properties"].isNull())
          {
            QJsonObject jsonProperties = jsonObject["properties"].toObject();

            if (!jsonProperties["name"].isNull())
              strDisplayName = jsonProperties["name"].toString();

            if (strDisplayName.isEmpty())
            {
              QStringList listNameParts;

              foreach(const QString &strKey, listNameAttributeOrder)
              {
                if (!jsonProperties[strKey].isNull())
                {
                  QString strValue = jsonProperties[strKey].toString();
                  if (!strValue.isEmpty())
                    listNameParts.append(strValue);
                }
              }

              strDisplayName = listNameParts.join(", ");
            }

            QStringList listPropertyNames = jsonProperties.keys();
            foreach(const QString &strKey, listPropertyNames)
            {
              if (listNameAttributeOrder.contains(strKey, Qt::CaseInsensitive))
                continue;

              QString strValue;
              if (jsonProperties[strKey].isString())
                strValue = jsonProperties[strKey].toString();
              else if (jsonProperties[strKey].isDouble())
                strValue = QString::number(jsonProperties[strKey].toDouble());

              if (!strValue.isEmpty())
                strExtras += "[" + strKey + "] " + strValue + "\n";
            }
          }

          if (!strDisplayName.isEmpty() && !rcBounds.isEmpty())
          {
            sbAddressServicesGui::AddressDetails addr(strDisplayName, rcBounds, strExtras);
            mComboResults->addItem("[Photon] " + strDisplayName, QVariant(addr.toJson()));

            bRes = true;
          }
        }
      }
    }
  }

  return bRes;
}

bool sbAddressServicesGui::processPeliasSearchReply(const QString& strReply)
{
  bool bRes = false;

  const QStringList listNameAttributeOrder = { "housenumber", "street", "locality", "district", "postalccode", "city", "county", "macrocounty", "country" };

  QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
  if (jsonResponse.isObject())
  {
    QJsonObject jsonCollection = jsonResponse.object();
    if (!jsonCollection["features"].isNull())
    {
      QJsonArray jsonArray = jsonCollection["features"].toArray();

      if (jsonArray.count() > 0)
      {
        for (int i = 0; i < jsonArray.count(); i++)
        {
          QString strDisplayName;
          QString strExtras;
          QgsRectangle rcBounds;

          QJsonObject jsonObject = jsonArray[i].toObject();
          if (!jsonObject["geometry"].isNull())
          {
            if (jsonObject["geometry"].toObject()["type"].toString().compare("Point", Qt::CaseInsensitive) == 0)
            {
              double dX = jsonObject["geometry"].toObject()["coordinates"].toArray()[0].toDouble();
              double dY = jsonObject["geometry"].toObject()["coordinates"].toArray()[1].toDouble();

              rcBounds.set(dX - 0.0001, dY - 0.00005, dX + 0.0001, dY + 0.00005);
            }
          }

          if (!jsonObject["properties"].isNull())
          {
            QJsonObject jsonProperties = jsonObject["properties"].toObject();

            if (!jsonProperties["label"].isNull())
              strDisplayName = jsonProperties["label"].toString();
            else if (!jsonProperties["name"].isNull())
              strDisplayName = jsonProperties["name"].toString();

            if (strDisplayName.isEmpty())
            {
              QStringList listNameParts;

              foreach(const QString & strKey, listNameAttributeOrder)
              {
                if (!jsonProperties[strKey].isNull())
                {
                  QString strValue = jsonProperties[strKey].toString();
                  if (!strValue.isEmpty())
                    listNameParts.append(strValue);
                }
              }

              strDisplayName = listNameParts.join(", ");
            }

            QStringList listPropertyNames = jsonProperties.keys();
            foreach(const QString & strKey, listPropertyNames)
            {
              if (listNameAttributeOrder.contains(strKey, Qt::CaseInsensitive))
                continue;

              QString strValue;
              if (jsonProperties[strKey].isString())
                strValue = jsonProperties[strKey].toString();
              else if (jsonProperties[strKey].isDouble())
                strValue = QString::number(jsonProperties[strKey].toDouble());

              if (!strValue.isEmpty())
                strExtras += "[" + strKey + "] " + strValue + "\n";
            }
          }

          if (!strDisplayName.isEmpty() && !rcBounds.isEmpty())
          {
            sbAddressServicesGui::AddressDetails addr(strDisplayName, rcBounds, strExtras);
            mComboResults->addItem("[Pelias] " + strDisplayName, QVariant(addr.toJson()));

            bRes = true;
          }
        }
      }
    }
  }

  return bRes;
}

sbAddressServicesGui::AddressDetails::AddressDetails(const QString &strName, const QgsRectangle &rcBounds, const QString &strExtras)
{
  mstrName = strName;
  mrcBounds = rcBounds;
  mstrExtras = strExtras;
}

const QString& sbAddressServicesGui::AddressDetails::getName()
{
  return mstrName;
}

const QString& sbAddressServicesGui::AddressDetails::getExtras()
{
  return mstrExtras;
}

const QgsRectangle&  sbAddressServicesGui::AddressDetails::getBounds()
{
  return mrcBounds;
}

const QString sbAddressServicesGui::AddressDetails::toJson()
{
  QJsonObject jsonObject;

  jsonObject["name"] = mstrName;
  jsonObject["bounds"] = mrcBounds.asWktPolygon();
  jsonObject["extras"] = mstrExtras;

  QJsonDocument jsonDoc(jsonObject);
  return jsonDoc.toJson(QJsonDocument::Compact);
}

sbAddressServicesGui::AddressDetails sbAddressServicesGui::AddressDetails::fromJson(const QString &strJson)
{
  QJsonDocument jsonResponse = QJsonDocument::fromJson(strJson.toUtf8());
  QJsonObject jsonObject = jsonResponse.object();

  QString strName = jsonObject["name"].toString();
  QgsRectangle rcBounds = QgsRectangle::fromWkt(jsonObject["bounds"].toString());
  QString strExtras = jsonObject["extras"].toString();

  return sbAddressServicesGui::AddressDetails(strName, rcBounds, strExtras);
}
