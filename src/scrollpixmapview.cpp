/*
Gwenview - A simple image viewer for KDE
Copyright (C) 2000-2002 Aurélien Gâteau

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

#include <math.h>

// Qt includes
#include <qcursor.h>
#include <qevent.h>
#include <qlabel.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qstyle.h>

// KDE includes
#include <kconfig.h>
#include <kdebug.h>

// Our includes
#include "gvpixmap.h"

#include "scrollpixmapview.moc"

const double MAX_ZOOM=16.0; // Same value as GIMP

ScrollPixmapView::ScrollPixmapView(QWidget* parent,GVPixmap* pixmap,bool enabled)
: QScrollView(parent,0L,WResizeNoErase|WRepaintNoErase), mGVPixmap(pixmap), mPopupMenu(0L), mZoom(1), mLockZoom(false), mDragStarted(false)
{
	unsetCursor();
	setFocusPolicy(StrongFocus);
	setFullScreen(false);
	enableView(enabled);
}


void ScrollPixmapView::enableView(bool enabled) {
	disconnect(mGVPixmap,0,this,0);

	if (enabled) {
		connect(mGVPixmap,SIGNAL(urlChanged(const KURL&,const QString&)),
			this,SLOT(updateView()) );
	}
}


void ScrollPixmapView::updateView() {
	if (mGVPixmap->isNull()) {
		resizeContents(0,0);
		viewport()->repaint(false);
		return;
	}

	if (!mLockZoom) {
		setZoom(1.0);
	} else {
		updateContentSize();
		updateImageOffset();
		viewport()->repaint(false);
	}
	horizontalScrollBar()->setValue(0);
	verticalScrollBar()->setValue(0);
}


void ScrollPixmapView::setZoom(double zoom) {
	mZoom=zoom;
	updateContentSize();
	updateImageOffset();
	viewport()->repaint(false);
	emit zoomChanged(mZoom);
}


void ScrollPixmapView::setFullScreen(bool fullScreen) {
	if (fullScreen) {
		viewport()->setBackgroundColor(black);
		setFrameStyle(NoFrame);
	} else {
		viewport()->setBackgroundMode(PaletteDark);
		setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
		setLineWidth( style().defaultFrameWidth() );
	}
}


bool ScrollPixmapView::isZoomSetToMax() {
	return mZoom>=MAX_ZOOM;
}


bool ScrollPixmapView::isZoomSetToMin() {
	return mZoom<=1/MAX_ZOOM;
}


//-Overloaded methods----------------------------------
void ScrollPixmapView::resizeEvent(QResizeEvent* event) {
	updateContentSize();
	updateImageOffset();
	QScrollView::resizeEvent(event);
}



void ScrollPixmapView::drawContents(QPainter* painter,int clipx,int clipy,int clipw,int cliph) {
/*
	static QColor colors[4]={QColor(255,0,0),QColor(0,255,0),QColor(0,0,255),QColor(255,255,0) };
	static int numColor=0;*/ // DEBUG rects

	if (mGVPixmap->isNull()) {
		painter->eraseRect(clipx,clipy,clipw,cliph);
		return;
	}

	int roundedClipx=int( int (  clipx        /mZoom)*mZoom );
	int roundedClipy=int( int (  clipy        /mZoom)*mZoom );
	int roundedClipw=int( ceil( (clipx+clipw) /mZoom)*mZoom ) - roundedClipx;
	int roundedCliph=int( ceil( (clipy+cliph) /mZoom)*mZoom ) - roundedClipy;

	if (roundedClipw<=0 || roundedCliph<=0) return;

	QPixmap buffer(roundedClipw,roundedCliph);
	{
		QPainter bufferPainter(&buffer);
		bufferPainter.setBackgroundColor(painter->backgroundColor());
		bufferPainter.eraseRect(0,0,roundedClipw,roundedCliph);
		bufferPainter.scale(mZoom,mZoom);
		bufferPainter.drawPixmap(0,0,
			mGVPixmap->pixmap(),
			int(roundedClipx/mZoom) - int(mXOffset/mZoom), int(roundedClipy/mZoom) - int(mYOffset/mZoom),
			int(roundedClipw/mZoom), int(roundedCliph/mZoom) );
	}

	painter->drawPixmap(clipx,clipy,buffer,clipx-roundedClipx,clipy-roundedClipy,clipw,cliph);

/*	painter->setPen(colors[numColor]);
	numColor=(numColor+1)%4;
	painter->drawRect(clipx,clipy,clipw,cliph);*/ // DEBUG Rects
}


void ScrollPixmapView::viewportMousePressEvent(QMouseEvent* event) {
	if (event->button()==RightButton) return;
	
	mScrollStartX=event->x();
	mScrollStartY=event->y();
	mDragStarted=true;
	setCursor(QCursor(SizeAllCursor));
}


void ScrollPixmapView::viewportMouseMoveEvent(QMouseEvent* event) {
	if (!mDragStarted) return;

	int deltaX,deltaY;

	deltaX=mScrollStartX - event->x();
	deltaY=mScrollStartY - event->y();

	mScrollStartX=event->x();
	mScrollStartY=event->y();
	scrollBy(deltaX,deltaY);
}


void ScrollPixmapView::viewportMouseReleaseEvent(QMouseEvent*) {
	if (mDragStarted) {
		unsetCursor();
		mDragStarted=false;
	}
}


//-Slots----------------------------------------------
void ScrollPixmapView::slotZoomIn() {
	double newZoom;
	for(newZoom=1/MAX_ZOOM;newZoom<=mZoom;newZoom*=2);
	setZoom(newZoom);
}


void ScrollPixmapView::slotZoomOut() {
	double newZoom;
	for(newZoom=MAX_ZOOM;newZoom>=mZoom;newZoom/=2);
	setZoom(newZoom);
}


void ScrollPixmapView::slotResetZoom() {
	setZoom(1.0);
}


void ScrollPixmapView::setLockZoom(bool value) {
	mLockZoom=value;
}


//-Private---------------------------------------------
void ScrollPixmapView::updateContentSize() {
	resizeContents(int(mGVPixmap->pixmap().width()*mZoom),int(mGVPixmap->pixmap().height()*mZoom));
}


void ScrollPixmapView::updateImageOffset() {
	int pixWidth=int( mGVPixmap->pixmap().width() * mZoom);
	int pixHeight=int( mGVPixmap->pixmap().height() * mZoom);

	mXOffset=QMAX( (width()-pixWidth)/2, 0);
	mYOffset=QMAX( (height()-pixHeight)/2, 0);
}


//-Config----------------------------------------------
void ScrollPixmapView::readConfig(KConfig*, const QString&) {
}


void ScrollPixmapView::writeConfig(KConfig*, const QString&) const {
}

