// SPDX-FileCopyrightText: 2021 Noah Davis <noahadvs@gmail.com>
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "decoratedtag.h"
#include <KColorUtils>
#include <QEvent>
#include <QPaintEvent>
#include <QStyleOptionFrame>
#include <QStylePainter>
#include <QTextItem>

using namespace Gwenview;

class Gwenview::DecoratedTagPrivate
{
    Q_DECLARE_PUBLIC(DecoratedTag)
public:
    DecoratedTagPrivate(DecoratedTag *q)
        : q_ptr(q)
    {
    }
    DecoratedTag *q_ptr = nullptr;

    void updateMargins();
    qreal horizontalMargin;
    qreal verticalMargin;
};

void DecoratedTagPrivate::updateMargins()
{
    Q_Q(DecoratedTag);
    horizontalMargin = q->fontMetrics().descent() + 2;
    verticalMargin = 2;
    q->setContentsMargins(horizontalMargin, verticalMargin, horizontalMargin, verticalMargin);
}

DecoratedTag::DecoratedTag(QWidget *parent, Qt::WindowFlags f)
    : QLabel(parent, f)
    , d_ptr(new DecoratedTagPrivate(this))
{
    Q_D(DecoratedTag);
    d->updateMargins();
}

DecoratedTag::DecoratedTag(const QString &text, QWidget *parent, Qt::WindowFlags f)
    : QLabel(text, parent, f)
    , d_ptr(new DecoratedTagPrivate(this))
{
    Q_D(DecoratedTag);
    d->updateMargins();
}

Gwenview::DecoratedTag::~DecoratedTag() noexcept = default;

void DecoratedTag::changeEvent(QEvent *event)
{
    Q_D(DecoratedTag);
    if (event->type() == QEvent::FontChange) {
        d->updateMargins();
    }
}

void DecoratedTag::paintEvent(QPaintEvent *event)
{
    Q_D(const DecoratedTag);
    QStylePainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QColor penColor = KColorUtils::mix(palette().base().color(), palette().text().color(), 0.3);
    // QPainter is bad at drawing lines that are exactly 1px.
    // Using QPen::setCosmetic(true) with a 1px pen width
    // doesn't look quite as good as just using 1.001px.
    const qreal penWidth = 1.001;
    const qreal penMargin = penWidth / 2;
    QPen pen(penColor, penWidth);
    pen.setCosmetic(true);
    QRectF rect = event->rect();
    rect.adjust(penMargin, penMargin, -penMargin, -penMargin);
    painter.setBrush(palette().base());
    painter.setPen(pen);
    painter.drawRoundedRect(rect, d->horizontalMargin, d->horizontalMargin);
    QLabel::paintEvent(event);
}
