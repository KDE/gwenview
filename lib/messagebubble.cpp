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
#include "messagebubble.moc"

// Qt
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QTimeLine>
#include <QToolButton>

// KDE
#include <kdebug.h>
#include <kguiitem.h>

// Local

namespace Gwenview
{

static const int TIMEOUT = 10000;

class PieWidget : public QWidget
{
public:
    PieWidget(QWidget* parent = 0)
        : QWidget(parent)
        , mValue(0) {
        setMinimumSize(16, 16);
    }

    void setValue(qreal value)
    {
        mValue = value;
        update();
    }

    int heightForWidth(int width) const {
        return width;
    }

protected:
    void paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const int circle = 5760;
        const int start = circle / 4; // Start at 12h, not 3h
        const int end = int(circle * mValue);
        painter.setBrush(palette().dark());
        painter.setPen(palette().light().color());

        QRectF square = QRectF(rect()).adjusted(.5, .5, -.5, -.5);
        qreal width = square.width();
        qreal height = square.height();
        if (width < height) {
            square.setHeight(width);
            square.moveTop((height - width) / 2);
        } else {
            square.setWidth(height);
            square.moveLeft((width - height) / 2);
        }
        painter.drawPie(square, start, end);
    }

private:
    qreal mValue;
};

struct MessageBubblePrivate {
    PieWidget* mCountDownWidget;
    QWidget* mWidget;
    QLabel* mLabel;
};

MessageBubble::MessageBubble(QWidget* parent)
: HudWidget(parent)
, d(new MessageBubblePrivate)
{
    d->mWidget = new QWidget;
    d->mCountDownWidget = new PieWidget;
    d->mCountDownWidget->setValue(1);
    d->mLabel = new QLabel;

    QTimeLine* timeLine = new QTimeLine(TIMEOUT, this);
    connect(timeLine, SIGNAL(valueChanged(qreal)),
            SLOT(slotTimeLineChanged(qreal)));
    connect(timeLine, SIGNAL(finished()),
            SLOT(deleteLater()));
    timeLine->start();

    QHBoxLayout* layout = new QHBoxLayout(d->mWidget);
    layout->setMargin(0);
    layout->addWidget(d->mCountDownWidget);
    layout->addWidget(d->mLabel);

    init(d->mWidget, HudWidget::OptionCloseButton);
}

MessageBubble::~MessageBubble()
{
    delete d;
}

void MessageBubble::setText(const QString& text)
{
    d->mLabel->setText(text);
    adjustSize();
}

QToolButton* MessageBubble::addButton(const KGuiItem& guiItem)
{
    QToolButton* button = new QToolButton;
    button->setText(guiItem.text());
    button->setIcon(guiItem.icon());
    button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    d->mWidget->layout()->addWidget(button);
    adjustSize();
    return button;
}

void MessageBubble::slotTimeLineChanged(qreal value)
{
    d->mCountDownWidget->setValue(1 - value);
}

} // namespace
