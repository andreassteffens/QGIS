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

#include "sbelevationservicesgui.h"
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

#define GOOGLE_ELEVATION_INFO_URL "https://maps.googleapis.com/maps/api/elevation/json?&locations=";



sbElevationServicesGui::sbElevationServicesGui(QgisInterface *pQgisIface, QWidget *parent, Qt::WindowFlags fl) : QWidget(parent, fl)
{
  mpQgisIface = pQgisIface;

  setupUi(this);
  setEnabled(true);
  
  connect(QgsProject::instance(), &QgsProject::cleared, this, &sbElevationServicesGui::onClearedProject);

  // info
  connect(mpQgisIface->mapCanvas(), &QgsMapCanvas::mapToolSet, this, &sbElevationServicesGui::onMapToolSet);
  connect(mpQgisIface->mapCanvas(), &QgsMapCanvas::destinationCrsChanged, this, &sbElevationServicesGui::onDestinationCrsChanged);
  onDestinationCrsChanged();
  
  mpMapTool = new sbElevationServicesMapTool(mpQgisIface->mapCanvas());
  connect(mpMapTool, &sbElevationServicesMapTool::mouseClicked, this, &sbElevationServicesGui::onMapToolMouseClicked);

  connect(mPbtnActivateInfo, &QPushButton::toggled, this, &sbElevationServicesGui::onActivateInfoBtnToggled);


  // settings
  QSettings s;
  mLeGoogleKey->setText(s.value(QStringLiteral("sbElevationServices/GoogleApiKey"), "").toString());
  
  connect(mLeGoogleKey, &QLineEdit::textChanged, this, &sbElevationServicesGui::onSettingsTextChanged);

  
  // general
  connect(mPbtnClearResults, &QPushButton::pressed, this, &sbElevationServicesGui::onClearResultsBtnPressed);

  mpRubberBand = new QgsRubberBand(mpQgisIface->mapCanvas(), QgsWkbTypes::PointGeometry);
  mpRubberBand->setColor(Qt::blue);
  mpRubberBand->setWidth(1);
  mpRubberBand->setIcon(QgsRubberBand::ICON_CIRCLE);
  mpRubberBand->setIconSize(8);
  mpRubberBand->setFillColor(QColor::fromRgba(qRgba(255, 0, 0, 128)));
}

sbElevationServicesGui::~sbElevationServicesGui()
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

void sbElevationServicesGui::onClearedProject()
{
  onClearResultsBtnPressed();
}

void sbElevationServicesGui::onMapToolSet(QgsMapTool* newTool, QgsMapTool* oldTool)
{
  if (newTool != mpMapTool.data())
    mPbtnActivateInfo->setChecked(false);
}

void sbElevationServicesGui::onMapToolMouseClicked(const QgsPointXY &point)
{
  QgsPointXY ptTransformed = mTransform.transform(point);

  if (mpNetworkReply)
  {
    mpNetworkReply->abort();
    mpNetworkReply.clear();
  }

  doInfo(ptTransformed);
}

void sbElevationServicesGui::onDestinationCrsChanged()
{
  mTransform.setSourceCrs(mpQgisIface->mapCanvas()->mapSettings().destinationCrs());
  
  QgsCoordinateReferenceSystem crsWgs84 = QgsCoordinateReferenceSystem::fromOgcWmsCrs(geoEpsgCrsAuthId());
  mTransform.setDestinationCrs(crsWgs84);
}

void sbElevationServicesGui::onSettingsTextChanged(const QString &text)
{
  QSettings s;

  s.setValue("sbElevationServices/GoogleApiKey", QVariant(mLeGoogleKey->text()));
}

void sbElevationServicesGui::onClearResultsBtnPressed()
{
  mPteResult->clear();

  mpRubberBand->reset(QgsWkbTypes::PointGeometry);
}

void sbElevationServicesGui::onActivateInfoBtnToggled(bool checked)
{
  if (mpMapTool.isNull())
    return;

  if (!checked)
    mpQgisIface->mapCanvas()->unsetMapTool(mpMapTool);
  else
    mpQgisIface->mapCanvas()->setMapTool(mpMapTool);
}

void sbElevationServicesGui::doInfo(const QgsPointXY& point)
{
  onClearResultsBtnPressed();

  if (mLeGoogleKey->text().isEmpty() || mLeGoogleKey->text().isNull())
    ;
  else
    doGoogleInfo(point);
}

void sbElevationServicesGui::doGoogleInfo(const QgsPointXY& point)
{
  QString currentLocale = QLocale().name();

  QString strUrl = GOOGLE_ELEVATION_INFO_URL;
  strUrl += QString::number(point.y()) + "," + QString::number(point.x());
  strUrl += "&key=" + mLeGoogleKey->text();

  QNetworkRequest rq;
  rq.setUrl(QUrl(strUrl));

  mpNetworkReply = mNetworkManager.get(rq);
  mpNetworkReply->setProperty("QUERY", point.asWkt());

  connect(mpNetworkReply, &QNetworkReply::finished, this, &sbElevationServicesGui::onNetworkReplyFinished);

  mPteResult->setPlainText(tr("Searching..."));
}

void sbElevationServicesGui::onNetworkReplyFinished()
{
  QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
  if (!reply)
    return;

  QString strQuery;

  try
  {
    if (reply->error() == QNetworkReply::NoError)
    {
      bool bRes = false;
      
      QString strReply = (QString)reply->readAll();
      if(!strReply.isNull() && !strReply.isEmpty())
      {
        QVariant varQuery = reply->property("QUERY");
        
        bRes = processGoogleInfoReply(strReply);
        strQuery = varQuery.toString();
      }

      if (!bRes)
      {
        if (!strReply.isNull() && !strReply.isEmpty())
          mPteResult->setPlainText(strReply);
        else
          mPteResult->setPlainText(tr("No results!"));
      }
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
    mPteResult->setPlainText("Unknown exception in sbelevationservicesgui.cpp:onNetworkRequestFinished");
  }

  delete reply;
  delete mpNetworkReply;
}

bool sbElevationServicesGui::processGoogleInfoReply(const QString& strReply)
{
  bool bRes = false;

  QJsonDocument jsonResponse = QJsonDocument::fromJson(strReply.toUtf8());
  QJsonObject jsonObject = jsonResponse.object();

  mPteResult->clear();
  mpRubberBand->reset(QgsWkbTypes::PointGeometry);
  
  if (!jsonObject.isEmpty())
  {
    if (!jsonObject["results"].isNull())
    {
      QJsonArray jsonResults = jsonObject["results"].toArray();
      for (int i = 0; i < jsonResults.count(); i++)
      {
        QJsonObject jsonResult = jsonResults[i].toObject();

        double dElevation = 0;
        double dResolution = 0;
        QgsPointXY ptLocation;

        if(!jsonResult["elevation"].isNull())
          dElevation = jsonResult["elevation"].toDouble();

        if (!jsonResult["resolution"].isNull())
          dResolution = jsonResult["resolution"].toDouble();

        if (!jsonResult["location"].isNull())
        {
          QJsonObject jsonLocation = jsonResult["location"].toObject();

          double dLat = jsonLocation["lat"].toDouble();
          double dLon = jsonLocation["lng"].toDouble();

          ptLocation.setX(dLon);
          ptLocation.setY(dLat);
        }

        if (i > 0)
          mPteResult->appendHtml("<br>");

        mPteResult->appendHtml(tr("Elevation") + ": " + QString::number(dElevation) + "m<br>" + tr("Resolution") + ": " + QString::number(dResolution) + "m<br>");

        QgsPointXY ptLocationTransformed = mTransform.transform(ptLocation, Qgis::TransformDirection::Reverse);
        mpRubberBand->addGeometry(QgsGeometry::fromPointXY(ptLocationTransformed), mTransform.sourceCrs());

        bRes = true;
      }

      mpRubberBand->show();
    }
  }

  return bRes;
}
