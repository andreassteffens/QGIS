/***************************************************************************
                              sbservercachefilter.cpp
                              -------------------
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

#include "sbservercachefilter.h"
#include "qgsproject.h"

#include <QDomDocument>
#include <qcryptographichash.h>

sbServerCacheFilter::sbServerCacheFilter( const QgsServerInterface *serverInterface, const QString &directory ) : QgsServerCacheFilter( serverInterface )
{
  mInitialized = false;

  mDirectory = strDirectory;

  if ( !QDir( mDirectory ).exists() )
    QDir().mkdir( mDirectory );

  mInitialized = QDir( mDirectory ).exists();
}

sbServerCacheFilter::~sbServerCacheFilter()
{
  // nothing to be done here for now
}

QString sbServerCacheFilter::initializeProjectDirectory( const QgsProject *project )
{
  QString encrypted = sbGetProjectCacheId( project );

  QString projectDirectory = QDir( m_strDirectory ).filePath( encrypted );
  if ( !QDir( projectDirectory ).exists() )
    QDir().mkdir( projectDirectory );

  QString documentDirectory = QDir( projectDirectory ).filePath( "documents" );
  if ( !QDir( documentDirectory ).exists() )
    QDir().mkdir( documentDirectory );

  QString imageDirectory = QDir( projectDirectory ).filePath( "images" );
  if ( !QDir( imageDirectory ).exists() )
    QDir().mkdir( imageDirectory );

  return projectDirectory;
}

QString sbServerCacheFilter::getCacheKey( const QgsServerRequest &request ) const
{
  QString key = "";

  QMap<QString, QString> params = request.parameters();

  QList<QString> keys = params.keys();
  std::sort( keys.begin(), keys.end() );

  QList<QString>::const_iterator iter;
  for ( iter = keys.constBegin(); iter != keys.constEnd(); iter++ )
    key += "&" + *iter + "=" + params[*iter];

  key = QString( QCryptographicHash::hash( ( key.toLower().toUtf8() ), QCryptographicHash::Md5 ).toHex() );

  return key;
}

QByteArray sbServerCacheFilter::getCachedDocument( const QgsProject *project, const QgsServerRequest &request, const QString &key )
{
  if ( !mInitialized )
    return QByteArray();

  QString projectDirectory = initializeProjectDirectory( project );
  QString cacheKey = getCacheKey( request );
  QString documentFilename = QDir( QDir( projectDirectory ).filePath( "documents" ) ).filePath( cacheKey );

  QByteArray document;
  QFile file( documentFilename );
  if ( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
  {
    QTextStream stream( &file );
    stream.autoDetectUnicode();
    QString content = stream.readAll();
    document = content.toUtf8();

    file.close();
  }

  return document;
}

bool sbServerCacheFilter::setCachedDocument( const QDomDocument *doc, const QgsProject *project, const QgsServerRequest &request, const QString &key )
{
  if ( !mInitialized )
    return false;

  QString projectDirectory = initializeProjectDirectory( project );
  QString cacheKey = getCacheKey( request );
  QString documentFilename = QDir( QDir( projectDirectory ).filePath( "documents" ) ).filePath( cacheKey );

  QFile file( documentFilename );
  if ( file.open( QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate ) )
  {
    QTextStream stream( &file );
    stream << doc->toString();

    file.close();
  }

  return true;
}

bool sbServerCacheFilter::deleteCachedDocument( const QgsProject *project, const QgsServerRequest &request, const QString &key )
{
  if ( !mInitialized )
    return false;

  QString projectDirectory = initializeProjectDirectory( project );
  QString cacheKey = getCacheKey( request );
  QString documentFilename = QDir( QDir( projectDirectory ).filePath( "documents" ) ).filePath( cacheKey );

  QFile file( documentFilename );
  if ( file.exists() )
    file.remove();

  return true;
}

bool sbServerCacheFilter::deleteCachedDocuments( const QgsProject *project )
{
  if ( !mInitialized )
    return false;

  QString projectDirectory = initializeProjectDirectory( project );
  QString documentDirectory = QDir( projectDirectory ).filePath( "documents" );

  QDir documents( documentDirectory );
  if ( documents.exists() )
  {
    if ( documents.removeRecursively() )
      QDir().mkdir( documentDirectory );
  }

  return true;
}

QByteArray sbServerCacheFilter::getCachedImage( const QgsProject *project, const QgsServerRequest &request, const QString &key )
{
  if ( !mInitialized )
    return QByteArray();

  QString projectDirectory = initializeProjectDirectory( project );
  QString cacheKey = getCacheKey( request );
  QString imageFilename = QDir( QDir( projectDirectory ).filePath( "images" ) ).filePath( cacheKey );

  QByteArray image;
  QFile file( imageFilename );
  if ( file.open( QIODevice::ReadOnly ) )
  {
    image = file.readAll();
    file.close();
  }

  return image;
}

bool sbServerCacheFilter::setCachedImage( const QByteArray *img, const QgsProject *project, const QgsServerRequest &request, const QString &key )
{
  if ( !mInitialized )
    return false;

  QString projectDirectory = initializeProjectDirectory( project );
  QString cacheKey = getCacheKey( request );
  QString imageFilename = QDir( QDir( projectDirectory ).filePath( "images" ) ).filePath( cacheKey );

  QFile file( imageFilename );
  if ( file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
  {
    file.write( *img );
    file.close();
  }

  return true;
}

bool sbServerCacheFilter::deleteCachedImage( const QgsProject *project, const QgsServerRequest &request, const QString &key )
{
  if ( !mInitialized )
    return false;

  QString projectDirectory = initializeProjectDirectory( project );
  QString cacheKey = getCacheKey( request );
  QString imageFilename = QDir( QDir( projectDirectory ).filePath( "images" ) ).filePath( cacheKey );

  QFile file( imageFilename );
  if ( file.exists() )
    file.remove();

  return true;
}

bool sbServerCacheFilter::deleteCachedImages( const QgsProject *project )
{
  if ( !mInitialized )
    return false;

  QString projectDirectory = initializeProjectDirectory( project );
  QString imageDirectory = QDir( strProjectDirectory ).filePath( "images" );

  QDir images( imageDirectory );
  if ( images.exists() )
  {
    if ( images.removeRecursively() )
      QDir().mkdir( imageDirectory );
  }

  return true;
}

QString sbServerCacheFilter::sbGetProjectCacheId( const QgsProject *project )
{
  QString id = "";

  QString key = project->absoluteFilePath().toLower().toUtf8();
  if ( !mProjectIds.contains( key ) )
  {
    id = QString( QCryptographicHash::hash( ( project->absoluteFilePath().toLower().toUtf8() ), QCryptographicHash::Md5 ).toHex() );
    mProjectIds[key] = id;
  }
  else
    id = mProjectIds[key];

  return id;
}
