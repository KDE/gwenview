// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
#include "croptool.h"

// Qt
#include <QtMath>
#include <QDialogButtonBox>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QRect>
#include <QDebug>

// KDE

// Local
#include <lib/documentview/rasterimageview.h>
#include "cropimageoperation.h"
#include "cropwidget.h"
#include "gwenviewconfig.h"

static const int HANDLE_SIZE = 15;

namespace Gwenview
{

enum CropHandleFlag {
    CH_None,
    CH_Top = 1,
    CH_Left = 2,
    CH_Right = 4,
    CH_Bottom = 8,
    CH_TopLeft = CH_Top | CH_Left,
    CH_BottomLeft = CH_Bottom | CH_Left,
    CH_TopRight = CH_Top | CH_Right,
    CH_BottomRight = CH_Bottom | CH_Right,
    CH_Content = 16
};

Q_DECLARE_FLAGS(CropHandle, CropHandleFlag)

} // namespace

inline QPoint boundPointX(const QPoint& point, const QRect& rect)
{
    return QPoint(
               qBound(rect.left(), point.x(), rect.right()),
               point.y()
           );
}

inline QPoint boundPointXY(const QPoint& point, const QRect& rect)
{
    return QPoint(
               qBound(rect.left(), point.x(), rect.right()),
               qBound(rect.top(),  point.y(), rect.bottom())
           );
}

Q_DECLARE_OPERATORS_FOR_FLAGS(Gwenview::CropHandle)

namespace Gwenview
{

struct CropToolPrivate
{
    CropTool* q;
    QRect mRect;
    QList<CropHandle> mCropHandleList;
    CropHandle mMovingHandle;
    QPoint mLastMouseMovePos;
    double mCropRatio;
    double mLockedCropRatio;
    CropWidget* mCropWidget;

    QRect viewportCropRect() const
    {
        return q->imageView()->mapToView(mRect);
    }

    QRect handleViewportRect(CropHandle handle)
    {
        QSize viewportSize = q->imageView()->size().toSize();
        QRect rect = viewportCropRect();
        int left, top;
        if (handle & CH_Top) {
            top = rect.top();
        } else if (handle & CH_Bottom) {
            top = rect.bottom() + 1 - HANDLE_SIZE;
        } else {
            top = rect.top() + (rect.height() - HANDLE_SIZE) / 2;
            top = qBound(0, top, viewportSize.height() - HANDLE_SIZE);
        }

        if (handle & CH_Left) {
            left = rect.left();
        } else if (handle & CH_Right) {
            left = rect.right() + 1 - HANDLE_SIZE;
        } else {
            left = rect.left() + (rect.width() - HANDLE_SIZE) / 2;
            left = qBound(0, left, viewportSize.width() - HANDLE_SIZE);
        }

        return QRect(left, top, HANDLE_SIZE, HANDLE_SIZE);
    }

    CropHandle handleAt(const QPointF& pos)
    {
        Q_FOREACH(const CropHandle & handle, mCropHandleList) {
            QRectF rect = handleViewportRect(handle);
            if (rect.contains(pos)) {
                return handle;
            }
        }
        QRectF rect = viewportCropRect();
        if (rect.contains(pos)) {
            return CH_Content;
        }
        return CH_None;
    }

    void updateCursor(CropHandle handle, bool buttonDown)
    {
        Qt::CursorShape shape;
        switch (handle) {
        case CH_TopLeft:
        case CH_BottomRight:
            shape = Qt::SizeFDiagCursor;
            break;

        case CH_TopRight:
        case CH_BottomLeft:
            shape = Qt::SizeBDiagCursor;
            break;

        case CH_Left:
        case CH_Right:
            shape = Qt::SizeHorCursor;
            break;

        case CH_Top:
        case CH_Bottom:
            shape = Qt::SizeVerCursor;
            break;

        case CH_Content:
            shape = buttonDown ? Qt::ClosedHandCursor : Qt::OpenHandCursor;
            break;

        default:
            shape = Qt::ArrowCursor;
            break;
        }
        q->imageView()->setCursor(shape);
    }

    void keepRectInsideImage()
    {
        const QSize imageSize = q->imageView()->documentSize().toSize();
        if (mRect.width() > imageSize.width() || mRect.height() > imageSize.height()) {
            // This can happen when the crop ratio changes
            QSize rectSize = mRect.size();
            rectSize.scale(imageSize, Qt::KeepAspectRatio);
            mRect.setSize(rectSize);
        }

        if (mRect.right() >= imageSize.width()) {
            mRect.moveRight(imageSize.width() - 1);
        } else if (mRect.left() < 0) {
            mRect.moveLeft(0);
        }
        if (mRect.bottom() >= imageSize.height()) {
            mRect.moveBottom(imageSize.height() - 1);
        } else if (mRect.top() < 0) {
            mRect.moveTop(0);
        }
    }

    void setupWidget()
    {
        RasterImageView* view = q->imageView();
        mCropWidget = new CropWidget(nullptr, view, q);
        QObject::connect(mCropWidget, SIGNAL(cropRequested()),
                         q, SLOT(slotCropRequested()));
        QObject::connect(mCropWidget, SIGNAL(done()),
                         q, SIGNAL(done()));

        // This is needed when crop ratio set to Current Image, and the image is rotated
        QObject::connect(view, &RasterImageView::imageRectUpdated, mCropWidget, &CropWidget::updateCropRatio);
    }

    QRect computeVisibleImageRect() const
    {
        RasterImageView* view = q->imageView();
        const QRect imageRect = QRect(QPoint(0, 0), view->documentSize().toSize());
        const QRect viewportRect = view->mapToImage(view->rect().toRect());
        return imageRect & viewportRect;
    }
};

CropTool::CropTool(RasterImageView* view)
: AbstractRasterImageViewTool(view)
, d(new CropToolPrivate)
{
    d->q = this;
    d->mCropHandleList << CH_Left << CH_Right << CH_Top << CH_Bottom << CH_TopLeft << CH_TopRight << CH_BottomLeft << CH_BottomRight;
    d->mMovingHandle = CH_None;
    d->mCropRatio = 0.;
    d->mLockedCropRatio = 0.;
    d->mRect = d->computeVisibleImageRect();
    d->setupWidget();
}

CropTool::~CropTool()
{
    // mCropWidget is a child of its container not of us, so it is not deleted automatically
    delete d->mCropWidget;
    delete d;
}

void CropTool::setCropRatio(double ratio)
{
    d->mCropRatio = ratio;
}

void CropTool::setRect(const QRect& rect)
{
    QRect oldRect = d->mRect;
    d->mRect = rect;
    d->keepRectInsideImage();
    if (d->mRect != oldRect) {
        rectUpdated(d->mRect);
    }
    imageView()->update();
}

QRect CropTool::rect() const
{
    return d->mRect;
}

void CropTool::paint(QPainter* painter)
{
    QRect rect = d->viewportCropRect();

    QRect imageRect = imageView()->rect().toRect();

    static const QColor outerColor  = QColor::fromHsvF(0, 0, 0, 0.5);
    // For some reason nothing gets drawn if borderColor is not fully opaque!
    //static const QColor borderColor = QColor::fromHsvF(0, 0, 1.0, 0.66);
    static const QColor borderColor = QColor::fromHsvF(0, 0, 1.0);
    static const QColor fillColor   = QColor::fromHsvF(0, 0, 0.75, 0.66);

    QRegion outerRegion = QRegion(imageRect) - QRegion(rect);
    Q_FOREACH(const QRect & outerRect, outerRegion.rects()) {
        painter->fillRect(outerRect, outerColor);
    }

    painter->setPen(borderColor);

    painter->drawRect(rect);

    if (d->mMovingHandle == CH_None) {
        // Only draw handles when user is not resizing
        painter->setBrush(fillColor);
        Q_FOREACH(const CropHandle & handle, d->mCropHandleList) {
            rect = d->handleViewportRect(handle);
            painter->drawRect(rect);
        }
    }
}

void CropTool::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    event->accept();
    if (event->buttons() != Qt::LeftButton) {
        return;
    }
    d->mMovingHandle = d->handleAt(event->pos());
    d->updateCursor(d->mMovingHandle, true /* down */);

    if (d->mMovingHandle == CH_Content) {
        d->mLastMouseMovePos = imageView()->mapToImage(event->pos().toPoint());
    }

    // Update to hide handles
    imageView()->update();
}

void CropTool::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    event->accept();
    if (event->buttons() != Qt::LeftButton) {
        return;
    }

    const QSize imageSize = imageView()->document()->size();

    QPoint point = imageView()->mapToImage(event->pos().toPoint());
    int posX = qBound(0, point.x(), imageSize.width() - 1);
    int posY = qBound(0, point.y(), imageSize.height() - 1);

    if (d->mMovingHandle == CH_None) {
        return;
    }

    // Adjust edge
    if (d->mMovingHandle & CH_Top) {
        d->mRect.setTop(posY);
    } else if (d->mMovingHandle & CH_Bottom) {
        d->mRect.setBottom(posY);
    }
    if (d->mMovingHandle & CH_Left) {
        d->mRect.setLeft(posX);
    } else if (d->mMovingHandle & CH_Right) {
        d->mRect.setRight(posX);
    }

    // Normalize rect and handles (this is useful when user drag the right side
    // of the crop rect to the left of the left side)
    if (d->mRect.height() < 0) {
        d->mMovingHandle = d->mMovingHandle ^(CH_Top | CH_Bottom);
    }
    if (d->mRect.width() < 0) {
        d->mMovingHandle = d->mMovingHandle ^(CH_Left | CH_Right);
    }
    d->mRect = d->mRect.normalized();

    // Enforce ratio:
    double ratioToEnforce = d->mCropRatio;
    //  - if user is holding down Ctrl/Shift when resizing rect, lock to current rect ratio
    if (event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier) && d->mLockedCropRatio > 0) {
        ratioToEnforce = d->mLockedCropRatio;
    }
    //  - if user has restricted the ratio via the GUI
    if (ratioToEnforce > 0.) {
        if (d->mMovingHandle == CH_Top || d->mMovingHandle == CH_Bottom) {
            // Top or bottom
            int width = int(d->mRect.height() / ratioToEnforce);
            d->mRect.setWidth(width);
        } else if (d->mMovingHandle == CH_Left || d->mMovingHandle == CH_Right) {
            // Left or right
            int height = int(d->mRect.width() * ratioToEnforce);
            d->mRect.setHeight(height);
        } else if (d->mMovingHandle & CH_Top) {
            // Top left or top right
            int height = int(d->mRect.width() * ratioToEnforce);
            d->mRect.setTop(d->mRect.bottom() - height);
        } else if (d->mMovingHandle & CH_Bottom) {
            // Bottom left or bottom right
            int height = int(d->mRect.width() * ratioToEnforce);
            d->mRect.setHeight(height);
        }
    }

    if (d->mMovingHandle == CH_Content) {
        d->mRect.translate(point - d->mLastMouseMovePos);
        d->mLastMouseMovePos = point;
    }

    d->keepRectInsideImage();

    imageView()->update();
    rectUpdated(d->mRect);
}

void CropTool::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    event->accept();
    d->mMovingHandle = CH_None;
    d->updateCursor(d->handleAt(event->lastPos()), false);

    // Update to show handles
    imageView()->update();
}

void CropTool::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    event->accept();
    // Make sure cursor is updated when moving over handles
    CropHandle handle = d->handleAt(event->lastPos());
    d->updateCursor(handle, false /* buttonDown */);
}

void CropTool::keyPressEvent(QKeyEvent* event)
{
    // Lock crop ratio to current rect when user presses Control or Shift
    if (event->key() == Qt::Key_Control || event->key() == Qt::Key_Shift) {
        d->mLockedCropRatio = 1. * d->mRect.height() / d->mRect.width();
    }

    QDialogButtonBox *buttons = d->mCropWidget->findChild<QDialogButtonBox *>();
    switch (event->key()) {
    case Qt::Key_Escape:
        event->accept();
        buttons->rejected();
        break;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        event->accept();
        buttons->accepted();
        break;
    default:
        break;
    }
}

void CropTool::toolActivated()
{
    imageView()->setCursor(Qt::CrossCursor);
    d->mCropWidget->setAdvancedSettingsEnabled(GwenviewConfig::cropAdvancedSettingsEnabled());
    d->mCropWidget->setPreserveAspectRatio(GwenviewConfig::cropPreserveAspectRatio());
    const int index = GwenviewConfig::cropRatioIndex();
    if (index >= 0) {
        // Preset ratio
        d->mCropWidget->setCropRatioIndex(index);
    } else {
        // Must be a custom ratio, or blank
        const QSizeF ratio = QSizeF(GwenviewConfig::cropRatioWidth(), GwenviewConfig::cropRatioHeight());
        d->mCropWidget->setCropRatio(ratio);
    }
}

void CropTool::toolDeactivated()
{
    GwenviewConfig::setCropAdvancedSettingsEnabled(d->mCropWidget->advancedSettingsEnabled());
    GwenviewConfig::setCropPreserveAspectRatio(d->mCropWidget->preserveAspectRatio());
    GwenviewConfig::setCropRatioIndex(d->mCropWidget->cropRatioIndex());
    const QSizeF ratio = d->mCropWidget->cropRatio();
    GwenviewConfig::setCropRatioWidth(ratio.width());
    GwenviewConfig::setCropRatioHeight(ratio.height());
}

void CropTool::slotCropRequested()
{
    CropImageOperation* op = new CropImageOperation(d->mRect);
    emit imageOperationRequested(op);
    emit done();
}

QWidget* CropTool::widget() const
{
    return d->mCropWidget;
}

} // namespace
