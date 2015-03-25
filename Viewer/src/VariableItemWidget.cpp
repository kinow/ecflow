//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "VariableItemWidget.hpp"

#include <QDebug>
#include <QItemSelectionModel>
#include <QMessageBox>
#include <QVBoxLayout>

#include "LineEdit.hpp"
#include "VariableModel.hpp"
#include "VariableModelData.hpp"
#include "VariableSearchLine.hpp"

//======================================
//
// ServerDialogChecked
//
//======================================

bool VariableDialogChecker::checkName(QString name)
{
	if(name.simplified().isEmpty())
	{
		error(QObject::tr("<b>Name</b> cannot be empty!"));
		return false;
	}
	else if(name.contains(","))
	{
		error(QObject::tr("<b>Name</b> cannot contain comma character!"));
		return false;
	}

	/*if(ServerList::instance()->find(name.toStdString()))
	{
			error(QObject::tr("The specified server already exists! Please select a different name!"));
			return false;
	}*/

	return true;
}

bool VariableDialogChecker::checkValue(QString host)
{
	if(host.simplified().isEmpty())
	{
		error(QObject::tr("<b>Host</b> cannot be empty!"));
		return false;
	}
	else if(host.contains(","))
	{
		error(QObject::tr("<b>Host</b> cannot contain comma character!"));
		return false;
	}

	return true;
}


void VariableDialogChecker::error(QString msg)
{
	QMessageBox::critical(0,QObject::tr("Server item"),errorText_ + "<br>"+ msg);
}

//======================================
//
// VariablePropDialog
//
//======================================

VariablePropDialog::VariablePropDialog(VariableModelData *data,QString name, QString value,bool genVar,QWidget *parent) :
   QDialog(parent),
   genVar_(genVar),
   data_(data)
{
	setupUi(this);

	parentLabel_->setText(QString::fromStdString(data_->fullPath()) + " (" +
			    QString::fromStdString(data_->type()) +")");
	typeLabel_->setText(genVar?tr("generated variable"):tr("variable"));
	nameEdit_->setText(name);
	valueEdit_->setPlainText(value);
}

void VariablePropDialog::accept()
{
	QString name=nameEdit_->text();
	QString value=valueEdit_->toPlainText();

	if(data_->hasName(name.toStdString()))
	{
		if(QMessageBox::question(0,tr("Confirm: overwrite variable"),
									tr("This variable is <b>already defined</b>. A new variable will be created \
									for the selected node and hide the previous one.<br>Do you want to proceed?"),
						    QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel) == QMessageBox::Cancel)
			{
				QDialog::reject();
				return;
			}
	}

	if(genVar_)
	{
		if(QMessageBox::Ok!=
				QMessageBox::question(0,QObject::tr("Confirm: change variable"),
						"You are about to modify a <b>generated variable</b>.<br>Do you want to proceed?"),
						QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel)
		{
			QDialog::reject();
			return;
		}
	}

	QDialog::accept();
}

QString VariablePropDialog::name() const
{
	return nameEdit_->text();
}

QString VariablePropDialog::value() const
{
	return valueEdit_->toPlainText();
}

//======================================
//
// VariableAddDialog
//
//======================================

VariableAddDialog::VariableAddDialog(VariableModelData *data,QWidget *parent) :
   QDialog(parent),
   data_(data)
{
	setupUi(this);

	label_->setText(tr("Add new variable for ") +
			"<b>" + QString::fromStdString(data->type()) + "</b>: " +
			QString::fromStdString(data->name()));
}

VariableAddDialog::VariableAddDialog(VariableModelData *data,QString name, QString value,QWidget *parent) :
   QDialog(parent),
   data_(data)
{
	setupUi(this);

	label_->setText(tr("Add new variable for ") +
			"<b>" + QString::fromStdString(data->type()) + "</b>: " +
			QString::fromStdString(data->name()));

	nameEdit_->setText(name + "_copy");
	valueEdit_->setText(value);
}

void VariableAddDialog::accept()
{
	QString name=nameEdit_->text();
	QString value=valueEdit_->text();

	if(data_->hasName(name.toStdString()))
	{
		if(QMessageBox::question(0,tr("Confirm: overwrite variable"),
								tr("This variable is <b>already defined</b>. A new variable will be created \
								for the selected node and hide the previous one.<br>Do you want to proceed?"),
					    QMessageBox::Ok|QMessageBox::Cancel,QMessageBox::Cancel) == QMessageBox::Cancel)
		{
			QDialog::reject();
			return;
		}
	}

	QDialog::accept();
}

QString VariableAddDialog::name() const
{
	return nameEdit_->text();
}

QString VariableAddDialog::value() const
{
	return valueEdit_->text();
}

//========================================================
//
// VariableItemWidget
//
//========================================================

VariableItemWidget::VariableItemWidget(QWidget *parent)
{
	//This item displays all the ancestors of the info object
    useAncestors_=true;
    
    setupUi(this);

	data_=new VariableModelDataHandler();

	//The model and the sort-filter model
	model_=new VariableModel(data_,this);
	sortModel_= new VariableSortModel(model_,this);
    
    //!!!!We need to do it because:
    //The background colour between the tree view's left border and the items in the first column cannot be
    //controlled by delegates or stylesheets. It always takes the QPalette::Highlight
    //colour from the palette. Here we set this to transparent so that Qt could leave
    //this area empty and we will fill it appropriately in our delegate.
    QPalette pal=varView->palette();
    pal.setColor(QPalette::Highlight,Qt::transparent);
    varView->setPalette(pal);

    //Set the model on the view
    varView->setModel(sortModel_);

    //Search and filter interface: we have a menu attached to a toolbutton and a
    //stackedwidget connected up.

    //Populate the toolbuttons menu
    findModeTb->addAction(actionFilter);
	findModeTb->addAction(actionSearch);

    //The filter line editor
    filterLine_=new LineEdit;
    stackedWidget->addWidget(filterLine_);

    //The search line editor. Its a custom widget handling its own signals and slots.
    searchLine_=new VariableSearchLine(this);
    stackedWidget->addWidget(searchLine_);
    searchLine_->setView(varView);
    
    //The filter editor changes
    connect(filterLine_,SIGNAL(textChanged(QString)),
        		this,SLOT(slotFilterTextChanged(QString)));

    //The selection changes in the view
	connect(varView->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),
			this,SLOT(slotItemSelected(QModelIndex,QModelIndex)));

	//Init the find mode selection
	if(sortModel_->matchMode() == VariableSortModel::FilterMode)
	{
		actionFilter->trigger();
	}
	else
	{
		actionSearch->trigger();
	}

	//Add context menu actions to the view
	QAction* sep1=new QAction(this);
	sep1->setSeparator(true);
	QAction* sep2=new QAction(this);
	sep2->setSeparator(true);
	QAction* sep3=new QAction(this);
	sep3->setSeparator(true);

	//Build context menu
	varView->addAction(actionAdd);
	varView->addAction(sep1);
	varView->addAction(actionCopy);
	varView->addAction(actionPaste);
	varView->addAction(sep2);
	varView->addAction(actionDelete);
	varView->addAction(sep3);
	varView->addAction(actionProp);

	//Add actions for the toolbuttons
	addTb->setDefaultAction(actionAdd);
	deleteTb->setDefaultAction(actionDelete);
	propTb->setDefaultAction(actionProp);
	exportTb->setDefaultAction(actionExport);

	//Initialise action state (it depends on the selection)
	checkActionState();
}

VariableItemWidget::~VariableItemWidget()
{

}

QWidget* VariableItemWidget::realWidget()
{
	return this;
}

//A new info object is set
void VariableItemWidget::reload(VInfo_ptr info)
{
	adjust(info);
	data_->reload(info);
	varView->expandAll();
	varView->resizeColumnToContents(0);
	//varView->reload(info);
	loaded_=true;
}

void VariableItemWidget::clearContents()
{
	loaded_=false;
}


void VariableItemWidget::slotItemSelected(const QModelIndex& idx,const QModelIndex& /*prevIdx*/)
{
	checkActionState();
}

void VariableItemWidget::checkActionState()
{
	QModelIndex vIndex=varView->currentIndex();
	QModelIndex index=sortModel_->mapToSource(vIndex);

	//The index is invalid (no selection)
	if(!index.isValid())
	{
		actionAdd->setEnabled(false);
		actionProp->setEnabled(false);
		actionDelete->setEnabled(false);
	}
	else
	{
		//Variables
		if(model_->isVariable(index))
		{
			actionAdd->setEnabled(true);
			actionProp->setEnabled(true);
			actionDelete->setEnabled(true);
		}
		//Server or nodes
		else
		{
			actionAdd->setEnabled(true);
			actionProp->setEnabled(false);
			actionDelete->setEnabled(false);
		}
	}
}

void VariableItemWidget::editItem(const QModelIndex& index)
{
	QString name;
	QString value;
	bool genVar;

	QModelIndex vIndex=sortModel_->mapToSource(index);

	VariableModelData* data=model_->indexToData(vIndex);

	//Get the data from the model
	if(data && model_->variable(vIndex,name,value,genVar))
	{
		//Start edit dialog
		VariablePropDialog d(data,name,value,genVar,this);

		if(d.exec()== QDialog::Accepted)
		{
			model_->setVariable(vIndex,name,d.value());
		}
	}
}

void VariableItemWidget::duplicateItem(const QModelIndex& index)
{
	QString name;
	QString value;
	bool genVar;

	QModelIndex vIndex=sortModel_->mapToSource(index);

	VariableModelData* data=model_->indexToData(vIndex);

	//Get the data from the model
	if(data && model_->variable(vIndex,name,value,genVar))
	{
		//Start add dialog
		VariableAddDialog d(data,name,value,this);

		if(d.exec() == QDialog::Accepted)
		{
			data->add(d.name().toStdString(),d.value().toStdString());
		}
	}
}

void VariableItemWidget::addItem(const QModelIndex& index)
{
	QModelIndex vIndex=sortModel_->mapToSource(index);

	if(VariableModelData* data=model_->indexToData(vIndex))
	{
		//Start add dialog
		VariableAddDialog d(data,this);

		if(d.exec() == QDialog::Accepted)
		{
			data->add(d.name().toStdString(),d.value().toStdString());
		}
	}
}

void VariableItemWidget::removeItem(const QModelIndex& index)
{
	QString name;
	QString value;
	bool genVar;

	QModelIndex vIndex=sortModel_->mapToSource(index);

	//Get the data from the model
	if(model_->variable(vIndex,name,value,genVar))
	{
		if(QMessageBox::question(0,tr("Confirm: delete variable"),
						tr("Are you sure that you want to delete variable <b>") + name + "</b>?",
					    QMessageBox::Ok | QMessageBox::Cancel,QMessageBox::Cancel) == QMessageBox::Ok)
		{
			model_->removeVariable(vIndex,name,value);
		}
	}
}


void VariableItemWidget::on_varView_doubleClicked(const QModelIndex& index)
{
	editItem(index);
}

void VariableItemWidget::on_actionProp_triggered()
{
	QModelIndex index=varView->currentIndex();
	editItem(index);
}

void VariableItemWidget::on_actionAdd_triggered()
{
	QModelIndex index=varView->currentIndex();
	addItem(index);
}

void VariableItemWidget::on_actionDelete_triggered()
{
	QModelIndex index=varView->currentIndex();
	removeItem(index);
}

void VariableItemWidget::on_actionFilter_triggered()
{
	findModeTb->setIcon(actionFilter->icon());
	sortModel_->setMatchMode(VariableSortModel::FilterMode);
	filterLine_->clear();
	searchLine_->clear();

	//Notify stackedwidget
	stackedWidget->setCurrentIndex(0);
}

void VariableItemWidget::on_actionSearch_triggered()
{
	findModeTb->setIcon(actionSearch->icon());
	sortModel_->setMatchMode(VariableSortModel::SearchMode);
	filterLine_->clear();
	searchLine_->clear();

	//Notify stackedwidget
	stackedWidget->setCurrentIndex(1);

}

void VariableItemWidget::slotFilterTextChanged(QString text)
{
	sortModel_->setMatchText(text);
}


void VariableItemWidget::nodeChanged(const Node* node, const std::vector<ecf::Aspect::Type>& aspect)
{
	data_->nodeChanged(node,aspect);
}

//Register at the factory
static InfoPanelItemMaker<VariableItemWidget> maker1("variable");
