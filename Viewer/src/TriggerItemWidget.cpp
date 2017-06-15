//============================================================================
// Copyright 2009-2017 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "TriggerItemWidget.hpp"

#include "Highlighter.hpp"
#include "TriggerCollector.hpp"
#include "TriggeredScanner.hpp"
#include "VNode.hpp"
#include "VSettings.hpp"

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QGuiApplication>
#endif

//========================================================
//
// TriggerItemWidget
//
//========================================================

TriggerItemWidget::TriggerItemWidget(QWidget *parent) : QWidget(parent)
{
    //This item will listen to any changes in nodes
    handleAnyChange_=true;

    setupUi(this);

    messageLabel_->hide();
    messageLabel_->setShowTypeTitle(false);

    dependInfoTb_->setChecked(false);
    on_dependInfoTb__toggled(false);

    //Expression
    exprTb_->setChecked(true);

    Highlighter* ih=new Highlighter(exprTe_->document(),"trigger");
    exprTe_->setReadOnly(true);
    exprTe_->setBackgroundVisible(true);

    //Set the height of the trigger expression display area
    QFont fTe;
    fTe.setBold(true);
    QFontMetrics fm(fTe);
    exprTe_->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
    exprTe_->setFixedHeight(fm.size(0,"A\nA\nA").height()+fm.height()/2);

    //Dependencies
    dependTb_->setChecked(false);

    connect(triggerTable_,SIGNAL(depInfoWidgetClosureRequested()),
            this,SLOT(slotHandleDefInfoWidgetClosure()));

    connect(triggerTable_,SIGNAL(linkSelected(VInfo_ptr)),
            this,SLOT(slotLinkSelected(VInfo_ptr)));

    connect(triggerTable_,SIGNAL(infoPanelCommand(VInfo_ptr,QString)),
        this,SLOT(slotInfoPanelCommand(VInfo_ptr,QString)));

    connect(triggerTable_,SIGNAL(dashboardCommand(VInfo_ptr,QString)),
        this,SLOT(slotDashboardCommand(VInfo_ptr,QString)));

    //The collectors
    triggerCollector_=new TriggerTableCollector(false);
    triggeredCollector_=new TriggerTableCollector(false);

    //Scanner
    scanner_=new TriggeredScanner(this);

    connect(scanner_,SIGNAL(scanStarted()),
            this,SLOT(scanStarted()));

    connect(scanner_,SIGNAL(scanFinished()),
            this,SLOT(scanFinished()));

    connect(scanner_,SIGNAL(scanProgressed(int)),
            this,SLOT(scanProgressed(int)));
}

TriggerItemWidget::~TriggerItemWidget()
{
    clearContents();
    delete triggerCollector_;
    delete triggeredCollector_;
}

QWidget* TriggerItemWidget::realWidget()
{
	return this;
}

void TriggerItemWidget::reload(VInfo_ptr info)
{
    assert(active_);

    if(suspended_)
        return;

    clearContents();

    //set the info
    adjust(info);

    triggerTable_->setInfo(info_);

    //messageLabel_->hide();

    //Info must be a node
    load();
}

void TriggerItemWidget::load()
{
    if(info_ && info_->isNode() && info_->node())
    {
        VNode* n=info_->node();
        Q_ASSERT(n);

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
#endif

        //Display trigger expression
        std::string te,ce;
        n->triggerExpr(te,ce);
        QString txt=QString::fromStdString(te);
        if(txt.isEmpty()) txt=tr("No trigger expression is available for the selected node!");
        exprTe_->setPlainText(txt);

        //Load table
        triggerTable_->beginTriggerUpdate();

        //collect the list of triggers of this node
        triggerCollector_->setDependency(dependency());
        n->triggers(triggerCollector_);

        triggeredCollector_->setDependency(dependency());
        n->triggered(triggeredCollector_,triggeredScanner());

        triggerTable_->setTriggerCollector(triggerCollector_,triggeredCollector_);
        triggerTable_->endTriggerUpdate();

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
        QGuiApplication::restoreOverrideCursor();
#endif
    }
}

void TriggerItemWidget::clearContents()
{
    InfoPanelItem::clear();
    triggerTable_->clear();

    //At this point the tables are cleared so it is safe to clear the collectors
    triggerCollector_->clear();
    triggeredCollector_->clear();
}

void TriggerItemWidget::updateState(const FlagSet<ChangeFlag>& flags)
{
    if(flags.isSet(SuspendedChanged))
    {
        //if(suspended_)
        //   textBrowser_->suspend();
    }

    checkActionState();
}

void TriggerItemWidget::checkActionState()
{
    if(suspended_)
    {
         dependTb_->setEnabled(false);         
         return;
    }

    dependTb_->setEnabled(true);
}

void TriggerItemWidget::on_dependTb__toggled(bool)
{   
    load();
}

void TriggerItemWidget::on_dependInfoTb__toggled(bool b)
{
    triggerTable_->slotShowDependencyInfo(b);
}

void TriggerItemWidget::slotHandleDefInfoWidgetClosure()
{
    dependInfoTb_->setChecked(false);
}


void TriggerItemWidget::on_exprTb__toggled(bool b)
{
    exprTe_->setVisible(b);
}

bool TriggerItemWidget::dependency() const
{
    return dependTb_->isChecked();
}


void TriggerItemWidget::slotLinkSelected(VInfo_ptr info)
{
   InfoPanelItem::linkSelected(info);
}

void TriggerItemWidget::slotInfoPanelCommand(VInfo_ptr info,QString cmd)
{
    InfoPanelItem::relayInfoPanelCommand(info,cmd);
}

void TriggerItemWidget::slotDashboardCommand(VInfo_ptr info,QString cmd)
{
    InfoPanelItem::relayDashboardCommand(info,cmd);
}

#if 0
void TriggerItemWidget::infoProgressStart(const std::string& text,int max)
{
    messageLabel_->showInfo(QString::fromStdString(text));
    messageLabel_->startProgress(max);
}

void TriggerItemWidget::infoProgress(const std::string& text,int value)
{
    messageLabel_->progress(QString::fromStdString(text),value);
}

#endif

void TriggerItemWidget::scanStarted()
{
    messageLabel_->showInfo("Mapping trigger connections in the whole tree ...");
    messageLabel_->startProgress(100);
}

void TriggerItemWidget::scanFinished()
{
    messageLabel_->stopProgress();
    messageLabel_->hide();
}

void TriggerItemWidget::scanProgressed(int value)
{
    std::string text="";
    messageLabel_->progress(QString::fromStdString(text),value);
}

void TriggerItemWidget::writeSettings(VComboSettings* vs)
{
    vs->beginGroup("triggers");
    vs->putAsBool("dependency",dependency());
    vs->putAsBool("dependencyInfo",dependInfoTb_->isChecked());
    vs->putAsBool("expression",exprTb_->isChecked());

    triggerTable_->writeSettings(vs);
    vs->endGroup();
}

void TriggerItemWidget::readSettings(VComboSettings* vs)
{
    vs->beginGroup("triggers");
    dependTb_->setChecked(vs->getAsBool("dependency",dependency()));
    dependInfoTb_->setChecked(vs->getAsBool("dependencyInfo",dependInfoTb_->isChecked()));
    exprTb_->setChecked(vs->getAsBool("expression",exprTb_->isChecked()));
    triggerTable_->readSettings(vs);
    vs->endGroup();
}

//-------------------------
// Update
//-------------------------

void TriggerItemWidget::nodeChanged(const VNode* n, const std::vector<ecf::Aspect::Type>& aspect)
{
    if(!info_ || !info_->isNode())
        return;

    //For certain changes we need to reload the triggers
    for(std::vector<ecf::Aspect::Type>::const_iterator it=aspect.begin(); it != aspect.end(); ++it)
    {
        if(*it == ecf::Aspect::ADD_REMOVE_ATTR || *it == ecf::Aspect::EXPR_TRIGGER)
        {
            load();
            return;
        }
        else if(*it == ecf::Aspect::NODE_VARIABLE || *it == ecf::Aspect::METER || *it == ecf::Aspect::LIMIT ||
                *it == ecf::Aspect::EVENT)
        {
           if(triggerCollector_->contains(n,true) || triggeredCollector_->contains(n,true))
           {
               load();
               return;
           }
        }
    }

    //See if there is a change in the collected items
    triggerTable_->nodeChanged(n,aspect);
}

static InfoPanelItemMaker<TriggerItemWidget> maker1("triggers");
