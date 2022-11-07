/***************************************************************************
                          qgsbufferserverrequest.cpp

  Define response wrapper for bbuffer response
  -------------------
  begin                : 2017-01-03
  copyright            : (C) 2017 by David Marteau
  email                : david dot marteau at 3liz dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsbufferserverrequest.h"
#include "qgsmessagelog.h"

QgsBufferServerRequest::QgsBufferServerRequest( const QString &url, Method method, const QgsServerRequest::Headers &headers, QByteArray *data )
  : QgsServerRequest( url, method, headers )
{
  // Store the URL before the server rewrite that could have been set in QUERY_STRING
  setOriginalUrl(url);

  if ( data )
  {
    mData = *data;
  }
}

QgsBufferServerRequest::QgsBufferServerRequest( const QUrl &url, Method method, const QgsServerRequest::Headers &headers, QByteArray *data )
  : QgsServerRequest( url, method, headers )
{
  // Store the URL before the server rewrite that could have been set in QUERY_STRING
  setOriginalUrl(url.toString());

  if ( data )
  {
    mData = *data;
  }
}
