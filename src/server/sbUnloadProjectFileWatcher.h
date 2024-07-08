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

#define SIP_NO_FILE


#include <qthread.h>
#include <QFileInfo>
#include <qfilesystemwatcher.h>
#include "qgsconfigcache.h"


class sbUnloadProjectFileWatcher
{
  private:
    QString             m_strUnloadFilename;
    QgsStringMap        m_mapUnloadProjectFiles;
    QDateTime           m_dtLastRead;

    void clearUnloadProjects();

  public:
    sbUnloadProjectFileWatcher();
    ~sbUnloadProjectFileWatcher();

    QStringList unloadedProjects();

    void setWatchedPath( const QString &strUnloadFilename );

    void readUnloadProjects();

    bool isUnloaded( const QString &strProjectFilename );
};

#endif // SBUNLOADPROJECTFILEWATCHER_H
