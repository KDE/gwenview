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
#include <QLabel>
#include <QHBoxLayout>
#include <QSlider>

// KDE

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

    StatusBarToolButton* mZoomToFitButton;
    StatusBarToolButton* mActualSizeButton;
    StatusBarToolButton* mZoomToFitWidthButton;
    QLabel* mZoomLabel;
    ZoomSlider* mZoomSlider;
    QAction* mZoomToFitAction;
    QAction* mActualSizeAction;
    QAction* mZoomToFitWidthAction;

    bool mZoomUpdatedBySlider;

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

    d->mZoomToFitButton = new StatusBarToolButton;
    d->mActualSizeButton = new StatusBarToolButton;
    d->mZoomToFitWidthButton = new StatusBarToolButton;
    d->mZoomToFitButton->setCheckable(true);
    d->mActualSizeButton->setCheckable(true);
    d->mZoomToFitWidthButton->setCheckable(true);
    d->mZoomToFitButton->setChecked(true);

    if (QApplication::isLeftToRight()) {
        d->mZoomToFitButton->setGroupPosition(StatusBarToolButton::GroupLeft);
        d->mZoomToFitWidthButton->setGroupPosition(StatusBarToolButton::GroupCenter);
        d->mActualSizeButton->setGroupPosition(StatusBarToolButton::GroupRight);
    } else {
        d->mActualSizeButton->setGroupPosition(StatusBarToolButton::GroupLeft);
        d->mZoomToFitWidthButton->setGroupPosition(StatusBarToolButton::GroupCenter);
        d->mZoomToFitButton->setGroupPosition(StatusBarToolButton::GroupRight);
    }

    d->mZoomLabel = new QLabel;
    d->mZoomLabel->setFixedWidth(d->mZoomLabel->fontMetrics().width(" 1000% "));
    d->mZoomLabel->setAlignment(Qt::AlignCenter);

    d->mZoomSlider = new ZoomSlider;
    d->mZoomSlider->setMinimumWidth(150);
    d->mZoomSlider->slider()->setSingleStep(int(PRECISION));
    d->mZoomSlider->slider()->setPageStep(3 * int(PRECISION));
    connect(d->mZoomSlider->slider(), SIGNAL(actionTriggered(int)), SLOT(slotZoomSliderActionTriggered()));

    // Layout
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(d->mZoomToFitButton);
    layout->addWidget(d->mZoomToFitWidthButton);
    layout->addWidget(d->mActualSizeButton);
    layout->addWidget(d->mZoomSlider);
    layout->addWidget(d->mZoomLabel);
}

ZoomWidget::~ZoomWidget()
{
    delete d;
}

void ZoomWidget::setActions(QAction* zoomToFitAction, QAction* actualSizeAction, QAction* zoomInAction, QAction* zoomOutAction, QAction* zoomToFitWidthAction)
{
    d->mZoomToFitAction = zoomToFitAction;
    d->mActualSizeAction = actualSizeAction;
    d->mZoomToFitWidthAction = zoomToFitWidthAction;

    d->mZoomToFitButton->setDefaultAction(zoomToFitAction);
    d->mActualSizeButton->setDefaultAction(actualSizeAction);
    d->mZoomToFitWidthButton->setDefaultAction(zoomToFitWidthAction);

    d->mZoomSlider->setZoomInAction(zoomInAction);
    d->mZoomSlider->setZoomOutAction(zoomOutAction);

    QActionGroup *actionGroup = new QActionGroup(d->q);
    actionGroup->addAction(d->mZoomToFitAction);
    actionGroup->addAction(d->mZoomToFitWidthAction);
    actionGroup->addAction(d->mActualSizeAction);
    actionGroup->setExclusive(true);

    // Adjust sizes
    int width = qMax(d->mZoomToFitButton->sizeHint().width(), d->mActualSizeButton->sizeHint().width());
    d->mZoomToFitButton->setFixedWidth(width);
    d->mActualSizeButton->setFixedWidth(width);
}

void ZoomWidget::slotZoomSliderActionTriggered()
{
    // The slider value changed because of the user (not because of range
    // changes). In this case disable zoom and apply slider value.
    d->emitZoomChanged();
}

void ZoomWidget::setZoom(qreal zoom)
{
    int intZoom = qRound(zoom * 100);
    d->mZoomLabel->setText(QString("%1%").arg(intZoom));

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

void ZoomWidget::setMinimumZoom(qreal minimumZoom)
{
    d->mZoomSlider->setMinimum(sliderValueForZoom(minimumZoom));
}

void ZoomWidget::setMaximumZoom(qreal zoom)
{
    d->mZoomSlider->setMaximum(sliderValueForZoom(zoom));
}

} // namespace
