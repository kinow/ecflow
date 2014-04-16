//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef SERVERLISTMODEL_H
#define SERVERLISTMODEL_H

#include <QAbstractItemModel>
#include <vector>

class ServerFilter;
class ServerItem;

class ServerListModel : public QAbstractItemModel
{
public:
	ServerListModel(ServerFilter *,QObject *parent=0);
	~ServerListModel();

	virtual int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	virtual int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	virtual QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
   	virtual bool setData( const QModelIndex &, const QVariant &, int role = Qt::EditRole );
	virtual QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

	//void profileIsAboutToChange();

	Qt::ItemFlags flags ( const QModelIndex &) const;

	const std::vector<ServerItem*> selection() const {return selection_;}

protected:
	std::vector<ServerItem*> selection_;

};

#endif