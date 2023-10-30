/***************************************************************************
                              qgsserverfeatureid.cpp
                              -----------------------
  begin                : May 17, 2019
  copyright            : (C) 2019 by René-Luc DHONT
  email                : rldhont at 3liz dot com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsserverfeatureid.h"
#include "qgsfeature.h"
#include "qgsfeaturerequest.h"
#include "qgsvectordataprovider.h"
#include "qgsexpression.h"
#include "qgsmessagelog.h"
#include <qstringbuilder.h>


QString QgsServerFeatureId::getServerFid( const QgsFeature &feature, const QgsAttributeList &pkAttributes )
{
  if ( pkAttributes.isEmpty() )
  {
    return QString::number( feature.id() );
  }

  QStringList pkValues;
  for ( const auto &attrIdx : std::as_const( pkAttributes ) )
  {
    pkValues.append( feature.attribute( attrIdx ).toString() );
  }
  return pkValues.join( pkSeparator() );
}

QgsFeatureRequest QgsServerFeatureId::updateFeatureRequestFromServerFids( QgsFeatureRequest &featureRequest, const QStringList &serverFids, const QgsVectorDataProvider *provider )
{
  const QgsAttributeList &pkAttributes = provider->pkAttributeIndexes();

  if ( pkAttributes.isEmpty() )
  {
    QgsFeatureIds fids;
    for ( const QString &serverFid : serverFids )
    {
      fids.insert( STRING_TO_FID( serverFid ) );
    }
    featureRequest.setFilterFids( fids );
    return featureRequest;
  }

  if ( serverFids.count() > 0 )
  {
    if ( sbGetPkExpressionSize( serverFids.at( 0 ), provider ) == 1 )
    {
      const QgsFields &fields = provider->fields();
      QString pkFieldName = fields[pkAttributes.at( 0 )].name();
      QVariant::Type pkFieldType = fields[pkAttributes.at( 0 )].type();
      QStringList pkValues = serverFids.at( 0 ).split( pkSeparator() );

      QString fullExpression = QgsExpression::quotedColumnRef( pkFieldName ) + " IN (";

      if ( serverFids.count() > 100 )
        fullExpression.reserve( serverFids.count() * 32 );

      int i = 0;
      for ( const QString &serverFid : serverFids )
      {
        if ( i > 0 )
          fullExpression.append( "," );

        fullExpression.append( QgsExpression::quotedValue( serverFid, pkFieldType ) );

        i++;
      }

      fullExpression += ")";

      featureRequest.combineFilterExpression( fullExpression );
      return featureRequest;
    }
  }

  QStringList expList;
  for ( const QString &serverFid : serverFids )
  {
    expList.append( QgsServerFeatureId::getExpressionFromServerFid( serverFid, provider ) );
  }

  if ( expList.count() == 1 )
  {
    featureRequest.combineFilterExpression( expList.at( 0 ) );
  }
  else
  {
    QString fullExpression;

    if ( expList.count() > 100 )
      fullExpression.reserve( expList.count() * 32 );

    for ( const QString &exp : std::as_const( expList ) )
    {
      if ( !fullExpression.isEmpty() )
      {
        fullExpression.append( QStringLiteral( " OR " ) );
      }
      fullExpression.append( QStringLiteral( "( " ) );
      fullExpression.append( exp );
      fullExpression.append( QStringLiteral( " )" ) );
    }

    featureRequest.combineFilterExpression( fullExpression );
  }

  return featureRequest;
}

QString QgsServerFeatureId::getExpressionFromServerFid( const QString &serverFid, const QgsVectorDataProvider *provider )
{
  const QgsAttributeList &pkAttributes = provider->pkAttributeIndexes();

  if ( pkAttributes.isEmpty() )
  {
    return QString();
  }

  const QgsFields &fields = provider->fields();

  QString expressionString;
  QStringList pkValues = serverFid.split( pkSeparator() );
  int pkExprSize = std::min( pkAttributes.size(), pkValues.size() );
  for ( int i = 0; i < pkExprSize; ++i )
  {
    if ( i > 0 )
    {
      expressionString.append( QStringLiteral( " AND " ) );
    }

    QString fieldName = fields[ pkAttributes.at( i ) ].name();
    expressionString.append( QgsExpression::createFieldEqualityExpression( fieldName, QVariant( pkValues.at( i ) ) ) );
  }

  return expressionString;
}

QString QgsServerFeatureId::pkSeparator()
{
  return QStringLiteral( "@@" );
}

int QgsServerFeatureId::sbGetPkExpressionSize( const QString &serverFid, const QgsVectorDataProvider *provider )
{
  const QgsAttributeList &pkAttributes = provider->pkAttributeIndexes();

  if ( pkAttributes.isEmpty() )
  {
    return 0;
  }

  QString expressionString;
  QStringList pkValues = serverFid.split( pkSeparator() );
  int pkExprSize = std::min( pkAttributes.size(), pkValues.size() );
  return pkExprSize;
}
