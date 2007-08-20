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
	CH_BottomRight = CH_Bottom | CH_Right
};


struct CropToolPrivate {
	CropTool* mCropTool;
	QRect mRect;
	QList<CropHandle> mCropHandleList;
	CropHandle mMovingHandle;

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
};


CropTool::CropTool(ImageView* view)
: AbstractImageViewTool(view)
, d(new CropToolPrivate) {
	d->mCropTool = this;
	d->mCropHandleList << CH_Left << CH_Right << CH_Top << CH_Bottom << CH_TopLeft << CH_TopRight << CH_BottomLeft << CH_BottomRight;
	d->mMovingHandle = CH_None;
}


CropTool::~CropTool() {
	delete d;
}


void CropTool::setRect(const QRect& rect) {
	d->mRect = rect;
	imageView()->viewport()->update();
}


void CropTool::paint(QPainter* painter) {
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
	Q_FOREACH(CropHandle handle, d->mCropHandleList) {
		QRect rect = d->handleViewportRect(handle);
		if (rect.contains(event->pos())) {
			d->mMovingHandle = handle;
			break;
		}
	}

	if (d->mMovingHandle == CH_None) {
		return;
	}
}


void CropTool::mouseMoveEvent(QMouseEvent* event) {
	if (d->mMovingHandle == CH_None) {
		return;
	}

	QPoint point = imageView()->mapToImage(event->pos());
	int posX = point.x(), posY = point.y();
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

	imageView()->viewport()->update();
	rectUpdated(d->mRect);
}


void CropTool::mouseReleaseEvent(QMouseEvent*) {
	if (d->mMovingHandle == CH_None) {
		return;
	}
	d->mMovingHandle = CH_None;
}


} // namespace
