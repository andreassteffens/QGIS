/***************************************************************************
                              qgssbgetdecryptedpath.cpp
                              -------------------------
  begin                : 2024-04-24
  copyright            : (C) 2024 by Andreas Steffens
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
#include "SimpleCrypt.h"

#include "qgssbgetdecryptedpath.h"
#include "qgsserverprojectutils.h"
#include "qgsconfigcache.h"

#include "qgsexception.h"
#include "qgsexpressionnodeimpl.h"

namespace QgsSb
{
  void writeGetDecryptedPath( QgsServerInterface *serverIface, const QgsProject *project,
                              const QString &version, const QgsServerRequest &request,
                              QgsServerResponse &response, const QString &path )
  {
    Q_UNUSED( version );

    QString qstrDecrypted = SimpleCrypt::sbDecrypt( path );

    response.setStatusCode( 200 );
    response.setHeader( QStringLiteral( "Content-Type" ), QStringLiteral( "text/plain; charset=utf-8" ) );
    response.write( qstrDecrypted );
  }
} // namespace QgsSb
