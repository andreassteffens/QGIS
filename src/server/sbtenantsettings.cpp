/***************************************************************************
sbtenantsettings.cpp

sb tenant specific settings implementation
-----------------------------
begin                : 2026-08-05
copyright            : (C) 2026 by Andreas Steffens
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

#include "sbtenantsettings.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSaveFile>
#include "qgsmessagelog.h"


sbTenantSettings::sbTenantSettings()
{
  mRootDataFolder = QString();
  mAcceptEncryptedPathsOnly = false;
}

QString sbTenantSettings::rootDataFolder() const
{
  return mRootDataFolder;
}

bool sbTenantSettings::acceptEncryptedPathsOnly() const
{
  return mAcceptEncryptedPathsOnly;
}

QJsonObject sbTenantSettings::toJson() const
{
  QJsonObject json;

  json["RootDataFolder"] = mRootDataFolder;
  json["AcceptEncryptedPathsOnly"] = mAcceptEncryptedPathsOnly;

  return json;
}

bool sbTenantSettings::fromJson( const QJsonObject &json )
{
  if ( json.contains( "RootDataFolder" ) )
    mRootDataFolder = json.value( "RootDataFolder" ).toString();

  if (json.contains( "AcceptEncryptedPathsOnly" ))
    mAcceptEncryptedPathsOnly = json.value( "AcceptEncryptedPathsOnly" ).toBool( mAcceptEncryptedPathsOnly );

  return true;
}

bool sbTenantSettings::load( const QString &fileName )
{
  QFile file( fileName );

  if ( !file.open( QIODevice::ReadOnly ) )
    return false;

  const QByteArray data = file.readAll();

  file.close();

  // Parse JSON

  QJsonParseError parseError;

  const QJsonDocument document = QJsonDocument::fromJson( data, &parseError );

  if ( parseError.error != QJsonParseError::NoError )
    return false;

  if ( !document.isObject() )
    return false;

  return fromJson( document.object() );
}
