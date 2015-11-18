//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include <QCloseEvent>
#include <QDebug>
#include <QSettings>

#include "NodeSearchDialog.hpp"
#include "VConfig.hpp"

NodeSearchDialog::NodeSearchDialog(QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose);

	QString wt=windowTitle();
	wt+="  -  " + QString::fromStdString(VConfig::instance()->appLongName());
	setWindowTitle(wt);

    connect(queryWidget_,SIGNAL(closeClicked()),
    		this,SLOT(accept()));

    //Read the qt settings
    readSettings();
}

NodeSearchDialog::~NodeSearchDialog()
{
}

NodeSearchWidget* NodeSearchDialog::queryWidget() const
{
	return queryWidget_;
}

void NodeSearchDialog::closeEvent(QCloseEvent * event)
{
	queryWidget_->slotStop(); //The search thread might be running!!
	event->accept();
	writeSettings();
}

void NodeSearchDialog::accept()
{
	writeSettings();
    QDialog::accept();
}

void NodeSearchDialog::reject()
{
	writeSettings();
	QDialog::reject();
}

//------------------------------------------
// Settings read/write
//------------------------------------------

void NodeSearchDialog::writeSettings()
{
	QSettings settings("ECMWF","ecFlowUI-NodeSearchDialog");

	//We have to clear it so that should not remember all the previous values
	settings.clear();

	settings.beginGroup("main");
	settings.setValue("size",size());
	//settings.setValue("current",list_->currentRow());
	settings.endGroup();
}

void NodeSearchDialog::readSettings()
{
	QSettings settings("ECMWF","ecFlowUI-NodeSearchDialog");

	settings.beginGroup("main");
	if(settings.contains("size"))
	{
		resize(settings.value("size").toSize());
	}
	else
	{
	  	resize(QSize(550,540));
	}

	/*if(settings.contains("current"))
	{
		int current=settings.value("current").toInt();
		if(current >=0)
			list_->setCurrentRow(current);
	}*/
	settings.endGroup();
}