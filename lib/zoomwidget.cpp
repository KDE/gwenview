// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "zoomwidget.h"

// stdc++
#include <cmath>

// Qt
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QHBoxLayout>
#include <QSlider>
#include <QDebug>
// KF

// Local
#include "zoomslider.h"
#include "signalblocker.h"
#include "statusbartoolbutton.h"
#include "zoomcombobox/zoomcombobox.h"

namespace Gwenview
{

static const qreal MAGIC_K = 1.04;
static const qreal MAGIC_OFFSET = 16.;
static const qreal PRECISION = 100.;

inline int sliderValueForZoom(qreal zoom)
{
    return qRound(PRECISION * (log(zoom) / log(MAGIC_K) + MAGIC_OFFSET));
}

inline int comboBoxValueForZoom(qreal zoom)
{
    return qRound(zoom * PRECISION);
}

inline qreal zoomForSliderValue(int sliderValue)
{
    return pow(MAGIC_K, sliderValue / PRECISION - MAGIC_OFFSET);
}

inline qreal zoomForComboBoxValue(int comboBoxValue)
{
    return comboBoxValue / PRECISION;
}

inline int sliderValueForComboBoxValue(int comboBoxValue)
{
    return qRound(log(comboBoxValue) / log(MAGIC_K) + MAGIC_OFFSET);
}

inline int comboBoxValueForSliderValue(int sliderValue)
{
    return qRound(pow(MAGIC_K, sliderValue - MAGIC_OFFSET));
}

struct ZoomWidgetPrivate
{
    ZoomWidget* q;

    ZoomSlider* mZoomSlider;
    ZoomComboBox* mZoomComboBox;
    QAction* mZoomToFitAction;
    QAction* mActualSizeAction;
    QAction* mZoomToFillAction;

    bool mZoomUpdatedBySlider = false;
    bool mZoomUpdatedByComboBox = false;

    qreal mZoom = -1;
    qreal mMinimumZoom = -1;
    qreal mMaximumZoom = -1;
};

ZoomWidget::ZoomWidget(QWidget* parent)
: QFrame(parent)
, d(new ZoomWidgetPrivate)
{
    d->q = this;
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);

    d->mZoomSlider = new ZoomSlider;
    d->mZoomSlider->setMinimumWidth(150);
//     d->mZoomSlider->slider()->setRange(sliderValueForZoom(d->mMinimumZoom),
//                                        sliderValueForZoom(d->mMaximumZoom));
    d->mZoomSlider->slider()->setSingleStep(int(PRECISION));
    d->mZoomSlider->slider()->setPageStep(3 * int(PRECISION));

    d->mZoomComboBox = new ZoomComboBox(this);
//     d->mZoomComboBox->setRange(comboBoxValueForZoom(d->mMinimumZoom),
//                                comboBoxValueForZoom(d->mMaximumZoom));

    // Layout
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(d->mZoomSlider);
    layout->addWidget(d->mZoomComboBox);

    connect(d->mZoomSlider->slider(), &QAbstractSlider::actionTriggered,
            this, &ZoomWidget::setZoomFromSlider);
    connect(d->mZoomComboBox, &ZoomComboBox::valueChanged, this, &ZoomWidget::setZoomFromComboBox);
    connect(d->mZoomComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
        if (index > -1 && index < d->mZoomComboBox->actions().length()) {
            auto action = d->mZoomComboBox->actions().at(index);
            action->trigger();
        }
    });
}

ZoomWidget::~ZoomWidget()
{
    delete d;
}

void ZoomWidget::setActions(QAction* zoomToFitAction, QAction* actualSizeAction, QAction* zoomInAction, QAction* zoomOutAction, QAction* zoomToFillAction)
{
    d->mZoomToFitAction = zoomToFitAction;
    d->mZoomToFillAction = zoomToFillAction;
    d->mActualSizeAction = actualSizeAction;

    d->mZoomSlider->setZoomInAction(zoomInAction);
    d->mZoomSlider->setZoomOutAction(zoomOutAction);

    QActionGroup *actionGroup = new QActionGroup(d->mZoomComboBox);
    actionGroup->addAction(d->mZoomToFitAction);
    actionGroup->addAction(d->mZoomToFillAction);
    actionGroup->addAction(d->mActualSizeAction);
    actionGroup->setExclusive(true);

    d->mZoomComboBox->addActions(actionGroup->actions());
    d->mZoomComboBox->addItem(zoomToFitAction->iconText());
    d->mZoomComboBox->addItem(zoomToFillAction->iconText());
    d->mZoomComboBox->addItem(actualSizeAction->iconText());
}

void ZoomWidget::setZoom(qreal zoom)
{
    if (d->mZoom == zoom) {
        return;
    }

    d->mZoom = zoom;

    // Don't change slider value if we come here because the slider change,
    // avoids choppy sliding scroll.
    if (!d->mZoomUpdatedBySlider) {
        d->mZoomSlider->setValue(sliderValueForZoom(zoom));
    }

    if (!d->mZoomUpdatedByComboBox) {
        d->mZoomComboBox->setValue(comboBoxValueForZoom(zoom));
    }

    if(d->mZoomUpdatedByComboBox || d->mZoomUpdatedBySlider) {
        Q_EMIT zoomChanged(zoom);
    }
}

void ZoomWidget::setZoomFromSlider()
{
    d->mZoomUpdatedBySlider = true;
    // Use QSlider::sliderPosition(), not QSlider::value() because
    // QSlider::value() has not been updated yet.
    setZoom(zoomForSliderValue(d->mZoomSlider->slider()->sliderPosition()));
    d->mZoomUpdatedBySlider = false;
}

void ZoomWidget::setZoomFromComboBox()
{
    d->mZoomUpdatedByComboBox = true;
    setZoom(zoomForComboBoxValue(d->mZoomComboBox->value()));
    d->mZoomUpdatedByComboBox = false;
}

void ZoomWidget::setMinimumZoom(qreal minimumZoom)
{
    if (d->mMinimumZoom == minimumZoom) {
        return;
    }
    d->mMinimumZoom = minimumZoom;
    d->mZoomSlider->setMinimum(sliderValueForZoom(minimumZoom));
    d->mZoomComboBox->setMinimum(comboBoxValueForZoom(minimumZoom));
    if (minimumZoom > d->mZoom) {
        setZoom(minimumZoom);
    }
}

void ZoomWidget::setMaximumZoom(qreal maximumZoom)
{
    if (d->mMaximumZoom == maximumZoom) {
        return;
    }
    d->mMaximumZoom = maximumZoom;
    d->mZoomSlider->setMaximum(sliderValueForZoom(maximumZoom));
    d->mZoomComboBox->setMaximum(comboBoxValueForZoom(maximumZoom));
    if (maximumZoom < d->mZoom) {
        setZoom(maximumZoom);
    }
}

} // namespace
