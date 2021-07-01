/*
 * SPDX-FileCopyrightText: 2021 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "alphabackgrounditem.h"

#include <QApplication>
#include <QPainter>

using namespace Gwenview;

AlphaBackgroundItem::AlphaBackgroundItem(AbstractImageView *parent)
    : QGraphicsItem(parent)
    , mParent(parent)
{
}

AlphaBackgroundItem::~AlphaBackgroundItem()
{
}

AbstractImageView::AlphaBackgroundMode AlphaBackgroundItem::mode() const
{
    return mMode;
}

void Gwenview::AlphaBackgroundItem::setMode(AbstractImageView::AlphaBackgroundMode mode)
{
    if (mode == mMode) {
        return;
    }

    mMode = mode;
    update();
}

QColor AlphaBackgroundItem::color()
{
    return mColor;
}

void AlphaBackgroundItem::setColor(const QColor &color)
{
    if (color == mColor) {
        return;
    }

    mColor = color;
    update();
}

void AlphaBackgroundItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    // We need to floor the image size. Unfortunately, QPointF and QSizeF both
    // _round_ when converting instead of flooring. This means that we need to
    // manually do the flooring here, because we otherwise run into pixel
    // alignment issues with the image that is drawn on top of the background.
    const auto width = int((mParent->documentSize().width() / qApp->devicePixelRatio()) * mParent->zoom());
    const auto height = int((mParent->documentSize().height() / qApp->devicePixelRatio()) * mParent->zoom());
    const auto imageRect = QRectF{mParent->imageOffset().toPoint(), QSize{width, height}};

    switch (mMode) {
    case AbstractImageView::AlphaBackgroundNone:
        // No background, do not paint anything.
        break;
    case AbstractImageView::AlphaBackgroundCheckBoard: {
        if (!mCheckBoardTexture) {
            createCheckBoardTexture();
        }

        painter->drawTiledPixmap(imageRect, *mCheckBoardTexture, mParent->scrollPos());
        break;
    }
    case AbstractImageView::AlphaBackgroundSolid:
        painter->fillRect(imageRect, mColor);
        break;
    default:
        break;
    }
}

QRectF AlphaBackgroundItem::boundingRect() const
{
    return mParent->boundingRect();
}

void AlphaBackgroundItem::createCheckBoardTexture()
{
    mCheckBoardTexture = std::make_unique<QPixmap>(32, 32);
    QPainter painter(mCheckBoardTexture.get());
    painter.fillRect(mCheckBoardTexture->rect(), QColor(128, 128, 128));
    const QColor light = QColor(192, 192, 192);
    painter.fillRect(0, 0, 16, 16, light);
    painter.fillRect(16, 16, 16, 16, light);
    // Don't set the pixmap's devicePixelRatio, just let Qt take care of scaling
    // this, otherwise we get some ugly glitches.
}
