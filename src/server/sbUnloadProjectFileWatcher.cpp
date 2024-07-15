/***************************************************************************
sbunloadprojectfilewatcher.cpp

sb service implementation
-----------------------------
begin                : 2021-04-18
copyright            : (C) 2021 by Andreas Steffens
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

#include "sbunloadprojectfilewatcher.h"
#include "qgsmessagelog.h"
#include "sbutils.h"
#include "qgsserverexception.h"

sbUnloadProjectFileWatcher::sbUnloadProjectFileWatcher()
{
  m_dtLastRead = QDateTime::currentDateTime().addYears( -10 );
}

QStringList sbUnloadProjectFileWatcher::unloadedProjects()
{
  QStringList listUnloadedProjects;

  for ( QgsStringMap::const_iterator it = m_mapUnloadProjectFiles.constBegin(); it != m_mapUnloadProjectFiles.constEnd(); it++ )
    listUnloadedProjects.append( it.key() );

  return listUnloadedProjects;
}

void sbUnloadProjectFileWatcher::setWatchedPath( const QString &strUnloadFilename )
{
  m_strUnloadFilename = strUnloadFilename;
}

sbUnloadProjectFileWatcher::~sbUnloadProjectFileWatcher()
{
  // nothing to be done here for now
}

bool sbUnloadProjectFileWatcher::isUnloaded( const QString &strProjectFilename )
{
  bool bRes = false;

  QString strProjectFilenameStandard = sbGetStandardizedPath( strProjectFilename );
  bRes = m_mapUnloadProjectFiles.contains( strProjectFilenameStandard );

  return bRes;
}

void sbUnloadProjectFileWatcher::clearUnloadProjects()
{
  m_mapUnloadProjectFiles.clear();
}

void sbUnloadProjectFileWatcher::readUnloadProjects()
{
  if ( m_strUnloadFilename.isNull() || m_strUnloadFilename.isEmpty() )
  {
    QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::run - Timeout and/or unload filename not set" ), QStringLiteral( "Server" ), Qgis::Warning );
    return;
  }

  try
  {
    m_mapUnloadProjectFiles.clear();

    QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::readUnloadProjects - Reading unload projects from path '%1'" ).arg( m_strUnloadFilename ), QStringLiteral( "Server" ), Qgis::Info );

    QFile unloadFile( m_strUnloadFilename );
    if ( unloadFile.exists() )
    {
      QFileInfo info( m_strUnloadFilename );
      QDateTime dtLastModified = info.lastModified();
      if ( dtLastModified > m_dtLastRead )
      {
        if ( unloadFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
        {
          QTextStream streamIn( &unloadFile );
          QString strLine;
          while ( streamIn.readLineInto( &strLine ) )
          {
            if ( strLine.isNull() || strLine.isEmpty() )
              continue;

            strLine = sbGetStandardizedPath( strLine );

            if ( !m_mapUnloadProjectFiles.contains( strLine ) )
              m_mapUnloadProjectFiles.insert( strLine, strLine );

            bool bRes = QgsConfigCache::instance()->removeEntry( strLine );

            QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::readUnloadProjects - Trying to unload project '%1' ... %2" ).arg( strLine ).arg( bRes ), QStringLiteral( "Server" ), Qgis::Info );
          }

          unloadFile.close();
          m_dtLastRead = dtLastModified;

          int iSplitIndex = m_strUnloadFilename.lastIndexOf( "/" );
          if ( iSplitIndex < 0 )
            iSplitIndex = m_strUnloadFilename.lastIndexOf( "\\" );

          QString strDirectory = m_strUnloadFilename.left( iSplitIndex + 1 );
          QString strFilename = m_strUnloadFilename.right( m_strUnloadFilename.length() - iSplitIndex - 1 );
          QString strTimestampFilename = strDirectory + "." + strFilename;
          QFile timestampFile( strTimestampFilename );
          if ( timestampFile.open( QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate ) )
            timestampFile.close();
        }
        else
          QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::readUnloadProjects - Failed to open unload projects file '%1'" ).arg( m_strUnloadFilename ), QStringLiteral( "Server" ), Qgis::Warning );
      }
    }
    else
      QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::readUnloadProjects - Unload projects file '%1' doesn't exist" ).arg( m_strUnloadFilename ), QStringLiteral( "Server" ), Qgis::Info );
  }
  catch ( QgsServerException &ex )
  {
    QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::readUnloadProjects - QgsServerException: %1" ).arg( QString( ex.what() ) ), QStringLiteral( "Server" ), Qgis::Critical );
  }
  catch ( QgsException &ex )
  {
    QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::readUnloadProjects - QgsException: %1" ).arg( QString( ex.what() ) ), QStringLiteral( "Server" ), Qgis::Critical );
  }
  catch ( std::runtime_error &ex )
  {
    QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::readUnloadProjects - RuntimeError: %1" ).arg( QString( ex.what() ) ), QStringLiteral( "Server" ), Qgis::Critical );
  }
  catch ( std::exception &ex )
  {
    QString message = QString::fromUtf8( ex.what() );
    QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::readUnloadProjects - std::exception: %1" ).arg( message ), QStringLiteral( "Server" ), Qgis::Critical );
  }
  catch ( ... )
  {
    QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::readUnloadProjects - Unknown exception" ), QStringLiteral( "Server" ), Qgis::Critical );
  }
}
