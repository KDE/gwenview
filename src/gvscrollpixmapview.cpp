// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aur�ien G�eau

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

// Qt 
#include <qbitmap.h>
#include <qcolor.h>
#include <qcursor.h>
#include <qdatetime.h>
#include <qevent.h>
#include <qlabel.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qpopupmenu.h>
#include <qstyle.h>
#include <qtimer.h>

// KDE 
#include <kaction.h>
#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>
#include <kpropsdlg.h>
#include <kstandarddirs.h>
#include <kstdaction.h>
#include <kapplication.h>

// Local
#include "fileoperation.h"
#include "gvexternaltoolmanager.h"
#include "gvexternaltoolcontext.h"
#include "gvdocument.h"
#include "gvimageutils.h"

#include "gvscrollpixmapview.moc"

const char* CONFIG_SHOW_PATH="show path";
const char* CONFIG_SMOOTH_SCALE="smooth scale";
const char* CONFIG_ENLARGE_SMALL_IMAGES="enlarge small images";
const char* CONFIG_SHOW_SCROLL_BARS="show scroll bars";
const char* CONFIG_MOUSE_WHEEL_SCROLL="mouse wheel scrolls image";
const char* CONFIG_LOCK_ZOOM="lock zoom";
const char* CONFIG_AUTO_ZOOM="auto zoom";
const char* CONFIG_AUTO_ZOOM_BROWSE="auto zoom browse";

const int AUTO_HIDE_TIMEOUT=2000;

const double MAX_ZOOM=16.0; // Same value as GIMP


//------------------------------------------------------------------------
//
// GVScrollPixmapView ToolControllers
//
//------------------------------------------------------------------------
class GVScrollPixmapView::ToolController {
protected:
	GVScrollPixmapView* mView;

	// Helper function
	QCursor loadCursor(const QString& name) {
		QString path;
		path=locate("data", QString("gwenview/cursors/%1.png").arg(name));
		return QCursor(QPixmap(path));
	}

public:
	ToolController(GVScrollPixmapView* view)
	: mView(view) {}
	virtual ~ToolController() {}
	virtual void leftButtonPressEvent(QMouseEvent*) {}
	virtual void mouseMoveEvent(QMouseEvent*) {}
	virtual void leftButtonReleaseEvent(QMouseEvent*) {}
	virtual void midButtonReleaseEvent(QMouseEvent*) {
		mView->autoZoom()->activate();
	}

	virtual void rightButtonReleaseEvent(QMouseEvent* event) {
		mView->openContextMenu(event->globalPos());
	}

	virtual void wheelEvent(QWheelEvent* event) { event->accept(); }
	virtual void updateCursor() {
		mView->viewport()->setCursor(ArrowCursor);
	}
};


class GVScrollPixmapView::ZoomToolController : public GVScrollPixmapView::ToolController {
private:
	QCursor mZoomCursor;

	void zoomTo(const QPoint& pos, bool in) {
		KAction* zoomAction=in?mView->zoomIn():mView->zoomOut();
		if (!zoomAction->isEnabled()) return;

		if (mView->autoZoom()->isChecked()) {
			mView->autoZoom()->setChecked(false);
			mView->updateScrollBarMode();
		}
		QPoint centerPos=QPoint(mView->visibleWidth(), mView->visibleHeight())/2;

		// Compute image position
		QPoint imgPos=mView->viewportToContents(pos) - QPoint(mView->mXOffset, mView->mYOffset);
		imgPos/=mView->zoom();

		double newZoom=mView->computeZoom(in);

		imgPos*=newZoom;
		imgPos=imgPos-pos+centerPos;
		mView->setZoom(newZoom, imgPos.x(), imgPos.y());
	}

public:
	ZoomToolController(GVScrollPixmapView* view) : ToolController(view) {
		mZoomCursor=loadCursor("zoom");
	}

	void leftButtonReleaseEvent(QMouseEvent* event) {
		zoomTo(event->pos(), true);
	}

	void wheelEvent(QWheelEvent* event) {
		zoomTo(event->pos(), event->delta()<0);
		event->accept();
	}

	void rightButtonReleaseEvent(QMouseEvent* event) {
		zoomTo(event->pos(), false);
	}

	void updateCursor() {
		mView->viewport()->setCursor(mZoomCursor);
	}
};


class GVScrollPixmapView::ScrollToolController : public GVScrollPixmapView::ToolController {
	int mScrollStartX,mScrollStartY;
	bool mDragStarted;

protected:
	QCursor mDragCursor,mDraggingCursor;

public:
	ScrollToolController(GVScrollPixmapView* view)
	: ToolController(view)
	, mScrollStartX(0), mScrollStartY(0)
	, mDragStarted(false) {
		mDragCursor=loadCursor("drag");
		mDraggingCursor=loadCursor("dragging");
	}

	void leftButtonPressEvent(QMouseEvent* event) {
		mScrollStartX=event->x();
		mScrollStartY=event->y();
		mView->viewport()->setCursor(mDraggingCursor);
		mDragStarted=true;
	}

	void mouseMoveEvent(QMouseEvent* event) {
		if (!mDragStarted) return;

		int deltaX,deltaY;

		deltaX=mScrollStartX - event->x();
		deltaY=mScrollStartY - event->y();

		mScrollStartX=event->x();
		mScrollStartY=event->y();
		mView->scrollBy(deltaX,deltaY);
	}

	void leftButtonReleaseEvent(QMouseEvent*) {
		if (!mDragStarted) return;

		mDragStarted=false;
		mView->viewport()->setCursor(mDragCursor);
	}

	void wheelEvent(QWheelEvent* event) {
		if (mView->mMouseWheelScroll) {
			int deltaX, deltaY;

			if (event->state() & ControlButton || event->orientation()==Horizontal) {
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

	void updateCursor() {
		if (mDragStarted) {
			mView->viewport()->setCursor(mDraggingCursor);
		} else {
			mView->viewport()->setCursor(mDragCursor);
		}
	}
};


//------------------------------------------------------------------------
//
// GVScrollPixmapView implementation
//
//------------------------------------------------------------------------
GVScrollPixmapView::GVScrollPixmapView(QWidget* parent,GVDocument* pixmap,KActionCollection* actionCollection)
: QScrollView(parent,0L,WResizeNoErase|WRepaintNoErase|WPaintClever)
, mDocument(pixmap)
, mAutoHideTimer(new QTimer(this))
, mPathLabel(new QLabel(parent))
, mTool(NONE)
, mXOffset(0),mYOffset(0)
, mZoom(1)
, mActionCollection(actionCollection)
, mFullScreen(false)
, mOperaLikePrevious(false)
, mZoomBeforeAuto(1)
, mFilter( this )
, mSmoothingSuspended( false )
, mEmptyImage( false )
{
	setFocusPolicy(StrongFocus);
	setFrameStyle(NoFrame);

	mToolControllers[SCROLL]=new ScrollToolController(this);
	mToolControllers[ZOOM]=new ZoomToolController(this);

	// Init path label
	mPathLabel->setBackgroundColor(white);
	QFont font=mPathLabel->font();
	font.setWeight(QFont::Bold);
	mPathLabel->setFont(font);
	mPathLabel->hide();

	// Create actions
	mAutoZoom=new KToggleAction(i18n("&Auto Zoom"),"viewmagfit",0,mActionCollection,"view_zoom_auto");
	connect(mAutoZoom,SIGNAL(toggled(bool)),
		this,SLOT(setAutoZoom(bool)) );

	mZoomIn=KStdAction::zoomIn(this,SLOT(slotZoomIn()),mActionCollection);

	mZoomOut=KStdAction::zoomOut(this,SLOT(slotZoomOut()),mActionCollection);

	mResetZoom=KStdAction::actualSize(this,SLOT(slotResetZoom()),mActionCollection);
	mResetZoom->setIcon("viewmag1");

	mLockZoom=new KToggleAction(i18n("&Lock Zoom"),"lock",0,mActionCollection,"view_zoom_lock");

	// Connect to some interesting signals
	connect(mDocument,SIGNAL(loaded(const KURL&,const QString&)),
		this,SLOT(slotLoaded()) );

	connect(mDocument,SIGNAL(loading()),
		this,SLOT( loadingStarted()) );

	connect(mDocument,SIGNAL(modified()),
		this,SLOT(slotModified()) );

	connect(mDocument, SIGNAL(sizeUpdated(int, int)),
		this, SLOT(slotImageSizeUpdated()) );

	connect(mDocument, SIGNAL(rectUpdated(const QRect&)),
		this, SLOT(slotImageRectUpdated(const QRect&)) );

	connect(mAutoHideTimer,SIGNAL(timeout()),
		this,SLOT(hideCursor()) );

	connect(&mPendingPaintTimer,SIGNAL(timeout()),
		this,SLOT(paintPending()) );

	// This event filter is here to make sure the pixmap view is aware of the changes
	// in the keyboard modifiers, even if it isn't focused. However, making this widget
	// itself the filter would lead to doubled paint events, because QScrollView
	// installs an event filter on its viewport, and doesn't filter out the paint
	// events -> it'd get it twice, first from app filter, second from viewport filter.
	kapp->installEventFilter( &mFilter );

	viewport()->setBackgroundMode(PaletteMid);
}


GVScrollPixmapView::~GVScrollPixmapView() {
	delete mToolControllers[NONE];
	delete mToolControllers[BROWSE];
	delete mToolControllers[SCROLL];
	delete mToolControllers[ZOOM];
}


void GVScrollPixmapView::slotLoaded() {
	updateZoomActions();

	if (mDocument->isNull()) {
		resizeContents(0,0);
		viewport()->repaint(false);
		return;
	}

	updateContentSize();
	updateImageOffset();
	if (mFullScreen && mShowPathInFullScreen) updatePathLabel();
	if (mSmoothScale >= SMOOTH2) scheduleOperation( SMOOTH_PASS );
}


void GVScrollPixmapView::slotModified() {
	if (mAutoZoom->isChecked()) {
		setZoom(computeAutoZoom());
	} else {
		updateContentSize();
		updateImageOffset();
		updateZoomActions();
		fullRepaint();
	}
}


void GVScrollPixmapView::loadingStarted() {
	cancelPending();
	mSmoothingSuspended = true;
	mEmptyImage = true;
	// every loading() signal from GVDocument must be followed by a signal that turns this off
	QPainter painter( viewport());
	painter.eraseRect( viewport()->rect());
}

//------------------------------------------------------------------------
//
// Properties
//
//------------------------------------------------------------------------
void GVScrollPixmapView::setSmoothScale(int value) {
	if( mSmoothScale == value ) return;
	mSmoothScale= static_cast< SmoothScale >( value );
	if( mSmoothScale >= SMOOTH2 ) {
		scheduleOperation( SMOOTH_PASS );
	} else {
		fullRepaint();
	}
}


void GVScrollPixmapView::setEnlargeSmallImages(bool value) {
	mEnlargeSmallImages=value;
	if (mAutoZoom->isChecked()) {
		setZoom(computeAutoZoom());
	}
}


void GVScrollPixmapView::setShowPathInFullScreen(bool value) {
	mShowPathInFullScreen=value;
}


void GVScrollPixmapView::setShowScrollBars(bool value) {
	mShowScrollBars=value;
	updateScrollBarMode();
}


void GVScrollPixmapView::setMouseWheelScroll(bool value) {
	mMouseWheelScroll=value;
}


void GVScrollPixmapView::setZoom(double zoom, int centerX, int centerY) {
	int viewWidth=visibleWidth();
	int viewHeight=visibleHeight();
	double oldZoom=mZoom;
	mZoom=zoom;

	viewport()->setUpdatesEnabled(false);

	updateContentSize();

	// Find the coordinate of the center of the image
	// and center the view on it
	if (centerX==-1) {
		centerX=int( ((viewWidth/2+contentsX()-mXOffset)/oldZoom)*mZoom );
	}
	if (centerY==-1) {
		centerY=int( ((viewHeight/2+contentsY()-mYOffset)/oldZoom)*mZoom );
	}
	center(centerX,centerY);

	updateImageOffset();
	updateZoomActions();

	viewport()->setUpdatesEnabled(true);
		fullRepaint();

	emit zoomChanged(mZoom);
}


void GVScrollPixmapView::setFullScreen(bool fullScreen) {
	mFullScreen=fullScreen;
	viewport()->setMouseTracking(mFullScreen);
	if (mFullScreen) {
		viewport()->setBackgroundColor(black);
		restartAutoHideTimer();
	} else {
		viewport()->setBackgroundMode(PaletteMid);
		mAutoHideTimer->stop();
		mToolControllers[mTool]->updateCursor();
	}

	if (mFullScreen && mShowPathInFullScreen) {
		updatePathLabel();
		mPathLabel->show();
	} else {
		mPathLabel->hide();
	}
}


//------------------------------------------------------------------------
//
// Overloaded methods
//
//------------------------------------------------------------------------
void GVScrollPixmapView::resizeEvent(QResizeEvent* event) {
	QScrollView::resizeEvent(event);
	if (mAutoZoom->isChecked()) {
		setZoom(computeAutoZoom());
	} else {
		updateContentSize();
		updateImageOffset();
	}
}


//#define DEBUG_RECTS
inline int roundDown(int value,double factor) {
	return int(int(value/factor)*factor);
}
inline int roundUp(int value,double factor) {
	return int(ceil(value/factor)*factor);
}


inline void composite(uint* rgba,uint value) {
	uint alpha=(*rgba) >> 24;
	if (alpha<255) {
		uint alphaValue=(255-alpha)*value;

		uint c1=( ( (*rgba & 0xFF0000) >> 16 ) * alpha + alphaValue ) >> 8;
		uint c2=( ( (*rgba & 0x00FF00) >>  8 ) * alpha + alphaValue ) >> 8;
		uint c3=( ( (*rgba & 0x0000FF) >>  0 ) * alpha + alphaValue ) >> 8;
		*rgba=0xFF000000 + (c1<<16) + (c2<<8) + c3;
	}
}

void GVScrollPixmapView::drawContents(QPainter* painter,int clipx,int clipy,int clipw,int cliph) {
	if( !mEmptyImage ) {
		addPendingPaint( false, QRect( clipx, clipy, clipw, cliph ));
	} else {
		// image is empty, simply clear
		painter->eraseRect( clipx, clipy, clipw, cliph );
	}
}

// How this pending stuff works:
// There's a queue of areas to paint (each with bool saying whether it's smooth pass).
// Also, there's a bitfield of pending operations, operations are handled only after
// there's nothing more to paint (so that loading is resumed or smooth pass is started).
void GVScrollPixmapView::addPendingPaint( bool smooth, QRect rect ) {
	if( mSmoothingSuspended && smooth ) return;

	// try to avoid scheduling already scheduled areas
	QRegion& region = smooth ? mPendingSmoothRegion : mPendingNormalRegion;
	if( region.intersect( rect ) == QRegion( rect ))
		return; // whole rect has already pending paints
	// at least try to remove the part that's already scheduled
	rect = ( QRegion( rect ) - region ).boundingRect();
	region += rect;
	// split to several repaints limited by pixels to be painted
	const int MAX_REPAINT_SIZE = 10000;
	int step = 0;
	if (rect.width() > 0) {
		step=(( MAX_REPAINT_SIZE + rect.width() - 1 ) / rect.width()); // round up
	}
	step = QMAX( step, 5 ); // at least 5 lines together
	for( int line = 0;
		 line < rect.height();
		 line += step ) {
		step = QMIN( step, rect.height() - line );
		QRect area( rect.x(), rect.y() + line, rect.width(), step );
		const long long MAX_DIM = 1000000; // if monitors get larger than this, we're in trouble :)
		// QMap will ensure ordering (non-smooth first, top-to-bottom, left-to-right)
		long long key = ( smooth ? MAX_DIM * MAX_DIM : 0 ) + ( area.y() + line ) * MAX_DIM + area.x();
		// handle the case of two different paints at the same position (just in case)
		key *= 100;
		bool insert = true;
		while( mPendingPaints.contains( key )) {
			if( mPendingPaints[ key ].rect.contains( area )) {
				insert = false;
				break;
			}
			if( area.contains( mPendingPaints[ key ].rect )) {
				break;
			}
			++key;
		}
		if( insert ) {
			mPendingPaints[ key ] = PendingPaint( smooth, area );
		}
	}

	if( !mPendingPaintTimer.isActive()) mPendingPaintTimer.start( 0 );
}

void GVScrollPixmapView::paintPending() {
	while( !mPendingPaints.isEmpty()) {
		PendingPaint paint = *mPendingPaints.begin();
		mPendingPaints.remove( mPendingPaints.begin());
		QRegion& region = paint.smooth ? mPendingSmoothRegion : mPendingNormalRegion;
		region -= paint.rect;
		QRect visibleRect( contentsX(), contentsY(), visibleWidth(), visibleHeight());
		QRect paintRect = paint.rect.intersect( visibleRect );
		if( !paintRect.isEmpty()) {
			QPainter painter( viewport());
			painter.translate( -contentsX(), -contentsY());
			performPaint( &painter, paintRect.x(), paintRect.y(),
				paintRect.width(), paintRect.height(), paint.smooth );
			return;
		}
	}
	mPendingPaintTimer.stop();
	checkPendingOperations();
}

void GVScrollPixmapView::scheduleOperation( Operation operation )
{
	mPendingOperations |= operation;
	if( mPendingPaints.isEmpty()) {
		checkPendingOperations();
	}
}

void GVScrollPixmapView::checkPendingOperations()
{
	if( mPendingOperations & RESUME_LOADING ) {
		mDocument->resumeLoading();
		mPendingOperations &= ~RESUME_LOADING;
		return;
	}
	if( mPendingOperations & SMOOTH_PASS ) {
		mSmoothingSuspended = false;
		if( mSmoothScale >= SMOOTH2 ) {
			QRect visibleRect( contentsX(), contentsY(), visibleWidth(), visibleHeight());
			addPendingPaint( true, visibleRect );
		}
		mPendingOperations &= ~SMOOTH_PASS;
		return;
	}
}

// How to do painting:
// When something needs to be erased: QPainter on viewport and eraseRect()
// When whole picture needs to be repainted: fullRepaint()
// When a part of the picture needs to be updated: viewport()->repaint(area,false)
// All other paints will be changed to progressive painting.
void GVScrollPixmapView::fullRepaint() {
	if( !viewport()->isUpdatesEnabled()) return;
	cancelPending();
	viewport()->repaint(false);
}

void GVScrollPixmapView::cancelPending() {
	mPendingPaints.clear();
	mPendingNormalRegion = QRegion();
	mPendingSmoothRegion = QRegion();
	mPendingPaintTimer.stop();
	mPendingOperations = 0;
}

// do the actual painting
void GVScrollPixmapView::performPaint( QPainter* painter, int clipx, int clipy, int clipw, int cliph, bool smooth ) {
	#ifdef DEBUG_RECTS
	static QColor colors[4]={QColor(255,0,0),QColor(0,255,0),QColor(0,0,255),QColor(255,255,0) };
	static int numColor=0;
	#endif

	QRect updateRect=QRect(clipx,clipy,clipw,cliph);
	if (mDocument->isNull()) {
		painter->eraseRect(clipx,clipy,clipw,cliph);
		return;
	}

	// If we are enlarging the image, we grow the update rect so that it
	// contains full zoomed image pixels. We also take one more image pixel to
	// avoid gaps in images.
	if (mZoom>1.0) {
		updateRect.setLeft	( roundDown(updateRect.left(),	mZoom)-int(mZoom) );
		updateRect.setTop	( roundDown(updateRect.top(),	mZoom)-int(mZoom) );
		updateRect.setRight ( roundUp(	updateRect.right(), mZoom)+int(mZoom)-1 );
		updateRect.setBottom( roundUp(	updateRect.bottom(),mZoom)+int(mZoom)-1 );
	}

	QRect zoomedImageRect=QRect(mXOffset, mYOffset, int(mDocument->width()*mZoom), int(mDocument->height()*mZoom));
	updateRect=updateRect.intersect(zoomedImageRect);

	if (updateRect.isEmpty()) {
		painter->eraseRect(clipx,clipy,clipw,cliph);
		return;
	}

	QImage image=mDocument->image().copy(
		int(updateRect.x()/mZoom) - int(mXOffset/mZoom), int(updateRect.y()/mZoom) - int(mYOffset/mZoom),
		int(updateRect.width()/mZoom), int(updateRect.height()/mZoom) );

	// There's a small problem somewhere here above. If updateRect.height() is 1 and mZoom > 1,
	// the resulting image height will be 0, yet there's one pixel row to paint. For now, at least
	// avoid using a null image.
	if (image.isNull()) {
		painter->eraseRect(clipx,clipy,clipw,cliph);
		return;
	}

	if (smooth) {
		if( mZoom != 1.0 ) {
			image=image.convertDepth(32);
			static GVImageUtils::SmoothAlgorithm algs[] = {
				GVImageUtils::SMOOTH_NONE,
				GVImageUtils::SMOOTH_FAST,
				GVImageUtils::SMOOTH_NORMAL,
				GVImageUtils::SMOOTH_BEST,
				GVImageUtils::SMOOTH_FAST,
				GVImageUtils::SMOOTH_NORMAL,
				GVImageUtils::SMOOTH_BEST };
			image=GVImageUtils::scale(image,updateRect.width(),updateRect.height(), algs[ mSmoothScale ] );
		}
	} else {
		static GVImageUtils::SmoothAlgorithm algs[] = {
			GVImageUtils::SMOOTH_NONE,
			GVImageUtils::SMOOTH_FAST,
			GVImageUtils::SMOOTH_NORMAL,
			GVImageUtils::SMOOTH_BEST,
			GVImageUtils::SMOOTH_NONE,
			GVImageUtils::SMOOTH_NONE,
			GVImageUtils::SMOOTH_NONE }; // none for 2-pass ones
		image=GVImageUtils::scale(image,updateRect.width(),updateRect.height(), algs[ mSmoothScale ] );
		if( mSmoothScale >= SMOOTH2 && mZoom != 1.0 ) {
			addPendingPaint( true, QRect( clipx, clipy, clipw, cliph ));
		}
	}

	if (image.hasAlphaBuffer()) {
		if (image.depth()!=32) {
			image=image.convertDepth(32);
		}

		bool light;

		int imageXOffset=int(updateRect.x())-int(mXOffset);
		int imageYOffset=int(updateRect.y())-int(mYOffset);
		int imageWidth=image.width();
		int imageHeight=image.height();
		for (int y=0;y<imageHeight;++y) {
			uint* rgba=(uint*)(image.scanLine(y));
			for(int x=0;x<imageWidth;x++) {
				light= ((x+imageXOffset) & 16) ^ ((y+imageYOffset) & 16);
				composite(rgba,light?192:128);
				rgba++;
			}
		}
		image.setAlphaBuffer(false);
	}

	QPixmap buffer(clipw,cliph);
	{
		QPainter bufferPainter(&buffer);
		bufferPainter.setBackgroundColor(painter->backgroundColor());
		bufferPainter.eraseRect(0,0,clipw,cliph);
		bufferPainter.drawImage(updateRect.x()-clipx,updateRect.y()-clipy,image);
	}
	painter->drawPixmap(clipx,clipy,buffer);


	#ifdef DEBUG_RECTS
	painter->setPen(colors[numColor]);
	numColor=(numColor+1)%4;
	painter->drawRect(clipx,clipy,clipw,cliph);
	#endif
}


void GVScrollPixmapView::viewportMousePressEvent(QMouseEvent* event) {
	viewport()->setFocus();
	if (event->button()==LeftButton) {
		mToolControllers[mTool]->leftButtonPressEvent(event);
	}
}


void GVScrollPixmapView::viewportMouseMoveEvent(QMouseEvent* event) {
	selectTool(event->state(), true);
	mToolControllers[mTool]->mouseMoveEvent(event);
	if (mFullScreen) {
		restartAutoHideTimer();
	}
}


void GVScrollPixmapView::viewportMouseReleaseEvent(QMouseEvent* event) {
	switch (event->button()) {
	case Qt::LeftButton:
		if (event->stateAfter() & Qt::RightButton) {
			mOperaLikePrevious=true;
			emit selectPrevious();
			return;
		}
		mToolControllers[mTool]->leftButtonReleaseEvent(event);
		break;

	case Qt::MidButton:
		mToolControllers[mTool]->midButtonReleaseEvent(event);
		break;

	case Qt::RightButton:
		if (event->stateAfter() & Qt::LeftButton) {
			emit selectNext();
			return;
		}

		if (mOperaLikePrevious) { // Avoid showing the popup menu after Opera like previous
			mOperaLikePrevious=false;
		} else {
			mToolControllers[mTool]->rightButtonReleaseEvent(event);
		}
		break;

	default: // Avoid compiler complain
		break;
	}
}


bool GVScrollPixmapView::eventFilter(QObject* obj, QEvent* event) {
	switch (event->type()) {
	case QEvent::KeyPress:
	case QEvent::KeyRelease:
	case QEvent::AccelOverride:
		return viewportKeyEvent(static_cast<QKeyEvent*>(event));

	// Getting/loosing focus causes repaints, but repainting here is expensive,
	// and there's no need to repaint on focus changes, as the focus is not
	// indicated.
	case QEvent::FocusIn:
	case QEvent::FocusOut:
		return true;

	default:
		break;
	}
	return QScrollView::eventFilter(obj,event);
}

GVScrollPixmapViewFilter::GVScrollPixmapViewFilter( GVScrollPixmapView* parent )
: QObject( parent )
{
}

bool GVScrollPixmapViewFilter::eventFilter(QObject*, QEvent* event) {
	switch (event->type()) {
	case QEvent::KeyPress:
	case QEvent::KeyRelease:
	case QEvent::AccelOverride:
		return static_cast< GVScrollPixmapView* >( parent())
					->viewportKeyEvent(static_cast<QKeyEvent*>(event));
	default:
		break;
	}
	return false;
}

bool GVScrollPixmapView::viewportKeyEvent(QKeyEvent* event) {
	selectTool(event->stateAfter(), false);
	if (mFullScreen) {
		restartAutoHideTimer();
	}
	return false;
}


/**
 * If force is set, the cursor will be updated even if the tool is not
 * different from the current one.
 */
void GVScrollPixmapView::selectTool(ButtonState state, bool force) {
	Tool oldTool=mTool;
	if (state & ShiftButton) {
		mTool=ZOOM;
	} else {
		mTool=SCROLL;
	}

	if (mTool!=oldTool || force) {
		mToolControllers[mTool]->updateCursor();
	}
}


void GVScrollPixmapView::wheelEvent(QWheelEvent* event) {
	mToolControllers[mTool]->wheelEvent(event);
}


//------------------------------------------------------------------------
//
// Slots
//
//------------------------------------------------------------------------
void GVScrollPixmapView::slotZoomIn() {
	if (mAutoZoom->isChecked()) {
		mAutoZoom->setChecked(false);
		updateScrollBarMode();
	}
	setZoom(computeZoom(true));
}


void GVScrollPixmapView::slotZoomOut() {
	if (mAutoZoom->isChecked()) {
		mAutoZoom->setChecked(false);
		updateScrollBarMode();
	}
	setZoom(computeZoom(false));
}


void GVScrollPixmapView::slotResetZoom() {
	if (mAutoZoom->isChecked()) {
		mAutoZoom->setChecked(false);
		updateScrollBarMode();
	}
	setZoom(1.0);
}


void GVScrollPixmapView::setAutoZoom(bool value) {
	updateScrollBarMode();
	if (value) {
		mZoomBeforeAuto=mZoom;
		mXCenterBeforeAuto=width()/2  + contentsX() + mXOffset;
		mYCenterBeforeAuto=height()/2 + contentsY() + mYOffset;
		setZoom(computeAutoZoom());
	} else {
		setZoom(mZoomBeforeAuto, mXCenterBeforeAuto, mYCenterBeforeAuto);
	}
}


//------------------------------------------------------------------------
//
// Private
//
//------------------------------------------------------------------------
void GVScrollPixmapView::slotImageSizeUpdated() {
	mXOffset=0;
	mYOffset=0;

	if (mAutoZoom->isChecked()) {
		mXCenterBeforeAuto=0;
		mYCenterBeforeAuto=0;
		setZoom(computeAutoZoom());
	} else {
		horizontalScrollBar()->setValue(0);
		verticalScrollBar()->setValue(0);
	}
	updateImageOffset();
	QRect imageRect(mXOffset, mYOffset, mDocument->width(), mDocument->height());

		QPainter painter( viewport());
	// Top rect
	painter.eraseRect( 0, 0,
		viewport()->width(), imageRect.top());

	// Bottom rect
		painter.eraseRect( 0, imageRect.bottom(),
		viewport()->width(), viewport()->height()-imageRect.bottom());

	// Left rect
	painter.eraseRect( 0, imageRect.top(),
		imageRect.left(), imageRect.height());

	// Right rect
	painter.eraseRect( imageRect.right(), imageRect.top(),
		viewport()->width()-imageRect.right(), imageRect.height());
}

void GVScrollPixmapView::slotImageRectUpdated(const QRect& imageRect) {
		mEmptyImage = false;
	QRect widgetRect;
	// We add a one pixel border to avoid missing parts of the image
	widgetRect.setLeft( int(imageRect.left()*mZoom) + mXOffset-1);
	widgetRect.setTop( int(imageRect.top()*mZoom) + mYOffset-1);
	widgetRect.setWidth( int(imageRect.width()*mZoom) +2);
	widgetRect.setHeight( int(imageRect.height()*mZoom) +2);
	viewport()->repaint(widgetRect,false);
	mDocument->suspendLoading();
	scheduleOperation( RESUME_LOADING );
}


void GVScrollPixmapView::restartAutoHideTimer() {
	mAutoHideTimer->start(AUTO_HIDE_TIMEOUT,true);
}


void GVScrollPixmapView::openContextMenu(const QPoint& pos) {
	//KPart
	if (QString(parent()->name()) == QString("KonqFrame") ||
		QString(parent()->name()) == QString("gwenview-kpart-splitter")) {
		emit contextMenu();
		return;
	}

	QPopupMenu menu(this);
	bool noImage=mDocument->filename().isEmpty();
	bool validImage=!mDocument->isNull();

	// The fullscreen item is always there, to be able to leave fullscreen mode
	// if necessary
	mActionCollection->action("fullscreen")->plug(&menu);

	if (validImage) {
		menu.insertSeparator();

		mAutoZoom->plug(&menu);
		mZoomIn->plug(&menu);
		mZoomOut->plug(&menu);
		mResetZoom->plug(&menu);
		mLockZoom->plug(&menu);
	}

	menu.insertSeparator();

	mActionCollection->action("first")->plug(&menu);
	mActionCollection->action("previous")->plug(&menu);
	mActionCollection->action("next")->plug(&menu);
	mActionCollection->action("last")->plug(&menu);

	if (validImage) {
		menu.insertSeparator();

		QPopupMenu* editMenu=new QPopupMenu(&menu);
		mActionCollection->action("rotate_left")->plug(editMenu);
		mActionCollection->action("rotate_right")->plug(editMenu);
		mActionCollection->action("mirror")->plug(editMenu);
		mActionCollection->action("flip")->plug(editMenu);
		menu.insertItem( i18n("Edit"), editMenu );

		GVExternalToolContext* externalToolContext=
			GVExternalToolManager::instance()->createContext(
			this, mDocument->url());

		menu.insertItem(
			i18n("External Tools"), externalToolContext->popupMenu());
	}

	if (!noImage) {
		menu.insertSeparator();

		menu.connectItem(
			menu.insertItem( i18n("&Rename...") ),
			this,SLOT(renameFile()) );
		menu.connectItem(
			menu.insertItem( i18n("&Copy To...") ),
			this,SLOT(copyFile()) );
		menu.connectItem(
			menu.insertItem( i18n("&Move To...") ),
			this,SLOT(moveFile()) );
		menu.connectItem(
			menu.insertItem( i18n("&Delete") ),
			this,SLOT(deleteFile()) );

		menu.insertSeparator();

		menu.connectItem(
			menu.insertItem( i18n("Properties") ),
			this,SLOT(showFileProperties()) );
	}

	menu.exec(pos);
}


void GVScrollPixmapView::updateScrollBarMode() {
	if (mAutoZoom->isChecked() || !mShowScrollBars) {
		setVScrollBarMode(AlwaysOff);
		setHScrollBarMode(AlwaysOff);
	} else {
		setVScrollBarMode(Auto);
		setHScrollBarMode(Auto);
	}
}


void GVScrollPixmapView::updateContentSize() {
	resizeContents(
		int(mDocument->width()*mZoom),
		int(mDocument->height()*mZoom)	);
}

// QSize.scale() does not exist in Qt 3.0.x
#if QT_VERSION<0x030100
static void sizeScaleMin(QSize* size, int w, int h) {
	int w0 = size->width();
	int h0 = size->height();
	int rw = h * w0 / h0;

	if ( rw <= w ) {
		size->setWidth( rw );
		size->setHeight( h );
	} else {
		size->setWidth( w );
		size->setHeight( w * h0 / w0 );
	}
}
#endif

double GVScrollPixmapView::computeAutoZoom() const {
	if (mDocument->isNull()) {
		return 1.0;
	}
	QSize size=mDocument->image().size();

#if QT_VERSION>=0x030100
	size.scale(width(),height(),QSize::ScaleMin);
#else
	sizeScaleMin(&size,width(),height());
#endif

	double zoom=double(size.width())/mDocument->width();
	if (zoom>1.0 && !mEnlargeSmallImages) return 1.0;
	return zoom;
}


double GVScrollPixmapView::computeZoom(bool in) const {
	if (in) {
		if (mZoom>=1.0) {
			return floor(mZoom)+1.0;
		} else {
			return 1/( ceil(1/mZoom)-1.0 );
		}
	} else {
		if (mZoom>1.0) {
			return ceil(mZoom)-1.0;
		} else {
			return 1/( floor(1/mZoom)+1.0 );
		}
	}
}


void GVScrollPixmapView::updateImageOffset() {
	int viewWidth=width();
	int viewHeight=height();

	// Compute mXOffset and mYOffset in case the image does not fit
	// the view width or height
	int zpixWidth=int(mDocument->width() * mZoom);
	int zpixHeight=int(mDocument->height() * mZoom);

	if (zpixWidth>viewWidth && hScrollBarMode()!=AlwaysOff) {
		viewHeight-=horizontalScrollBar()->height();
	}
	if (zpixHeight>viewHeight && vScrollBarMode()!=AlwaysOff) {
		viewWidth-=verticalScrollBar()->width();
	}

	mXOffset=QMAX(0,(viewWidth-zpixWidth)/2);
	mYOffset=QMAX(0,(viewHeight-zpixHeight)/2);
}


void GVScrollPixmapView::hideCursor() {
	viewport()->setCursor(blankCursor);
}


void GVScrollPixmapView::updatePathLabel() {
	QString path=mDocument->url().path();

	QPainter painter;

	QSize pathSize=mPathLabel->fontMetrics().size(0,path);
	pathSize.setWidth( pathSize.width() + 4);
	pathSize.setHeight( pathSize.height() + 15);
	int left=2;
	int top=mPathLabel->fontMetrics().height();

	// Create a mask for the text
	QBitmap mask(pathSize,true);
	painter.begin(&mask);
	painter.setFont(mPathLabel->font());
	painter.drawText(left-1,top-1,path);
	painter.drawText(left,top-1,path);
	painter.drawText(left+1,top-1,path);

	painter.drawText(left-1,top,path);
	painter.drawText(left+1,top,path);

	painter.drawText(left-1,top+1,path);
	painter.drawText(left,top+1,path);
	painter.drawText(left+1,top+1,path);
	painter.end();

	// Draw the text on a pixmap
	QPixmap pixmap(pathSize);
	painter.begin(&pixmap);
	painter.setFont(mPathLabel->font());

	painter.eraseRect(pixmap.rect());
	painter.setPen(black);
	painter.drawText(left,top,path);
	painter.end();

	// Update the path label
	mPathLabel->setPixmap(pixmap);
	mPathLabel->setMask(mask);
	mPathLabel->adjustSize();
}


void GVScrollPixmapView::updateZoomActions() {
	// Disable most actions if there's no image
	if (mDocument->isNull()) {
		mZoomIn->setEnabled(false);
		mZoomOut->setEnabled(false);
		mResetZoom->setEnabled(false);
		return;
	}

	mAutoZoom->setEnabled(true);
	mResetZoom->setEnabled(true);

	if (mAutoZoom->isChecked()) {
		mZoomIn->setEnabled(true);
		mZoomOut->setEnabled(true);
	} else {
		mZoomIn->setEnabled(mZoom<MAX_ZOOM);
		mZoomOut->setEnabled(mZoom>1/MAX_ZOOM);
	}
}


//------------------------------------------------------------------------
//
// File operations
//
//------------------------------------------------------------------------
void GVScrollPixmapView::showFileProperties() {
	(void)new KPropertiesDialog(mDocument->url());
}


void GVScrollPixmapView::renameFile() {
	FileOperation::rename(mDocument->url(),this);
}


void GVScrollPixmapView::copyFile() {
	KURL::List list;
	list << mDocument->url();
	FileOperation::copyTo(list,this);
}


void GVScrollPixmapView::moveFile() {
	KURL::List list;
	list << mDocument->url();
	FileOperation::moveTo(list,this);
}


void GVScrollPixmapView::deleteFile() {
	KURL::List list;
	list << mDocument->url();
	FileOperation::del(list,this);
}

KURL GVScrollPixmapView::pixmapURL() {
	return mDocument->url();
}

//------------------------------------------------------------------------
//
// Config
//
//------------------------------------------------------------------------
void GVScrollPixmapView::readConfig(KConfig* config, const QString& group) {
	config->setGroup(group);
	mShowPathInFullScreen=config->readBoolEntry(CONFIG_SHOW_PATH,true);
	int smooth = config->readNumEntry(CONFIG_SMOOTH_SCALE,SMOOTH_NORMAL2);
	if( smooth >= SMOOTH_NONE && smooth <= SMOOTH_BEST2 ) {
		mSmoothScale = static_cast< SmoothScale >( smooth );
	} else {
		mSmoothScale = SMOOTH_NORMAL2;
	}
	if( config->readEntry(CONFIG_SMOOTH_SCALE) == "true" ) {// backwards comp.
		mSmoothScale = SMOOTH_NORMAL2;
	}

	mEnlargeSmallImages=config->readBoolEntry(CONFIG_ENLARGE_SMALL_IMAGES,false);
	mShowScrollBars=config->readBoolEntry(CONFIG_SHOW_SCROLL_BARS,true);
	mMouseWheelScroll=config->readBoolEntry(CONFIG_MOUSE_WHEEL_SCROLL, true);
	mAutoZoom->setChecked(config->readBoolEntry(CONFIG_AUTO_ZOOM,false));
	updateScrollBarMode();
	mLockZoom->setChecked(config->readBoolEntry(CONFIG_LOCK_ZOOM,false));

	mButtonStateToolMap[NoButton]=SCROLL;
	mButtonStateToolMap[ShiftButton]=ZOOM;

	mTool=mButtonStateToolMap[NoButton];
	mToolControllers[mTool]->updateCursor();
}

void GVScrollPixmapView::kpartConfig() {
	mShowPathInFullScreen=true;
	mSmoothScale=SMOOTH_NORMAL2;
	mEnlargeSmallImages=false;
	mShowScrollBars=true;
	mMouseWheelScroll=true;
	mAutoZoom->setChecked(true);
	updateScrollBarMode();
	mLockZoom->setChecked(false);

	mButtonStateToolMap[NoButton]=SCROLL;
	mButtonStateToolMap[ShiftButton]=ZOOM;

	mTool=mButtonStateToolMap[NoButton];
	mToolControllers[mTool]->updateCursor();
}


void GVScrollPixmapView::writeConfig(KConfig* config, const QString& group) const {
	config->setGroup(group);
	config->writeEntry(CONFIG_SHOW_PATH,mShowPathInFullScreen);
	config->writeEntry(CONFIG_SMOOTH_SCALE,mSmoothScale);
	config->writeEntry(CONFIG_ENLARGE_SMALL_IMAGES,mEnlargeSmallImages);
	config->writeEntry(CONFIG_SHOW_SCROLL_BARS,mShowScrollBars);
	config->writeEntry(CONFIG_MOUSE_WHEEL_SCROLL,mMouseWheelScroll);
	config->writeEntry(CONFIG_AUTO_ZOOM,mAutoZoom->isChecked());
	config->writeEntry(CONFIG_LOCK_ZOOM,mLockZoom->isChecked());
}

