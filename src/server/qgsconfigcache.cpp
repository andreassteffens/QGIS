/***************************************************************************
                              qgsconfigcache.cpp
                              ------------------
  begin                : July 24th, 2010
  copyright            : (C) 2010 by Marco Hugentobler
  email                : marco dot hugentobler at sourcepole dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsconfigcache.h"
#include "qgsmessagelog.h"
#include "qgsaccesscontrol.h"
#include "qgsproject.h"
#include "qgsmapsettings.h"
#include "qgsserverprojectutils.h"
#include "qgsserverexception.h"
#include "qgsstorebadlayerinfo.h"

#include "sbutils.h"

#include <QFile>

QgsConfigCache *QgsConfigCache::instance()
{
  static QgsConfigCache *sInstance = nullptr;

  if ( !sInstance )
    sInstance = new QgsConfigCache();

  return sInstance;
}

QgsConfigCache::QgsConfigCache()
{
  QObject::connect( &mFileSystemWatcher, &QFileSystemWatcher::fileChanged, this, &QgsConfigCache::removeChangedEntry );
}

QStringList QgsConfigCache::sbLoadedProjects()
{
	QStringList listProjects;

	QList<QString> listPaths = mProjectCache.keys();
	for (int i = 0; i < listPaths.count(); i++)
		listProjects.append(listPaths[i]);

	return listProjects;
}

void QgsConfigCache::sbPurge()
{
	while (!mProjectCache.isEmpty())
	{
		QString strKey = *mProjectCache.keys().begin();
		removeChangedEntry(strKey);
	}
}

const QgsProject *QgsConfigCache::project( const QString &path, QgsServerSettings *settings )
{
  QgsProject::ReadFlags readFlags = QgsProject::ReadFlag();

  QString strLoadingPath = sbGetStandardizedPath(path);

  if ( ! mProjectCache[ strLoadingPath ] )
  {

    std::unique_ptr<QgsProject> prj( new QgsProject() );

    // This is required by virtual layers that call QgsProject::instance() inside the constructor :(
    QgsProject::setInstance( prj.get() );

    QgsStoreBadLayerInfo *badLayerHandler = new QgsStoreBadLayerInfo();
    prj->setBadLayerHandler( badLayerHandler );

	QObject::connect(prj.get(), &QgsProject::readProject, this, &QgsConfigCache::loadProjectCanvas);
	QObject::connect(prj.get(), &QgsProject::oldProjectVersionWarning, this, &QgsConfigCache::logOldProjectVersionWarning);
	QObject::connect(prj.get(), &QgsProject::loadingLayerMessageReceived, this, &QgsConfigCache::logLoadingLayerMessage);
	QObject::connect(prj.get(), &QgsProject::cleared, this, &QgsConfigCache::logProjectCleared);

	mLoadingPath = strLoadingPath;

	if (!mProjectWarnings.contains(mLoadingPath))
	{
		std::unique_ptr<QStringList> list(new QStringList());
		mProjectWarnings.insert(mLoadingPath, list.release());
	}
	
    if ( settings )
    {
      // Activate trust layer metadata flag
      if ( settings->trustLayerMetadata() )
      {
        readFlags |= QgsProject::ReadFlag::FlagTrustLayerMetadata;
      }
      // Activate don't load layouts flag
      if ( settings->getPrintDisabled() )
      {
        readFlags |= QgsProject::ReadFlag::FlagDontLoadLayouts;
      }
    }

    if ( prj->read(strLoadingPath, readFlags ) )
    {
      if ( !badLayerHandler->badLayers().isEmpty() )
      {
        // if bad layers are not restricted layers so service failed
        QStringList unrestrictedBadLayers;
        // test bad layers through restrictedlayers
        const QStringList badLayerIds = badLayerHandler->badLayers();
        const QMap<QString, QString> badLayerNames = badLayerHandler->badLayerNames();
        const QStringList resctrictedLayers = QgsServerProjectUtils::wmsRestrictedLayers( *prj );
        for ( const QString &badLayerId : badLayerIds )
        {
          // if this bad layer is in restricted layers
          // it doesn't need to be added to unrestricted bad layers
          if ( badLayerNames.contains( badLayerId ) &&
               resctrictedLayers.contains( badLayerNames.value( badLayerId ) ) )
          {
            continue;
          }
          unrestrictedBadLayers.append( badLayerId );
        }
        if ( !unrestrictedBadLayers.isEmpty() )
        {
          // This is a critical error unless QGIS_SERVER_IGNORE_BAD_LAYERS is set to TRUE
          if ( ! settings || ! settings->ignoreBadLayers() )
          {
            QgsMessageLog::logMessage(
              QStringLiteral( "Error, Layer(s) %1 not valid in project %2" ).arg( unrestrictedBadLayers.join( QLatin1String( ", " ) ), strLoadingPath),
              QStringLiteral( "Server" ), Qgis::Critical );
            throw QgsServerException( QStringLiteral( "Layer(s) not valid" ) );
          }
          else
          {
            QgsMessageLog::logMessage(
              QStringLiteral( "Warning, Layer(s) %1 not valid in project %2" ).arg( unrestrictedBadLayers.join( QLatin1String( ", " ) ), strLoadingPath),
              QStringLiteral( "Server" ), Qgis::Warning );
          }
        }
      }
      mProjectCache.insert(strLoadingPath, prj.release() );
      mFileSystemWatcher.addPath(strLoadingPath);
    }
    else
    {
      QgsMessageLog::logMessage(tr( "Error when loading project file '%1': %2 " ).arg(strLoadingPath, prj->error()), QStringLiteral( "Server" ), Qgis::Critical);
    }
  }
  
  QgsProject::setInstance( mProjectCache[ strLoadingPath ] );
  return mProjectCache[ strLoadingPath ];
}

QStringList *QgsConfigCache::projectWarnings(const QString &path)
{
	QString strPath = sbGetStandardizedPath(path);

	if (mProjectWarnings[strPath])
		return mProjectWarnings[strPath];
	
	return NULL;
}

QgsMapSettings *QgsConfigCache::mapSettings(const QString &path)
{
	QString strPath = sbGetStandardizedPath(path);

	if (mMapSettingsCache.contains(strPath))
		return mMapSettingsCache[strPath];

	return NULL;
}

void QgsConfigCache::logOldProjectVersionWarning(const QString &warning)
{
	if (mLoadingPath.isEmpty())
		return;

	if (!mProjectWarnings.contains(mLoadingPath))
		return;

	mProjectWarnings[mLoadingPath]->append("(WARNING) " + warning);
}

void QgsConfigCache::logProjectCleared()
{
	QgsMessageLog::logMessage(QStringLiteral("[sb] Project has been cleared!"), QStringLiteral("Server"), Qgis::Critical);
}

void QgsConfigCache::logLoadingLayerMessage(const QString &t1, const QList<QgsReadWriteContext::ReadWriteMessage> &listMessages)
{
	if (mLoadingPath.isEmpty())
		return;

	if (!mProjectWarnings.contains(mLoadingPath))
		return;

	for (int i = 0; i < listMessages.size(); i++)
	{
		QString strLevel = "None";
		switch (listMessages[i].level())
		{
			case Qgis::Warning:
				strLevel = "WARNING";
				break;
			case Qgis::Info:
				strLevel = "INFO";
				break;
			case Qgis::Critical:
				strLevel = "CRITICAL";
				break;
			case Qgis::Success:
				strLevel = "SUCESS";
				break;
			case Qgis::None:
				strLevel = "NONE";
				break;
		}

		QString qstrMessage = "[Layer '" + t1 + "'] (" + strLevel + ") " + listMessages[i].message();
		mProjectWarnings[mLoadingPath]->append(qstrMessage);
	}
}

void QgsConfigCache::loadProjectCanvas(const QDomDocument &doc)
{
	if (mLoadingPath.isEmpty())
		return;

	QDomNodeList nodes = doc.elementsByTagName(QStringLiteral("mapcanvas"));
	if (nodes.count())
	{
		// Search the specific MapCanvas node using the name
		for (int i = 0; i < nodes.size(); ++i)
		{
			QDomElement elementNode = nodes.at(i).toElement();

			if (elementNode.hasAttribute(QStringLiteral("name")) && elementNode.attribute(QStringLiteral("name")) == "theMapCanvas")
			{
				QDomNode node = nodes.at(i);
				
				QgsMapSettings *settings = new QgsMapSettings();
				settings->readXml(node);
				
				if(mMapSettingsCache.contains(mLoadingPath))
					mMapSettingsCache.remove(mLoadingPath);

				mMapSettingsCache.insert(mLoadingPath, settings);

				break;
			}
		}
	}
}

QDomDocument *QgsConfigCache::xmlDocument( const QString &filePath )
{
  //first open file
  QFile configFile( filePath );
  if ( !configFile.exists() )
  {
    QgsMessageLog::logMessage( "Error, configuration file '" + filePath + "' does not exist", QStringLiteral( "Server" ), Qgis::Critical );
    return nullptr;
  }

  if ( !configFile.open( QIODevice::ReadOnly ) )
  {
    QgsMessageLog::logMessage( "Error, cannot open configuration file '" + filePath + "'", QStringLiteral( "Server" ), Qgis::Critical );
    return nullptr;
  }

  QString strPath = sbGetStandardizedPath(filePath);

  // first get cache
  QDomDocument *xmlDoc = mXmlDocumentCache.object( strPath );
  if ( !xmlDoc )
  {
    //then create xml document
    xmlDoc = new QDomDocument();
    QString errorMsg;
    int line, column;
    if ( !xmlDoc->setContent( &configFile, true, &errorMsg, &line, &column ) )
    {
      QgsMessageLog::logMessage( "Error parsing file '" + filePath +
                                 QStringLiteral( "': parse error %1 at row %2, column %3" ).arg( errorMsg ).arg( line ).arg( column ), QStringLiteral( "Server" ), Qgis::Critical );
      delete xmlDoc;
      return nullptr;
    }
    mXmlDocumentCache.insert( strPath, xmlDoc );
    mFileSystemWatcher.addPath( filePath );
    xmlDoc = mXmlDocumentCache.object( filePath );
    Q_ASSERT( xmlDoc );
  }
  return xmlDoc;
}

bool QgsConfigCache::removeChangedEntry( const QString &path )
{
	QString strPath = sbGetStandardizedPath(path);

	bool bRes = mProjectCache.remove( strPath );

	//xml document must be removed last, as other config cache destructors may require it
	mXmlDocumentCache.remove( strPath );

	mMapSettingsCache.remove(strPath);
	mProjectWarnings.remove(strPath);
  
	mFileSystemWatcher.removePath(strPath);

	return bRes;
}

bool QgsConfigCache::removeEntry( const QString &path )
{
	QString strPath = sbGetStandardizedPath(path);

	return removeChangedEntry( strPath );
}
