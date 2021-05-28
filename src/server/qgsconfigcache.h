/***************************************************************************
                              qgsconfigcache.h
                              ----------------
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

#ifndef QGSCONFIGCACHE_H
#define QGSCONFIGCACHE_H

#include "qgsconfig.h"

#include <QCache>
#include <QFileSystemWatcher>
#include <QObject>
#include <QDomDocument>

#include "qgis_server.h"
#include "qgis_sip.h"
#include "qgsproject.h"
#include "qgsserversettings.h"
#include "qgsmapsettings.h"
#include "SimpleCrypt.h"

/**
 * \ingroup server
 * \brief Cache for server configuration.
 * \since QGIS 2.8
 */
class SERVER_EXPORT QgsConfigCache : public QObject
{
    Q_OBJECT
  public:

    /**
     * Returns the current instance.
     */
    static QgsConfigCache *instance();

    /**
     * Removes an entry from cache.
     * \param path The path of the project
     */
    bool removeEntry( const QString &path );

	QStringList sbLoadedProjects();

	void sbPurge();

    /**
     * If the project is not cached yet, then the project is read from the
     * path. If the project is not available, then NULLPTR is returned.
     * If the project contains any bad layer it is considered unavailable
     * unless the server configuration variable QGIS_SERVER_IGNORE_BAD_LAYERS
     * passed in the optional settings argument is set to TRUE (the default
     * value is FALSE).
     * \param path the filename of the QGIS project
     * \param settings QGIS server settings
     * \returns the project or NULLPTR if an error happened
     * \since QGIS 3.0
     */
    const QgsProject *project( const QString &path, QgsServerSettings *settings = nullptr );
	
	QgsMapSettings *mapSettings( const QString &path );
	QStringList *projectWarnings(const QString &path);

  private:
	QgsConfigCache() SIP_FORCE;

	//! Check for configuration file updates (remove entry from cache if file changes)
    QFileSystemWatcher mFileSystemWatcher;

    //! Returns xml document for project file / sld or 0 in case of errors
    QDomDocument *xmlDocument( const QString &filePath );

    QCache<QString, QDomDocument> mXmlDocumentCache;
    
	QCache<QString, QgsProject> mProjectCache;

	QCache<QString, QgsMapSettings> mMapSettingsCache;

	QCache<QString, QStringList> mProjectWarnings;

	QString mLoadingPath;
	
  private slots:
    //! Removes changed entry from this cache
    bool removeChangedEntry( const QString &path );
	
	void loadProjectCanvas(const QDomDocument &doc);
	void logOldProjectVersionWarning(const QString &warning);
	void logLoadingLayerMessage(const QString &layer, const QList<QgsReadWriteContext::ReadWriteMessage> & listMessages);
	void logProjectCleared();
};

#endif // QGSCONFIGCACHE_H
