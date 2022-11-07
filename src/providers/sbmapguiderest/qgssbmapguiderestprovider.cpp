/***************************************************************************
      qgssbmapguiderestprovider.cpp  -  Data provider for MapGuide OS server
                             -------------------
    begin                : 2018-10-08
    copyright            : (C) 2018 by Andreas Steffens
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

#include <algorithm>
#include <limits>
#include <cstring>
#include <cmath>

// Changed #include <qapp.h> to <qapplication.h>. Apparently some
// debian distros do not include the qapp.h wrapper and the compilation
// fails. [gsherman]
#include <QApplication>
#include <QFile>
#include <QTextCodec>
#include <QTextStream>
#include <QObject>

#include "qgis.h"
#include "qgsapplication.h"
#include "qgscoordinatereferencesystem.h"
#include "qgsdataprovider.h"
#include "qgsfeature.h"
#include "qgsfields.h"
#include "qgsgeometry.h"
#include "qgslogger.h"
#include "qgsrectangle.h"

