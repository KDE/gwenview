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
#include "graphicshudwidget.moc"

// Qt
#include <QGraphicsProxyWidget>
#include <QGraphicsLinearLayout>
#include <QLayout>
#include <QPainter>
#include <QToolButton>

// KDE
#include <kiconloader.h>

// Local
#include "fullscreentheme.h"

namespace Gwenview
{

static const int CORNER_RADIUS = 5;

struct GraphicsHudWidgetPrivate {
    QWidget* mMainWidget;
    QToolButton* mCloseButton;
};

GraphicsHudWidget::GraphicsHudWidget(QGraphicsWidget* parent)
: QGraphicsWidget(parent)
, d(new GraphicsHudWidgetPrivate)
{
    d->mMainWidget = 0;
    d->mCloseButton = 0;
}

GraphicsHudWidget::~GraphicsHudWidget()
{
    delete d;
}

void GraphicsHudWidget::init(QWidget* mainWidget, Options options)
{
    if (options & OptionOpaque) {
        setProperty("opaque", QVariant(true));
    }

    QPalette pal = FullScreenTheme::palette();

    QGraphicsLinearLayout* layout = new QGraphicsLinearLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    d->mMainWidget = mainWidget;
    if (d->mMainWidget) {
        d->mMainWidget->setPalette(pal);
        if (d->mMainWidget->layout()) {
            d->mMainWidget->layout()->setMargin(0);
        }
        QGraphicsProxyWidget* proxy = new QGraphicsProxyWidget(this);
        proxy->setWidget(d->mMainWidget);
        layout->addItem(proxy);
    }
    // FIXME: QGV
    /*
    if (options & OptionDoNotFollowChildSize) {
        adjustSize();
    } else {
        layout->setSizeConstraint(QLayout::SetFixedSize);
    }
    */

    if (options & OptionCloseButton) {
        d->mCloseButton = new QToolButton;
        d->mCloseButton->setAutoRaise(true);
        d->mCloseButton->setIcon(SmallIcon("window-close"));
        d->mCloseButton->setPalette(pal);

        QGraphicsProxyWidget* proxy = new QGraphicsProxyWidget(this);
        proxy->setWidget(d->mCloseButton);
        layout->addItem(proxy);
        layout->setAlignment(proxy, Qt::AlignTop | Qt::AlignHCenter);

        connect(d->mCloseButton, SIGNAL(clicked()), SLOT(slotCloseButtonClicked()));
    }
}

QWidget* GraphicsHudWidget::mainWidget() const
{
    return d->mMainWidget;
}

void GraphicsHudWidget::slotCloseButtonClicked()
{
    close();
    closed();
}

void GraphicsHudWidget::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    QPalette pal = FullScreenTheme::palette();
    painter->setPen(pal.midlight().color());
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(pal.window());
    painter->drawRoundedRect(boundingRect().adjusted(.5, .5, -.5, -.5), CORNER_RADIUS, CORNER_RADIUS);
}

} // namespace
