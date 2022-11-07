/***************************************************************************
                              qgssbgetserverprocessid.cpp
                              ---------------------------
  begin                : 2020-05-24
  copyright            : (C) 2020 by Andreas Steffens
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

#include "qgsmodule.h"
#include "qgssbserviceexception.h"
#include "qgsapplication.h"

#include "qgssbgetserverprocessid.h"
#include "qgsserverprojectutils.h"
#include "qgsconfigcache.h"
#include "qgssettings.h"

#include "qgsexception.h"
#include "qgsexpressionnodeimpl.h"

#include "qgsexception.h"

namespace QgsSb
{
  void writeGetServerProcessId( QgsServerInterface *serverIface, 
                        const QString &version, const QgsServerRequest &request,
                        QgsServerResponse &response )
  {
	quint64 ulProcessId = QgsApplication::applicationPid();
	QString strId = QString::number(ulProcessId);

	response.setStatusCode(200);
    response.setHeader( QStringLiteral( "Content-Type" ), QStringLiteral( "text/plain; charset=utf-8" ) );
    response.write( strId );
  }

} // namespace QgsWms
