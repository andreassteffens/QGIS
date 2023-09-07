/***************************************************************************
               sbservercachefilter.h

  sb server cache filter implementation
  -------------------------------------
  begin                : 2019-07-23
  copyright            : (C) 2019 by Andreas Steffens
  email                : a dot steffens at gds dash team dot de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SBSERVERCACHEFILTER_H
#define SBSERVERCACHEFILTER_H

#define SIP_NO_FILE

#include "qgsservercachefilter.h"
#include "qmap.h"

class sbServerCacheFilter : public QgsServerCacheFilter
{
  private:
    QString                m_strDirectory;
    bool                   m_bInitialized;
    QMap<QString, QString> m_mapProjectIds;

    QString   initializeProjectDirectory( const QgsProject *project );
    QString   getCacheKey( const QgsServerRequest &request ) const;

  public:
    sbServerCacheFilter( const QgsServerInterface *serverInterface, const QString &strDirectory );
    virtual   ~sbServerCacheFilter();

    virtual   QByteArray getCachedDocument( const QgsProject *project, const QgsServerRequest &request, const QString &key );
    virtual   bool       setCachedDocument( const QDomDocument *doc, const QgsProject *project, const QgsServerRequest &request, const QString &key );
    virtual   bool       deleteCachedDocument( const QgsProject *project, const QgsServerRequest &request, const QString &key );
    virtual   bool       deleteCachedDocuments( const QgsProject *project );
    virtual   QByteArray getCachedImage( const QgsProject *project, const QgsServerRequest &request, const QString &key );
    virtual   bool       setCachedImage( const QByteArray *img, const QgsProject *project, const QgsServerRequest &request, const QString &key );
    virtual   bool       deleteCachedImage( const QgsProject *project, const QgsServerRequest &request, const QString &key );
    virtual   bool       deleteCachedImages( const QgsProject *project );
    virtual   QString    sbGetProjectCacheId( const QgsProject *project );
};

#endif // SBSERVERCACHEFILTER_H
