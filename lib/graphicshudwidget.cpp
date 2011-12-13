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
#include <KIcon>
#include <KIconLoader>
#include <KLocale>

// Local
#include <fullscreentheme.h>
#include <graphicshudbutton.h>

namespace Gwenview
{

struct GraphicsHudWidgetPrivate {
    QGraphicsWidget* mMainWidget;
    GraphicsHudButton* mCloseButton;
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
    QGraphicsProxyWidget* proxy = new QGraphicsProxyWidget(this);
    proxy->setWidget(mainWidget);
    init(proxy, options);
}

void GraphicsHudWidget::init(QGraphicsWidget* mainWidget, Options options)
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
        d->mCloseButton = new GraphicsHudButton(this);
        d->mCloseButton->setIcon(KIcon("window-close"));
        d->mCloseButton->setToolTip(i18n("Close"));

        layout->addItem(d->mCloseButton);
        layout->setAlignment(d->mCloseButton, Qt::AlignTop | Qt::AlignHCenter);

        connect(d->mCloseButton, SIGNAL(clicked()), SLOT(slotCloseButtonClicked()));
    }
}

void GraphicsHudWidget::slotCloseButtonClicked()
{
    close();
    closed();
}

void GraphicsHudWidget::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
    FullScreenTheme::RenderInfo renderInfo = FullScreenTheme::renderInfo(FullScreenTheme::FrameWidget);
    painter->setPen(renderInfo.borderPen);
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(renderInfo.bgBrush);
    painter->drawRoundedRect(boundingRect().adjusted(.5, .5, -.5, -.5), renderInfo.borderRadius, renderInfo.borderRadius);
}

} // namespace
