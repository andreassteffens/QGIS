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

sbUnloadProjectFileWatcher::sbUnloadProjectFileWatcher()
{
  m_iTimeout = 8000;
}

void sbUnloadProjectFileWatcher::setTimeout( int iTimeout )
{
  m_iTimeout = iTimeout;
}

void sbUnloadProjectFileWatcher::setWatchedPath( QString strUnloadFilename )
{
  m_strUnloadFilename = strUnloadFilename;

  readUnloadProjects();
}

sbUnloadProjectFileWatcher::~sbUnloadProjectFileWatcher()
{
  if ( isFinished() )
    return;

  requestInterruption();
  wait( 2000 );

  if ( !isFinished() )
  {
    exit();
    wait( 2000 );

    if ( !isFinished() )
      terminate();
  }
}

void sbUnloadProjectFileWatcher::run()
{
  QgsMessageLog::logMessage( QStringLiteral( "([a]tapa) Starting unload project file watcher on file '%1' with timeout %2" ).arg( m_strUnloadFilename ).arg( QString::number( m_iTimeout ) ), QStringLiteral( "Server" ), Qgis::Info );

  if ( m_iTimeout <= 0 )
    return;

  QFile unloadFile( m_strUnloadFilename );
  if ( !unloadFile.exists() )
  {
    if ( unloadFile.open( QIODevice::ReadWrite | QIODevice::Text ) )
      unloadFile.close();
  }

  while ( !unloadFile.exists() )
    std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

  QDateTime dtLastRead = QDateTime::currentDateTime().addYears( -10 );
  while ( !isInterruptionRequested() )
  {
    msleep( m_iTimeout );

    if ( isInterruptionRequested() )
      break;

    if ( unloadFile.exists() )
    {
      QFileInfo info( m_strUnloadFilename );

      QDateTime dtLastModified = info.lastModified();
      if ( dtLastModified > dtLastRead )
      {
        readUnloadProjects();
        dtLastRead = dtLastModified;
      }
    }
    else
      clearUnloadProjects();
  }

  QgsMessageLog::logMessage( QStringLiteral( "([a]tapa) Terminating unload project file watcher on file '%1'" ).arg( m_strUnloadFilename ), QStringLiteral( "Server" ), Qgis::Info );
}

bool sbUnloadProjectFileWatcher::isUnloaded( QString strProjectFilename )
{
  bool bRes = false;

  if ( m_mutexProjectFiles.tryLock( 2000 ) )
  {
    QString strProjectFilenameStandard = sbGetStandardizedPath( strProjectFilename );
    bRes = m_mapUnloadProjectFiles.contains( strProjectFilenameStandard );

    m_mutexProjectFiles.unlock();
  }

  return bRes;
}

void sbUnloadProjectFileWatcher::clearUnloadProjects()
{
  m_mutexProjectFiles.lock();
  {
    m_mapUnloadProjectFiles.clear();
  }
  m_mutexProjectFiles.unlock();
}

void sbUnloadProjectFileWatcher::readUnloadProjects()
{
  m_mutexProjectFiles.lock();
  {
    try
    {
      m_mapUnloadProjectFiles.clear();

      QgsMessageLog::logMessage( QStringLiteral( "([a]tapa) Reading unload projects from path '%1'" ).arg( m_strUnloadFilename ), QStringLiteral( "Server" ), Qgis::Info );

      QFile unloadFile( m_strUnloadFilename );

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

          QgsMessageLog::logMessage( QStringLiteral( "([a]tapa) Trying to unload project '%1' ... %2" ).arg( strLine ).arg( bRes ), QStringLiteral( "Server" ), Qgis::Info );
        }

        unloadFile.close();

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
        QgsMessageLog::logMessage( QStringLiteral( "([a]tapa) Failed to open unload projects file '%1'" ).arg( m_strUnloadFilename ), QStringLiteral( "Server" ), Qgis::Warning );
    }
    catch ( ... )
    {
      QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher - Unknown exception" ), QStringLiteral( "Server" ), Qgis::Critical );
    }
  }
  m_mutexProjectFiles.unlock();
}
