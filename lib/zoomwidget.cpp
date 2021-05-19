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
// KF

// Local
#include "zoomslider.h"
#include "signalblocker.h"
#include "statusbartoolbutton.h"

namespace Gwenview
{

static const qreal MAGIC_K = 1.04;
static const qreal MAGIC_OFFSET = 16.;
static const qreal PRECISION = 100.;
inline int sliderValueForZoom(qreal zoom)
{
    return int(PRECISION * (log(zoom) / log(MAGIC_K) + MAGIC_OFFSET));
}

inline qreal zoomForSliderValue(int sliderValue)
{
    return pow(MAGIC_K, sliderValue / PRECISION - MAGIC_OFFSET);
}

struct ZoomWidgetPrivate
{
    ZoomWidget* q;

    ZoomSlider* mZoomSlider;
    QComboBox* mZoomComboBox;
    QAction* mZoomToFitAction;
    QAction* mActualSizeAction;
    QAction* mZoomToFillAction;

    QDoubleValidator *mValidator;

    bool mZoomUpdatedBySlider;
//     qreal mMinimumZoom;
//     qreal mMaximumZoom;

    void emitZoomChanged()
    {
        // Use QSlider::sliderPosition(), not QSlider::value() because when we are
        // called from slotZoomSliderActionTriggered(), QSlider::value() has not
        // been updated yet.
        qreal zoom = zoomForSliderValue(mZoomSlider->slider()->sliderPosition());
        mZoomUpdatedBySlider = true;
        emit q->zoomChanged(zoom);
        mZoomUpdatedBySlider = false;
    }
};

ZoomWidget::ZoomWidget(QWidget* parent)
: QFrame(parent)
, d(new ZoomWidgetPrivate)
{
    d->q = this;
    d->mZoomUpdatedBySlider = false;

    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);

    d->mZoomSlider = new ZoomSlider;
    d->mZoomSlider->setMinimumWidth(150);
    d->mZoomSlider->slider()->setSingleStep(int(PRECISION));
    d->mZoomSlider->slider()->setPageStep(3 * int(PRECISION));
    connect(d->mZoomSlider->slider(), &QAbstractSlider::actionTriggered,
            this, &ZoomWidget::slotZoomSliderActionTriggered);

    d->mZoomComboBox = new QComboBox;
    d->mZoomComboBox->setEditable(true);
    d->mValidator = new QDoubleValidator(d->mZoomSlider->slider()->minimum() * PRECISION,
                                         d->mZoomSlider->slider()->maximum() * PRECISION,
                                         2,
                                         d->mZoomComboBox);
    const QDoubleValidator *validator = d->mValidator;
    d->mZoomComboBox->setValidator(validator);
    d->mZoomComboBox->lineEdit()->setAlignment(Qt::AlignCenter);
//     d->mZoomSpinBox->setSuffix(QLatin1String("%"));
    connect(d->mZoomComboBox, &QComboBox::currentTextChanged, this, [this](const QString &text){
        qreal value = text.toDouble();
        if (d->mZoomComboBox->currentIndex() == -1
            && value >= d->mValidator->bottom()
            && value <= d->mValidator->top()) {
            setCustomZoomFromSpinBox(value);
        }
    });
    connect(d->mZoomComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index){
        if (index >= 0 && index < d->mZoomComboBox->count()) {
            d->mZoomComboBox->actions().at(index)->trigger();
        }
    });

    // Layout
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(d->mZoomSlider);
    layout->addWidget(d->mZoomComboBox);
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
    d->mZoomComboBox->addActions({zoomToFitAction, zoomToFillAction, actualSizeAction});
    d->mZoomComboBox->addItem(zoomToFitAction->iconText());
    d->mZoomComboBox->addItem(zoomToFillAction->iconText());
    d->mZoomComboBox->addItem(actualSizeAction->iconText());

    d->mZoomSlider->setZoomInAction(zoomInAction);
    d->mZoomSlider->setZoomOutAction(zoomOutAction);

    QActionGroup *actionGroup = new QActionGroup(d->q);
    actionGroup->addAction(d->mZoomToFitAction);
    actionGroup->addAction(d->mZoomToFillAction);
    actionGroup->addAction(d->mActualSizeAction);
    actionGroup->setExclusive(true);
}

void ZoomWidget::slotZoomSliderActionTriggered()
{
    // The slider value changed because of the user (not because of range
    // changes). In this case disable zoom and apply slider value.
    d->emitZoomChanged();
}

void ZoomWidget::setZoom(qreal zoom)
{
    d->mZoomComboBox->setCurrentText(QVariant(qRound(zoom * PRECISION)).toString());

    // Don't change slider value if we come here because the slider change,
    // avoids choppy sliding scroll.
    if (!d->mZoomUpdatedBySlider) {
        QSlider* slider = d->mZoomSlider->slider();
        SignalBlocker blocker(slider);
        int value = sliderValueForZoom(zoom);

        if (value < slider->minimum()) {
            // It is possible that we are called *before* setMinimumZoom() as
            // been called. In this case, define the minimum ourself.
            d->mZoomSlider->setMinimum(value);
        }
        d->mZoomSlider->setValue(value);
    }
}

void ZoomWidget::setCustomZoomFromSpinBox(qreal zoom)
{
    setZoom(zoom / PRECISION);
    if (d->mZoomComboBox->hasFocus()) {
        d->emitZoomChanged();
    }
}

void ZoomWidget::setMinimumZoom(qreal minimumZoom)
{
    d->mValidator->setBottom(minimumZoom);
    d->mZoomSlider->setMinimum(sliderValueForZoom(minimumZoom));
//     d->mZoomComboBox->setCurrentText(QVariant(qRound(minimumZoom * PRECISION)).toString());
}

void ZoomWidget::setMaximumZoom(qreal zoom)
{
    d->mValidator->setTop(zoom);
    d->mZoomSlider->setMaximum(sliderValueForZoom(zoom));
}

} // namespace
