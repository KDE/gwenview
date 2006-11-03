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
#include "imageviewtools.h"

// KDE 
#include <kaction.h>
#include <kdebug.h>
#include <klocale.h>
#include <kstandarddirs.h>

// Local
#include "imageviewconfig.h"

namespace Gwenview {


// Helper function
static QCursor loadCursor(const QString& name) {
	QString path;
	path=locate("data", QString("gwenview/cursors/%1.png").arg(name));
	return QCursor(QPixmap(path));
}


//----------------------------------------------------------------------------
//
// ImageView::ToolBase
//
//----------------------------------------------------------------------------
ImageView::ToolBase::ToolBase(ImageView* view)
: mView(view) {}


ImageView::ToolBase::~ToolBase() {}

void ImageView::ToolBase::mouseMoveEvent(QMouseEvent*) {}
void ImageView::ToolBase::leftButtonPressEvent(QMouseEvent*) {}
void ImageView::ToolBase::leftButtonReleaseEvent(QMouseEvent*) {}

void ImageView::ToolBase::midButtonReleaseEvent(QMouseEvent*) {
	mView->zoomToFit()->activate();
}

void ImageView::ToolBase::rightButtonPressEvent(QMouseEvent* event) {
	emit mView->requestContextMenu(event->globalPos());
}

void ImageView::ToolBase::rightButtonReleaseEvent(QMouseEvent*) {
}

void ImageView::ToolBase::wheelEvent(QWheelEvent* event) {
	event->accept();
}

void ImageView::ToolBase::updateCursor() {
	mView->viewport()->setCursor(ArrowCursor);
}


//----------------------------------------------------------------------------
//
// ImageView::ZoomTool
//
//----------------------------------------------------------------------------
ImageView::ZoomTool::ZoomTool(ImageView* view)
: ImageView::ToolBase(view) {
	mZoomCursor=loadCursor("zoom");
}


void ImageView::ZoomTool::zoomTo(const QPoint& pos, bool in) {
	if (!mView->canZoom(in)) return;

	QPoint centerPos=QPoint(mView->visibleWidth(), mView->visibleHeight())/2;
	// Compute image position
	QPoint imgPos=mView->viewportToContents(pos) - mView->offset();
	double newZoom=mView->computeZoom(in);

	imgPos*=newZoom/mView->zoom();
	imgPos=imgPos-pos+centerPos;
	mView->setZoom(newZoom, imgPos.x(), imgPos.y());
}


void ImageView::ZoomTool::leftButtonReleaseEvent(QMouseEvent* event) {
	zoomTo(event->pos(), true);
}


void ImageView::ZoomTool::wheelEvent(QWheelEvent* event) {
	zoomTo(event->pos(), event->delta()>0);
	event->accept();
}


void ImageView::ZoomTool::rightButtonPressEvent(QMouseEvent*) {
}


void ImageView::ZoomTool::rightButtonReleaseEvent(QMouseEvent* event) {
	zoomTo(event->pos(), false);
}


void ImageView::ZoomTool::updateCursor() {
	mView->viewport()->setCursor(mZoomCursor);
}


QString ImageView::ZoomTool::hint() const {
    return i18n("Left click to zoom in, right click to zoom out. You can also use the mouse wheel.");
}


//----------------------------------------------------------------------------
//
// ImageView::ScrollTool
//
//----------------------------------------------------------------------------
ImageView::ScrollTool::ScrollTool(ImageView* view)
: ImageView::ToolBase(view)
, mScrollStartX(0), mScrollStartY(0)
, mDragStarted(false) {
}


void ImageView::ScrollTool::leftButtonPressEvent(QMouseEvent* event) {
	mScrollStartX=event->x();
	mScrollStartY=event->y();
	mView->viewport()->setCursor(SizeAllCursor);
	mDragStarted=true;
}


void ImageView::ScrollTool::mouseMoveEvent(QMouseEvent* event) {
	if (!mDragStarted) return;

	int deltaX,deltaY;

	deltaX=mScrollStartX - event->x();
	deltaY=mScrollStartY - event->y();

	mScrollStartX=event->x();
	mScrollStartY=event->y();
	mView->scrollBy(deltaX,deltaY);
}


void ImageView::ScrollTool::leftButtonReleaseEvent(QMouseEvent*) {
	if (!mDragStarted) return;

	mDragStarted=false;
	mView->viewport()->setCursor(ArrowCursor);
}


void ImageView::ScrollTool::wheelEvent(QWheelEvent* event) {
	if (ImageViewConfig::mouseWheelScroll()) {
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


void ImageView::ScrollTool::updateCursor() {
	if (mDragStarted) {
		mView->viewport()->setCursor(SizeAllCursor);
	} else {
		mView->viewport()->setCursor(ArrowCursor);
	}
}


QString ImageView::ScrollTool::hint() const {
    return i18n("Drag to move the image, middle-click to toggle auto-zoom. Hold the Control key to switch to the zoom tool.");
}


} // namespace
