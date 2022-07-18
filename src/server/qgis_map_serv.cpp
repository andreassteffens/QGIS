/***************************************************************************
                              qgs_map_serv.cpp
 A server application supporting WMS / WFS / WCS
                              -------------------
  begin                : July 04, 2006
  copyright            : (C) 2006 by Marco Hugentobler & Ionut Iosifescu Enescu
  email                : marco dot hugentobler at karto dot baug dot ethz dot ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

//for CMAKE_INSTALL_PREFIX
#include "qgsconfig.h"
#include "qgsserver.h"
#include "qgsfcgiserverresponse.h"
#include "qgsfcgiserverrequest.h"
#include "qgsapplication.h"
#include "qgscommandlineutils.h"
#include "qgsmapserviceexception.h"

#include <fcgi_stdio.h>
#include <cstdlib>

#include <QFontDatabase>
#include <QString>

#ifdef Q_OS_WIN
  #include <eh.h>
#endif

int fcgi_accept()
{
#ifdef Q_OS_WIN
  if ( FCGX_IsCGI() )
    return FCGI_Accept();
  else
    return FCGX_Accept( &FCGI_stdin->fcgx_stream, &FCGI_stdout->fcgx_stream, &FCGI_stderr->fcgx_stream, &environ );
#else
  return FCGI_Accept();
#endif
}

void Debug(QString &strMessage)
{
  return;

  QFile file("d:\\qgis_server_debug.txt");
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append))
    return;

  QTextStream out(&file);
  out << strMessage;
}

#ifdef Q_OS_WIN
void SETranslator(unsigned int u, struct _EXCEPTION_POINTERS *pExp) 
{
  std::string error = "SE Exception: ";
  switch (u) {
    case 0xC0000005:
      error += "Access Violation";
      break;
    default:
      char result[11];
      sprintf_s(result, 11, "0x%08X", u);
      error += result;
  };
  throw std::exception(error.c_str());
}
#endif

int main( int argc, char *argv[] )
{
  if ( argc >= 2 )
  {
    if ( argv[1] == QLatin1String( "--version" ) || argv[1] == QLatin1String( "-v" ) )
    {
      std::cout << QgsCommandLineUtils::allVersions().toStdString();
      return 0;
    }
  }

  // Test if the environment variable DISPLAY is defined
  // if it's not, the server is running in offscreen mode
  // Qt supports using various QPA (Qt Platform Abstraction) back ends
  // for rendering. You can specify the back end to use with the environment
  // variable QT_QPA_PLATFORM when invoking a Qt-based application.
  // Available platform plugins are: directfbegl, directfb, eglfs, linuxfb,
  // minimal, minimalegl, offscreen, wayland-egl, wayland, xcb.
  // https://www.ics.com/blog/qt-tips-and-tricks-part-1
  // http://doc.qt.io/qt-5/qpa.html
  const char *display = getenv( "DISPLAY" );
  bool withDisplay = true;
  if ( !display )
  {
    withDisplay = false;
    qputenv( "QT_QPA_PLATFORM", "offscreen" );
    QgsMessageLog::logMessage( "DISPLAY not set, running in offscreen mode, all printing capabilities will not be available.", "Server", Qgis::MessageLevel::Info );
  }
  
#ifdef Q_OS_WIN
  _set_se_translator(SETranslator);
#endif
  
  // since version 3.0 QgsServer now needs a qApp so initialize QgsApplication
  const QgsApplication app( argc, argv, withDisplay, QString(), QStringLiteral( "server" ) );
  
  QgsMessageLog::logMessage(QStringLiteral("STARTING QGIS SERVER"), QStringLiteral("Server"), Qgis::Warning);
  
  try
  {
    QString strTenant = "";

    if (argc == 2)
      strTenant = argv[1];

    QgsServer server(strTenant);

#ifdef HAVE_SERVER_PYTHON_PLUGINS
    server.initPython();
#endif

#ifdef Q_OS_WIN
    // Initialize font database before fcgi_accept.
    // When using FCGI with IIS, environment variables (QT_QPA_FONTDIR in this case) are lost after fcgi_accept().
    QFontDatabase fontDB;

    int iRet = server.sbPreloadProjects();
#endif

    // Starts FCGI loop
    while ( fcgi_accept() >= 0 )
    {
      QgsFcgiServerRequest  request;
      QgsFcgiServerResponse response( request.method() );
      if ( ! request.hasError() )
      {
        server.handleRequest( request, response );
      }
      else
      {
        response.sendError( 400, "Bad request" );
      }
    }
  }
  catch (QgsServerException &ex)
  {
    QgsMessageLog::logMessage(QStringLiteral("QgsServerException: %1").arg(ex.what()), QStringLiteral("Server"), Qgis::Critical);
  }
  catch (QgsException &ex)
  {
    // Internal server error
    QgsMessageLog::logMessage(QStringLiteral("QgsServerException: %1").arg(ex.what()), QStringLiteral("Server"), Qgis::Critical);
  }
  catch (...)
  {
    QgsMessageLog::logMessage(QStringLiteral("Unknown exception"), QStringLiteral("Server"), Qgis::Critical);
  }

  QgsMessageLog::logMessage(QStringLiteral("STOPPING QGIS SERVER"), QStringLiteral("Server"), Qgis::Warning);

  QgsApplication::exitQgis();

  return 0;
}
