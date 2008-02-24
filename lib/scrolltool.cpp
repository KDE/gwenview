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
#include "scrolltool.moc"

// Qt
#include <QApplication>
#include <QMouseEvent>
#include <QScrollBar>

// KDE
#include <kdebug.h>

// Local
#include "imageview.h"

namespace Gwenview {

enum State { StateNone, StateZooming, StateDragging };

struct ScrollToolPrivate {
	ScrollTool::MouseWheelBehavior mMouseWheelBehavior;
	int mScrollStartX;
	int mScrollStartY;
	State mState;
};


ScrollTool::ScrollTool(ImageView* view)
: AbstractImageViewTool(view)
, d(new ScrollToolPrivate) {
	d->mState = StateNone;
	d->mMouseWheelBehavior = MouseWheelScroll;
}


ScrollTool::~ScrollTool() {
	delete d;
}


void ScrollTool::setMouseWheelBehavior(ScrollTool::MouseWheelBehavior behavior) {
	d->mMouseWheelBehavior = behavior;
}


ScrollTool::MouseWheelBehavior ScrollTool::mouseWheelBehavior() const {
	return d->mMouseWheelBehavior;
}


void ScrollTool::mousePressEvent(QMouseEvent* event) {
	if (event->modifiers() == Qt::ControlModifier) {
		// Ctrl + Left or right button => zoom in or out
		if (event->button() == Qt::LeftButton) {
			emit zoomInRequested(event->pos());
		} else if (event->button() == Qt::RightButton) {
			emit zoomOutRequested(event->pos());
		}
		return;
	}

	if (imageView()->zoomToFit()) {
		return;
	}

	if (event->button() != Qt::LeftButton) {
		return;
	}

	d->mScrollStartY = event->y();
	d->mScrollStartX = event->x();
	d->mState = StateDragging;
	imageView()->viewport()->setCursor(Qt::ClosedHandCursor);
}


void ScrollTool::mouseMoveEvent(QMouseEvent* event) {
	if (d->mState != StateDragging) {
		return;
	}

	int deltaX,deltaY;

	deltaX = d->mScrollStartX - event->x();
	deltaY = d->mScrollStartY - event->y();

	d->mScrollStartX = event->x();
	d->mScrollStartY = event->y();
	imageView()->horizontalScrollBar()->setValue(
		imageView()->horizontalScrollBar()->value() + deltaX);
	imageView()->verticalScrollBar()->setValue(
		imageView()->verticalScrollBar()->value() + deltaY);
}


void ScrollTool::mouseReleaseEvent(QMouseEvent* /*event*/) {
	if (d->mState != StateDragging) {
		return;
	}

	d->mState = StateNone;
	imageView()->viewport()->setCursor(Qt::OpenHandCursor);
}


void ScrollTool::wheelEvent(QWheelEvent* event) {
	if (event->modifiers() & Qt::ControlModifier) {
		// Ctrl + wheel => zoom in or out
		if (event->delta() > 0) {
			emit zoomOutRequested(event->pos());
		} else {
			emit zoomInRequested(event->pos());
		}
		return;
	}

	if (d->mMouseWheelBehavior == MouseWheelScroll) {
		// Forward events to the scrollbars, like
		// QAbstractScrollArea::wheelEvent() do.
		if (event->orientation() == Qt::Horizontal) {
			QApplication::sendEvent(imageView()->horizontalScrollBar(), event);
		} else {
			QApplication::sendEvent(imageView()->verticalScrollBar(), event);
		}
	} else {
		// Browse
		if (event->delta() > 0) {
			emit previousImageRequested();
		} else {
			emit nextImageRequested();
		}
	}
}


void ScrollTool::keyPressEvent(QKeyEvent* event) {
	if (event->modifiers() == Qt::ControlModifier && d->mState == StateNone) {
		d->mState = StateZooming;
		imageView()->viewport()->setCursor(Qt::UpArrowCursor);
	}
}


void ScrollTool::keyReleaseEvent(QKeyEvent* event) {
	if (!(event->modifiers() & Qt::ControlModifier) && d->mState == StateZooming) {
		d->mState = StateNone;
		imageView()->viewport()->setCursor(Qt::ArrowCursor);
	}
}


void ScrollTool::toolActivated() {
	imageView()->viewport()->setCursor(Qt::OpenHandCursor);
	imageView()->viewport()->installEventFilter(this);
}


void ScrollTool::toolDeactivated() {
	imageView()->viewport()->removeEventFilter(this);
	imageView()->viewport()->setCursor(Qt::ArrowCursor);
}


bool ScrollTool::eventFilter(QObject*, QEvent* event) {
	if (event->type() == QEvent::ContextMenu) {
		// Filter out context menu if Ctrl is down to avoid showing it when
		// zooming out with Ctrl + Right button
		QContextMenuEvent* contextMenuEvent = static_cast<QContextMenuEvent*>(event);
		if (contextMenuEvent->modifiers() == Qt::ControlModifier) {
			return true;
		}
	}

	return false;
}


} // namespace
