/***************************************************************************
                             qgsprojectservervalidator.cpp
                             ---------------------------
    begin                : March 2020
    copyright            : (C) 2020 by Etienne Trimaille
    email                : etienne dot trimaille at gmail dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#include "qgsapplication.h"
#include "qgslayertreelayer.h"
#include "qgsprojectservervalidator.h"
#include "qgsvectorlayer.h"
#include "qgsrenderer.h"
#include "qgsrendercontext.h"
#include "qgssymbollayer.h"
#include "qgssymbol.h"
#include "qgsmarkersymbollayer.h"
#include "qgslinesymbollayer.h"
#include "qgsfillsymbollayer.h"

#include <QRegularExpression>

QString QgsProjectServerValidator::sbResolveSvgPath( QString &resourcePath, const QgsPathResolver &pathResolver )
{
  return pathResolver.writePath( resourcePath );
}

QString QgsProjectServerValidator::sbResolveRasterPath( QString &resourcePath, const QgsPathResolver &pathResolver )
{
  if ( resourcePath.isEmpty() )
    return QString();

  if ( resourcePath.startsWith( QLatin1String( "base64:" ) ) )
    return resourcePath;

  if ( !QFileInfo::exists( resourcePath ) )
    return resourcePath;

  QString path = QFileInfo( resourcePath ).canonicalFilePath();

  QStringList svgPaths = QgsApplication::svgPaths();

  bool isInSvgPaths = false;
  for ( int i = 0; i < svgPaths.size(); i++ )
  {
    const QString dir = QFileInfo( svgPaths[i] ).canonicalFilePath();

    if ( !dir.isEmpty() && path.startsWith( dir ) )
    {
      path = path.mid( dir.size() + 1 );
      isInSvgPaths = true;
      break;
    }
  }

  if ( isInSvgPaths )
    return path;

  return pathResolver.writePath( path );
}

QString QgsProjectServerValidator::displayValidationError( QgsProjectServerValidator::ValidationError error )
{
  switch ( error )
  {
    case QgsProjectServerValidator::LayerEncoding:
      return QObject::tr( "Encoding is not correctly set. A non 'System' encoding is required" );
    case QgsProjectServerValidator::LayerShortName:
      return QObject::tr( "Layer short name is not valid. It must start with an unaccented alphabetical letter, followed by any alphanumeric letters, dot, dash or underscore" );
    case QgsProjectServerValidator::DuplicatedNames:
      return QObject::tr( "One or more layers or groups have the same name or short name. Both the 'name' and 'short name' for layers and groups must be unique" );
    case QgsProjectServerValidator::ProjectShortName:
      return QObject::tr( "The project root name (either the project short name or project title) is not valid. It must start with an unaccented alphabetical letter, followed by any alphanumeric letters, dot, dash or underscore" );
    case QgsProjectServerValidator::ProjectRootNameConflict:
      return QObject::tr( "The project root name (either the project short name or project title) is already used by a layer or a group" );
    case QgsProjectServerValidator::sbRasterLayerCheckEnabledLegend:
      return QObject::tr( "Make sure that keeping legend generally enabled for the layer makes sense. Otherwise you might want to disable it by setting the legend url to '0'." );
    case QgsProjectServerValidator::sbRasterLayerMissingTiledConstraint:
      return QObject::tr( "You might want to check whether the 'sb:TILED' option can be enabled for the layer." );
    case QgsProjectServerValidator::sbRasterLayerPublishableForClientFetching:
      return QObject::tr( "The layer could be published for client-side fetching." );
    case QgsProjectServerValidator::sbRasterLayerPublishedSourceNotSecure:
      return QObject::tr( "The layer is published for client-side fetching but doesn't have a secure datasource. Be aware of mixed-content browser issues!" );
    case QgsProjectServerValidator::sbRequiredWfsNotEnabled:
      return QObject::tr( "The layer has been marked to be used in a WebGIS tool that requires accessing the layer's features through WFS ... but access is not granted through WFS!" );
    case QgsProjectServerValidator::sbVectorLayerSearchNotDefined:
      return QObject::tr( "The layer has been marked searchable ... but no search expression has been defined!" );
    case QgsProjectServerValidator::sbVectorLayerDuplicateRuleKey:
      return QObject::tr( "The layer has duplicate rule keys ... probably stemming from a migration of an older project version to a newer one!" );
    case QgsProjectServerValidator::sbVectorLayerBase64SymbolContent:
      return QObject::tr( "The layer contains BASE64 symbology integrated into the project file!" );
    case QgsProjectServerValidator::sbVectorLayerAbsoluteSymbolPathContent:
      return QObject::tr( "The layer contains file based symbology with an absolute file path that might get lost when copying the project to a new location!" );
    case QgsProjectServerValidator::sbVectorLayerInvalidSymbolPath:
      return QObject::tr( "The layer contains file based symbology whose location cannot be validated!" );
    case QgsProjectServerValidator::sbVectorLayerObjectCountEnabled:
      return QObject::tr( "The layers object count is enabled. Consider disabling the to speed up project loading!" );
  }
  return QString();
}

void QgsProjectServerValidator::browseLayerTree( QgsProject *project, QgsLayerTreeGroup *treeGroup, QList<QPair<QString, QString>> &owsNames, QStringList &encodingMessages, QStringList &checkLegendMessages, QStringList &insecureSourceMessages, QStringList &tiledSourceMessages, QStringList &clientSidePublishingMessages, QStringList &missingWfsLayerMessages, QStringList &missingSearchTermMessages, QStringList &duplicateRuleKeyMessages, QStringList &absoluteSymbolPathMessages, QStringList &base64SymbolMessages, QStringList &invalidSymbolPathMessages, QStringList &layerObjectCountMessages )
{
  const QList< QgsLayerTreeNode * > treeGroupChildren = treeGroup->children();
  for ( int i = 0; i < treeGroupChildren.size(); ++i )
  {
    QgsLayerTreeNode *treeNode = treeGroupChildren.at( i );
    if ( treeNode->nodeType() == QgsLayerTreeNode::NodeGroup )
    {
      QString strPath = "";
      sbResolveLayerPath( treeNode, strPath );

      QgsLayerTreeGroup *treeGroupChild = static_cast<QgsLayerTreeGroup *>( treeNode );
      const QString shortName = treeGroupChild->customProperty( QStringLiteral( "wmsShortName" ) ).toString();
      if ( shortName.isEmpty() )
        owsNames.append( QPair<QString, QString>( treeGroupChild->name(), strPath ) );
      else
        owsNames.append( QPair<QString, QString>( shortName, strPath ) );

      browseLayerTree( project, treeGroupChild, owsNames, encodingMessages, checkLegendMessages, insecureSourceMessages, tiledSourceMessages, clientSidePublishingMessages, missingWfsLayerMessages, missingSearchTermMessages, duplicateRuleKeyMessages, absoluteSymbolPathMessages, base64SymbolMessages, invalidSymbolPathMessages, layerObjectCountMessages );
    }
    else
    {
      QgsLayerTreeLayer *treeLayer = static_cast<QgsLayerTreeLayer *>( treeNode );
      QgsMapLayer *layer = treeLayer->layer();
      if ( layer )
      {
        QString strPath = "";
        QgsLayerTreeLayer *pTreeLayer = project->layerTreeRoot()->findLayer( layer->id() );
        if ( pTreeLayer )
          sbResolveLayerPath( pTreeLayer, strPath );

        const QString shortName = layer->shortName();
        if ( shortName.isEmpty() )
          owsNames.append( QPair<QString, QString>( layer->name(), strPath ) );
        else
          owsNames.append( QPair<QString, QString>( shortName, strPath ) );

        if ( layer->type() == Qgis::LayerType::Vector )
        {
          QgsVectorLayer *vl = static_cast<QgsVectorLayer *>( layer );
          if ( vl->dataProvider() && vl->dataProvider()->encoding() == QLatin1String( "System" ) )
          {
            if ( !strPath.isEmpty() )
              encodingMessages << layer->name() + " (" + strPath + ")";
            else
              encodingMessages << layer->name();
          }

          bool showFeatureCount = vl->customProperty( QStringLiteral( "showFeatureCount" ), 0 ).toBool();
          if ( showFeatureCount )
            layerObjectCountMessages << layer->name() + " (" + strPath + ")";

          if ( vl->isSpatial() )
          {
            QgsFeatureRenderer *renderer = vl->renderer();
            if ( renderer != NULL )
            {
              QgsRenderContext context;

              QgsSymbolList listSymbols = const_cast<QgsFeatureRenderer *>( renderer )->symbols( context );
              for ( int iSymbol = 0; iSymbol < listSymbols.count(); iSymbol++ )
              {
                QgsSymbol *pSymbol = listSymbols[ iSymbol ];

                QgsSymbolLayerList listSymbolLayers = pSymbol->symbolLayers();
                for ( int iSymbolLayer = 0; iSymbolLayer < listSymbolLayers.count(); iSymbolLayer++ )
                {
                  QgsSymbolLayer *pSymbolLayer = listSymbolLayers[ iSymbolLayer ];

                  QString strResourcePath;
                  QString strResolvedResourcePath;

                  QString strLayerType = pSymbolLayer->layerType();
                  if ( strLayerType.compare( "RasterLine", Qt::CaseInsensitive ) == 0 )
                  {
                    strResourcePath = static_cast<QgsRasterLineSymbolLayer *>( pSymbolLayer )->path();
                    strResolvedResourcePath = sbResolveRasterPath( strResourcePath, QgsProject::instance()->pathResolver() );
                  }
                  else if ( strLayerType.compare( "SvgMarker", Qt::CaseInsensitive ) == 0 )
                  {
                    strResourcePath = static_cast<QgsSvgMarkerSymbolLayer *>( pSymbolLayer )->path();
                    strResolvedResourcePath = sbResolveSvgPath( strResourcePath, QgsProject::instance()->pathResolver() );
                  }
                  else if ( strLayerType.compare( "AnimatedMarker", Qt::CaseInsensitive ) == 0 )
                  {
                    strResourcePath = static_cast<QgsAnimatedMarkerSymbolLayer *>( pSymbolLayer )->path();
                    strResolvedResourcePath = sbResolveRasterPath( strResourcePath, QgsProject::instance()->pathResolver() );
                  }
                  else if ( strLayerType.compare( "RasterMarker", Qt::CaseInsensitive ) == 0 )
                  {
                    strResourcePath = static_cast<QgsRasterMarkerSymbolLayer *>( pSymbolLayer )->path();
                    strResolvedResourcePath = sbResolveRasterPath( strResourcePath, QgsProject::instance()->pathResolver() );
                  }
                  else if ( strLayerType.compare( "RasterFill", Qt::CaseInsensitive ) == 0 )
                  {
                    strResourcePath = static_cast<QgsRasterFillSymbolLayer *>( pSymbolLayer )->imageFilePath();
                    strResolvedResourcePath = sbResolveRasterPath( strResourcePath, QgsProject::instance()->pathResolver() );
                  }
                  else if ( strLayerType.compare( "SVGFill", Qt::CaseInsensitive ) == 0 )
                  {
                    strResourcePath = static_cast<QgsSVGFillSymbolLayer *>( pSymbolLayer )->svgFilePath();
                    strResolvedResourcePath = sbResolveSvgPath( strResourcePath, QgsProject::instance()->pathResolver() );
                  }

                  if ( !strResourcePath.isEmpty() )
                  {
                    if ( strResourcePath.startsWith( "base64:", Qt::CaseInsensitive ) )
                    {
                      base64SymbolMessages << QStringLiteral( "%1 (%2): %3 | %4" ).arg( layer->name() ).arg( strPath ).arg( iSymbol ).arg( iSymbolLayer );
                      continue;
                    }

                    QFileInfo info( strResourcePath );
                    if ( !info.exists() )
                      invalidSymbolPathMessages << QStringLiteral( "%1 (%2): %3 | %4 | %5" ).arg( layer->name() ).arg( strPath ).arg( iSymbol ).arg( iSymbolLayer ).arg( strResolvedResourcePath );

                    if ( strResourcePath.compare( strResolvedResourcePath, Qt::CaseInsensitive ) == 0 )
                      absoluteSymbolPathMessages << QStringLiteral( "%1 (%2): %3 | %4 | %5" ).arg( layer->name() ).arg( strPath ).arg( iSymbol ).arg( iSymbolLayer ).arg( strResolvedResourcePath );
                  }
                }
              }

              QMap<QString, QString> mapRuleIds;
              QgsLegendSymbolList listLegendSymbols = renderer->legendSymbolItems();
              for ( int iSymbol = 0; iSymbol < listLegendSymbols.count(); iSymbol++ )
              {
                QgsLegendSymbolItem &legendItem = listLegendSymbols[iSymbol];
                QString strKey = legendItem.ruleKey();
                if ( strKey.isEmpty() )
                  continue;

                if ( mapRuleIds.contains( strKey ) )
                {
                  QString strLabel = "unnamed";
                  if ( !legendItem.label().isEmpty() )
                    strLabel = legendItem.label();

                  if ( !strPath.isEmpty() )
                    duplicateRuleKeyMessages << layer->name() + " (" + strPath + "): " + strLabel + " (" + strKey + ")";
                  else
                    duplicateRuleKeyMessages << layer->name() + ": " + strLabel + " (" + strKey + ")";
                }
                else
                  mapRuleIds.insert( strKey, strKey );
              }
            }
          }
        }
        else if ( layer->type() == Qgis::LayerType::Raster )
        {
          QString strPath = "";
          QgsLayerTreeLayer *pTreeLayer = project->layerTreeRoot()->findLayer( layer->id() );
          if ( pTreeLayer )
            sbResolveLayerPath( pTreeLayer, strPath );

          QString strLegendUrl = layer->legendUrl();
          bool bIsLegendDisabled = strLegendUrl.compare( "0" ) == 0;
          bool bIsHardCodedLegend = strLegendUrl.startsWith( "http", Qt::CaseInsensitive );
          if ( !bIsLegendDisabled && !bIsHardCodedLegend )
          {
            if ( !strPath.isEmpty() )
              checkLegendMessages << layer->name() + " (" + strPath + ")";
            else
              checkLegendMessages << layer->name();
          }

          bool bHasTiledConstraint = false;

          QgsLayerMetadata meta = layer->metadata();
          QList<QgsLayerMetadata::Constraint> qlistConstraints = meta.constraints();
          for ( int iMeta = 0; iMeta < qlistConstraints.length(); iMeta++ )
          {
            if ( qlistConstraints[iMeta].type.compare( "tiled", Qt::CaseInsensitive ) == 0 || qlistConstraints[iMeta].type.compare( "sb:TILED", Qt::CaseInsensitive ) == 0 )
              bHasTiledConstraint = true;
          }

          const QgsDataProvider *provider = layer->dataProvider();
          if ( provider )
          {
            if ( provider->name().compare( "wms", Qt::CaseInsensitive ) == 0 )
            {
              QVariant wmsPublishDataSourceUrl = layer->customProperty( QStringLiteral( "WMSPublishDataSourceUrl" ), false );
              bool bIsPublished = wmsPublishDataSourceUrl.toBool();

              QString strUri = provider->dataSourceUri();
              QStringList qlistParts = strUri.split( '&' );
              for ( int iPart = 0; iPart < qlistParts.count(); iPart++ )
              {
                QStringList qlistPartParts = qlistParts[iPart].split( '=' );

                if ( qlistPartParts[0].compare( "url", Qt::CaseInsensitive ) == 0 && qlistPartParts.count() == 2 )
                {
                  bool bIsHttps = qlistPartParts[1].startsWith( "https", Qt::CaseInsensitive );

                  if ( !bIsPublished )
                  {
                    if ( bIsHttps )
                    {
                      if ( !strPath.isEmpty() )
                        clientSidePublishingMessages << layer->name() + " (" + strPath + ")";
                      else
                        clientSidePublishingMessages << layer->name();
                    }
                  }
                  else
                  {
                    if ( !bIsHttps )
                    {
                      if ( !strPath.isEmpty() )
                        insecureSourceMessages << layer->name() + " (" + strPath + ")";
                      else
                        insecureSourceMessages << layer->name();
                    }

                    QList<QVariant> resolutionList = provider->property( "resolutions" ).toList();
                    bHasTiledConstraint = bHasTiledConstraint || resolutionList.size() > 0;
                  }

                  break;
                }
              }
            }
          }

          if ( !bHasTiledConstraint )
          {
            if ( !strPath.isEmpty() )
              tiledSourceMessages << layer->name() + " (" + strPath + ")";
            else
              tiledSourceMessages << layer->name();
          }
        }
      }
    }
  }
}

bool QgsProjectServerValidator::validate( QgsProject *project, QList<QgsProjectServerValidator::ValidationResult> &results )
{
  results.clear();
  bool result = true;

  if ( !project )
    return false;

  if ( !project->layerTreeRoot() )
    return false;

  QList<QPair<QString, QString>> owsNames;
  QStringList encodingMessages, checkLegendMessages, insecureSourceMessages, tiledSourceMessages, clientSidePublishingMessages, missingWfsLayerMessages, missingSearchTermMessages, duplicateRuleKeyMessages, absoluteSymbolPathMessages, base64SymbolMessages, invalidSymbolPathMessages, layerObjectCountMessages;
  browseLayerTree( project, project->layerTreeRoot(), owsNames, encodingMessages, checkLegendMessages, insecureSourceMessages, tiledSourceMessages, clientSidePublishingMessages, missingWfsLayerMessages, missingSearchTermMessages, duplicateRuleKeyMessages, absoluteSymbolPathMessages, base64SymbolMessages, invalidSymbolPathMessages, layerObjectCountMessages );

  QStringList duplicateNames, regExpMessages;
  const thread_local QRegularExpression snRegExp = QgsApplication::shortNameRegularExpression();
  const auto constOwsNames = owsNames;
  for ( int i = 0; i < owsNames.count(); i++ )
  {
    QRegularExpressionMatch match = snRegExp.match( owsNames[i].first );
    if ( !match.hasMatch() )
      regExpMessages << owsNames[i].first;

    if ( duplicateNames.contains( owsNames[i].first ) )
      continue;

    int iCount = 0;
    for ( int j = 0; j < owsNames.count(); j++ )
    {
      if ( owsNames[j].first.compare( owsNames[i].first ) == 0 )
        iCount++;
    }
    if ( iCount > 1 )
      duplicateNames << owsNames[i].first;
  }

  if ( !duplicateNames.empty() )
  {
    result = false;

    for ( int i = 0; i < duplicateNames.count(); i++ )
      results << ValidationResult( QgsProjectServerValidator::DuplicatedNames, duplicateNames[i] );
  }

  if ( !regExpMessages.empty() )
  {
    result = false;

    for ( int i = 0; i < regExpMessages.count(); i++ )
      results << ValidationResult( QgsProjectServerValidator::LayerShortName, regExpMessages[i] );
  }

  if ( !encodingMessages.empty() )
  {
    result = false;

    for ( int i = 0; i < encodingMessages.count(); i++ )
      results << ValidationResult( QgsProjectServerValidator::LayerEncoding, encodingMessages[i] );
  }

  if ( !checkLegendMessages.empty() )
  {
    result = false;

    for ( int i = 0; i < checkLegendMessages.count(); i++ )
      results << ValidationResult( QgsProjectServerValidator::sbRasterLayerCheckEnabledLegend, checkLegendMessages[i] );
  }

  if ( !insecureSourceMessages.empty() )
  {
    result = false;

    for ( int i = 0; i < insecureSourceMessages.count(); i++ )
      results << ValidationResult( QgsProjectServerValidator::sbRasterLayerPublishedSourceNotSecure, insecureSourceMessages[i] );
  }

  if ( !tiledSourceMessages.empty() )
  {
    result = false;

    for ( int i = 0; i < tiledSourceMessages.count(); i++ )
      results << ValidationResult( QgsProjectServerValidator::sbRasterLayerMissingTiledConstraint, tiledSourceMessages[i] );
  }

  if ( !clientSidePublishingMessages.empty() )
  {
    result = false;

    for ( int i = 0; i < clientSidePublishingMessages.count(); i++ )
      results << ValidationResult( QgsProjectServerValidator::sbRasterLayerPublishableForClientFetching, clientSidePublishingMessages[i] );
  }

  if ( !missingWfsLayerMessages.empty() )
  {
    result = false;

    for ( int i = 0; i < missingWfsLayerMessages.count(); i++ )
      results << ValidationResult( QgsProjectServerValidator::sbRequiredWfsNotEnabled, missingWfsLayerMessages[i] );
  }

  if ( !missingSearchTermMessages.empty() )
  {
    result = false;

    for ( int i = 0; i < missingSearchTermMessages.count(); i++ )
      results << ValidationResult( QgsProjectServerValidator::sbVectorLayerSearchNotDefined, missingSearchTermMessages[i] );
  }

  if ( !duplicateRuleKeyMessages.empty() )
  {
    result = false;

    for ( int i = 0; i < duplicateRuleKeyMessages.count(); i++ )
      results << ValidationResult( QgsProjectServerValidator::sbVectorLayerDuplicateRuleKey, duplicateRuleKeyMessages[i] );
  }

  if ( !absoluteSymbolPathMessages.empty() )
  {
    result = false;

    for ( int i = 0; i < absoluteSymbolPathMessages.count(); i++ )
      results << ValidationResult( QgsProjectServerValidator::sbVectorLayerAbsoluteSymbolPathContent, absoluteSymbolPathMessages[i] );
  }

  if ( !base64SymbolMessages.empty() )
  {
    result = false;

    for ( int i = 0; i < base64SymbolMessages.count(); i++ )
      results << ValidationResult( QgsProjectServerValidator::sbVectorLayerBase64SymbolContent, base64SymbolMessages[i] );
  }

  if ( !invalidSymbolPathMessages.empty() )
  {
    result = false;

    for ( int i = 0; i < invalidSymbolPathMessages.count(); i++ )
      results << ValidationResult( QgsProjectServerValidator::sbVectorLayerInvalidSymbolPath, invalidSymbolPathMessages[i] );
  }

  if ( !layerObjectCountMessages.empty() )
  {
    result = false;

    for ( int i = 0; i < layerObjectCountMessages.count(); i++ )
      results << ValidationResult( QgsProjectServerValidator::sbVectorLayerObjectCountEnabled, layerObjectCountMessages[i] );
  }

  // Determine the root layername
  QString rootLayerName = project->readEntry( QStringLiteral( "WMSRootName" ), QStringLiteral( "/" ), "" );
  if ( rootLayerName.isEmpty() && !project->title().isEmpty() )
  {
    rootLayerName = project->title();
  }
  if ( !rootLayerName.isEmpty() )
  {
    int iCount = 0;
    for ( int i = 0; i < owsNames.count(); i++ )
    {
      if ( owsNames[i].first.compare( rootLayerName ) == 0 )
        iCount++;
    }

    if ( iCount >= 1 )
    {
      result = false;
      results << ValidationResult( QgsProjectServerValidator::ProjectRootNameConflict, rootLayerName );
    }

    if ( !snRegExp.match( rootLayerName ).hasMatch() )
    {
      result = false;
      results << ValidationResult( QgsProjectServerValidator::ProjectShortName, rootLayerName );
    }
  }

  return result;
}

void QgsProjectServerValidator::sbResolveLayerPath( QgsLayerTreeNode *pNode, QString &rstrPath )
{
  if ( !pNode )
  {
    if ( rstrPath.endsWith( "/" ) )
      rstrPath.truncate( rstrPath.length() - 1 );

    return;
  }

  rstrPath = pNode->name() + "/" + rstrPath;

  sbResolveLayerPath( pNode->parent(), rstrPath );
}
