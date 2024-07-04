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
  : m_iTimeout( 8000 )
{
}

QStringList sbUnloadProjectFileWatcher::unloadedProjects()
{
  QStringList listUnloadedProjects;

  if ( m_mutexProjectFiles.tryLock( 3000 ) )
  {
    for ( QgsStringMap::const_iterator it = m_mapUnloadProjectFiles.constBegin(); it != m_mapUnloadProjectFiles.constEnd(); it++ )
      listUnloadedProjects.append( it.key() );

    m_mutexProjectFiles.unlock();
  }
  else
    QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::unloadedProjects - Failed to access mutex" ), QStringLiteral( "Server" ), Qgis::Warning );

  return listUnloadedProjects;
}

void sbUnloadProjectFileWatcher::setTimeout( int iTimeout )
{
  m_iTimeout = iTimeout;
}

void sbUnloadProjectFileWatcher::setWatchedPath( const QString &strUnloadFilename )
{
  m_strUnloadFilename = strUnloadFilename;

  readUnloadProjects();
}

void sbUnloadProjectFileWatcher::setServerSettings( QgsServerSettings *pSettings )
{
  m_pServerSettings = pSettings;
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
  QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::run - Starting unload project file watcher on file '%1' with timeout %2" ).arg( m_strUnloadFilename ).arg( QString::number( m_iTimeout ) ), QStringLiteral( "Server" ), Qgis::Info );

  try
  {
    if ( m_iTimeout <= 0 || m_strUnloadFilename.isNull() || m_strUnloadFilename.isEmpty() )
    {
      QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::run - Timeout and/or unload filename not set" ), QStringLiteral( "Server" ), Qgis::Warning );
      return;
    }

    QFile unloadFile( m_strUnloadFilename );
    if ( !unloadFile.exists() )
    {
      if ( unloadFile.open( QIODevice::ReadWrite | QIODevice::Text ) )
        unloadFile.close();
    }

    int iCreationTimeoutIntervalls = 20;
    while ( !unloadFile.exists() && iCreationTimeoutIntervalls > 0 )
    {
      msleep( 100 );

      iCreationTimeoutIntervalls--;
    }

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
      {
        clearUnloadProjects();
      }
    }
  }
  catch ( QgsServerException &ex )
  {
    QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::run - QgsServerException: %1" ).arg( QString( ex.what() ) ), QStringLiteral( "Server" ), Qgis::Critical );
  }
  catch (QgsException& ex)
  {
    QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::run - QgsException: %1" ).arg( QString( ex.what() ) ), QStringLiteral( "Server" ), Qgis::Critical );
  }
  catch (std::runtime_error& ex)
  {
    QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::run - RuntimeError: %1" ).arg( QString( ex.what() ) ), QStringLiteral( "Server" ), Qgis::Critical );
  }
  catch ( const std::exception &ex )
  {
    QString message = QString::fromUtf8( ex.what() );
    QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::run - std::exception: %1" ).arg( message ), QStringLiteral( "Server" ), Qgis::Critical );
  }
  catch ( ... )
  {
    QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::run - Unknown exception" ), QStringLiteral( "Server" ), Qgis::Critical );
  }

  QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::run - Terminating unload project file watcher on file '%1'" ).arg( m_strUnloadFilename ), QStringLiteral( "Server" ), Qgis::Info );
}

bool sbUnloadProjectFileWatcher::isUnloaded( const QString &strProjectFilename )
{
  bool bRes = false;

  if ( m_mutexProjectFiles.tryLock( 3000 ) )
  {
    QString strProjectFilenameStandard = sbGetStandardizedPath( strProjectFilename );
    bRes = m_mapUnloadProjectFiles.contains( strProjectFilenameStandard );

    m_mutexProjectFiles.unlock();
  }
  else
    QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::isUnloaded - Failed to access mutex" ), QStringLiteral( "Server" ), Qgis::Warning );

  return bRes;
}

void sbUnloadProjectFileWatcher::clearUnloadProjects()
{
  if ( m_mutexProjectFiles.tryLock( 3000 ) )
  {
    m_mapUnloadProjectFiles.clear();

    m_mutexProjectFiles.unlock();
  }
  else
    QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::clearUnloadProjects - Failed to access mutex" ), QStringLiteral( "Server" ), Qgis::Warning );
}

void sbUnloadProjectFileWatcher::readUnloadProjects()
{
  if ( m_mutexProjectFiles.tryLock( 3000 ) )
  {
    try
    {
      m_mapUnloadProjectFiles.clear();

      QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::readUnloadProjects - Reading unload projects from path '%1'" ).arg( m_strUnloadFilename ), QStringLiteral( "Server" ), Qgis::Info );

      QFile unloadFile( m_strUnloadFilename );
      if ( unloadFile.exists() )
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
      else
        QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::readUnloadProjects - Unload projects file '%1' doesn't exist" ).arg( m_strUnloadFilename ), QStringLiteral( "Server" ), Qgis::Info );
    }
    catch ( QgsServerException &ex )
    {
      QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::readUnloadProjects - QgsServerException: %1" ).arg( QString( ex.what() ) ), QStringLiteral( "Server" ), Qgis::Critical );
    }
    catch (QgsException& ex)
    {
      QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::readUnloadProjects - QgsException: %1" ).arg( QString( ex.what() ) ), QStringLiteral( "Server" ), Qgis::Critical );
    }
    catch (std::runtime_error& ex)
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

    m_mutexProjectFiles.unlock();
  }
  else
    QgsMessageLog::logMessage( QStringLiteral( "sbUnloadProjectFileWatcher::readUnloadProjects - Failed to access mutex" ), QStringLiteral( "Server" ), Qgis::Warning );
}
