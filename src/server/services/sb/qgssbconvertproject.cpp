/***************************************************************************
                              qgssbconvertproject.cpp
                              -------------------------
  begin                : 2018-10-05
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

#include "qgssbconvertproject.h"
#include "qgsserverprojectutils.h"
#include "qgsconfigcache.h"

#include "qgsexception.h"
#include "qgsexpressionnodeimpl.h"


namespace QgsSb
{
  void writeConvertProject( QgsServerInterface *serverIface, const QgsProject *project,
                             const QString &version, const QgsServerRequest &request,
                             QgsServerResponse &response, const QString &outputFilename )
  {
	  Q_UNUSED(version);

	  QDomDocument doc;
	  QDomProcessingInstruction xmlDeclaration = doc.createProcessingInstruction(QStringLiteral("xml"),
		  QStringLiteral("version=\"1.0\" encoding=\"utf-8\""));

	  doc.appendChild(xmlDeclaration);

	  QDomElement serverInfoElem = doc.createElement(QStringLiteral("sbConvertProject"));
	  serverInfoElem.setAttribute(QStringLiteral("xmlns"), QStringLiteral("http://www.sb-partner-geo-it.de/sbConvertProject"));
	  serverInfoElem.setAttribute(QStringLiteral("version"), QStringLiteral("1.0.0"));
	  doc.appendChild(serverInfoElem);

	  QString qstrOriginal = project->fileName();
	  
	  QgsProject* p = const_cast<QgsProject*>(project);
	  bool bRes = p->write(outputFilename);
	  p->setFileName(qstrOriginal);

	  QDomElement resultElem = doc.createElement(QStringLiteral("Result"));
	  QDomText resultText = doc.createTextNode(bRes ? "ok" : "error");
	  resultElem.appendChild(resultText);
	  serverInfoElem.appendChild(resultElem);

	  if (!bRes)
	  {
		  QDomElement errorElem = doc.createElement(QStringLiteral("Error"));
		  QDomText errorText = doc.createTextNode(p->error());
		  errorElem.appendChild(errorText);
		  serverInfoElem.appendChild(errorElem);
	  }

	  response.setStatusCode(bRes ? 200 : 500);
	  response.setHeader(QStringLiteral("Content-Type"), QStringLiteral("text/xml; charset=utf-8"));
	  response.write(doc.toByteArray());
  }
} // namespace QgsSb