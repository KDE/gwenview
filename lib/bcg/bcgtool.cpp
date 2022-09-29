/*
Gwenview: an image viewer
Copyright 2022 Ilya Pominov <ipominov@astralinux.ru>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "bcgtool.h"

// Qt
#include <QAbstractButton>
#include <QDialogButtonBox>
#include <QPainter>

// KF

// Local
#include "bcgimageoperation.h"
#include "bcgwidget.h"
#include <lib/documentview/rasterimageview.h>

namespace Gwenview
{
struct BCGToolPrivate {
    BCGImageOperation::BrightnessContrastGamma getBcg() const
    {
        BCGImageOperation::BrightnessContrastGamma bcg;
        bcg.brightness = mBCGWidget->brightness();
        bcg.contrast = mBCGWidget->contrast();
        bcg.gamma = mBCGWidget->gamma();
        return bcg;
    }

    BCGTool *q = nullptr;
    BCGWidget *mBCGWidget = nullptr;
    QImage mImage;
};

BCGTool::BCGTool(RasterImageView *view)
    : AbstractRasterImageViewTool(view)
    , d(new BCGToolPrivate)
{
    d->q = this;
    d->mBCGWidget = new BCGWidget;
    connect(d->mBCGWidget, &BCGWidget::bcgChanged, this, &BCGTool::slotBCGRequested);
    connect(d->mBCGWidget, &BCGWidget::done, this, [this](bool accept) {
        if (accept) {
            auto op = new BCGImageOperation(d->getBcg());
            Q_EMIT imageOperationRequested(op);
        }
        Q_EMIT done();
    });

    d->mImage = view->document()->image();
}

BCGTool::~BCGTool()
{
    // mBCGWidget is a child of its container not of us, so it is not deleted automatically
    delete d->mBCGWidget;
    delete d;
}

void BCGTool::paint(QPainter *painter)
{
    const QRectF docRect = imageView()->mapToView(QRectF(QPointF(), imageView()->documentSize()));
    painter->eraseRect(docRect);

    painter->drawImage(docRect, d->mImage);
}

void BCGTool::keyPressEvent(QKeyEvent *event)
{
    auto buttons = d->mBCGWidget->findChild<QDialogButtonBox *>();
    switch (event->key()) {
    case Qt::Key_Escape:
        event->accept();
        Q_EMIT buttons->rejected();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter: {
        event->accept();
        auto focusButton = static_cast<QAbstractButton *>(buttons->focusWidget());
        if (focusButton && buttons->buttonRole(focusButton) == QDialogButtonBox::RejectRole) {
            Q_EMIT buttons->rejected();
        } else {
            Q_EMIT buttons->accepted();
        }
        break;
    }
    default:
        break;
    }
}

QWidget *BCGTool::widget() const
{
    return d->mBCGWidget;
}

void BCGTool::slotBCGRequested()
{
    d->mImage = imageView()->document()->image();
    BCGImageOperation::apply(d->mImage, d->getBcg());
    imageView()->update();
}

} // namespace
