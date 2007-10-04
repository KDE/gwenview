// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
#include "croptool.h"

// Qt
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QRect>

// KDE
#include <kdebug.h>

// Local
#include "imageview.h"

static const int HANDLE_RADIUS = 5;

static const int UNINITIALIZED_X = -1;

namespace Gwenview {


enum CropHandle {
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


struct CropToolPrivate {
	CropTool* mCropTool;
	QRect mRect;
	QList<CropHandle> mCropHandleList;
	CropHandle mMovingHandle;
	QPoint mLastMouseMovePos;

	QRect handleViewportRect(CropHandle handle) {
		QRect viewportCropRect = mCropTool->imageView()->mapToViewport(mRect);
		int left, top;
		if (handle & CH_Top) {
			top = viewportCropRect.top() - HANDLE_RADIUS;
		} else if (handle & CH_Bottom) {
			top = viewportCropRect.bottom() - HANDLE_RADIUS;
		} else {
			top = viewportCropRect.top() + viewportCropRect.height() / 2 - HANDLE_RADIUS;
		}

		if (handle & CH_Left) {
			left = viewportCropRect.left() - HANDLE_RADIUS;
		} else if (handle & CH_Right) {
			left = viewportCropRect.right() - HANDLE_RADIUS;
		} else {
			left = viewportCropRect.left() + viewportCropRect.width() / 2 - HANDLE_RADIUS;
		}

		return QRect(left, top, HANDLE_RADIUS * 2 + 1, HANDLE_RADIUS * 2 + 1);
	}


	CropHandle handleAt(const QPoint& pos) {
		Q_FOREACH(CropHandle handle, mCropHandleList) {
			QRect rect = handleViewportRect(handle);
			if (rect.contains(pos)) {
				return handle;
			}
		}
		QRect viewportCropRect = mCropTool->imageView()->mapToViewport(mRect);
		if (viewportCropRect.contains(pos)) {
			return CH_Content;
		}
		return CH_None;
	}

	void updateCursor(CropHandle handle, bool buttonDown) {
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
		mCropTool->imageView()->viewport()->setCursor(shape);
	}
};


CropTool::CropTool(ImageView* view)
: AbstractImageViewTool(view)
, d(new CropToolPrivate) {
	d->mCropTool = this;
	d->mCropHandleList << CH_Left << CH_Right << CH_Top << CH_Bottom << CH_TopLeft << CH_TopRight << CH_BottomLeft << CH_BottomRight;
	d->mMovingHandle = CH_None;
	d->mRect.setX(UNINITIALIZED_X);
}


CropTool::~CropTool() {
	delete d;
}


void CropTool::setRect(const QRect& rect) {
	d->mRect = rect;
	imageView()->viewport()->update();
}


void CropTool::paint(QPainter* painter) {
	if (d->mRect.x() == UNINITIALIZED_X) {
		return;
	}
	QRect rect = imageView()->mapToViewport(d->mRect);

	QRect imageRect = imageView()->rect();

	QRegion outerRegion = QRegion(imageRect) - QRegion(rect);
	Q_FOREACH(QRect outerRect, outerRegion.rects()) {
		painter->fillRect(outerRect, QColor(0, 0, 0, 128));
	}

	painter->setPen(QPen(Qt::black));

	rect.adjust(0, 0, -1, -1);
	painter->drawRect(rect);

	painter->setBrush(Qt::gray);
	painter->setRenderHint(QPainter::Antialiasing);
	Q_FOREACH(CropHandle handle, d->mCropHandleList) {
		rect = d->handleViewportRect(handle);
		painter->drawEllipse(rect);
	}
}


void CropTool::mousePressEvent(QMouseEvent* event) {
	Q_ASSERT(d->mMovingHandle == CH_None);
	d->mMovingHandle = d->handleAt(event->pos());
	d->updateCursor(d->mMovingHandle, event->buttons() != Qt::NoButton);

	if (d->mRect.x() == UNINITIALIZED_X) {
		// Nothing selected, user is creating the crop rect
		QPoint pos = imageView()->mapToImage(event->pos());
		d->mRect = QRect(pos, QSize(0, 0));

		imageView()->viewport()->update();
		rectUpdated(d->mRect);
	}

	if (d->mMovingHandle == CH_Content) {
		d->mLastMouseMovePos = imageView()->mapToImage(event->pos());
	}
}


void CropTool::mouseMoveEvent(QMouseEvent* event) {
	if (event->buttons() == Qt::NoButton) {
		// Make sure cursor is updated when moving over handles
		CropHandle handle = d->handleAt(event->pos());
		d->updateCursor(handle, false/* buttonDown*/);
		return;
	}

	QPoint point = imageView()->mapToImage(event->pos());
	int posX = point.x(), posY = point.y();

	if (d->mRect.x() != UNINITIALIZED_X && d->mRect.size() == QSize(0, 0)) {
		// User is creating rect, thus d->mMovingHandle has not been set yet,
		// figure it out now
		if (posX == d->mRect.x() || posY == d->mRect.y()) {
			// We can't figure the handle yet
			return;
		}
		if (posX < d->mRect.x()) {
			d->mMovingHandle = CH_Left;
		} else {
			d->mMovingHandle = CH_Right;
		}

		if (posY < d->mRect.y()) {
			d->mMovingHandle = CropHandle(d->mMovingHandle | CH_Top);
		} else {
			d->mMovingHandle = CropHandle(d->mMovingHandle | CH_Bottom);
		}

		// Now that we have d->mMovingHandle, we can set the matching cursor shape
		d->updateCursor(d->mMovingHandle, true /*buttonDown*/);
	}

	if (d->mMovingHandle == CH_None) {
		return;
	}

	if (d->mMovingHandle & CH_Top) {
		d->mRect.setTop( qMin(posY, d->mRect.bottom()) );
	} else if (d->mMovingHandle & CH_Bottom) {
		d->mRect.setBottom( qMax(posY, d->mRect.top()) );
	}
	if (d->mMovingHandle & CH_Left) {
		d->mRect.setLeft( qMin(posX, d->mRect.right()) );
	} else if (d->mMovingHandle & CH_Right) {
		d->mRect.setRight( qMax(posX, d->mRect.left()) );
	}
	if (d->mMovingHandle == CH_Content) {
		QPoint delta = point - d->mLastMouseMovePos;
		d->mRect.adjust(delta.x(), delta.y(), delta.x(), delta.y());
		d->mLastMouseMovePos = imageView()->mapToImage(event->pos());
	}

	imageView()->viewport()->update();
	rectUpdated(d->mRect);
}


void CropTool::mouseReleaseEvent(QMouseEvent*) {
	d->mMovingHandle = CH_None;
	d->updateCursor(d->mMovingHandle, false /*buttonDown*/);
}


} // namespace
