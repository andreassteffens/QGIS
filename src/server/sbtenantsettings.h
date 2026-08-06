/***************************************************************************
sbtenantsettings.h

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

#ifndef SBTENANTSETTINGS_H
#define SBTENANTSETTINGS_H

#define SIP_NO_FILE

#include <QString>
#include <QStringList>
#include <QJsonObject>

class sbTenantSettings
{
  public:
    sbTenantSettings();

    QString rootDataFolder() const;
    bool acceptEncryptedPathsOnly() const;

    // File I/O
    bool load( const QString &fileName );

  private:
    QString m_strRootDataFolder;
    bool m_bAcceptEncryptedPathsOnly;
  
    QJsonObject toJson() const;
    bool fromJson( const QJsonObject &json );
};

#endif // SBTENANTSETTINGS_H
