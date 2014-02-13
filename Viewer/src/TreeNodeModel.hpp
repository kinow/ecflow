#ifndef TREENODEMODEL_H
#define TREENODEMODEL_H

#include <QAbstractItemModel>

#include <vector>

#include "ViewNodeInfo.hpp"

class Node;
class ServerHandler;


class TreeNodeModel : public QAbstractItemModel
{

public:
   	TreeNodeModel(QObject *parent=0);

   	int columnCount (const QModelIndex& parent = QModelIndex() ) const;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const;

   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const;
	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const;
   	QModelIndex parent (const QModelIndex & ) const;

	void addServer(ServerHandler *);
	void setRootNode(Node *node);
	void dataIsAboutToChange();
	ViewNodeInfo_ptr nodeInfo(const QModelIndex& index) const;

protected:
	bool hasData() const;
	bool isServer(const QModelIndex & index) const;
	ServerHandler* indexToServer(const QModelIndex & index) const;
	QModelIndex serverToIndex(ServerHandler*) const;
	QModelIndex indexFromNode(Node*) const;
	Node* indexToNode( const QModelIndex & index) const;

	QVariant serverData(const QModelIndex& index,int role) const;
	QVariant nodeData(const QModelIndex& index,int role) const;
	Node *rootNode(ServerHandler*) const;

	QList<ServerHandler*> servers_;
	QMap<ServerHandler*,Node*> rootNodes_;

};

#endif