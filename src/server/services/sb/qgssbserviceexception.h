/***************************************************************************
                              qgssbserviceexception.h
                              ------------------------
  begin                : September 05, 2018
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

#ifndef QGSSBSERVICEEXCEPTION_H
#define QGSSBSERVICEEXCEPTION_H

#include <QString>

#include "qgsserverexception.h"

namespace QgsSb
{

  /**
   * \ingroup server
   * \class  QgsserviceException
   * \brief Exception class for WMS service exceptions.
   *
   * The most important codes are:
   *  * "InvalidFormat"
   *  * "Invalid CRS"
   *  * "LayerNotDefined" / "StyleNotDefined"
   *  * "OperationNotSupported"
   */
  class QgsServiceException : public QgsOgcServiceException
  {
    public:
      QgsServiceException( const QString &code, const QString &message, const QString &locator = QString(),
                           int responseCode = 200 )
        : QgsOgcServiceException( code, message, locator, responseCode, QStringLiteral( "1.0.0" ) )
      {}

      QgsServiceException( const QString &code, const QString &message, int responseCode )
        : QgsOgcServiceException( code, message, QString(), responseCode, QStringLiteral( "1.0.0" ) )
      {}

  };

  /**
   * \ingroup server
   * \class  QgsSecurityException
   * \brief Exception thrown when data access violates access controls
   */
  class QgsSecurityException: public QgsServiceException
  {
    public:
      QgsSecurityException( const QString &message, const QString &locator = QString() )
        : QgsServiceException( QStringLiteral( "Security" ), message, locator, 403 )
      {}
  };

  /**
   * \ingroup server
   * \class  QgsBadRequestException
   * \brief Exception thrown in case of malformed request
   */
  class QgsBadRequestException: public QgsServiceException
  {
    public:
      QgsBadRequestException( const QString &code, const QString &message, const QString &locator = QString() )
        : QgsServiceException( code, message, locator, 400 )
      {}
  };


} // namespace QgsSb

#endif
