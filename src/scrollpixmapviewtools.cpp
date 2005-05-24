// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aurélien Gâteau

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// Our header
#include "scrollpixmapviewtools.h"

// KDE 
#include <kaction.h>
#include <kdebug.h>
#include <kstandarddirs.h>


// Helper function
static QCursor loadCursor(const QString& name) {
	QString path;
	path=locate("data", QString("gwenview/cursors/%1.png").arg(name));
	return QCursor(QPixmap(path));
}


//----------------------------------------------------------------------------
//
// ScrollPixmapView::ToolBase
//
//----------------------------------------------------------------------------
ScrollPixmapView::ToolBase::ToolBase(ScrollPixmapView* view)
: mView(view) {}


ScrollPixmapView::ToolBase::~ToolBase() {}

void ScrollPixmapView::ToolBase::mouseMoveEvent(QMouseEvent*) {}
void ScrollPixmapView::ToolBase::leftButtonPressEvent(QMouseEvent*) {}
void ScrollPixmapView::ToolBase::leftButtonReleaseEvent(QMouseEvent*) {}

void ScrollPixmapView::ToolBase::midButtonReleaseEvent(QMouseEvent*) {
	mView->autoZoom()->activate();
}

void ScrollPixmapView::ToolBase::rightButtonPressEvent(QMouseEvent*) {}
void ScrollPixmapView::ToolBase::rightButtonReleaseEvent(QMouseEvent* event) {
	mView->openContextMenu(event->globalPos());
}

void ScrollPixmapView::ToolBase::wheelEvent(QWheelEvent* event) {
	event->accept();
}

void ScrollPixmapView::ToolBase::updateCursor() {
	mView->viewport()->setCursor(ArrowCursor);
}


//----------------------------------------------------------------------------
//
// ScrollPixmapView::ZoomTool
//
//----------------------------------------------------------------------------
ScrollPixmapView::ZoomTool::ZoomTool(ScrollPixmapView* view)
: ScrollPixmapView::ToolBase(view) {
	mZoomCursor=loadCursor("zoom");
}


void ScrollPixmapView::ZoomTool::zoomTo(const QPoint& pos, bool in) {
	KAction* zoomAction=in?mView->zoomIn():mView->zoomOut();
	if (!zoomAction->isEnabled()) return;

	if (mView->autoZoom()->isChecked()) {
		mView->autoZoom()->setChecked(false);
		mView->updateScrollBarMode();
	}
	QPoint centerPos=QPoint(mView->visibleWidth(), mView->visibleHeight())/2;
	// Compute image position
	QPoint imgPos=mView->viewportToContents(pos) - mView->offset();
	double newZoom=mView->computeZoom(in);

	imgPos*=newZoom/mView->zoom();
	imgPos=imgPos-pos+centerPos;
	mView->setZoom(newZoom, imgPos.x(), imgPos.y());
}


void ScrollPixmapView::ZoomTool::leftButtonReleaseEvent(QMouseEvent* event) {
	zoomTo(event->pos(), true);
}


void ScrollPixmapView::ZoomTool::wheelEvent(QWheelEvent* event) {
	zoomTo(event->pos(), event->delta()>0);
	event->accept();
}


void ScrollPixmapView::ZoomTool::rightButtonPressEvent(QMouseEvent*) {
}


void ScrollPixmapView::ZoomTool::rightButtonReleaseEvent(QMouseEvent* event) {
	zoomTo(event->pos(), false);
}


void ScrollPixmapView::ZoomTool::updateCursor() {
	mView->viewport()->setCursor(mZoomCursor);
}


//----------------------------------------------------------------------------
//
// ScrollPixmapView::ScrollTool
//
//----------------------------------------------------------------------------
ScrollPixmapView::ScrollTool::ScrollTool(ScrollPixmapView* view)
: ScrollPixmapView::ToolBase(view)
, mScrollStartX(0), mScrollStartY(0)
, mDragStarted(false) {
	mDragCursor=loadCursor("drag");
	mDraggingCursor=loadCursor("dragging");
}


void ScrollPixmapView::ScrollTool::leftButtonPressEvent(QMouseEvent* event) {
	mScrollStartX=event->x();
	mScrollStartY=event->y();
	mView->viewport()->setCursor(mDraggingCursor);
	mDragStarted=true;
}


void ScrollPixmapView::ScrollTool::mouseMoveEvent(QMouseEvent* event) {
	if (!mDragStarted) return;

	int deltaX,deltaY;

	deltaX=mScrollStartX - event->x();
	deltaY=mScrollStartY - event->y();

	mScrollStartX=event->x();
	mScrollStartY=event->y();
	mView->scrollBy(deltaX,deltaY);
}


void ScrollPixmapView::ScrollTool::leftButtonReleaseEvent(QMouseEvent*) {
	if (!mDragStarted) return;

	mDragStarted=false;
	mView->viewport()->setCursor(mDragCursor);
}


void ScrollPixmapView::ScrollTool::wheelEvent(QWheelEvent* event) {
	if (mView->mouseWheelScroll()) {
		int deltaX, deltaY;

		if (event->state() & AltButton || event->orientation()==Horizontal) {
			deltaX = event->delta();
			deltaY = 0;
		} else {
			deltaX = 0;
			deltaY = event->delta();
		}
		mView->scrollBy(-deltaX, -deltaY);
	} else {
		if (event->delta()<0) {
			mView->emitSelectNext();
		} else {
			mView->emitSelectPrevious();
		}
	}
	event->accept();
}


void ScrollPixmapView::ScrollTool::updateCursor() {
	if (mDragStarted) {
		mView->viewport()->setCursor(mDraggingCursor);
	} else {
		mView->viewport()->setCursor(mDragCursor);
	}
}

