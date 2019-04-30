//============================================================================
// Copyright 2009-2019 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef SERVERLISTDIALOG_HPP_
#define SERVERLISTDIALOG_HPP_

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include "ui_ServerListDialog.h"
#include "ui_ServerEditDialog.h"
#include "ui_ServerAddDialog.h"

class ServerFilter;
class ServerItem;
class ServerListModel;
class ServerListFilterModel;

class ServerDialogChecker
{
protected:
    explicit ServerDialogChecker(QString txt) : errorText_(txt) {}

    bool checkName(QString name,QString oriName=QString());
	bool checkHost(QString host);
	bool checkPort(QString port);
	void error(QString msg);

	QString errorText_;
};


class ServerEditDialog : public QDialog, private Ui::ServerEditDialog, public ServerDialogChecker
{
Q_OBJECT

public:
    ServerEditDialog(QString name,QString host, QString port, bool favourite, bool ssl, QWidget* parent=nullptr);

	QString name() const;
	QString host() const;
	QString port() const;
    bool isSsl() const;
	bool isFavourite() const;

public Q_SLOTS:
	void accept() override;

private:
    QString oriName_;

};

class ServerAddDialog : public QDialog, private Ui::ServerAddDialog, public ServerDialogChecker
{
Q_OBJECT

public:
	explicit ServerAddDialog(QWidget* parent=nullptr);

	QString name() const;
	QString host() const;
	QString port() const;
	bool addToView() const;
    bool isSsl() const;

public Q_SLOTS:
	void accept() override;
};


class ServerListDialog : public QDialog, protected Ui::ServerListDialog
{
Q_OBJECT

public:
	enum Mode {SelectionMode,ManageMode};

	ServerListDialog(Mode,ServerFilter*,QWidget *parent=nullptr);
	~ServerListDialog() override;

     void showSysSyncLog();

public Q_SLOTS:
	 void accept() override;
	 void reject() override;

protected Q_SLOTS:
	 void on_actionEdit_triggered();
	 void on_actionAdd_triggered();
	 void on_actionDuplicate_triggered();
	 void on_actionDelete_triggered();
	 void on_actionRescan_triggered();
	 void on_serverView_doubleClicked(const QModelIndex& index);
     void on_actionFavourite_triggered(bool checked);
     void on_actionSsl_triggered(bool checked);
     void on_sysSyncTb_clicked(bool);
     void on_sysSyncLogTb_toggled(bool);
     void slotItemSelected(const QModelIndex&,const QModelIndex&);
	 void slotItemClicked(const QModelIndex&);
	 void slotFilter(QString);
	 void slotFilterFavourite(bool);

protected:
	void closeEvent(QCloseEvent*) override;
	void editItem(const QModelIndex& index);
	void duplicateItem(const QModelIndex& index);
	void addItem();
	void removeItem(const QModelIndex& index);
    void setFavouriteItem(const QModelIndex& index,bool b);
    void setSslItem(const QModelIndex& index,bool b);
	void checkActionState();
	void writeSettings();
	void readSettings();

	ServerFilter* filter_;
	ServerListModel* model_;
	ServerListFilterModel* sortModel_;
	Mode mode_;
};


class ServerListModel : public QAbstractItemModel
{
public:
	explicit ServerListModel(ServerFilter*,QObject *parent=nullptr);
	~ServerListModel() override;

	int columnCount (const QModelIndex& parent = QModelIndex() ) const override;
   	int rowCount (const QModelIndex& parent = QModelIndex() ) const override;

   	QVariant data (const QModelIndex& , int role = Qt::DisplayRole ) const override;
   	bool setData( const QModelIndex &, const QVariant &, int role = Qt::EditRole ) override;
	QVariant headerData(int,Qt::Orientation,int role = Qt::DisplayRole ) const override;

   	QModelIndex index (int, int, const QModelIndex& parent = QModelIndex() ) const override;
   	QModelIndex parent (const QModelIndex & ) const override;
   	Qt::ItemFlags flags ( const QModelIndex & index) const override;

   	void dataIsAboutToChange();
   	void dataChangeFinished();
   	ServerItem* indexToServer(const QModelIndex& index);

    enum Columns {LoadColumn=0, NameColumn=1, HostColumn=2, PortColumn=3, SystemColumn=4, SslColumn=5,
                  FavouriteColumn=6, UseColumn=7};
    enum CustomItemRole {IconStatusRole = Qt::UserRole+1};

protected:
   	ServerFilter* filter_;
   	QPixmap favPix_;
   	QPixmap favEmptyPix_;
    QPixmap sysPix_;
   	QFont loadFont_;
};

class ServerListFilterModel : public QSortFilterProxyModel
{
public:
	explicit ServerListFilterModel(QObject *parent=nullptr);
    ~ServerListFilterModel() override = default;
	void setFilterStr(QString);
	void setFilterFavourite(bool b);

protected:
	 bool filterAcceptsRow(int sourceRow,const QModelIndex &sourceParent) const override;
     bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

	 QString filterStr_;
	 bool filterFavourite_{false};
};

#endif

