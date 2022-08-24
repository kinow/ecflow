//============================================================================
// Copyright 2009- ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "OutputDirWidget.hpp"

#include <QItemSelectionModel>
#include <QTimer>

#include "OutputModel.hpp"
#include "TextFormat.hpp"
#include "UiLog.hpp"
#include "ViewerUtil.hpp"
#include "VReply.hpp"

#include "ui_OutputDirWidget.h"

#define UI_OUTPUTDIRWIDGET_DEBUG_

int OutputDirWidget::updateDirTimeout_=1000*60;

class DirWidgetState
{
public:
    DirWidgetState(OutputDirWidget* owner,  DirWidgetState* prev);
    virtual ~DirWidgetState() = default;
    virtual void handleReload();
    virtual void handleLoad(VReply*);
    virtual void handleFailed(VReply*)=0;
    virtual void handleClear();
    virtual void handleEnable() {}
    virtual void handleDisable();
    virtual void handleSuspendAutoUpdate();
    virtual bool isDisabled() const {return false;}

protected:
    OutputDirWidget* owner_{nullptr};
    bool timerSuspended_{false};
};

class DirWidgetSuccessState : public DirWidgetState
{
public:
    DirWidgetSuccessState(OutputDirWidget* owner, DirWidgetState* prev, VReply*);
    void handleLoad(VReply*) override;
    void handleFailed(VReply*) override;

protected:
    void handleLoadInternal(VReply* reply);
};

class DirWidgetFirstFailedState : public DirWidgetState
{
public:
    DirWidgetFirstFailedState(OutputDirWidget* owner, DirWidgetState* prev, VReply* reply);
    void handleFailed(VReply*) override;
};

class DirWidgetFailedState : public DirWidgetState
{
public:
    DirWidgetFailedState(OutputDirWidget* owner, DirWidgetState* prev, VReply* reply);
    void handleFailed(VReply*) override;

protected:
    void handleFailedInternal(VReply* reply);
};

class DirWidgetEmptyState : public DirWidgetState
{
public:
    DirWidgetEmptyState(OutputDirWidget* owner, DirWidgetState* prev);
    void handleFailed(VReply*) override;
};

class DirWidgetDisabledState : public DirWidgetState
{
public:
    DirWidgetDisabledState(OutputDirWidget* owner, DirWidgetState* prev);
    void handleReload() override {}
    void handleLoad(VReply*) override {}
    void handleFailed(VReply*) override {}
    void handleClear() override {}
    void handleEnable() override;
    void handleDisable() override {}
    void handleSuspendAutoUpdate() override {}
    bool isDisabled() const override {return true;}
};

//-------------------------------
// DirWidgetState
//-------------------------------

DirWidgetState::DirWidgetState(OutputDirWidget* owner,  DirWidgetState* prev) :
    owner_(owner)
{
    if (prev)
        timerSuspended_ = prev->timerSuspended_;
}

void DirWidgetState::handleReload()
{
    // if reload is requested the timer must come back from its suspended
    // state
    timerSuspended_ = false;

    // the timer must be stopped while the reload (might be asynch)
    // is being executed
    owner_->stopTimer();
    owner_->ui_->reloadTb->setEnabled(false);
    owner_->requestReload();
}

void DirWidgetState::handleLoad(VReply* reply)
{
    owner_->transitionTo(new DirWidgetSuccessState(owner_, this, reply));
}

void DirWidgetState::handleClear()
{
    owner_->transitionTo(new DirWidgetEmptyState(owner_, this));
}

void DirWidgetState::handleDisable()
{
    owner_->transitionTo(new DirWidgetDisabledState(owner_, this));
}

void DirWidgetState::handleSuspendAutoUpdate()
{
#ifdef UI_OUTPUTDIRWIDGET_DEBUG_
    UI_FN_DBG
#endif
    owner_->stopTimer();
    timerSuspended_ = true;
}

//-------------------------------
// DirWidgetSuccessState
//-------------------------------

DirWidgetSuccessState::DirWidgetSuccessState(OutputDirWidget* owner, DirWidgetState* prev, VReply* reply) :
    DirWidgetState(owner, prev)
{
    handleLoadInternal(reply);
}

void DirWidgetSuccessState::handleLoad(VReply* reply)
{
    handleLoadInternal(reply);
}

void DirWidgetSuccessState::handleLoadInternal(VReply* reply)
{
    // we ensure the update timer is stopped
    owner_->stopTimer();

    //We do not display info/warning here! The dirMessageLabel_ is not part of the dirWidget_
    //and is only supposed to display error messages!
    owner_->ui_->messageLabelTop->hide();
    owner_->ui_->infoLabel->show();
    owner_->ui_->reloadTb->show();
    owner_->ui_->messageLabelBottom->hide();
    owner_->ui_->view->show();

    //Update the dir widget and select the proper file in the list
    owner_->updateContents(reply->directories());

    //Update the dir label
    owner_->ui_->infoLabel->update(reply);

    //Enable the update button
    owner_->ui_->reloadTb->setEnabled(true);

    owner_->show();

    //The update timer is restarted since we seem to have access to the directories
    //so we want automatic updates
    if (!timerSuspended_) {
#ifdef UI_OUTPUTDIRWIDGET_DEBUG_
        UiLog().dbg() << UI_FN_INFO << "start timer";
#endif
        owner_->startTimer();
    }
#ifdef UI_OUTPUTDIRWIDGET_DEBUG_
    UiLog().dbg() << UI_FN_INFO << "timerSuspended_=" << timerSuspended_;
#endif
}

void DirWidgetSuccessState::handleFailed(VReply* reply)
{
    owner_->transitionTo(new DirWidgetFirstFailedState(owner_, this, reply));
}

//-------------------------------
// DirWidgetFirstFailedState
//-------------------------------

DirWidgetFirstFailedState::DirWidgetFirstFailedState(OutputDirWidget* owner, DirWidgetState* prev, VReply* reply) :
    DirWidgetState(owner, prev)
{
    // we ensure the update timer is stopped
    owner_->stopTimer();

    //We do not have directories
    owner_->dirModel_->clearData();

    // only show the top row with a messageLabel
    owner_->ui_->messageLabelTop->show();
    owner_->ui_->infoLabel->hide();
    owner_->ui_->reloadTb->show();
    owner_->ui_->messageLabelBottom->hide();
    owner_->ui_->view->hide();

    auto err = owner_->formatErrors(reply->errorTextVec());
#ifdef UI_OUTPUTDIRWIDGET_DEBUG_
    UiLog().dbg() << UI_FN_INFO << "err=" << err;
#endif
    if (!err.isEmpty()) {
        owner_->ui_->messageLabelTop->showError(err);
    } else {
        owner_->ui_->messageLabelTop->showError("Failed to fetch directory listing");
    }

    owner_->ui_->reloadTb->setEnabled(true);
    owner_->show();

    // we start the update timer since it was the first failure, we
    // allow one more reload try
    if (!timerSuspended_) {
        owner_->startTimer();
    }
#ifdef UI_OUTPUTDIRWIDGET_DEBUG_
    UiLog().dbg() << UI_FN_INFO << "timerSuspended_=" << timerSuspended_;
#endif
}

void DirWidgetFirstFailedState::handleFailed(VReply* reply)
{
    owner_->transitionTo(new DirWidgetFailedState(owner_, this, reply));
}

//-------------------------------
// DirWidgetFailedState
//-------------------------------

DirWidgetFailedState::DirWidgetFailedState(OutputDirWidget* owner, DirWidgetState* prev, VReply* reply) :
    DirWidgetState(owner, prev)
{
    handleFailedInternal(reply);
}

void DirWidgetFailedState::handleFailed(VReply* reply)
{
    handleFailedInternal(reply);
}

void DirWidgetFailedState::handleFailedInternal(VReply* reply)
{
    //The timer is stopped and will not be restarted. Since we had at least two
    //failures in a row there is probably no access to the dir contents. Manual
    //reload is still possible.
    owner_->stopTimer();

    owner_->dirModel_->clearData();

    // only show the top row with a messageLabel
    owner_->ui_->messageLabelTop->show();
    owner_->ui_->infoLabel->hide();
    owner_->ui_->reloadTb->show();
    owner_->ui_->messageLabelBottom->hide();
    owner_->ui_->view->hide();

    // we only show a warning
    owner_->ui_->messageLabelTop->showWarning("No access to output directories");

    owner_->ui_->reloadTb->setEnabled(true);
    owner_->show();
}

//===========================================================
// DirWidgetEmptyState
//===========================================================

DirWidgetEmptyState::DirWidgetEmptyState(OutputDirWidget* owner, DirWidgetState* prev) :
    DirWidgetState(owner, prev)
{
    owner_->stopTimer();
    owner_->hide();
    owner_->joboutFile_.clear();
    owner_->dirModel_->clearData();
}

void DirWidgetEmptyState::handleFailed(VReply* reply)
{
    owner_->transitionTo(new DirWidgetFirstFailedState(owner_, this, reply));
}

//===========================================================
// DirWidgetDisabledState
//===========================================================

DirWidgetDisabledState::DirWidgetDisabledState(OutputDirWidget* owner, DirWidgetState* prev) :
    DirWidgetState(owner, prev)
{
    owner_->stopTimer();
    owner_->hide();
    owner_->joboutFile_.clear();
    owner_->dirModel_->clearData();
}

void DirWidgetDisabledState::handleEnable()
{
    owner_->transitionTo(new DirWidgetEmptyState(owner_, this));
    owner_->reload();
}

//===========================================================
//
// OutputDirWidget
//
//==========================================================

OutputDirWidget::OutputDirWidget(QWidget* parent) :
    QWidget(parent),
    ui_(new Ui::OutputDirWidget)
{
    ui_->setupUi(this);

    //--------------------------------
    // The dir contents
    //--------------------------------

    ui_->messageLabelTop->hide();
    ui_->messageLabelTop->setShowTypeTitle(false);
    ui_->messageLabelBottom->hide();
    ui_->messageLabelBottom->setShowTypeTitle(false);

    //dirLabel_->hide();
    ui_->infoLabel->setProperty("fileInfo","1");

    //The view
    auto* dirDelegate=new OutputDirListDelegate(this);
    ui_->view->setItemDelegate(dirDelegate);
    ui_->view->setRootIsDecorated(false);
    ui_->view->setAllColumnsShowFocus(true);
    ui_->view->setUniformRowHeights(true);
    ui_->view->setAlternatingRowColors(true);
    ui_->view->setSortingEnabled(true);

    //Sort by column "modifiied (ago)", latest files first
    ui_->view->sortByColumn(3, Qt::AscendingOrder);

    //The models
    dirModel_=new OutputModel(this);
    dirSortModel_=new OutputSortModel(this);
    dirSortModel_->setSourceModel(dirModel_);
    dirSortModel_->setSortRole(Qt::UserRole);
    dirSortModel_->setDynamicSortFilter(true);

    ui_->view->setModel(dirSortModel_);

    //When the selection changes in the view
    connect(ui_->view->selectionModel(),SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            this,SLOT(slotItemSelected(QModelIndex,QModelIndex)));

    connect(ui_->reloadTb,SIGNAL(clicked()),
            this,SLOT(reload()));

    connect(ui_->closeTb,SIGNAL(clicked()),
            this,SLOT(closeByButton()));

    //Dir contents update timer
    updateTimer_=new QTimer(this);
    updateTimer_->setInterval(updateDirTimeout_);

    connect(updateTimer_,SIGNAL(timeout()),
            this,SLOT(reload()));

    // initialise state
    transitionTo(new DirWidgetEmptyState(this, nullptr));
}

// must be called externally
void OutputDirWidget::showIt(bool st)
{
    if (st) {
        state_->handleEnable();
    } else {
        state_->handleDisable();
    }
}

void OutputDirWidget::closeByButton()
{
    state_->handleDisable();
    Q_EMIT closedByButton();
}

void OutputDirWidget::clear()
{
    state_->handleClear();
}

void OutputDirWidget::load(VReply* reply, const std::string& joboutFile)
{
    joboutFile_ = joboutFile;
    state_->handleLoad(reply);
}

void OutputDirWidget::failed(VReply* reply, const std::string& joboutFile)
{
    joboutFile_ = joboutFile;
    state_->handleFailed(reply);
}

void OutputDirWidget::suspendAutoUpdate()
{
    UI_FN_DBG
    state_->handleSuspendAutoUpdate();
}

void OutputDirWidget::startTimer()
{
    updateTimer_->start();
}

void OutputDirWidget::stopTimer()
{
    updateTimer_->stop();
}

void OutputDirWidget::reload()
{
    state_->handleReload();
}

void OutputDirWidget::requestReload()
{
    Q_EMIT updateRequested();
}

bool OutputDirWidget::currentSelection(std::string& fPath, VDir::FetchMode& mode) const
{
    QModelIndex current=dirSortModel_->mapToSource(ui_->view->currentIndex());
    if(current.isValid())
    {
        dirModel_->itemDesc(current, fPath, mode);
        return true;
    }
    return false;
}

void OutputDirWidget::adjustCurrentSelection(VFile_ptr loadedFile)
{
    if (!state_->isDisabled() && loadedFile) {
        adjustCurrentSelection(loadedFile->sourcePath(), loadedFile->fetchMode());
    }
}

void OutputDirWidget::adjustCurrentSelection(const std::string& fPath, VFile::FetchMode fMode)
{
    ignoreOutputSelection_ = true;
    setCurrentSelection(fPath, fMode);
    ignoreOutputSelection_ = false;
}

// set the current item in the directory list based on the contents of the browser. If no mathing
// dir items found the current index is not set. This relies on the sourcePath in the current file
// objects so it has to be propely set!!
void OutputDirWidget::setCurrentSelection(const std::string& fPath, VFile::FetchMode fMode)
{
    if(!dirModel_->isEmpty()) {

#ifdef UI_OUTPUTDIRWIDGET_DEBUG_
        UiLog().dbg() << UI_FN_INFO;
#endif
        QModelIndex idx;

        if (!fPath.empty()) {
#ifdef UI_OUTPUTDIRWIDGET_DEBUG_
            UiLog().dbg() << " fPath=" << fPath;
#endif
            if (fPath == joboutFile_) {
                idx = dirModel_->itemToIndex(fPath);
            } else {
                VDir::FetchMode dMode=VDir::NoFetchMode;
                switch(fMode) {
                case VFile::LocalFetchMode:
                    dMode = VDir::LocalFetchMode;
                    break;
                case VFile::LogServerFetchMode:
                    dMode = VDir::LogServerFetchMode;
                    break;
                case VFile::TransferFetchMode:
                    dMode = VDir::TransferFetchMode;
                    break;
                default:
                     dMode=VDir::NoFetchMode;
                    break;
                }
                idx = dirModel_->itemToIndex(fPath, dMode);
#ifdef UI_OUTPUTDIRWIDGET_DEBUG_
                UiLog().dbg() << " idx=" << idx <<  " dMode=" << dMode;
#endif
            }
        }

        if (idx.isValid()) {
            ui_->view->setCurrentIndex(dirSortModel_->mapFromSource(idx));
        }
    }
}

void OutputDirWidget::updateContents(const std::vector<VDir_ptr>& dirs)
{
    bool status=false;
    for(const auto & dir : dirs)
    {
        if(dir && dir->count() > 0)
        {
            status=true;
            break;
        }
    }

    if(status)
    {
        ui_->view->selectionModel()->clearSelection();
        dirModel_->resetData(dirs,joboutFile_);

        //Adjust column width
        if(!dirColumnsAdjusted_)
        {
            dirColumnsAdjusted_=true;
            for(int i=0; i< dirModel_->columnCount()-1; i++)
                ui_->view->resizeColumnToContents(i);

            if(dirModel_->columnCount() > 1)
                ui_->view->setColumnWidth(1,ui_->view->columnWidth(0));

        }
#ifdef UI_OUTPUTDIRWIDGET_DEBUG_
        UiLog().dbg() << UI_FN_INFO << "dir item count=" << dirModel_->rowCount();
#endif
    }
    else
    {
        dirModel_->clearData();
    }
}

bool OutputDirWidget::isEmpty() const
{
    return dirModel_->isEmpty();
}

bool OutputDirWidget::isNotInDisabledState() const
{
    return !state_->isDisabled();
}

QString OutputDirWidget::formatErrors(const std::vector<std::string>& errorVec) const
{
    QString s;
    if(errorVec.size() > 0)
    {
        QColor col(70,71,72);
        s=Viewer::formatBoldText("Output directory: ",col);

        if(errorVec.size() > 1)
        {
            for(size_t i=0; i < errorVec.size(); i++)
                s+=Viewer::formatBoldText("[" + QString::number(i+1) + "] ",col) +
                    QString::fromStdString(errorVec[i]) + ". &nbsp;&nbsp;";
        }
        else if(errorVec.size() == 1)
            s+=QString::fromStdString(errorVec[0]);
    }
    return s;
}


//This slot is called when a file item is selected in the dir view
void OutputDirWidget::slotItemSelected(const QModelIndex& currentIdx,const QModelIndex& /*idx2*/)
{
    if(!ignoreOutputSelection_) {
        Q_EMIT itemSelected();
    }
}

void OutputDirWidget::transitionTo(DirWidgetState* state)
{
#ifdef UI_OUTPUTDIRWIDGET_DEBUG_
    UiLog().dbg() << UI_FN_INFO << typeid(*state).name() << "\n";
#endif
    if (state_ != nullptr) {
        delete state_;
    }
    state_ = state;
}