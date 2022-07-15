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
#include <qjsonarray.h>
#include <qnetworkreply.h>

#define GOOGLE_ADDRESS_SEARCH_URL "https://maps.google.com/maps/api/geocode/json?sensor=false&address=";
#define GOOGLE_ADDRESS_INFO_URL "https://maps.googleapis.com/maps/api/geocode/json?sensor=false&latlng=";

enum sbAddressServiceRequestType
{
  ASRT_INVALID = -1,
  ASRT_GOOGLE_SEARCH = 0,
  ASRT_OSM_SEARCH = 1,
  ASRT_GOOGLE_INFO = 2,
  ASRT_OSM_INFO = 3
};


sbAddressServicesGui::sbAddressServicesGui(QgisInterface *pQgisIface, QWidget *parent, Qt::WindowFlags fl) : QWidget(parent, fl)
{
  mpQgisIface = pQgisIface;

  setupUi(this);
  setEnabled(true);

  connect(QgsProject::instance(), &QgsProject::cleared, this, &sbAddressServicesGui::onClearedProject);

  QSettings s;

  // search
  mCheckRestrictSearchBounds->setCheckState((Qt::CheckState)s.value("sbAddressServices/SearchRestrictToViewBounds", (int)Qt::CheckState::Checked).toInt());

  connect(mLeSearch, &QLineEdit::returnPressed, this, &sbAddressServicesGui::onSearchTextReturnPressed);
  connect(mTbtnSearch, &QToolButton::pressed, this, &sbAddressServicesGui::onSearchBtnPressed);
  connect(mCheckRestrictSearchBounds, &QCheckBox::stateChanged, this, &sbAddressServicesGui::onSearchCheckBoxStateChanged);

  // info
  connect(mpQgisIface->mapCanvas(), &QgsMapCanvas::mapToolSet, this, &sbAddressServicesGui::onMapToolSet);
  connect(mpQgisIface->mapCanvas(), &QgsMapCanvas::destinationCrsChanged, this, &sbAddressServicesGui::onDestinationCrsChanged);
  onDestinationCrsChanged();

  mpMapTool = new sbAddressServicesMapTool(mpQgisIface->mapCanvas());
  connect(mpMapTool, &sbAddressServicesMapTool::mouseClicked, this, &sbAddressServicesGui::onMapToolMouseClicked);

  connect(mPbtnActivateInfo, &QPushButton::toggled, this, &sbAddressServicesGui::onActivateInfoBtnToggled);


  // funny
  connect(mComboFunnyPlaces, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &sbAddressServicesGui::onFunnyPlacesComboIndexChanged);


  // settings
  mLeGoogleKey->setText(s.value(QStringLiteral("sbAddressServices/GoogleApiKey"), "").toString());
  mLeGoogleQueryOptions->setText(s.value(QStringLiteral("sbAddressServices/GoogleQueryOptions"), "").toString());
  mLeNominatimService->setText(s.value(QStringLiteral("sbAddressServices/OsmNominatimService"), "https://tileserver.gis24.eu/nominatim").toString());
  mLeNominatimQueryOptions->setText(s.value(QStringLiteral("sbAddressServices/OsmNominatimQueryOptions"), "").toString());
  mCheckDebugMode->setCheckState((Qt::CheckState)s.value("sbAddressServices/DebugMode", (int)Qt::CheckState::Checked).toInt());

  connect(mLeGoogleKey, &QLineEdit::textChanged, this, &sbAddressServicesGui::onSettingsTextChanged);
  connect(mLeGoogleQueryOptions, &QLineEdit::textChanged, this, &sbAddressServicesGui::onSettingsTextChanged);
  connect(mLeNominatimService, &QLineEdit::textChanged, this, &sbAddressServicesGui::onSettingsTextChanged);
  connect(mLeNominatimQueryOptions, &QLineEdit::textChanged, this, &sbAddressServicesGui::onSettingsTextChanged);


  // general
  connect(mPbtnClearResults, &QPushButton::pressed, this, &sbAddressServicesGui::onClearResultsBtnPressed);
  connect(mComboResults, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &sbAddressServicesGui::onResultsComboIndexChanged);
  connect(mPbtnNavigateToResult, &QPushButton::pressed, this, &sbAddressServicesGui::onNavigateToResultBtnPressed);

  mpRubberBand = new QgsRubberBand(mpQgisIface->mapCanvas(), QgsWkbTypes::PolygonGeometry);
  mpRubberBand->setColor(Qt::blue);
  mpRubberBand->setWidth(2);
  mpRubberBand->setFillColor(QColor::fromRgba(qRgba(255, 0, 0, 128)));
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

void sbAddressServicesGui::onMapToolMouseClicked(const QgsPointXY &point)
{
  QgsPointXY ptTransformed = mTransform.transform(point);

  if (mpNetworkReply)
  {
    mpNetworkReply->abort();
    mpNetworkReply.clear();
  }

  doInfo(ptTransformed);
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

void sbAddressServicesGui::onSettingsTextChanged(const QString &text)
{
  QSettings s;

  s.setValue("sbAddressServices/GoogleApiKey", QVariant(mLeGoogleKey->text()));
  s.setValue("sbAddressServices/GoogleQueryOptions", QVariant(mLblGoogleQueryOptions->text()));
  s.setValue("sbAddressServices/OsmNominatimService", QVariant(mLeNominatimService->text()));
  s.setValue("sbAddressServices/OsmNominatimQueryOptions", QVariant(mLeNominatimQueryOptions->text()));
}

void sbAddressServicesGui::onSearchCheckBoxStateChanged(int state)
{
  QSettings s;

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
  doSearch(mLeSearch->text());
}

void sbAddressServicesGui::onSearchBtnPressed()
{
  doSearch(mLeSearch->text());
}

void sbAddressServicesGui::onResultsComboIndexChanged(int index)
{
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

    QgsRectangle rcBoundsTransformed = mTransform.transformBoundingBox(addr.getBounds(), Qgis::TransformDirection::Reverse);
    
    mpRubberBand->reset(QgsWkbTypes::PolygonGeometry);
    mpRubberBand->addGeometry(QgsGeometry::fromRect(rcBoundsTransformed), mTransform.sourceCrs());
    mpRubberBand->show();
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

    QgsRectangle rcBoundsTransformed = mTransform.transformBoundingBox(addr.getBounds(), Qgis::TransformDirection::Reverse);

    mpQgisIface->mapCanvas()->setExtent(rcBoundsTransformed);
    mpQgisIface->mapCanvas()->refresh();
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

void sbAddressServicesGui::doSearch(const QString& strText)
{
  if (strText.isEmpty() || strText.isNull())
    return;

  onClearResultsBtnPressed();

  if (mpNetworkReply)
  {
    mpNetworkReply->abort();
    mpNetworkReply.clear();
  }

  if (mLeGoogleKey->text().isEmpty() || mLeGoogleKey->text().isNull())
    doOsmSearch(strText);
  else
    doGoogleSearch(strText);
}

void sbAddressServicesGui::doFunnySearch()
{
  if (mComboFunnyPlaces->currentIndex() < 0)
    return;

  doSearch(mComboFunnyPlaces->currentText());
}

void sbAddressServicesGui::doGoogleSearch(const QString& strText)
{
  QString currentLocale = QLocale().name();

  QString strUrl = GOOGLE_ADDRESS_SEARCH_URL;
  strUrl += QUrl::toPercentEncoding(strText);
  strUrl += "&language=" + QLocale().name();
  strUrl += "&key=" + mLeGoogleKey->text();

  if (mCheckRestrictSearchBounds->checkState() == Qt::CheckState::Checked)
  {
    QgsRectangle rcMapExtent = mpQgisIface->mapCanvas()->extent();
    QgsRectangle rcMapExtentTransformed = mTransform.transformBoundingBox(rcMapExtent, Qgis::TransformDirection::Forward);

    strUrl += "&bounds=" + QString::number(rcMapExtentTransformed.xMinimum()) + "," + QString::number(rcMapExtentTransformed.yMinimum()) + "|" + QString::number(rcMapExtentTransformed.xMaximum()) + "," + QString::number(rcMapExtentTransformed.yMaximum());
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
    QgsMessageLog::logMessage( QStringLiteral( "Google Address Search URL: %1" ).arg( strUrl ), QObject::tr( "[a]tapa Address Services" ), Qgis::MessageLevel::Info );
  }

  QNetworkRequest rq;
  rq.setUrl(QUrl(strUrl));

  mpNetworkReply = mNetworkManager.get(rq);
  mpNetworkReply->setProperty("REQUEST_TYPE", ASRT_GOOGLE_SEARCH);
  mpNetworkReply->setProperty("QUERY", strText);

  connect(mpNetworkReply, &QNetworkReply::finished, this, &sbAddressServicesGui::onNetworkReplyFinished);

  mPteResult->setPlainText(tr("Searching..."));
}

void sbAddressServicesGui::doOsmSearch(const QString& strText)
{
  QString strUrl = mLeNominatimService->text();
  if (strUrl.isEmpty() || strUrl.isNull())
    return;

  QString strLocale = QLocale().name();
  if(strLocale.contains("-"))
  {
    QStringList listParts = strLocale.split("-");
    strLocale = listParts[0];
  }

  if (!strUrl.endsWith('/'))
    strUrl += "/";
  strUrl += "search?q=" + QUrl::toPercentEncoding(strText);
  strUrl += "&accept-language=" + strLocale + ",en";
  strUrl += "&format=json&polygon=0&addressdetails=0&extratags=1";

  if (mCheckRestrictSearchBounds->checkState() == Qt::CheckState::Checked)
  {
    QgsRectangle rcMapExtent = mpQgisIface->mapCanvas()->extent();
    QgsRectangle rcMapExtentTransformed = mTransform.transformBoundingBox(rcMapExtent, Qgis::TransformDirection::Forward);

    strUrl += "&viewbox=" + QString::number(rcMapExtentTransformed.xMinimum()) + "," + QString::number(rcMapExtentTransformed.yMinimum()) + "," + QString::number(rcMapExtentTransformed.xMaximum()) + "," + QString::number(rcMapExtentTransformed.yMaximum());
    strUrl += "&bounded=1";
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
    QgsMessageLog::logMessage( QStringLiteral( "OpenStreetMap Nominatim Search URL: %1" ).arg( strUrl ), QObject::tr( "[a]tapa Address Services" ), Qgis::MessageLevel::Info );
  }

  QNetworkRequest rq;
  rq.setUrl(QUrl(strUrl));
  
  mpNetworkReply = mNetworkManager.get(rq);
  mpNetworkReply->setProperty("REQUEST_TYPE", ASRT_OSM_SEARCH);
  mpNetworkReply->setProperty("QUERY", strText);

  connect(mpNetworkReply, &QNetworkReply::finished, this, &sbAddressServicesGui::onNetworkReplyFinished);

  mPteResult->setPlainText(tr("Searching..."));
}

void sbAddressServicesGui::doInfo(const QgsPointXY& point)
{
  onClearResultsBtnPressed();

  if (mLeGoogleKey->text().isEmpty() || mLeGoogleKey->text().isNull())
    doOsmInfo(point);
  else
    doGoogleInfo(point);
}

void sbAddressServicesGui::doGoogleInfo(const QgsPointXY& point)
{
  QString currentLocale = QLocale().name();

  QString strUrl = GOOGLE_ADDRESS_INFO_URL;
  strUrl += QString::number(point.y()) + "," + QString::number(point.x());
  strUrl += "&language=" + currentLocale;
  strUrl += "&key=" + mLeGoogleKey->text();

  QNetworkRequest rq;
  rq.setUrl(QUrl(strUrl));

  if (mCheckDebugMode->checkState() == Qt::CheckState::Checked)
  {
    QgsMessageLog::logMessage( QStringLiteral( "Google Address Search URL: %1" ).arg( strUrl ), QObject::tr( "[a]tapa Address Services" ), Qgis::MessageLevel::Info );
  }

  mpNetworkReply = mNetworkManager.get(rq);
  mpNetworkReply->setProperty("REQUEST_TYPE", ASRT_GOOGLE_INFO);
  mpNetworkReply->setProperty("QUERY", point.asWkt());

  connect(mpNetworkReply, &QNetworkReply::finished, this, &sbAddressServicesGui::onNetworkReplyFinished);

  mPteResult->setPlainText(tr("Searching..."));
}

void sbAddressServicesGui::doOsmInfo(const QgsPointXY& point)
{
  QString strUrl = mLeNominatimService->text();
  if (strUrl.isEmpty() || strUrl.isNull())
    return;

  QString strLocale = QLocale().name();
  if (strLocale.contains("-"))
  {
    QStringList listParts = strLocale.split("-");
    strLocale = listParts[0];
  }

  if (!strUrl.endsWith('/'))
    strUrl += "/";
  strUrl += "reverse?q=";
  strUrl += "&accept-language=" + strLocale + ",en";
  strUrl += "&format=json&zoom=18&addressdetails=0&extratags=1";
  strUrl += "&lat=" + QString::number(point.y()) + "&lon=" + QString::number(point.x());

  QNetworkRequest rq;
  rq.setUrl(QUrl(strUrl));

  if (mCheckDebugMode->checkState() == Qt::CheckState::Checked)
  {
    QgsMessageLog::logMessage( QStringLiteral( "OpenStreetMap Nominatim Info URL: %1" ).arg( strUrl ), QObject::tr( "[a]tapa Address Services" ), Qgis::MessageLevel::Info );
  }

  mpNetworkReply = mNetworkManager.get(rq);
  mpNetworkReply->setProperty("REQUEST_TYPE", ASRT_OSM_INFO);
  mpNetworkReply->setProperty("QUERY", point.asWkt());

  connect(mpNetworkReply, &QNetworkReply::finished, this, &sbAddressServicesGui::onNetworkReplyFinished);

  mPteResult->setPlainText(tr("Searching..."));
}

void sbAddressServicesGui::onNetworkReplyFinished()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
  if (!reply)
    return;

  QString strQuery;
  sbAddressServiceRequestType enType = ASRT_INVALID;

  try
  {
    if (reply->error() == QNetworkReply::NoError)
    {
      bool bRes = false;
      
      QString strReply = (QString)reply->readAll();
      if(!strReply.isNull() && !strReply.isEmpty())
      {
        QVariant varQuery = reply->property("QUERY");
        QVariant varType = reply->property("REQUEST_TYPE");
        if (!varType.isNull())
        {
          enType = (sbAddressServiceRequestType)varType.toInt();
          switch (enType)
          {
            case sbAddressServiceRequestType::ASRT_GOOGLE_SEARCH:
              {
                bRes = processGoogleSearchReply(strReply);
                strQuery = varQuery.toString();
              }
              break;
            case sbAddressServiceRequestType::ASRT_OSM_SEARCH:
              bRes = processOsmSearchReply(strReply);
              break;
            case sbAddressServiceRequestType::ASRT_GOOGLE_INFO:
              {
                bRes = processGoogleInfoReply(strReply);
                strQuery = varQuery.toString();
              }
              break;
            case sbAddressServiceRequestType::ASRT_OSM_INFO:
              bRes = processOsmInfoReply(strReply);
              break;
          }
        }
      }

      if (!bRes && mComboResults->count() == 0)
        mPteResult->setPlainText(tr("No results!"));
      else
        onResultsComboIndexChanged(mComboResults->currentIndex());
    }
    else
      mPteResult->setPlainText(reply->errorString());
  }
  catch (const std::runtime_error& re)
  {
    mPteResult->setPlainText(re.what());
  }
  catch (const std::exception& e)
  {
    mPteResult->setPlainText(e.what());
  }
  catch (...)
  {
    mPteResult->setPlainText("Unknown exception in sbaddressservicesgui.cpp:onNetworkRequestFinished");
  }

  delete reply;
  delete mpNetworkReply;

  if (!strQuery.isNull() && !strQuery.isEmpty())
  {
    switch (enType)
    {
      case sbAddressServiceRequestType::ASRT_GOOGLE_SEARCH:
        doOsmSearch(strQuery);
        break;
      case sbAddressServiceRequestType::ASRT_GOOGLE_INFO:
        {
          QgsPoint pt;
          if (pt.fromWkt(strQuery))
          {
            QgsPointXY ptXy(pt.x(), pt.y());
            doOsmInfo(ptXy);
          }
        }
        break;
    }
  }
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

bool sbAddressServicesGui::processGoogleSearchReply(const QString& strReply)
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
