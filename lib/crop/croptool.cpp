// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
#include "croptool.moc"

#include <math.h>

// Qt
#include <QApplication>
#include <QFlags>
#include <QMouseEvent>
#include <QPainter>
#include <QRect>
#include <QTimer>
#include <QToolButton>

// KDE
#include <kdebug.h>
#include <kdialog.h>

// Local
#include "cropimageoperation.h"
#include "cropwidget.h"
#include "imageview.h"
#include "hudwidget.h"

static const int HANDLE_SIZE = 15;

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

enum HudSide {
	HS_None = 0, // Special value used to avoid initial animation
	HS_Top = 1,
	HS_Bottom = 2,
	HS_Inside = 4,
	HS_TopInside = HS_Top | HS_Inside,
	HS_BottomInside = HS_Bottom | HS_Inside
};

static const int HUD_TIMER_MAX_PIXELS_PER_UPDATE = 20;
static const int HUD_TIMER_ANIMATION_INTERVAL = 20;

Q_DECLARE_FLAGS(CropHandle, CropHandleFlag)

} // namespace

Q_DECLARE_OPERATORS_FOR_FLAGS(Gwenview::CropHandle)

namespace Gwenview {

struct CropToolPrivate {
	CropTool* mCropTool;
	HudSide mHudSide;
	QRect mRect;
	QList<CropHandle> mCropHandleList;
	CropHandle mMovingHandle;
	QPoint mLastMouseMovePos;
	double mCropRatio;
	HudWidget* mHudWidget;
	CropWidget* mCropWidget;

	QTimer* mHudTimer;
	QPoint mHudEndPos;


	QRect viewportCropRect() const {
		return mCropTool->imageView()->mapToViewport(mRect.adjusted(0, 0, 1, 1));
	}


	QRect handleViewportRect(CropHandle handle) {
		QRect rect = viewportCropRect();
		int left, top;
		if (handle & CH_Top) {
			top = rect.top();
		} else if (handle & CH_Bottom) {
			top = rect.bottom() - HANDLE_SIZE;
		} else {
			top = rect.top() + (rect.height() - HANDLE_SIZE) / 2;
		}

		if (handle & CH_Left) {
			left = rect.left();
		} else if (handle & CH_Right) {
			left = rect.right() - HANDLE_SIZE;
		} else {
			left = rect.left() + (rect.width() - HANDLE_SIZE) / 2;
		}

		return QRect(left, top, HANDLE_SIZE, HANDLE_SIZE);
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
		mHudSide = HS_None;

		ImageView* view = mCropTool->imageView();
		mHudWidget = new HudWidget(view->viewport());
		QObject::connect(mHudWidget, SIGNAL(closed()),
			mCropTool, SIGNAL(done()));

		mCropWidget = new CropWidget(0, view, mCropTool);
		QObject::connect(mCropWidget, SIGNAL(cropRequested()),
			mCropTool, SLOT(slotCropRequested()));

		mHudWidget->init(mCropWidget, HudWidget::OptionCloseButton);
		mHudWidget->setCursor(Qt::ArrowCursor);
		mHudWidget->show();

		mHudWidget->installEventFilter(mCropTool);

		mHudTimer = new QTimer(mCropTool);
		mHudTimer->setInterval(HUD_TIMER_ANIMATION_INTERVAL);
		QObject::connect(mHudTimer, SIGNAL(timeout()),
			mCropTool, SLOT(moveHudWidget()));
	}

	typedef QPair<QPoint, HudSide> OptimalPosition;

	OptimalPosition computeOptimalHudWidgetPosition() {
		const ImageView* view = mCropTool->imageView();
		const QRect rect = view->mapToViewport(mRect);
		const int margin = HANDLE_SIZE;
		const int hudHeight = mHudWidget->height();
		const QRect hudMaxRect = view->viewport()->rect().adjusted(0, 0, 0, -hudHeight);

		OptimalPosition ret;

		// Compute preferred and fallback positions. Preferred is outside rect
		// on the same side, fallback is outside on the other side.
		OptimalPosition preferred = OptimalPosition(
			QPoint(rect.left(), rect.bottom() + margin),
			HS_Bottom);
		OptimalPosition fallback = OptimalPosition(
			QPoint(rect.left(), rect.top() - margin - hudHeight),
			HS_Top);

		if (mHudSide & HS_Top) {
			qSwap(preferred, fallback);
		}

		// Check if a position outside rect fits
		if (hudMaxRect.contains(preferred.first)) {
			ret = preferred;
		} else if (hudMaxRect.contains(fallback.first)) {
			ret= fallback;
		} else {
			// Does not fit outside, use a position inside rect
			QPoint pos;
			if (mHudSide & HS_Top) {
				pos = QPoint(rect.left() + margin, rect.top() + margin);
			} else {
				pos = QPoint(rect.left() + margin, rect.bottom() - margin - hudHeight);
			}
			ret = OptimalPosition(pos, HudSide(mHudSide | HS_Inside));
		}

		// Ensure it's always fully visible
		ret.first.rx() = qMin(ret.first.rx(), hudMaxRect.width() - mHudWidget->width());
		return ret;
	}


	void updateHudWidgetPosition() {
		OptimalPosition result = computeOptimalHudWidgetPosition();
		if (mHudSide == HS_None) {
			mHudSide = result.second;
		}
		if (mHudSide == result.second && !mHudTimer->isActive()) {
			// Not changing side and not in an animation, move directly the hud
			// to the final position to avoid lagging effect
			mHudWidget->move(result.first);
		} else {
			mHudEndPos = result.first;
			mHudSide = result.second;
			if (!mHudTimer->isActive()) {
				mHudTimer->start();
			}
		}
	}
};


CropTool::CropTool(ImageView* view)
: AbstractImageViewTool(view)
, d(new CropToolPrivate) {
	d->mCropTool = this;
	d->mCropHandleList << CH_Left << CH_Right << CH_Top << CH_Bottom << CH_TopLeft << CH_TopRight << CH_BottomLeft << CH_BottomRight;
	d->mHudSide = HS_Bottom;
	d->mMovingHandle = CH_None;
	const QRect imageRect = QRect(QPoint(0, 0), view->document()->size());
	const QRect viewportRect = view->mapToImage(view->viewport()->rect());
	d->mRect = imageRect & viewportRect;
	d->mCropRatio = 0.;

	d->setupHudWidget();
	d->updateHudWidgetPosition();
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


QRect CropTool::rect() const {
	return d->mRect;
}


void CropTool::paint(QPainter* painter) {
	QRect rect = d->viewportCropRect();

	QRect imageRect = imageView()->rect();

	static const QColor outerColor  = QColor::fromHsvF(0, 0, 0, 0.5);
	static const QColor borderColor = QColor::fromHsvF(0, 0, 1.0, 0.66);
	static const QColor fillColor   = QColor::fromHsvF(0, 0, 0.75, 0.66);

	QRegion outerRegion = QRegion(imageRect) - QRegion(rect);
	Q_FOREACH(const QRect& outerRect, outerRegion.rects()) {
		painter->fillRect(outerRect, outerColor);
	}

	painter->setPen(borderColor);

	rect.adjust(0, 0, -1, -1);
	painter->drawRect(rect);

	if (QApplication::mouseButtons() == Qt::NoButton) {
		// Only draw handles when user is not resizing
		painter->setBrush(fillColor);
		Q_FOREACH(const CropHandle& handle, d->mCropHandleList) {
			rect = d->handleViewportRect(handle);
			painter->drawRect(rect);
		}
	}
}


void CropTool::mousePressEvent(QMouseEvent* event) {
	// FIXME: Fade out?
	//d->mHudWidget->hide();
	d->mMovingHandle = d->handleAt(event->pos());
	d->updateCursor(d->mMovingHandle, event->buttons() != Qt::NoButton);

	if (d->mMovingHandle == CH_Content) {
		d->mLastMouseMovePos = imageView()->mapToImage(event->pos());
	}

	// Update to hide handles
	imageView()->viewport()->update();
}


void CropTool::mouseMoveEvent(QMouseEvent* event) {
	if (event->buttons() == Qt::NoButton) {
		// Make sure cursor is updated when moving over handles
		CropHandle handle = d->handleAt(event->pos());
		d->updateCursor(handle, false/* buttonDown*/);
		return;
	}

	const QSize imageSize = imageView()->document()->size();

	QPoint point = imageView()->mapToImage(event->pos());
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
	d->updateHudWidgetPosition();
}


void CropTool::mouseReleaseEvent(QMouseEvent* event) {
	// FIXME: Fade in?
	//d->mHudWidget->show();
	d->mMovingHandle = CH_None;
	d->updateCursor(d->handleAt(event->pos()), false);

	// Update to show handles
	imageView()->viewport()->update();
}


void CropTool::toolActivated() {
	imageView()->viewport()->setCursor(Qt::CrossCursor);
}


void CropTool::toolDeactivated() {
	delete d->mHudWidget;
}


void CropTool::slotCropRequested() {
	CropImageOperation* op = new CropImageOperation(d->mRect);
	emit imageOperationRequested(op);
	emit done();
}


bool CropTool::eventFilter(QObject*, QEvent* event) {
	if (event->type() == QEvent::Resize) {
		d->updateHudWidgetPosition();
	}
	return false;
}


void CropTool::moveHudWidget() {
	const QPoint delta = d->mHudEndPos - d->mHudWidget->pos();

	const double distance = sqrt(pow(double(delta.x()), 2) + pow(double(delta.y()), 2));
	QPoint pos;
	if (distance > double(HUD_TIMER_MAX_PIXELS_PER_UPDATE)) {
		pos = d->mHudWidget->pos() + delta * double(HUD_TIMER_MAX_PIXELS_PER_UPDATE) / distance;
	} else {
		pos = d->mHudEndPos;
		d->mHudTimer->stop();
	}

	d->mHudWidget->move(pos);
}


} // namespace
