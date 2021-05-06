/*
 * SPDX-FileCopyrightText: 2021 Arjen Hiemstra <ahiemstra@heimr.nl>
 * 
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#ifndef BACKGROUNDITEM_H
#define BACKGROUNDITEM_H

#include <QGraphicsItem>

#include <lib/gwenviewlib_export.h>

#include "abstractimageview.h"

namespace Gwenview
{

/**
 * A QGraphicsItem subclass that draws the appropriate background for alpha images.
 */
class GWENVIEWLIB_EXPORT AlphaBackgroundItem : public QGraphicsItem
{
public:
    AlphaBackgroundItem(AbstractImageView* parent);
    ~AlphaBackgroundItem() override;

    AbstractImageView::AlphaBackgroundMode mode() const;
    void setMode(AbstractImageView::AlphaBackgroundMode mode);

    QColor color();
    void setColor(const QColor& color);

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/) override;

    virtual QRectF boundingRect() const override;

private:
    void createCheckBoardTexture();

    AbstractImageView* mParent;
    AbstractImageView::AlphaBackgroundMode mMode = AbstractImageView::AlphaBackgroundCheckBoard;
    QColor mColor = Qt::black;

    // This pixmap will be used to fill the background when mMode is
    // AlphaBackgroundCheckBoard.
    std::unique_ptr<QPixmap> mCheckBoardTexture;
};

}

#endif // BACKGROUNDITEM_H
