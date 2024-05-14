/***************************************************************************
                              qgssbgetdecryptedpath.h
                              -----------------------
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

namespace QgsSb
{

  /**
   * Output GetDecryptedPath response
   */
  void writeGetDecryptedPath( QgsServerInterface *serverIface, const QgsProject *project,
                              const QString &version, const QgsServerRequest &request,
                              QgsServerResponse &response, const QString &path );


} // namespace QgsSb
