//============================================================================
// Copyright 2009-2018 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef TIMELINEINFODELEGATE_HPP
#define TIMELINEINFODELEGATE_HPP

#include <QBrush>
#include <QDateTime>
#include <QPen>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>

#include "NodeViewDelegate.hpp"
#include "VProperty.hpp"

#include <string>

class TimelineInfoDailyModel;

class TimelineInfoDelegate : public NodeViewDelegate
{
public:
    explicit TimelineInfoDelegate(QWidget *parent=0);
    ~TimelineInfoDelegate();

    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const;
    void paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const;

protected:
    void updateSettings();

    QPen borderPen_;

};

class TimelineInfoDailyDelegate : public QStyledItemDelegate, public VPropertyObserver
{

public:
    explicit TimelineInfoDailyDelegate(TimelineInfoDailyModel* model,QWidget *parent);
    ~TimelineInfoDailyDelegate();

    QSize sizeHint(const QStyleOptionViewItem & option, const QModelIndex & index ) const;
    void paint(QPainter *painter,const QStyleOptionViewItem &option,
                   const QModelIndex& index) const;

    void notifyChange(VProperty* p);

    //void setStartDate(QDateTime);
    //void setEndDate(QDateTime);
    //void setPeriod(QDateTime t1,QDateTime t2);
    //void setMaxDurations(int submittedDuration,int activeDuration);

//Q_SIGNALS:
//    void sizeHintChangedGlobal();

protected:
    void updateSettings();
    void renderTimeline(QPainter *painter,const QStyleOptionViewItem& option,const QModelIndex& index) const;
    void drawCell(QPainter *painter,QRect r,QColor fillCol,bool hasGrad,bool lighter) const;
    int timeToPos(QRect r,unsigned int time) const;

    TimelineInfoDailyModel* model_;
    PropertyMapper* prop_;
    QFont font_;
    QFontMetrics fm_;
    QPen borderPen_;
    int topPadding_;
    int bottomPadding_;
    QDateTime startDate_;
    QDateTime endDate_;
    //int submittedMaxDuration_;
    //int activeMaxDuration_;
    //int durationMaxTextWidth_;
};


#endif // TIMELINEINFODELEGATE_HPP