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
#include <QGraphicsProxyWidget>
#include <QLabel>
#include <QPainter>
#include <QTimeLine>
#include <QToolButton>

// KDE
#include <KDebug>
#include <KGuiItem>

// Local
#include <lib/fullscreentheme.h>
#include <lib/hud/hudbutton.h>
#include <lib/hud/hudlabel.h>

namespace Gwenview
{

static const int TIMEOUT = 10000;

class CountDownWidget : public QGraphicsWidget
{
public:
    CountDownWidget(QGraphicsWidget* parent = 0)
    : QGraphicsWidget(parent)
    , mValue(0)
    {
        // Use an odd value so that the vertical line is aligned to pixel
        // boundaries
        setMinimumSize(17, 17);
    }

    void setValue(qreal value)
    {
        mValue = value;
        update();
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
    {
        FullScreenTheme::RenderInfo info = FullScreenTheme::renderInfo(FullScreenTheme::CountDownWidget);
        painter->setRenderHint(QPainter::Antialiasing);
        const int circle = 5760;
        const int start = circle / 4; // Start at 12h, not 3h
        const int end = int(circle * mValue);
        painter->setBrush(info.bgBrush);
        painter->setPen(info.borderPen);

        QRectF square = boundingRect().adjusted(.5, .5, -.5, -.5);
        qreal width = square.width();
        qreal height = square.height();
        if (width < height) {
            square.setHeight(width);
            square.moveTop((height - width) / 2);
        } else {
            square.setWidth(height);
            square.moveLeft((width - height) / 2);
        }
        painter->drawPie(square, start, end);
    }

private:
    qreal mValue;
};

struct HudMessageBubblePrivate
{
    QGraphicsWidget* mWidget;
    QGraphicsLinearLayout* mLayout;
    CountDownWidget* mCountDownWidget;
    HudLabel* mLabel;
};

HudMessageBubble::HudMessageBubble(QGraphicsWidget* parent)
: HudWidget(parent)
, d(new HudMessageBubblePrivate)
{
    d->mWidget = new QGraphicsWidget;
    d->mCountDownWidget = new CountDownWidget;
    d->mCountDownWidget->setValue(1);
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
    d->mLayout->addItem(d->mCountDownWidget);
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
    d->mCountDownWidget->setValue(1 - value);
}

} // namespace
