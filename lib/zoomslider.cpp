// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
#include "zoomslider.h"

// Qt
#include <QAction>
#include <QHBoxLayout>
#include <QSlider>
#include <QToolButton>
#include <QIcon>

// KDE

// Local

namespace Gwenview
{

struct ZoomSliderPrivate
{
    QToolButton* mZoomOutButton;
    QToolButton* mZoomInButton;
    QSlider* mSlider;
    QAction* mZoomInAction;
    QAction* mZoomOutAction;

    void updateButtons()
    {
        mZoomOutButton->setEnabled(mSlider->value() > mSlider->minimum());
        mZoomInButton->setEnabled(mSlider->value() < mSlider->maximum());
    }
};

static QToolButton* createZoomButton(const char* iconName)
{
    QToolButton* button = new QToolButton;
    button->setIcon(QIcon::fromTheme(iconName));
    button->setAutoRaise(true);
    button->setAutoRepeat(true);
    return button;
}

ZoomSlider::ZoomSlider(QWidget* parent)
: QWidget(parent)
, d(new ZoomSliderPrivate)
{
    d->mZoomInButton = createZoomButton("zoom-in");
    d->mZoomOutButton = createZoomButton("zoom-out");
    d->mZoomInAction = 0;
    d->mZoomOutAction = 0;

    d->mSlider = new QSlider;
    d->mSlider->setOrientation(Qt::Horizontal);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(d->mZoomOutButton);
    layout->addWidget(d->mSlider);
    layout->addWidget(d->mZoomInButton);

    connect(d->mSlider, &QSlider::actionTriggered, this, &ZoomSlider::slotActionTriggered);
    connect(d->mSlider, &QSlider::valueChanged, this, &ZoomSlider::valueChanged);

    connect(d->mZoomOutButton, &QToolButton::clicked, this, &ZoomSlider::zoomOut);
    connect(d->mZoomInButton, &QToolButton::clicked, this, &ZoomSlider::zoomIn);
}

ZoomSlider::~ZoomSlider()
{
    delete d;
}

int ZoomSlider::value() const
{
    return d->mSlider->value();
}

void ZoomSlider::setValue(int value)
{
    d->mSlider->setValue(value);
    d->updateButtons();
}

void ZoomSlider::setMinimum(int value)
{
    d->mSlider->setMinimum(value);
    d->updateButtons();
}

void ZoomSlider::setMaximum(int value)
{
    d->mSlider->setMaximum(value);
    d->updateButtons();
}

void ZoomSlider::setZoomInAction(QAction* action)
{
    d->mZoomInAction = action;
}

void ZoomSlider::setZoomOutAction(QAction* action)
{
    d->mZoomOutAction = action;
}

void ZoomSlider::slotActionTriggered(int)
{
    d->updateButtons();
}

void ZoomSlider::zoomOut()
{
    if (d->mZoomOutAction) {
        d->mZoomOutAction->trigger();
    } else {
        d->mSlider->triggerAction(QAbstractSlider::SliderPageStepSub);
    }
}

void ZoomSlider::zoomIn()
{
    if (d->mZoomInAction) {
        d->mZoomInAction->trigger();
    } else {
        d->mSlider->triggerAction(QAbstractSlider::SliderPageStepAdd);
    }
}

QSlider* ZoomSlider::slider() const
{
    return d->mSlider;
}

} // namespace
