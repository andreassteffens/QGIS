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
#include "sbjoinedtoggleutils.h"
#include "qgslogger.h"
#include "qgsapplication.h"
#include "qgsproject.h"
#include "qgsmessagelog.h"

sbJoinedToggleWidget::sbJoinedToggleWidget( QWidget *pParent, QgsMapLayer *pLayer )
  : QWidget( pParent ),
    mpLayer( pLayer )
{
  setupUi( this );
  
  QList<QgsMapLayer*> listExcluded;
  listExcluded.append( pLayer );

  mComboReferenceLayer->setExceptedLayerList( listExcluded );

  sbJoinedToggleLayerSettings settings = sbJoinedToggleUtils::getReferencedLayer( pLayer );
  if( !settings.layerId.isEmpty() )
  {
    QgsMapLayer* layerReference = pLayer->project()->mapLayer( settings.referencedLayerId );
    if ( layerReference != NULL )
      mComboReferenceLayer->setLayer( layerReference );
    else
      mComboReferenceLayer->setLayer( NULL );

    mCheckActivate->setCheckState( settings.activateWithReference ? Qt::CheckState::Checked : Qt::CheckState::Unchecked );
    mCheckDeactivate->setCheckState( settings.deactivateWithReference ? Qt::CheckState::Checked : Qt::CheckState::Unchecked );
    mCheckInvert->setCheckState( settings.invertBehavior ? Qt::CheckState::Checked : Qt::CheckState::Unchecked );
  }
  else
    mComboReferenceLayer->setLayer(NULL);
}

void sbJoinedToggleWidget::applyToLayer()
{
  sbJoinedToggleUtils::removeJoinedToggleLayer( mpLayer );

  QgsMapLayer* pLayer = mComboReferenceLayer->currentLayer();
  if (pLayer)
    sbJoinedToggleUtils::addJoinedToggleLayer( pLayer, mpLayer, mCheckActivate->checkState() == Qt::Checked, mCheckDeactivate->checkState() == Qt::Checked, mCheckInvert->checkState() == Qt::Checked );
}
