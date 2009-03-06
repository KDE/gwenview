// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#include "croptool.moc"

// Qt
#include <QFlags>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QRect>
#include <QToolButton>

// KDE
#include <kdebug.h>
#include <kdialog.h>

// Local
#include "cropimageoperation.h"
#include "cropwidget.h"
#include "imageview.h"

static const int HANDLE_RADIUS = 5;

static const int UNINITIALIZED_X = -1;

namespace Gwenview {


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

Q_DECLARE_OPERATORS_FOR_FLAGS(Gwenview::CropHandle)

namespace Gwenview {

struct CropToolPrivate {
	CropTool* mCropTool;
	QRect mRect;
	QList<CropHandle> mCropHandleList;
	CropHandle mMovingHandle;
	QPoint mLastMouseMovePos;
	double mCropRatio;
	QPointer<CropWidget> mCropWidget;


	QRect viewportCropRect() const {
		return mCropTool->imageView()->mapToViewport(mRect.adjusted(0, 0, 1, 1));
	}


	QRect handleViewportRect(CropHandle handle) {
		QRect rect = viewportCropRect();
		int left, top;
		if (handle & CH_Top) {
			top = rect.top() - HANDLE_RADIUS;
		} else if (handle & CH_Bottom) {
			top = rect.bottom() - HANDLE_RADIUS;
		} else {
			top = rect.top() + rect.height() / 2 - HANDLE_RADIUS;
		}

		if (handle & CH_Left) {
			left = rect.left() - HANDLE_RADIUS;
		} else if (handle & CH_Right) {
			left = rect.right() - HANDLE_RADIUS;
		} else {
			left = rect.left() + rect.width() / 2 - HANDLE_RADIUS;
		}

		return QRect(left, top, HANDLE_RADIUS * 2 + 1, HANDLE_RADIUS * 2 + 1);
	}


	CropHandle handleAt(const QPoint& pos) {
		Q_FOREACH(const CropHandle& handle, mCropHandleList) {
			QRect rect = handleViewportRect(handle);
			if (rect.contains(pos)) {
				return handle;
			}
		}
		QRect rect = viewportCropRect();
		if (rect.contains(pos)) {
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

	void keepRectInsideImage() {
		const QSize imageSize = mCropTool->imageView()->document()->size();
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


	void setupHudWidget() {
		ImageView* view = mCropTool->imageView();
		mCropWidget = new CropWidget(view, view, mCropTool);
		mCropWidget->setAttribute(Qt::WA_DeleteOnClose);

		QObject::connect(mCropWidget, SIGNAL(cropRequested()),
			mCropTool, SLOT(slotCropRequested()));
		QObject::connect(mCropWidget, SIGNAL(destroyed()),
			mCropTool, SIGNAL(done()));

		mCropWidget->show();
	}
};


CropTool::CropTool(ImageView* view)
: AbstractImageViewTool(view)
, d(new CropToolPrivate) {
	d->mCropTool = this;
	d->mCropHandleList << CH_Left << CH_Right << CH_Top << CH_Bottom << CH_TopLeft << CH_TopRight << CH_BottomLeft << CH_BottomRight;
	d->mMovingHandle = CH_None;
	d->mRect.setX(UNINITIALIZED_X);
	d->mCropRatio = 0.;

	d->setupHudWidget();
}


CropTool::~CropTool() {
	delete d;
}


void CropTool::setCropRatio(double ratio) {
	d->mCropRatio = ratio;
}


void CropTool::setRect(const QRect& rect) {
	d->mRect = rect;
	d->keepRectInsideImage();
	if (d->mRect != rect) {
		rectUpdated(d->mRect);
	}
	imageView()->viewport()->update();
}


void CropTool::paint(QPainter* painter) {
	if (d->mRect.x() == UNINITIALIZED_X) {
		return;
	}
	QRect rect = d->viewportCropRect();

	QRect imageRect = imageView()->rect();

	QRegion outerRegion = QRegion(imageRect) - QRegion(rect);
	Q_FOREACH(const QRect& outerRect, outerRegion.rects()) {
		painter->fillRect(outerRect, QColor(0, 0, 0, 128));
	}

	painter->setPen(QPen(Qt::black));

	rect.adjust(0, 0, -1, -1);
	painter->drawRect(rect);

	painter->setBrush(Qt::gray);
	painter->setRenderHint(QPainter::Antialiasing);
	Q_FOREACH(const CropHandle& handle, d->mCropHandleList) {
		rect = d->handleViewportRect(handle);
		painter->drawEllipse(rect);
	}
}


void CropTool::mousePressEvent(QMouseEvent* event) {
	d->mCropWidget->setWindowOpacity(0.4);
	if (d->mRect.x() == UNINITIALIZED_X) {
		// Nothing selected, user is creating the crop rect
		QPoint pos = imageView()->mapToImage(event->pos());
		d->mRect = QRect(pos, QSize(0, 0));

		imageView()->viewport()->update();
		rectUpdated(d->mRect);
		return;
	}
	d->mMovingHandle = d->handleAt(event->pos());
	d->updateCursor(d->mMovingHandle, event->buttons() != Qt::NoButton);

	if (d->mMovingHandle == CH_Content) {
		d->mLastMouseMovePos = imageView()->mapToImage(event->pos());
	}
}


void CropTool::mouseMoveEvent(QMouseEvent* event) {
	if (event->buttons() == Qt::NoButton && d->mRect.x() != UNINITIALIZED_X) {
		// Make sure cursor is updated when moving over handles
		CropHandle handle = d->handleAt(event->pos());
		d->updateCursor(handle, false/* buttonDown*/);
		return;
	}

	const QSize imageSize = imageView()->document()->size();

	QPoint point = imageView()->mapToImage(event->pos());
	int posX = qBound(0, point.x(), imageSize.width() - 1);
	int posY = qBound(0, point.y(), imageSize.height() - 1);

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
		d->mMovingHandle = d->mMovingHandle ^ (CH_Top | CH_Bottom);
	}
	if (d->mRect.width() < 0) {
		d->mMovingHandle = d->mMovingHandle ^ (CH_Left | CH_Right);
	}
	d->mRect = d->mRect.normalized();

	// Enforce ratio
	if (d->mCropRatio > 0.) {
		if (d->mMovingHandle == CH_Top || d->mMovingHandle == CH_Bottom) {
			// Top or bottom
			int width = int(d->mRect.height() / d->mCropRatio);
			d->mRect.setWidth(width);
		} else if (d->mMovingHandle == CH_Left || d->mMovingHandle == CH_Right) {
			// Left or right
			int height = int(d->mRect.width() * d->mCropRatio);
			d->mRect.setHeight(height);
		} else if (d->mMovingHandle & CH_Top) {
			// Top left or top right
			int height = int(d->mRect.width() * d->mCropRatio);
			d->mRect.setTop(d->mRect.bottom() - height);
		} else if (d->mMovingHandle & CH_Bottom) {
			// Bottom left or bottom right
			int height = int(d->mRect.width() * d->mCropRatio);
			d->mRect.setHeight(height);
		}
	}

	if (d->mMovingHandle == CH_Content) {
		QPoint delta = point - d->mLastMouseMovePos;
		d->mRect.adjust(delta.x(), delta.y(), delta.x(), delta.y());
		d->mLastMouseMovePos = imageView()->mapToImage(event->pos());
	}

	d->keepRectInsideImage();

	imageView()->viewport()->update();
	rectUpdated(d->mRect);
}


void CropTool::mouseReleaseEvent(QMouseEvent* event) {
	d->mMovingHandle = CH_None;
	d->updateCursor(d->handleAt(event->pos()), false);
	d->mCropWidget->setWindowOpacity(1.);
}


void CropTool::toolActivated() {
	imageView()->viewport()->setCursor(Qt::CrossCursor);
}


void CropTool::toolDeactivated() {
	if (d->mCropWidget) {
		disconnect(d->mCropWidget, 0, this, 0);
		delete d->mCropWidget;
	}
}


void CropTool::slotCropRequested() {
	CropImageOperation* op = new CropImageOperation(d->mRect);
	emit imageOperationRequested(op);
	emit done();
}


} // namespace
