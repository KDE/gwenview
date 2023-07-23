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
#include <QIcon>
#include <QProxyStyle>
#include <QSlider>
#include <QToolButton>

// KF

// Local

namespace Gwenview
{
class ZoomSliderStyle : public QProxyStyle
{
public:
    explicit ZoomSliderStyle(QObject *parent = nullptr)
        : QProxyStyle()
    {
        setParent(parent);
    }

    int styleHint(QStyle::StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *returnData) const override
    {
        int styleHint = QProxyStyle::styleHint(hint, option, widget, returnData);
        if (hint == QStyle::SH_Slider_AbsoluteSetButtons) {
            styleHint |= Qt::LeftButton;
        }

        return styleHint;
    }
};

struct ZoomSliderPrivate {
    QToolButton *mZoomOutButton = nullptr;
    QToolButton *mZoomInButton = nullptr;
    QSlider *mSlider = nullptr;

    void updateButtons()
    {
        // Use QSlider::sliderPosition(), not QSlider::value() because when we are
        // called from slotZoomSliderActionTriggered(), QSlider::value() has not
        // been updated yet.
        mZoomOutButton->setEnabled(mSlider->sliderPosition() > mSlider->minimum());
        mZoomInButton->setEnabled(mSlider->sliderPosition() < mSlider->maximum());
    }
};

static QToolButton *createZoomButton(const QString &iconName)
{
    auto button = new QToolButton;
    button->setIcon(QIcon::fromTheme(iconName));
    button->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    button->setAutoRaise(true);
    button->setAutoRepeat(true);
    return button;
}

ZoomSlider::ZoomSlider(QWidget *parent)
    : QWidget(parent)
    , d(new ZoomSliderPrivate)
{
    d->mZoomInButton = createZoomButton(QStringLiteral("zoom-in"));
    d->mZoomOutButton = createZoomButton(QStringLiteral("zoom-out"));

    d->mSlider = new QSlider;
    d->mSlider->setOrientation(Qt::Horizontal);
    d->mSlider->setStyle(new ZoomSliderStyle(this));

    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
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

void ZoomSlider::setZoomInAction(QAction *action)
{
    disconnect(d->mZoomInButton, &QToolButton::clicked, this, &ZoomSlider::zoomIn);
    d->mZoomInButton->setDefaultAction(action);
}

void ZoomSlider::setZoomOutAction(QAction *action)
{
    disconnect(d->mZoomOutButton, &QToolButton::clicked, this, &ZoomSlider::zoomOut);
    d->mZoomOutButton->setDefaultAction(action);
}

void ZoomSlider::slotActionTriggered(int)
{
    d->updateButtons();
}

void ZoomSlider::zoomOut()
{
    if (!d->mZoomOutButton->defaultAction()) {
        d->mSlider->triggerAction(QAbstractSlider::SliderPageStepSub);
    }
}

void ZoomSlider::zoomIn()
{
    if (!d->mZoomInButton->defaultAction()) {
        d->mSlider->triggerAction(QAbstractSlider::SliderPageStepAdd);
    }
}

QSlider *ZoomSlider::slider() const
{
    return d->mSlider;
}

} // namespace

#include "moc_zoomslider.cpp"
