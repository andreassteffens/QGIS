/***************************************************************************
                              qgssbgetencryptedpath.cpp
                              -------------------------
  begin                : 2018-12-13
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

#include "qgsmodule.h"
#include "qgssbserviceexception.h"
#include "SimpleCrypt.h"

#include "qgssbgetencryptedpath.h"
#include "qgsserverprojectutils.h"
#include "qgsconfigcache.h"

#include "qgsexception.h"
#include "qgsexpressionnodeimpl.h"

namespace QgsSb
{
  void writeGetEncryptedPath( QgsServerInterface *serverIface, const QgsProject *project,
                              const QString &version, const QgsServerRequest &request,
                              QgsServerResponse &response, const QString &path )
  {
    Q_UNUSED( version );

    SimpleCrypt crypto( Q_UINT64_C( 0x0c2ad4a4acb9f023 ) ); //some random number
    QString qstrEncrypted = crypto.sbEncryptToBase64String( path );

    response.setHeader( QStringLiteral( "Content-Type" ), QStringLiteral( "text/plain; charset=utf-8" ) );
    response.write( qstrEncrypted );
  }
} // namespace QgsSb
