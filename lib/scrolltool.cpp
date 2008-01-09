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
#include "scrolltool.h"

// Qt
#include <QMouseEvent>
#include <QScrollBar>

// KDE

// Local
#include "imageview.h"

namespace Gwenview {


struct ScrollToolPrivate {
	int mScrollStartX;
	int mScrollStartY;
	bool mDragStarted;
};


ScrollTool::ScrollTool(ImageView* view)
: AbstractImageViewTool(view)
, d(new ScrollToolPrivate) {
	d->mDragStarted = false;
}


ScrollTool::~ScrollTool() {
	delete d;
}


void ScrollTool::mousePressEvent(QMouseEvent* event) {
	if (imageView()->zoomToFit()) {
		return;
	}

	if (event->button() != Qt::LeftButton) {
		return;
	}

	d->mScrollStartY = event->y();
	d->mScrollStartX = event->x();
	d->mDragStarted = true;
	imageView()->viewport()->setCursor(Qt::ClosedHandCursor);
}


void ScrollTool::mouseMoveEvent(QMouseEvent* event) {
	if (!d->mDragStarted) {
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
	if (!d->mDragStarted) {
		return;
	}

	d->mDragStarted = false;
	imageView()->viewport()->setCursor(Qt::OpenHandCursor);
}


void ScrollTool::toolActivated() {
	imageView()->viewport()->setCursor(Qt::OpenHandCursor);
}


void ScrollTool::toolDeactivated() {
	imageView()->viewport()->setCursor(Qt::ArrowCursor);
}


} // namespace
