/***************************************************************************
                              qgssbsetpreloadprojects.h
                              -------------------------
  begin                : 2020-01-29
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

namespace QgsSb
{
  /**
   * Output SetPreloadProjects response
   */
  void writeSetPreloadProjects(QgsServerInterface *serverIface, const QgsProject *project,
			  const QString &version, const QgsServerRequest &request,
			  QgsServerResponse &response, const QString &projects);


} // namespace QgsSb


