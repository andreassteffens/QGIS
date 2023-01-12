/***************************************************************************
                          sbjoinedtogglewidget.h  -  description
                             -------------------
    begin                : 11/01/2023
    copyright            : (C) 2023 by Andreas Steffens
    email                : a dot steffens at gds dash team dot de
 ***************************************************************************/

#include <QComboBox>
#include <QString>

#include "sbjoinedtogglewidget.h"
#include "qgslogger.h"
#include "qgsapplication.h"
#include "qgsproject.h"
#include "qgsmessagelog.h"

#define REFERENCED_LAYER_KEY "sb:JOINED_TOGGLE_REFERENCED_LAYER"
#define ACTIVATE_BY_REFERENCE_KEY "sb:JOINED_TOGGLE_ACTIVATE_BY_REFERENCE"
#define DEACTIVATE_BY_REFERENCE_KEY "sb:JOINED_TOGGLE_DEACTIVATE_BY_REFERENCE"
#define INVERT_BEHAVIOR_KEY "sb:JOINED_TOGGLE_INVERT_BEHAVIOR"

sbJoinedToggleWidget::sbJoinedToggleWidget( QWidget *pParent, QgsMapLayer *pLayer )
  : QWidget( pParent ),
    mpLayer( pLayer )
{
  setupUi( this );
  
  QList<QgsMapLayer*> listExcluded;
  listExcluded.append( pLayer );

  mComboReferenceLayer->setExceptedLayerList( listExcluded );
  QVariant varLayer = pLayer->customProperty( REFERENCED_LAYER_KEY );
  if ( !varLayer.isNull() && varLayer.type() == QVariant::Type::String )
  {
    QgsMapLayer* layerReference = pLayer->project()->mapLayer( varLayer.toString() );
    if (layerReference != NULL)
      mComboReferenceLayer->setLayer( layerReference );
  }

  QVariant varActivate = pLayer->customProperty( ACTIVATE_BY_REFERENCE_KEY );
  if ( !varActivate.isNull() && varActivate.type() == QVariant::Type::Int )
    mCheckActivate->setCheckState( (Qt::CheckState)varActivate.toInt() );

  QVariant varDeactivate = pLayer->customProperty( DEACTIVATE_BY_REFERENCE_KEY );
  if ( !varDeactivate.isNull() && varDeactivate.type() == QVariant::Type::Int )
    mCheckDeactivate->setCheckState( (Qt::CheckState)varDeactivate.toInt() );

  QVariant varInvert = pLayer->customProperty( INVERT_BEHAVIOR_KEY );
  if ( !varInvert.isNull() && varInvert.type() == QVariant::Type::Int )
    mCheckInvert->setCheckState( (Qt::CheckState)varInvert.toInt() );

  connect( mCheckActivate, &QCheckBox::stateChanged, this, &sbJoinedToggleWidget::onCheckBoxStateChanged );
  connect( mCheckDeactivate, &QCheckBox::stateChanged, this, &sbJoinedToggleWidget::onCheckBoxStateChanged );
  connect( mCheckInvert, &QCheckBox::stateChanged, this, &sbJoinedToggleWidget::onCheckBoxStateChanged );
  connect( mComboReferenceLayer, &QgsMapLayerComboBox::layerChanged, this, &sbJoinedToggleWidget::onReferencedLayerChanged );
}

void sbJoinedToggleWidget::onCheckBoxStateChanged( int iState )
{
  mpLayer->setCustomProperty( ACTIVATE_BY_REFERENCE_KEY, QVariant( (int)mCheckActivate->checkState() ) );
  mpLayer->setCustomProperty( DEACTIVATE_BY_REFERENCE_KEY, QVariant( (int)mCheckDeactivate->checkState() ) );
  mpLayer->setCustomProperty( INVERT_BEHAVIOR_KEY, QVariant( (int)mCheckInvert->checkState() ) );
}

void sbJoinedToggleWidget::onReferencedLayerChanged( QgsMapLayer* pLayer )
{
  if (pLayer)
    mpLayer->setCustomProperty( REFERENCED_LAYER_KEY, pLayer->id() );
  else
    mpLayer->removeCustomProperty( REFERENCED_LAYER_KEY );
}
