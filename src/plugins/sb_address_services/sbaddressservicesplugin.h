/***************************************************************************
    sbaddressservices.h
    -------------------
    begin                : December 2018
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
/***************************************************************************
 *   QGIS Programming conventions:
 *
 *   mVariableName - a class level member variable
 *   sVariableName - a static class level member variable
 *   variableName() - accessor for a class member (no 'get' in front of name)
 *   setVariableName() - mutator for a class member (prefix with 'set')
 *
 *   Additional useful conventions:
 *
 *   variableName - a method parameter (prefix with 'the')
 *   myVariableName - a locally declared variable within a method ('my' prefix)
 *
 *   DO: Use mixed case variable names - myVariableName
 *   DON'T: separate variable names using underscores: my_variable_name (NO!)
 *
 * **************************************************************************/
#ifndef SBADDRESSSERVICESPLUGIN_H
#define SBADDRESSSERVICESPLUGIN_H

//QT includes
#include <QObject>
#include <QPointer>
#include <QApplication>

//QGIS includes
#include "../qgisplugin.h"
#include "sbaddressservicesmaptool.h"
#include "qgscoordinatereferencesystem.h"
#include "qgscoordinatetransform.h"

//forward declarations
class QAction;
class QToolBar;
class QToolButton;
class QPushButton;
class QgsDockWidget;
class QLineEdit;
class QIcon;
class QLabel;

class QgisInterface;
class QgsPointXY;

/**
* \class Plugin
* \brief [name] plugin for QGIS
* [description]
*/
class sbAddressServicesPlugin: public QObject, public QgisPlugin
{
    Q_OBJECT

  public:
    explicit sbAddressServicesPlugin( QgisInterface *interface );

  public slots:
    //! init the gui
    void initGui() override;

    //! unload the plugin
    void unload() override;

    //! Show/hide the dockwidget
    void showOrHide();

    //! show the help document
    void help();

  private:
    //! Container for the coordinate info
    QPointer<QgsDockWidget> mpDockWidget;

    //! Pointer to the QGIS interface object
    QgisInterface *mpQgisIface = nullptr;

    //!pointer to the qaction for this plugin
    QPointer<QAction> mpQActionPointer = nullptr;
};

static const QString sName = QApplication::translate( "sbAddressServicesPlugin", "[a]tapa Address Services" );
static const QString sDescription = QApplication::translate( "sbAddressServicesPlugin", "Forward and reverse address search based on Google and OSM services" );
static const QString sCategory = QApplication::translate( "sbAddressServicesPlugin", "Geocoding" );
static const QString sPluginVersion = QApplication::translate( "sbAddressServicesPlugin", "Version 1.2" );
static const QString sPluginIcon = QStringLiteral( ":/sbaddressservices/icons/sb_address_services.png" );
static const QgisPlugin::PluginType sPluginType = QgisPlugin::UI;
static const QString sExperimental = QStringLiteral( "false" );
static const QString sCreateDate = QStringLiteral( "2018-12-16T08:00:00" );
static const QString sUpdateDate = QStringLiteral( "2022-07-22T08:00:00" );

#endif //SBADDRESSSERVICESPLUGIN_H
