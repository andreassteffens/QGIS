/***************************************************************************
                          sbjoinedtogglewidget.h  -  description
                             -------------------
    begin                : 11/01/2023
    copyright            : (C) 2023 by Andreas Steffens
    email                : a dot steffens at gds dash team dot de
 ***************************************************************************/

#ifndef sbjoinedtogglewidget_h
#define sbjoinedtogglewidget_h

#include "qgis_gui.h"
#include "qgis_sip.h"
#include "ui_sbjoinedtogglewidget.h"

class QgsMapLayer;

/**
 * \ingroup gui
 * \class sbJoinedToggleWidget
 * \brief A wizard to edit metadata on a map layer.
 *
 * \since QGIS 3.0
 */

class GUI_EXPORT sbJoinedToggleWidget : public QWidget, private Ui::sbJoinedToggleWidgetBase
{
    Q_OBJECT

  public:

    /**
     * Constructor for the widget.
     *
     * If \a layer is set, then this constructor automatically sets the widget's metadata() to match
     * the layer's metadata.
     */
    sbJoinedToggleWidget( QWidget *pParent SIP_TRANSFERTHIS = nullptr, QgsMapLayer *pLayer = nullptr );

  public slots:

  signals:

  private slots:
    void onCheckBoxStateChanged( int iState );
    void onReferencedLayerChanged( QgsMapLayer* pLayer);
    
  private:
    QgsMapLayer *mpLayer = nullptr;
};

#endif
