/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2003 Aurélien Gâteau

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

// KDE includes
#include <kaction.h>
#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>
#include <kpropsdlg.h>
#include <kstdaction.h>

// Our includes
#include "fileoperation.h"
#include "gvpixmap.h"

#include "gvscrollpixmapview.moc"

const char* CONFIG_SHOW_PATH="show path";
const char* CONFIG_SMOOTH_SCALE="smooth scale";
const char* CONFIG_ENLARGE_SMALL_IMAGES="enlarge small images";
const char* CONFIG_SHOW_SCROLL_BARS="show scroll bars";
const char* CONFIG_LOCK_ZOOM="lock zoom";
const char* CONFIG_AUTO_ZOOM="auto zoom";
const char* CONFIG_WHEEL_BEHAVIOUR_NONE=   "wheel behaviour none";
const char* CONFIG_WHEEL_BEHAVIOUR_SHIFT=  "wheel behaviour shift";
const char* CONFIG_WHEEL_BEHAVIOUR_CONTROL="wheel behaviour control";
const char* CONFIG_WHEEL_BEHAVIOUR_ALT=    "wheel behaviour alt";

const int AUTO_HIDE_TIMEOUT=1000;

const double MAX_ZOOM=16.0; // Same value as GIMP

GVScrollPixmapView::GVScrollPixmapView(QWidget* parent,GVPixmap* pixmap,KActionCollection* actionCollection)
: QScrollView(parent,0L,WResizeNoErase|WRepaintNoErase)
, mGVPixmap(pixmap)
, mAutoHideTimer(new QTimer(this))
, mPathLabel(new QLabel(parent))
, mXOffset(0),mYOffset(0)
, mZoom(1)
, mouseZoomAction(false)
, menu(NULL)
, mActionCollection(actionCollection)
, mFullScreen(false)
, mMouseToggleZoom(false)
, mDragStarted(false)
, mOperaLikePrevious(false)
, mLastZoomBeforeAuto(1)
{
	setFocusPolicy(StrongFocus);
	setFrameStyle(NoFrame);
	
	// Init path label
	mPathLabel->setBackgroundColor(white);
	QFont font=mPathLabel->font();
	font.setWeight(QFont::Bold);
	mPathLabel->setFont(font);
	mPathLabel->hide();

	// Create actions
	mAutoZoom=new KToggleAction(i18n("&Auto Zoom"),"autozoom",0,mActionCollection,"view_zoom_auto");
	connect(mAutoZoom,SIGNAL(toggled(bool)),
		this,SLOT(setAutoZoom(bool)) );

	mZoomIn=KStdAction::zoomIn(this,SLOT(slotZoomIn()),mActionCollection);
	mZoomIn->setIcon("view_zoom_in");
	
	mZoomOut=KStdAction::zoomOut(this,SLOT(slotZoomOut()),mActionCollection);
	mZoomOut->setIcon("view_zoom_out");
	
	mResetZoom=KStdAction::actualSize(this,SLOT(slotResetZoom()),mActionCollection);
	mResetZoom->setIcon("view_zoom_actual_size");

	mLockZoom=new KToggleAction(i18n("&Lock Zoom"),"lockzoom",0,mActionCollection,"view_zoom_lock");

	// Connect to some interesting signals
	connect(mGVPixmap,SIGNAL(urlChanged(const KURL&,const QString&)),
		this,SLOT(slotURLChanged()) );

	connect(mGVPixmap,SIGNAL(modified()),
		this,SLOT(slotModified()) );
	
	connect(mAutoHideTimer,SIGNAL(timeout()),
		this,SLOT(hideCursor()) );
}


void GVScrollPixmapView::slotURLChanged() {
	mXOffset=0;
	mYOffset=0;

	if (mGVPixmap->isNull()) {
		resizeContents(0,0);
		viewport()->repaint(false);
		updateZoomActions();
		return;
	}

	if (mAutoZoom->isChecked()) {
		setZoom(computeAutoZoom());
	} else {
		if (mLockZoom->isChecked()) {
			updateContentSize();
			updateImageOffset();
			updateZoomActions();
			viewport()->repaint(false);
		} else {
			setZoom(1.0);
		}
		horizontalScrollBar()->setValue(0);
		verticalScrollBar()->setValue(0);
	}
	
	if (mFullScreen && mShowPathInFullScreen) updatePathLabel();
}


void GVScrollPixmapView::slotModified() {
	updateContentSize();
	updateImageOffset();
	updateZoomActions();
	viewport()->repaint(false);
}


//------------------------------------------------------------------------
//
// Properties
// 
//------------------------------------------------------------------------
void GVScrollPixmapView::setSmoothScale(bool value) {
	mSmoothScale=value;
	viewport()->update();
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


void GVScrollPixmapView::setZoom(double zoom) {
	mZoom=zoom;

	viewport()->setUpdatesEnabled(false);
	
	updateContentSize();

	if (!mAutoZoom->isChecked()) {
		// Find the coordinate of the center of the image
		// and center the view on it
		if (!mouseZoomAction) {
			mPictX = mGVPixmap->width()/2;
			mPictY = mGVPixmap->height()/2;
			mViewX = visibleWidth()/2;
			mViewY = visibleHeight()/2;
		}
		int centerX=int(mZoom*mPictX+visibleWidth()/2.0)-mViewX;
		int centerY=int(mZoom*mPictY+visibleHeight()/2.0)-mViewY;
		center(centerX,centerY);
	}
	
	updateImageOffset();
	updateZoomActions();
	
	viewport()->setUpdatesEnabled(true);
	viewport()->repaint(false);
	
	emit zoomChanged(mZoom);
}


void GVScrollPixmapView::setFullScreen(bool fullScreen) {
	mFullScreen=fullScreen;
	viewport()->setMouseTracking(mFullScreen);
	if (mFullScreen) {
		viewport()->setBackgroundColor(black);
		viewport()->setCursor(blankCursor);
	} else {
		viewport()->setBackgroundMode(PaletteDark);
		mAutoHideTimer->stop();
		viewport()->unsetCursor();
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
	#ifdef DEBUG_RECTS
	static QColor colors[4]={QColor(255,0,0),QColor(0,255,0),QColor(0,0,255),QColor(255,255,0) };
	static int numColor=0;
	#endif

	
	if (mGVPixmap->isNull()) {
		painter->eraseRect(clipx,clipy,clipw,cliph);
		return;
	}

	QRect updateRect=QRect(clipx,clipy,clipw,cliph);
	// If we are enlarging the image, we grow the update rect so that it
	// contains full zoomed image pixels. We also take one more image pixel to
	// avoid gaps in images.
	if (mZoom>1.0) {
		updateRect.setLeft	( roundDown(updateRect.left(),	mZoom)-int(mZoom) );
		updateRect.setTop	( roundDown(updateRect.top(),	mZoom)-int(mZoom) );
		updateRect.setRight ( roundUp(	updateRect.right(), mZoom)+int(mZoom)-1 );
		updateRect.setBottom( roundUp(	updateRect.bottom(),mZoom)+int(mZoom)-1 );
	}
	
	QRect zoomedImageRect=QRect(mXOffset, mYOffset, int(mGVPixmap->width()*mZoom), int(mGVPixmap->height()*mZoom));
	updateRect=updateRect.intersect(zoomedImageRect);
	
	if (updateRect.isEmpty()) return;

	QImage image(updateRect.size()/mZoom,32);
	image=mGVPixmap->image().copy(
		int(updateRect.x()/mZoom) - int(mXOffset/mZoom), int(updateRect.y()/mZoom) - int(mYOffset/mZoom),
		int(updateRect.width()/mZoom), int(updateRect.height()/mZoom) );

	if (mSmoothScale) {
		image=image.smoothScale(updateRect.size());
	} else {
		image=image.scale(updateRect.size());
	}

	if (image.hasAlphaBuffer()) {
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
	if (event->button()==RightButton) return;
	
	mScrollStartX=event->x();
	mScrollStartY=event->y();
	mMouseToggleZoom=true;
}


void GVScrollPixmapView::viewportMouseMoveEvent(QMouseEvent* event) {
	if (mMouseToggleZoom) {
		mMouseToggleZoom=false;
		if (!mAutoZoom->isChecked()) {
			mDragStarted=true;
			viewport()->setCursor(QCursor(SizeAllCursor));
		}
	}
	if (mFullScreen && !mDragStarted) {
		viewport()->unsetCursor();
		mAutoHideTimer->start(AUTO_HIDE_TIMEOUT,true);
	}
	
	if (!mDragStarted) {
		return;
	}

	int deltaX,deltaY;

	deltaX=mScrollStartX - event->x();
	deltaY=mScrollStartY - event->y();

	mScrollStartX=event->x();
	mScrollStartY=event->y();
	scrollBy(deltaX,deltaY);
}
void GVScrollPixmapView::viewportMouseReleaseEvent(QMouseEvent* event) {
	// If the mouse was released for toggling the zoom, end here
	if (mMouseToggleZoom && event->button() == Qt::LeftButton
		// do not interfere with opera gestures
		&& !(event->stateAfter() & Qt::RightButton)
		// do not zoom when 
		&& (!menu || !menu->isVisible())) {
		mMouseToggleZoom=false;
		// switch zoom
		if (mAutoZoom->isChecked()) {
			mPictX = (event->x()-mXOffset)/mZoom;
			mPictY = (event->y()-mYOffset)/mZoom;
			mViewX = visibleWidth()/2;
			mViewY = visibleHeight()/2;
			mouseZoomAction = true;
			mAutoZoom->setChecked(false);
			setAutoZoom(false);
			mouseZoomAction = false;
		} else {
			mAutoZoom->setChecked(true);
			setAutoZoom(true);
		}
		return;
	}
		
	// If the mouse was released because of the end of a drag, end here
	if (mDragStarted) {
		mDragStarted=false;
		if (mFullScreen) {
			viewport()->unsetCursor();
			mAutoHideTimer->start(AUTO_HIDE_TIMEOUT,true);
		} else {
			viewport()->unsetCursor();
		}
		return;
	}
	
	switch (event->button()) {
	case Qt::LeftButton:
		if (event->stateAfter() & Qt::RightButton) {
			mOperaLikePrevious=true;
			emit selectPrevious();
			return;
		}
		break;
			
	case Qt::RightButton:
		if (event->stateAfter() & Qt::LeftButton) {
			emit selectNext();
			return;
		}

		if (mOperaLikePrevious) { // Avoid showing the popup menu after Opera like previous
			mOperaLikePrevious=false;
		} else {
			if (!(mGVPixmap->isNull() && !mFullScreen)) {
				makeContextMenu();
				menu->exec(event->globalPos());
				return;
			}
		}
		break;
	
	default: // Avoid compiler complain
		break;
	}
}


void GVScrollPixmapView::wheelEvent(QWheelEvent* event) {
	// If auto zoom is on, we always browse
	if (mAutoZoom->isChecked()) {
		if (event->delta()<0) {
			emit selectNext();
		} else {
			emit selectPrevious();
		}
		event->accept();
		return;
	}

	// No auto zoom, use  mWheelBehaviours
	ButtonState modifier=ButtonState( event->state() & (ShiftButton | ControlButton | AltButton) );
	switch (mWheelBehaviours[modifier]) {
	case Browse:
		if (event->delta()<0) {
			emit selectNext();
		} else {
			emit selectPrevious();
		}
		event->accept();
		break;

	case Scroll:
		QScrollView::wheelEvent(event);
		break;

	case Zoom:
		// only record the view and pict variables once per action,
		// so that they are reversible
		if (lastWheelZoomTime.isNull()
				|| lastWheelZoomTime.elapsed() > 1000) {
			mViewX = event->x();
			mViewY = event->y();
			mPictX = double(mViewX-mXOffset)/mZoom;
			mPictY = double(mViewY-mYOffset)/mZoom;
		}
		lastWheelZoomTime.start();
		mouseZoomAction = true;
		if (event->delta()<0) {
			if (mZoomIn->isEnabled()) mZoomIn->activate();
		} else {
			if (mZoomOut->isEnabled()) mZoomOut->activate();
		}
		mouseZoomAction = true;
		event->accept();
		break;

	case None:
		event->accept();
		break;
	}
}

//------------------------------------------------------------------------
//
// Slots
//
//------------------------------------------------------------------------
void GVScrollPixmapView::slotZoomIn() {
	double newZoom;
	if (mZoom>=1.0) {
		newZoom=floor(mZoom)+1.0;
	} else {
		newZoom=1/( ceil(1/mZoom)-1.0 );
	}
	if (mAutoZoom->isChecked()) {
		mAutoZoom->setChecked(false);
		updateScrollBarMode();
	}
	setZoom(newZoom);
}


void GVScrollPixmapView::slotZoomOut() {
	double newZoom;
	if (mZoom>1.0) {
		newZoom=ceil(mZoom)-1.0;
	} else {
		newZoom=1/( floor(1/mZoom)+1.0 );
	}
	if (mAutoZoom->isChecked()) {
		mAutoZoom->setChecked(false);
		updateScrollBarMode();
	}
	setZoom(newZoom);
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
		mLastZoomBeforeAuto=mZoom;
		setZoom(computeAutoZoom());
	} else {
		setZoom(mLastZoomBeforeAuto);
	}
}


//------------------------------------------------------------------------
//
// Private
//
//------------------------------------------------------------------------
void GVScrollPixmapView::makeContextMenu() {
	if (menu) return;

	menu = new QPopupMenu(this);

	mActionCollection->action("view_fullscreen")->plug(menu);
	
	if (!mGVPixmap->isNull()) {
		menu->insertSeparator();
		
		mAutoZoom->plug(menu);
		mZoomIn->plug(menu);
		mZoomOut->plug(menu);
		mResetZoom->plug(menu);
		mLockZoom->plug(menu);

		menu->insertSeparator();
		
		mActionCollection->action("first")->plug(menu);
		mActionCollection->action("previous")->plug(menu);
		mActionCollection->action("next")->plug(menu);
		mActionCollection->action("last")->plug(menu);
		
		menu->insertSeparator();
		
		QPopupMenu* editMenu=new QPopupMenu(menu);
		mActionCollection->action("edit_rotate_left")->plug(editMenu);
		mActionCollection->action("edit_rotate_right")->plug(editMenu);
		mActionCollection->action("edit_mirror")->plug(editMenu);
		mActionCollection->action("edit_flip")->plug(editMenu);
		editMenu->connectItem(
			editMenu->insertItem( i18n("Open with &Editor") ),
			this,SLOT(openWithEditor()) );
			
		menu->insertItem( i18n("Edit"), editMenu );
		
		menu->insertSeparator();
		
		menu->connectItem(
			menu->insertItem( i18n("&Rename...") ),
			this,SLOT(renameFile()) );
		menu->connectItem(
			menu->insertItem( i18n("&Copy To...") ),
			this,SLOT(copyFile()) );
		menu->connectItem(
			menu->insertItem( i18n("&Move To...") ),
			this,SLOT(moveFile()) );
		menu->connectItem(
			menu->insertItem( i18n("&Delete") ),
			this,SLOT(deleteFile()) );
		
		menu->insertSeparator();
		
		menu->connectItem(
			menu->insertItem( i18n("Properties") ),
			this,SLOT(showFileProperties()) );
	}
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
		int(mGVPixmap->width()*mZoom), 
		int(mGVPixmap->height()*mZoom)	);
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

double GVScrollPixmapView::computeAutoZoom() {

	if (mGVPixmap->isNull()) {
		return 1.0;
	}
	QSize size=mGVPixmap->image().size();
	
#if QT_VERSION>=0x030100
	size.scale(width(),height(),QSize::ScaleMin);
#else
	sizeScaleMin(&size,width(),height());
#endif
	
	double zoom=double(size.width())/mGVPixmap->width();
	if (zoom>1.0 && !mEnlargeSmallImages) return 1.0;
	return zoom;
}


void GVScrollPixmapView::updateImageOffset() {
	int viewWidth=width();
	int viewHeight=height();
	
	// Compute mXOffset and mYOffset in case the image does not fit
	// the view width or height
	int zpixWidth=int(mGVPixmap->width() * mZoom);
	int zpixHeight=int(mGVPixmap->height() * mZoom);

	if (zpixWidth>viewWidth) {
		viewHeight-=horizontalScrollBar()->height();
	}
	if (zpixHeight>viewHeight) {
		viewWidth-=verticalScrollBar()->width();
	}
	
	mXOffset=QMAX(0,(viewWidth-zpixWidth)/2);
	mYOffset=QMAX(0,(viewHeight-zpixHeight)/2);
}


void GVScrollPixmapView::hideCursor() {
	viewport()->setCursor(blankCursor);
}


void GVScrollPixmapView::updatePathLabel() {
	QString path=mGVPixmap->url().path();
	
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
	if (mGVPixmap->isNull()) {
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
	(void)new KPropertiesDialog(mGVPixmap->url());
}


void GVScrollPixmapView::openWithEditor() {
	FileOperation::openWithEditor(mGVPixmap->url());
}


void GVScrollPixmapView::renameFile() {
	FileOperation::rename(mGVPixmap->url(),this);
}


void GVScrollPixmapView::copyFile() {
	KURL::List list;
	list << mGVPixmap->url();
	FileOperation::copyTo(list,this);
}


void GVScrollPixmapView::moveFile() {
	KURL::List list;
	list << mGVPixmap->url();
	FileOperation::moveTo(list,this);
}


void GVScrollPixmapView::deleteFile() {
	KURL::List list;
	list << mGVPixmap->url();
	FileOperation::del(list,this);
}


//------------------------------------------------------------------------
//
// Config
//
//------------------------------------------------------------------------
void GVScrollPixmapView::readConfig(KConfig* config, const QString& group) {
	config->setGroup(group);
	mShowPathInFullScreen=config->readBoolEntry(CONFIG_SHOW_PATH,true);
	mSmoothScale=config->readBoolEntry(CONFIG_SMOOTH_SCALE,false);
	mEnlargeSmallImages=config->readBoolEntry(CONFIG_ENLARGE_SMALL_IMAGES,false);
	mShowScrollBars=config->readBoolEntry(CONFIG_SHOW_SCROLL_BARS,true);
	mAutoZoom->setChecked(config->readBoolEntry(CONFIG_AUTO_ZOOM,false));
	updateScrollBarMode();
	mLockZoom->setChecked(config->readBoolEntry(CONFIG_LOCK_ZOOM,false));

	mWheelBehaviours[NoButton]=		WheelBehaviour( config->readNumEntry(CONFIG_WHEEL_BEHAVIOUR_NONE,Scroll) );
	mWheelBehaviours[ControlButton]=WheelBehaviour( config->readNumEntry(CONFIG_WHEEL_BEHAVIOUR_CONTROL,Zoom) );
	mWheelBehaviours[ShiftButton]=	WheelBehaviour( config->readNumEntry(CONFIG_WHEEL_BEHAVIOUR_SHIFT,Browse) );
	mWheelBehaviours[AltButton]=	WheelBehaviour( config->readNumEntry(CONFIG_WHEEL_BEHAVIOUR_ALT,None) );
}


void GVScrollPixmapView::writeConfig(KConfig* config, const QString& group) const {
	config->setGroup(group);
	config->writeEntry(CONFIG_SHOW_PATH,mShowPathInFullScreen);
	config->writeEntry(CONFIG_SMOOTH_SCALE,mSmoothScale);
	config->writeEntry(CONFIG_ENLARGE_SMALL_IMAGES,mEnlargeSmallImages);
	config->writeEntry(CONFIG_SHOW_SCROLL_BARS,mShowScrollBars);
	config->writeEntry(CONFIG_AUTO_ZOOM,mAutoZoom->isChecked());
	config->writeEntry(CONFIG_LOCK_ZOOM,mLockZoom->isChecked());
	
	config->writeEntry(CONFIG_WHEEL_BEHAVIOUR_NONE	 , int(mWheelBehaviours[NoButton]) );
	config->writeEntry(CONFIG_WHEEL_BEHAVIOUR_CONTROL, int(mWheelBehaviours[ControlButton]) );
	config->writeEntry(CONFIG_WHEEL_BEHAVIOUR_SHIFT  , int(mWheelBehaviours[ShiftButton]) );
	config->writeEntry(CONFIG_WHEEL_BEHAVIOUR_ALT	 , int(mWheelBehaviours[AltButton]) );
}

