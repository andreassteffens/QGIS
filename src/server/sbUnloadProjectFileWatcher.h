/***************************************************************************
sbunloadprojectfilewatcher.h

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

#ifndef SBUNLOADPROJECTFILEWATCHER_H
#define SBUNLOADPROJECTFILEWATCHER_H

#include <qthread.h>
#include <QFileInfo>
#include <qfilesystemwatcher.h>
#include "qgsconfigcache.h"

class sbUnloadProjectFileWatcher
{
  private:
    QString mUnloadFilename;
    QgsStringMap mUnloadProjectFiles;
    QDateTime mLastRead;

    void clearUnloadProjects();

  public:
    sbUnloadProjectFileWatcher();
    ~sbUnloadProjectFileWatcher();

    QStringList unloadedProjects() const;

    void setWatchedPath( const QString &unloadFilename );

    void readUnloadProjects();

    bool isUnloaded( const QString &projectFilename ) const;
};

#endif // SBUNLOADPROJECTFILEWATCHER_H
