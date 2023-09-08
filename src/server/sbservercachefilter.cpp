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

sbServerCacheFilter::sbServerCacheFilter( const QgsServerInterface *serverInterface, const QString &strDirectory ) : QgsServerCacheFilter( serverInterface )
{
  m_bInitialized = false;

  m_strDirectory = strDirectory;

  if ( !QDir( m_strDirectory ).exists() )
    QDir().mkdir( m_strDirectory );

  m_bInitialized = QDir( m_strDirectory ).exists();
}

sbServerCacheFilter::~sbServerCacheFilter()
{
  // nothing to be done here for now
}

QString sbServerCacheFilter::initializeProjectDirectory( const QgsProject *project )
{
  QString strEncrypted = sbGetProjectCacheId( project );

  QString strProjectDirectory = QDir( m_strDirectory ).filePath( strEncrypted );
  if ( !QDir( strProjectDirectory ).exists() )
    QDir().mkdir( strProjectDirectory );

  QString strDocumentDirectory = QDir( strProjectDirectory ).filePath( "documents" );
  if ( !QDir( strDocumentDirectory ).exists() )
    QDir().mkdir( strDocumentDirectory );

  QString strImageDirectory = QDir( strProjectDirectory ).filePath( "images" );
  if ( !QDir( strImageDirectory ).exists() )
    QDir().mkdir( strImageDirectory );

  return strProjectDirectory;
}

QString sbServerCacheFilter::getCacheKey( const QgsServerRequest &request ) const
{
  QString strKey = "";

  QMap<QString, QString> mapParams = request.parameters();

  QList<QString> listKeys = mapParams.keys();
  std::sort( listKeys.begin(), listKeys.end() );

  QList<QString>::const_iterator iter;
  for ( iter = listKeys.constBegin(); iter != listKeys.constEnd(); iter++ )
    strKey += "&" + *iter + "=" + mapParams[*iter];

  strKey = QString( QCryptographicHash::hash( ( strKey.toLower().toUtf8() ), QCryptographicHash::Md5 ).toHex() );

  return strKey;
}

QByteArray sbServerCacheFilter::getCachedDocument( const QgsProject *project, const QgsServerRequest &request, const QString &key )
{
  if ( !m_bInitialized )
    return QByteArray();

  QString strProjectDirectory = initializeProjectDirectory( project );
  QString strCacheKey = getCacheKey( request );
  QString strDocumentFilename = QDir( QDir( strProjectDirectory ).filePath( "documents" ) ).filePath( strCacheKey );

  QByteArray arrDocument;
  QFile file( strDocumentFilename );
  if ( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
  {
    QTextStream stream( &file );
    stream.autoDetectUnicode();
    QString strFile = stream.readAll();
    arrDocument = strFile.toUtf8();

    file.close();
  }

  return arrDocument;
}

bool sbServerCacheFilter::setCachedDocument( const QDomDocument *doc, const QgsProject *project, const QgsServerRequest &request, const QString &key )
{
  if ( !m_bInitialized )
    return false;

  QString strProjectDirectory = initializeProjectDirectory( project );
  QString strCacheKey = getCacheKey( request );
  QString strDocumentFilename = QDir( QDir( strProjectDirectory ).filePath( "documents" ) ).filePath( strCacheKey );

  QFile file( strDocumentFilename );
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
  if ( !m_bInitialized )
    return false;

  QString strProjectDirectory = initializeProjectDirectory( project );
  QString strCacheKey = getCacheKey( request );
  QString strDocumentFilename = QDir( QDir( strProjectDirectory ).filePath( "documents" ) ).filePath( strCacheKey );

  QFile file( strDocumentFilename );
  if ( file.exists() )
    file.remove();

  return true;
}

bool sbServerCacheFilter::deleteCachedDocuments( const QgsProject *project )
{
  if ( !m_bInitialized )
    return false;

  QString strProjectDirectory = initializeProjectDirectory( project );
  QString strDocumentDirectory = QDir( strProjectDirectory ).filePath( "documents" );

  QDir dirDocuments( strDocumentDirectory );
  if ( dirDocuments.exists() )
  {
    if ( dirDocuments.removeRecursively() )
      QDir().mkdir( strDocumentDirectory );
  }

  return true;
}

QByteArray sbServerCacheFilter::getCachedImage( const QgsProject *project, const QgsServerRequest &request, const QString &key )
{
  if ( !m_bInitialized )
    return false;

  QString strProjectDirectory = initializeProjectDirectory( project );
  QString strCacheKey = getCacheKey( request );
  QString strImageFilename = QDir( QDir( strProjectDirectory ).filePath( "images" ) ).filePath( strCacheKey );

  QByteArray arrImage;
  QFile file( strImageFilename );
  if ( file.open( QIODevice::ReadOnly ) )
  {
    arrImage = file.readAll();
    file.close();
  }

  return arrImage;
}

bool sbServerCacheFilter::setCachedImage( const QByteArray *img, const QgsProject *project, const QgsServerRequest &request, const QString &key )
{
  if ( !m_bInitialized )
    return false;

  QString strProjectDirectory = initializeProjectDirectory( project );
  QString strCacheKey = getCacheKey( request );
  QString strImageFilename = QDir( QDir( strProjectDirectory ).filePath( "images" ) ).filePath( strCacheKey );

  QFile file( strImageFilename );
  if ( file.open( QIODevice::WriteOnly | QIODevice::Truncate ) )
  {
    file.write( *img );
    file.close();
  }

  return true;
}

bool sbServerCacheFilter::deleteCachedImage( const QgsProject *project, const QgsServerRequest &request, const QString &key )
{
  if ( !m_bInitialized )
    return false;

  QString strProjectDirectory = initializeProjectDirectory( project );
  QString strCacheKey = getCacheKey( request );
  QString strImageFilename = QDir( QDir( strProjectDirectory ).filePath( "images" ) ).filePath( strCacheKey );

  QFile file( strImageFilename );
  if ( file.exists() )
    file.remove();

  return true;
}

bool sbServerCacheFilter::deleteCachedImages( const QgsProject *project )
{
  if ( !m_bInitialized )
    return false;

  QString strProjectDirectory = initializeProjectDirectory( project );
  QString strImageDirectory = QDir( strProjectDirectory ).filePath( "images" );

  QDir dirImages( strImageDirectory );
  if ( dirImages.exists() )
  {
    if ( dirImages.removeRecursively() )
      QDir().mkdir( strImageDirectory );
  }

  return true;
}

QString sbServerCacheFilter::sbGetProjectCacheId(const QgsProject* project)
{
  QString strKey = project->absoluteFilePath().toLower().toUtf8();
  if (!m_mapProjectIds.contains(strKey))
  {
    QString strId = QString(QCryptographicHash::hash((project->absoluteFilePath().toLower().toUtf8()), QCryptographicHash::Md5).toHex());
    m_mapProjectIds[strKey] = strId;
  }

  return m_mapProjectIds[strKey];
}
