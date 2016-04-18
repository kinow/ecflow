//============================================================================
// Copyright 2016 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#include "LabelEditDialog.hpp"

#include <QtGlobal>

#include "VAttribute.hpp"
#include "VAttributeType.hpp"
#include "ServerHandler.hpp"

LabelEditDialog::LabelEditDialog(VInfo_ptr info,QWidget* parent) : AttributeEditor(info,parent)
{
    setupUi(this);

#if QT_VERSION >= QT_VERSION_CHECK(5, 2, 0)
    valueLe_->setClearButtonEnabled(true);
#endif

    VAttribute* a=info_->attribute();

    Q_ASSERT(a);
    Q_ASSERT(a->type());
    Q_ASSERT(a->type()->name() == "label");

    if(a->data().count() < 2)
            return;

    QString name=a->data().at(1);
    QString val;
    if(a->data().count() > 2)
        val=a->data().at(2);

    nameLabel_->setText(name);
    valueLe_->setText(val);

    header_->setInfo(QString::fromStdString(info_->path()),"Label");
}

void LabelEditDialog::apply()
{
    std::string val=valueLe_->text().toStdString();
    std::string name=nameLabel_->text().toStdString();

    std::vector<std::string> cmd;
    VAttribute::buildAlterCommand(cmd,"change","label",name,val);
    ServerHandler::command(info_,cmd);
}

static AttributeEditorMaker<LabelEditDialog> makerStr("label");
