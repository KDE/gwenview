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
#include "hudmessagebubble.moc"

// Qt
#include <QGraphicsLinearLayout>
#include <QTimeLine>

// KDE
#include <KDebug>
#include <KGuiItem>

// Local
#include <lib/hud/hudbutton.h>
#include <lib/hud/hudcountdown.h>
#include <lib/hud/hudlabel.h>
#include <lib/hud/hudtheme.h>

namespace Gwenview
{

static const int TIMEOUT = 10000;

struct HudMessageBubblePrivate
{
    QGraphicsWidget* mWidget;
    QGraphicsLinearLayout* mLayout;
    HudCountDown* mCountDown;
    HudLabel* mLabel;
};

HudMessageBubble::HudMessageBubble(QGraphicsWidget* parent)
: HudWidget(parent)
, d(new HudMessageBubblePrivate)
{
    d->mWidget = new QGraphicsWidget;
    d->mCountDown = new HudCountDown;
    d->mCountDown->setValue(1);
    d->mLabel = new HudLabel;

    QTimeLine* timeLine = new QTimeLine(TIMEOUT, this);
    connect(timeLine, SIGNAL(valueChanged(qreal)),
            SLOT(slotTimeLineChanged(qreal)));
    connect(timeLine, SIGNAL(finished()),
            SLOT(fadeOut()));
    connect(this, SIGNAL(fadedOut()),
            SLOT(deleteLater()));
    timeLine->start();

    d->mLayout = new QGraphicsLinearLayout(d->mWidget);
    d->mLayout->setContentsMargins(0, 0, 0, 0);
    d->mLayout->addItem(d->mCountDown);
    d->mLayout->addItem(d->mLabel);

    init(d->mWidget, HudWidget::OptionCloseButton);
}

HudMessageBubble::~HudMessageBubble()
{
    delete d;
}

void HudMessageBubble::setText(const QString& text)
{
    d->mLabel->setText(text);
}

HudButton* HudMessageBubble::addButton(const KGuiItem& guiItem)
{
    HudButton* button = new HudButton;
    button->setText(guiItem.text());
    button->setIcon(guiItem.icon());
    d->mLayout->addItem(button);
    return button;
}

void HudMessageBubble::slotTimeLineChanged(qreal value)
{
    d->mCountDown->setValue(1 - value);
}

} // namespace
