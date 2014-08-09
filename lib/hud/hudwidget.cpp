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
#include "hud/hudwidget.moc"

// Qt
#include <QGraphicsProxyWidget>
#include <QGraphicsLinearLayout>
#include <QLayout>
#include <QPainter>
#include <QPropertyAnimation>
#include <QToolButton>

// KDE
#include <QIcon>
#include <KIconLoader>
#include <KLocale>

// Local
#include <hud/hudtheme.h>
#include <hud/hudbutton.h>

namespace Gwenview
{

struct HudWidgetPrivate
{
    HudWidget* q;
    QPropertyAnimation* mAnim;
    QGraphicsWidget* mMainWidget;
    HudButton* mCloseButton;
    bool mAutoDeleteOnFadeout;

    void fadeTo(qreal value)
    {
        if (qFuzzyCompare(q->opacity(), value)) {
            return;
        }
        mAnim->stop();
        mAnim->setStartValue(q->opacity());
        mAnim->setEndValue(value);
        mAnim->start();
    }
};

HudWidget::HudWidget(QGraphicsWidget* parent)
: QGraphicsWidget(parent)
, d(new HudWidgetPrivate)
{
    d->q = this;
    d->mAnim = new QPropertyAnimation(this, "opacity", this);
    d->mMainWidget = 0;
    d->mCloseButton = 0;
    d->mAutoDeleteOnFadeout = false;

    connect(d->mAnim, SIGNAL(finished()), SLOT(slotFadeAnimationFinished()));
}

HudWidget::~HudWidget()
{
    delete d;
}

void HudWidget::init(QWidget* mainWidget, Options options)
{
    QGraphicsProxyWidget* proxy = new QGraphicsProxyWidget(this);
    proxy->setWidget(mainWidget);
    init(proxy, options);
}

void HudWidget::init(QGraphicsWidget* mainWidget, Options options)
{
    if (options & OptionOpaque) {
        setProperty("opaque", QVariant(true));
    }

    QGraphicsLinearLayout* layout = new QGraphicsLinearLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    d->mMainWidget = mainWidget;
    if (d->mMainWidget) {
        if (d->mMainWidget->layout()) {
            d->mMainWidget->layout()->setContentsMargins(0, 0, 0, 0);
        }
        layout->addItem(d->mMainWidget);
    }

    if (options & OptionCloseButton) {
        d->mCloseButton = new HudButton(this);
        d->mCloseButton->setIcon(QIcon::fromTheme("window-close"));
        d->mCloseButton->setToolTip(i18nc("@info:tooltip", "Close"));

        layout->addItem(d->mCloseButton);
        layout->setAlignment(d->mCloseButton, Qt::AlignTop | Qt::AlignHCenter);

        connect(d->mCloseButton, SIGNAL(clicked()), SLOT(slotCloseButtonClicked()));
    }
}

void HudWidget::slotCloseButtonClicked()
{
    close();
    closed();
}

void HudWidget::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    HudTheme::RenderInfo renderInfo = HudTheme::renderInfo(HudTheme::FrameWidget);
    painter->setPen(renderInfo.borderPen);
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(renderInfo.bgBrush);
    painter->drawRoundedRect(boundingRect().adjusted(.5, .5, -.5, -.5), renderInfo.borderRadius, renderInfo.borderRadius);
}

void HudWidget::fadeIn()
{
    d->fadeTo(1.);
}

void HudWidget::fadeOut()
{
    d->fadeTo(0.);
}

void HudWidget::slotFadeAnimationFinished()
{
    if (qFuzzyCompare(opacity(), 1)) {
        fadedIn();
    } else {
        fadedOut();
        if (d->mAutoDeleteOnFadeout) {
            deleteLater();
        }
    }
}

void HudWidget::setAutoDeleteOnFadeout(bool value)
{
    d->mAutoDeleteOnFadeout = value;
}

} // namespace
