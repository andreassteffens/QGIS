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
  mLastRead = QDateTime::currentDateTime().addYears( -10 );
}

QStringList sbUnloadProjectFileWatcher::unloadedProjects() const
{
  QStringList unloadedProjects;

  for ( QgsStringMap::const_iterator it = mUnloadProjectFiles.constBegin(); it != mUnloadProjectFiles.constEnd(); it++ )
    unloadedProjects.append( it.key() );

  return unloadedProjects;
}

void sbUnloadProjectFileWatcher::setWatchedPath( const QString &unloadFilename )
{
  mUnloadFilename = unloadFilename;
}

sbUnloadProjectFileWatcher::~sbUnloadProjectFileWatcher()
{
  // nothing to be done here for now
}

bool sbUnloadProjectFileWatcher::isUnloaded( const QString &projectFilename ) const
{
  bool res = false;

  QString projectFilenameStandard = sbGetStandardizedPath( projectFilename );
  res = mUnloadProjectFiles.contains( projectFilenameStandard );

  return res;
}

void sbUnloadProjectFileWatcher::clearUnloadProjects()
{
  mUnloadProjectFiles.clear();
}

void sbUnloadProjectFileWatcher::readUnloadProjects()
{
  if ( mUnloadFilename.isNull() || mUnloadFilename.isEmpty() )
  {
    QgsMessageLog::logMessage( u"sbUnloadProjectFileWatcher::run - Timeout and/or unload filename not set"_s, u"Server"_s, Qgis::Warning );
    return;
  }

  try
  {
    mUnloadProjectFiles.clear();

    QgsMessageLog::logMessage( u"sbUnloadProjectFileWatcher::readUnloadProjects - Reading unload projects from path '%1'"_s.arg( mUnloadFilename ), u"Server"_s, Qgis::Info );

    QFile unloadFile( mUnloadFilename );
    if ( unloadFile.exists() )
    {
      QFileInfo info( mUnloadFilename );
      QDateTime lastModified = info.lastModified();
      if ( lastModified > mLastRead )
      {
        if ( unloadFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
        {
          QTextStream stream( &unloadFile );
          QString line;
          while ( stream.readLineInto( &line ) )
          {
            if ( line.isNull() || line.isEmpty() )
              continue;

            line = sbGetStandardizedPath( line );

            if ( !mUnloadProjectFiles.contains( line ) )
              mUnloadProjectFiles.insert( line, line );

            bool res = QgsConfigCache::instance()->removeEntry( line );

            QgsMessageLog::logMessage( u"sbUnloadProjectFileWatcher::readUnloadProjects - Trying to unload project '%1' ... %2"_s.arg( line ).arg( res ), u"Server"_s, Qgis::Info );
          }

          unloadFile.close();
          mLastRead = lastModified;

          int splitIndex = mUnloadFilename.lastIndexOf( "/" );
          if ( splitIndex < 0 )
            splitIndex = mUnloadFilename.lastIndexOf( "\\" );

          QString directory = mUnloadFilename.left( splitIndex + 1 );
          QString filename = mUnloadFilename.right( mUnloadFilename.length() - splitIndex - 1 );
          QString timestampFilename = directory + "." + filename;
          QFile timestampFile( timestampFilename );
          if ( timestampFile.open( QIODevice::ReadWrite | QIODevice::Text | QIODevice::Truncate ) )
            timestampFile.close();
        }
        else
          QgsMessageLog::logMessage( u"sbUnloadProjectFileWatcher::readUnloadProjects - Failed to open unload projects file '%1'"_s.arg( mUnloadFilename ), u"Server"_s, Qgis::Warning );
      }
    }
    else
      QgsMessageLog::logMessage( u"sbUnloadProjectFileWatcher::readUnloadProjects - Unload projects file '%1' doesn't exist"_s.arg( mUnloadFilename ), u"Server"_s, Qgis::Info );
  }
  catch ( QgsServerException &ex )
  {
    QgsMessageLog::logMessage( u"sbUnloadProjectFileWatcher::readUnloadProjects - QgsServerException: %1"_s.arg( QString( ex.what() ) ), u"Server"_s, Qgis::Critical );
  }
  catch ( QgsException &ex )
  {
    QgsMessageLog::logMessage( u"sbUnloadProjectFileWatcher::readUnloadProjects - QgsException: %1"_s.arg( QString( ex.what() ) ), u"Server"_s, Qgis::Critical );
  }
  catch ( std::runtime_error &ex )
  {
    QgsMessageLog::logMessage( u"sbUnloadProjectFileWatcher::readUnloadProjects - RuntimeError: %1"_s.arg( QString( ex.what() ) ), u"Server"_s, Qgis::Critical );
  }
  catch ( std::exception &ex )
  {
    QString message = QString::fromUtf8( ex.what() );
    QgsMessageLog::logMessage( u"sbUnloadProjectFileWatcher::readUnloadProjects - std::exception: %1"_s.arg( message ), u"Server"_s, Qgis::Critical );
  }
  catch ( ... )
  {
    QgsMessageLog::logMessage( u"sbUnloadProjectFileWatcher::readUnloadProjects - Unknown exception"_s, QStringLiteral( "Server" ), Qgis::Critical );
  }
}
