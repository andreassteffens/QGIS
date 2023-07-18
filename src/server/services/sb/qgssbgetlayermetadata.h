/***************************************************************************
                              qgssbgetlayermetadata.h
                              -------------------------
  begin                : 2018-10-11
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

namespace QgsSb
{

  /**
   * Output GetLayerMetadata response
   */
  void writeGetLayerMetadata( QgsServerInterface *serverIface, const QgsProject *project,
                              const QString &version, const QgsServerRequest &request,
                              QgsServerResponse &response, const QString &layerName );


} // namespace QgsSb
