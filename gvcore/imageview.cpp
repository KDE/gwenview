// vim:set tabstop=4 shiftwidth=4 noexpandtab:
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

#include "config.h"

#include "imageview.moc"

#include <assert.h>
#include <math.h>

// Qt 
#include <qcolor.h>
#include <qcombobox.h>
#include <qcursor.h>
#include <qdatetime.h>
#include <qevent.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qvaluevector.h>

// KDE 
#include <kaction.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdeversion.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kstdaction.h>
#include <kurldrag.h>
#include <kapplication.h>

// Local
#include "document.h"
#include "imageutils/imageutils.h"
#include "bcgdialog.h"
#include "busylevelmanager.h"
#include "imageviewtools.h"
#include "imageutils/croppedqimage.h"
#include "imageviewconfig.h"

namespace Gwenview {

/*

Coordinates:

The image can be zoomed, can have a position offset, and additionally there is
QScrollView's viewport. This means there are several coordinate systems.



Let's start from simple things. Viewport ignored, zoom ignored:

 A-----------------------------------
 |                                  |
 |                                  |
 |     B---------------------       |
 |     |                    |       |
 |     |                    |       |
 |     |                    |       |
 |     |                    |       |
 |     |                    |       |
 |     |                    |       |
 |     ---------------------C       |
 |                                  |
 |                                  |
 ------------------------------------


The inner rectangle is the image, outer rectangle is the widget.
A = [ 0, 0 ]
B = [ mXOffset, mYOffset ]
C = B + [ mDocument->width(), mDocument->height() ]



The same, additionally the image is zoomed.

A = [ 0, 0 ]
B = [ mXOffset, mYOffset ]
C = [ mZoom * mDocument->width(), mZoom * mDocument->height()) ]

The groups of functions imageToWidget() and widgetToImage() do conversions
between the image and widget coordinates, i.e. imageToWidget() accepts coordinates
in the image (original,not zoomed,image's topleft corner is [0,0]) and returns
coordinates in the picture above, widgetToImage() works the other way around.

There's no bounds checking, so widgetToImage( A ) in the example above would
return image coordinate with negative x,y.

The widgetToImage() functions round the values (in order to have the conversion
as approximate as possible). However when converting from widget to image and back
this can result in the final rectangle being smaller than the original.
The widgetToImageBounding() function converts from widget to image coordinates
in a way which makes sure the reverse conversion will be at least as large
as the original geometry.

There are no conversion functions for only width/height, as their conversion
depends on the position (because of the rounding etc.). For similar reasons
conversions should not be done with the bottomright corner of a rectangle,
but with the point next to it.



For conversions from/to QScrollView's viewport, usually QScrollView methods should
be used: contentsX(), contentsY(), contentsWidth(), contentsHeight(), visibleWidth(),
visibleHeight(), contentsToViewport() and viewportToContents().

*/

const double MAX_ZOOM=16.0; // Same value as GIMP

const int DEFAULT_MAX_REPAINT_SIZE = 10000;
const int LIMIT_MAX_REPAINT_SIZE = 10000000;

#ifndef HAVE_LROUND
inline
long int lround( double x ) {
	return static_cast< long int >( x >= 0 ? x + 0.5 : x - 0.5 );
}
#endif


struct ImageView::Private {
	Document* mDocument;
	
	Tools mTools;

	ToolID mToolID;

	// Offset to center images
	int mXOffset, mYOffset;

	// Zoom info
	ZoomMode mZoomMode;
	double mZoom;

	// Gamma, brightness, contrast - multiplied by 100
	int mGamma, mBrightness, mContrast;

	// Our actions
	QComboBox* mZoomCombo;
	// We do not use KSelectAction because it's not possible to set the combo text
	KWidgetAction* mZoomComboAction;
	KToggleAction* mZoomToFit;
	KToggleAction* mZoomToWidth;
	KToggleAction* mZoomToHeight;
	QValueVector<KToggleAction*> mZoomComboActions;
	KAction* mZoomIn;
	KAction* mZoomOut;
	KAction* mResetZoom;
	KToggleAction* mLockZoom;
	KAction* mAdjustBCG;
	KAction* mIncreaseGamma;
	KAction* mDecreaseGamma;
	KAction* mIncreaseBrightness;
	KAction* mDecreaseBrightness;
	KAction* mIncreaseContrast;
	KAction* mDecreaseContrast;
	KActionCollection* mActionCollection;
	BCGDialog* mBCGDialog;

	// Fullscreen stuff
	bool mFullScreen;
	
	// Object state info
	bool mOperaLikePrevious; // Flag to avoid showing the popup menu on Opera like previous
	double mZoomBeforeAuto;
	int mXCenterBeforeAuto, mYCenterBeforeAuto;
	
	QMap< long long, PendingPaint > mPendingPaints;
	QRegion mPendingNormalRegion;
	QRegion mPendingSmoothRegion;
	int mPendingOperations;
	QTimer mPendingPaintTimer;
	bool mSmoothingSuspended;
	QRegion mValidImageArea;

	int imageToWidgetX( int x ) const {
		if( mZoom == 1.0 ) return x + mXOffset;
		return lround( x * mZoom ) + mXOffset;
	}

	int imageToWidgetY( int y ) const {
		if( mZoom == 1.0 ) return y + mYOffset;
		return lround( y * mZoom ) + mYOffset;
	}

	QPoint imageToWidget( const QPoint& p ) const {
		return QPoint( imageToWidgetX( p.x()), imageToWidgetY( p.y()));
	}

	QRect imageToWidget( const QRect& r ) const {
		return QRect( imageToWidget( r.topLeft()),
			// don't use bottomright corner for conversion, but the one next to it
			imageToWidget( r.bottomRight() + QPoint( 1, 1 )) - QPoint( 1, 1 ));
	}

	int widgetToImageX( int x ) const {
		if( mZoom == 1.0 ) return x - mXOffset;
		return lround( ( x - mXOffset ) / mZoom );
	}

	int widgetToImageY( int y ) const {
		if( mZoom == 1.0 ) return y - mYOffset;
		return lround( ( y - mYOffset ) / mZoom );
	}

	QPoint widgetToImage( const QPoint& p ) const {
		return QPoint( widgetToImageX( p.x()), widgetToImageY( p.y()));
	}

	QRect widgetToImage( const QRect& r ) const {
		return QRect( widgetToImage( r.topLeft()),
			// don't use bottomright corner for conversion, but the one next to it
			widgetToImage( r.bottomRight() + QPoint( 1, 1 )) - QPoint( 1, 1 ));
	}

	QRect widgetToImageBounding( const QRect& r, int extra ) const {
		QRect ret = widgetToImage( r );
		// make sure converting to image and back always returns QRect at least as large as 'r'
		extra += mZoom == 1.0 ? 0 : int( ceil( 1 / mZoom )); 
		ret.addCoords( -extra, -extra, extra, extra );
		return ret;
	}

	void initZoomCombo() {
		mZoomCombo->clear();
		for (QValueVector<KToggleAction*>::iterator it=mZoomComboActions.begin();
			it!=mZoomComboActions.end();
			++it)
		{
			QString txt=(*it)->plainText();
			mZoomCombo->insertItem(txt);
		}
		
		const double zoomValues[] = { 0.5, 1, 2 };
		int nbValues=sizeof(zoomValues) / sizeof(double);
		for (int pos=0; pos<nbValues; ++pos) {
			QString txt=QString("%1%").arg( int(zoomValues[pos]*100) );
			mZoomCombo->insertItem(txt);
		}
	}
};


inline bool doDelayedSmoothing() {
	return ImageViewConfig::delayedSmoothing()
		&& ImageViewConfig::smoothAlgorithm()!=ImageUtils::SMOOTH_NONE;
}


class ImageView::EventFilter : public QObject {
public:
	EventFilter(ImageView* parent)
	: QObject(parent) {}
		
	bool eventFilter(QObject*, QEvent* event) {
		switch (event->type()) {
		case QEvent::KeyPress:
		case QEvent::KeyRelease:
		case QEvent::AccelOverride:
			return static_cast< ImageView* >( parent())
						->viewportKeyEvent(static_cast<QKeyEvent*>(event));
		default:
			break;
		}
		return false;
	}
};



ImageView::ImageView(QWidget* parent,Document* document, KActionCollection* actionCollection)
: QScrollView(parent,0L,WResizeNoErase|WRepaintNoErase|WPaintClever)
{
	d=new Private;
	d->mDocument=document;
	d->mToolID=SCROLL;
	d->mXOffset=0;
	d->mYOffset=0;
	d->mZoomMode=static_cast<ZoomMode>( ImageViewConfig::zoomMode() );
	d->mZoom=1;
	d->mActionCollection=actionCollection;
	d->mFullScreen=false;
	d->mOperaLikePrevious=false;
	d->mZoomBeforeAuto=1;
	d->mPendingOperations= 0 ;
	d->mSmoothingSuspended= false ;
	d->mGamma = 100;
	d->mBrightness = 0;
	d->mContrast = 100;
	d->mBCGDialog = 0;

	viewport()->setFocusPolicy(WheelFocus);
	setFrameStyle(NoFrame);
	setAcceptDrops( true );
	viewport()->setAcceptDrops( true );

	updateScrollBarMode();
	viewport()->setBackgroundColor(ImageViewConfig::backgroundColor() );

	d->mTools[SCROLL]=new ScrollTool(this);
	d->mTools[ZOOM]=new ZoomTool(this);
	d->mTools[d->mToolID]->updateCursor();

	// Create actions
	d->mZoomToFit=new KToggleAction(i18n("Fit to &Window"),"viewmagfit",0,d->mActionCollection,"view_zoom_to_fit");
	connect(d->mZoomToFit,SIGNAL(toggled(bool)),
		this,SLOT(setZoomToFit(bool)) );
	d->mZoomToWidth=new KToggleAction(i18n("Fit to &Width"),0,0,d->mActionCollection,"view_zoom_to_width");
	connect(d->mZoomToWidth,SIGNAL(toggled(bool)),
		this,SLOT(setZoomToWidth(bool)) );
	d->mZoomToHeight=new KToggleAction(i18n("Fit to &Height"),0,0,d->mActionCollection,"view_zoom_to_height");
	connect(d->mZoomToHeight,SIGNAL(toggled(bool)),
		this,SLOT(setZoomToHeight(bool)) );

	d->mZoomIn=KStdAction::zoomIn(this,SLOT(slotZoomIn()),d->mActionCollection);

	d->mZoomOut=KStdAction::zoomOut(this,SLOT(slotZoomOut()),d->mActionCollection);

	d->mResetZoom=KStdAction::actualSize(this,SLOT(slotResetZoom()),d->mActionCollection);
	d->mResetZoom->setIcon("viewmag1");

	d->mLockZoom=new KToggleAction(i18n("&Lock Zoom"),"lock",0,d->mActionCollection,"view_zoom_lock");
	d->mLockZoom->setChecked(ImageViewConfig::lockZoom());
	connect(d->mLockZoom,SIGNAL(toggled(bool)),
		this,SLOT(setLockZoom(bool)) );

	d->mZoomCombo=new QComboBox(true);
	// Avoid stealing focus
	d->mZoomCombo->setFocusPolicy(ClickFocus);
	connect(d->mZoomCombo, SIGNAL(activated(int)),
		this, SLOT(slotSelectZoom()) );
	
	d->mZoomComboAction=new KWidgetAction(d->mZoomCombo, i18n("Zoom"), 0, 0, 0, d->mActionCollection, "view_zoom_to");

	d->mZoomComboActions.append(d->mZoomToFit);
	d->mZoomComboActions.append(d->mZoomToWidth);
	d->mZoomComboActions.append(d->mZoomToHeight);
	if (d->mZoomMode!=ZOOM_FREE) {
		d->mZoomComboActions[d->mZoomMode]->setChecked(true);
	}
	d->initZoomCombo();

	d->mAdjustBCG=new KAction(i18n("Adjust Brightness/Contrast/Gamma"), "colorize", 0,
		this, SLOT(showBCGDialog()), d->mActionCollection, "adjust_bcg");
	d->mIncreaseGamma=new KAction(i18n("Increase Gamma"),0,CTRL+Key_G,
		this,SLOT(increaseGamma()),d->mActionCollection,"increase_gamma");
	d->mDecreaseGamma=new KAction(i18n("Decrease Gamma"),0,SHIFT+CTRL+Key_G,
		this,SLOT(decreaseGamma()),d->mActionCollection,"decrease_gamma");
	d->mIncreaseBrightness=new KAction(i18n("Increase Brightness" ),0,CTRL+Key_B,
		this,SLOT(increaseBrightness()),d->mActionCollection,"increase_brightness");
	d->mDecreaseBrightness=new KAction(i18n("Decrease Brightness" ),0,SHIFT+CTRL+Key_B,
		this,SLOT(decreaseBrightness()),d->mActionCollection,"decrease_brightness");
	d->mIncreaseContrast=new KAction(i18n("Increase Contrast" ),0,CTRL+Key_C,
		this,SLOT(increaseContrast()),d->mActionCollection,"increase_contrast");
	d->mDecreaseContrast=new KAction(i18n("Decrease Contrast" ),0,SHIFT+CTRL+Key_C,
		this,SLOT(decreaseContrast()),d->mActionCollection,"decrease_contrast");

	// Connect to some interesting signals
	connect(d->mDocument,SIGNAL(loaded(const KURL&)),
		this,SLOT(slotLoaded()) );

	connect(d->mDocument,SIGNAL(loading()),
		this,SLOT( loadingStarted()) );

	connect(d->mDocument,SIGNAL(modified()),
		this,SLOT(slotModified()) );

	connect(d->mDocument, SIGNAL(sizeUpdated(int, int)),
		this, SLOT(slotImageSizeUpdated()) );

	connect(d->mDocument, SIGNAL(rectUpdated(const QRect&)),
		this, SLOT(slotImageRectUpdated(const QRect&)) );

	connect(&d->mPendingPaintTimer,SIGNAL(timeout()),
		this,SLOT(checkPendingOperations()) );

	connect(BusyLevelManager::instance(),SIGNAL(busyLevelChanged(BusyLevel)),
		this,SLOT(slotBusyLevelChanged(BusyLevel) ));

	// This event filter is here to make sure the pixmap view is aware of the changes
	// in the keyboard modifiers, even if it isn't focused. However, making this widget
	// itself the filter would lead to doubled paint events, because QScrollView
	// installs an event filter on its viewport, and doesn't filter out the paint
	// events -> it'd get it twice, first from app filter, second from viewport filter.
	EventFilter* filter=new EventFilter(this);
	kapp->installEventFilter(filter);
}


ImageView::~ImageView() {
	ImageViewConfig::setZoomMode(d->mZoomMode);
	ImageViewConfig::setLockZoom(d->mLockZoom->isChecked());
	ImageViewConfig::self()->writeConfig();
	delete d->mTools[SCROLL];
	delete d->mTools[ZOOM];
	delete d;
}


void ImageView::slotLoaded() {
	if (d->mDocument->isNull()) {
		resizeContents(0,0);
		viewport()->repaint(false);
		return;
	}

	if (doDelayedSmoothing()) scheduleOperation( SMOOTH_PASS );
}


void ImageView::slotModified() {
	if (d->mZoomMode!=ZOOM_FREE) {
		updateZoom(d->mZoomMode);
	} else {
		updateContentSize();
		updateImageOffset();
		updateZoomActions();
		fullRepaint();
	}
}


void ImageView::loadingStarted() {
	cancelPending();
	d->mSmoothingSuspended = true;
	d->mValidImageArea = QRegion();
	d->mGamma = 100;
	d->mBrightness = 0;
	d->mContrast = 100;
}

//------------------------------------------------------------------------
//
// Properties
//
//------------------------------------------------------------------------
double ImageView::zoom() const {
	return d->mZoom;
}


bool ImageView::fullScreen() const {
	return d->mFullScreen;
}


QPoint ImageView::offset() const {
	return QPoint(d->mXOffset, d->mYOffset);
}
	

bool ImageView::canZoom(bool in) const {
	KAction* zoomAction=in ? d->mZoomIn : d->mZoomOut;
	return zoomAction->isEnabled();
}


KToggleAction* ImageView::zoomToFit() const {
	return d->mZoomToFit;
}


void ImageView::updateFromSettings() {
	// Reset, so that next repaint doesn't possibly take longer because of
	// smoothing
	ImageViewConfig::setMaxRepaintSize(DEFAULT_MAX_REPAINT_SIZE);
	ImageViewConfig::setMaxScaleRepaintSize(DEFAULT_MAX_REPAINT_SIZE);
	ImageViewConfig::setMaxSmoothRepaintSize(DEFAULT_MAX_REPAINT_SIZE);
	
	if( doDelayedSmoothing() ) {
		scheduleOperation( SMOOTH_PASS );
	} else {
		fullRepaint();
	}
	
	// If enlargeSmallImage changed
	if (d->mZoomMode!=ZOOM_FREE) {
		updateZoom(d->mZoomMode);
	}
	
	updateScrollBarMode();
	
	if (!d->mFullScreen) {
		viewport()->setBackgroundColor(ImageViewConfig::backgroundColor() );
	}
}


void ImageView::setZoom(double zoom, int centerX, int centerY) {
	updateZoom(ZOOM_FREE, zoom, centerX, centerY);
}


void ImageView::updateZoom(ZoomMode zoomMode, double value, int centerX, int centerY) {
	double oldZoom=d->mZoom;
	d->mZoomMode=zoomMode;
	KAction* checkedZoomAction=0;
	
	viewport()->setUpdatesEnabled(false);
	
	if (zoomMode==ZOOM_FREE) {
		Q_ASSERT(value!=0);
		d->mZoom=value;
	} else {
		d->mZoomBeforeAuto=d->mZoom;
		d->mXCenterBeforeAuto=width()/2  + contentsX() + d->mXOffset;
		d->mYCenterBeforeAuto=height()/2 + contentsY() + d->mYOffset;
		
		if (zoomMode==ZOOM_FIT) {
			d->mZoom=computeZoomToFit();
			checkedZoomAction=d->mZoomToFit;
			
		} else if (zoomMode==ZOOM_FIT_WIDTH) {
			d->mZoom=computeZoomToWidth();
			checkedZoomAction=d->mZoomToWidth;
			
		} else {
			d->mZoom=computeZoomToHeight();
			checkedZoomAction=d->mZoomToHeight;
		}
	}

	// Make sure only one zoom action is toggled on
	d->mZoomToFit->setChecked(   checkedZoomAction==d->mZoomToFit);
	d->mZoomToWidth->setChecked( checkedZoomAction==d->mZoomToWidth);
	d->mZoomToHeight->setChecked(checkedZoomAction==d->mZoomToHeight);
	
	updateContentSize();
	
	// Find the coordinate of the center of the image
	// and center the view on it
	if (centerX==-1) {
		centerX=int( ((visibleWidth()/2+contentsX()-d->mXOffset)/oldZoom)*d->mZoom );
	}
	if (centerY==-1) {
		centerY=int( ((visibleHeight()/2+contentsY()-d->mYOffset)/oldZoom)*d->mZoom );
	}
	center(centerX,centerY);
	
	updateScrollBarMode();
	updateImageOffset();
	updateZoomActions();

	viewport()->setUpdatesEnabled(true);
	fullRepaint();
}


void ImageView::setFullScreen(bool fullScreen) {
	d->mFullScreen=fullScreen;

	if (d->mFullScreen) {
		viewport()->setBackgroundColor(black);
	} else {
		viewport()->setBackgroundColor(ImageViewConfig::backgroundColor() );
	}
}


//------------------------------------------------------------------------
//
// Overloaded methods
//
//------------------------------------------------------------------------
void ImageView::resizeEvent(QResizeEvent* event) {
	QScrollView::resizeEvent(event);
	if (d->mZoomMode!=ZOOM_FREE) {
		updateZoom(d->mZoomMode);
	} else {
		updateContentSize();
		updateImageOffset();
	}
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

void ImageView::drawContents(QPainter* painter,int clipx,int clipy,int clipw,int cliph) {
	// Erase borders
	QRect imageRect(0, 0, d->mDocument->width(), d->mDocument->height());
	imageRect = d->imageToWidget(imageRect);
	
	QRect widgetRect = QRect(0, 0, visibleWidth(), visibleHeight());

	QRegion region = QRegion(widgetRect) - imageRect;
	QMemArray<QRect> rects = region.rects();
	for(unsigned int pos = 0; pos < rects.count(); ++pos ) {
		painter->eraseRect(rects[pos]);
	}
	
	// Repaint
	if( !d->mValidImageArea.isEmpty()) {
		addPendingPaint( false, QRect( clipx, clipy, clipw, cliph ));
	}
}

// How this pending stuff works:
// There's a queue of areas to paint (each with bool saying whether it's smooth pass).
// Also, there's a bitfield of pending operations, operations are handled only after
// there's nothing more to paint (so that smooth pass is started).
void ImageView::addPendingPaint( bool smooth, QRect rect ) {
	if( d->mSmoothingSuspended && smooth ) return;

	// try to avoid scheduling already scheduled areas
	QRegion& region = smooth ? d->mPendingSmoothRegion : d->mPendingNormalRegion;
	if( region.intersect( rect ) == QRegion( rect ))
		return; // whole rect has already pending paints
	// at least try to remove the part that's already scheduled
	rect = ( QRegion( rect ) - region ).boundingRect();
	region += rect;
	if( rect.isEmpty())
		return;
	addPendingPaintInternal( smooth, rect );
}

void ImageView::addPendingPaintInternal( bool smooth, QRect rect ) {
	const long long MAX_DIM = 1000000; // if monitors get larger than this, we're in trouble :)
	// QMap will ensure ordering (non-smooth first, top-to-bottom, left-to-right)
	long long key = ( smooth ? MAX_DIM * MAX_DIM : 0 ) + rect.y() * MAX_DIM + rect.x();
	// handle the case of two different paints at the same position (just in case)
	key *= 100;
	bool insert = true;
	while( d->mPendingPaints.contains( key )) {
		if( d->mPendingPaints[ key ].rect.contains( rect )) {
			insert = false;
			break;
		}
		if( rect.contains( d->mPendingPaints[ key ].rect )) {
			break;
		}
		++key;
	}
	if( insert ) {
		d->mPendingPaints[ key ] = PendingPaint( smooth, rect );
	}
	scheduleOperation( CHECK_OPERATIONS );
}

void ImageView::checkPendingOperations() {
	checkPendingOperationsInternal();
	if( d->mPendingPaints.isEmpty() && d->mPendingOperations == 0 ) {
		d->mPendingPaintTimer.stop();
	}
	updateBusyLevels();
}

void ImageView::limitPaintSize( PendingPaint& paint ) {
	// The only thing that makes time spent in performPaint() vary
	// is whether there will be scaling and whether there will be smoothing.
	// So there are three max sizes for each mode.
	int maxSize = ImageViewConfig::maxRepaintSize();
	if( d->mZoom != 1.0 ) {
		if( paint.smooth || !doDelayedSmoothing() ) {
			maxSize = ImageViewConfig::maxSmoothRepaintSize();
		} else {
			maxSize = ImageViewConfig::maxScaleRepaintSize();
		}
	}
	// don't paint more than max_size pixels at a time
	int maxHeight = ( maxSize + paint.rect.width() - 1 ) / paint.rect.width(); // round up
	maxHeight = QMAX( maxHeight, 5 ); // at least 5 lines together
	// can't repaint whole paint at once, adjust height and schedule the rest
	if( maxHeight < paint.rect.height()) {
		QRect remaining = paint.rect;
		remaining.setTop( remaining.top() + maxHeight );
		addPendingPaintInternal( paint.smooth, remaining );
		paint.rect.setHeight( maxHeight );
	}
}


void ImageView::checkPendingOperationsInternal() {
	if( !d->mPendingPaintTimer.isActive()) // suspended
		return;
	while( !d->mPendingPaints.isEmpty()) {
		PendingPaint paint = *d->mPendingPaints.begin();
		d->mPendingPaints.remove( d->mPendingPaints.begin());
		limitPaintSize( paint ); // modifies paint.rect if necessary
		QRegion& region = paint.smooth ? d->mPendingSmoothRegion : d->mPendingNormalRegion;
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
	if( d->mPendingOperations & SMOOTH_PASS ) {
		d->mSmoothingSuspended = false;
		if( doDelayedSmoothing() ) {
			QRect visibleRect( contentsX(), contentsY(), visibleWidth(), visibleHeight());
			addPendingPaint( true, visibleRect );
		}
		d->mPendingOperations &= ~SMOOTH_PASS;
		return;
	}
}

void ImageView::scheduleOperation( Operation operation )
{
	d->mPendingOperations |= operation;
	slotBusyLevelChanged( BusyLevelManager::instance()->busyLevel());
	updateBusyLevels();
}

void ImageView::updateBusyLevels() {
	if( !d->mPendingPaintTimer.isActive()) {
		BusyLevelManager::instance()->setBusyLevel( this, BUSY_NONE );
	} else if( !d->mPendingPaints.isEmpty() && !(*d->mPendingPaints.begin()).smooth ) {
		BusyLevelManager::instance()->setBusyLevel( this, BUSY_PAINTING );
	} else if(( d->mPendingOperations & SMOOTH_PASS )
		|| ( !d->mPendingPaints.isEmpty() && (*d->mPendingPaints.begin()).smooth )) {
		BusyLevelManager::instance()->setBusyLevel( this, BUSY_SMOOTHING );
	} else {
		assert( false );
	}
}

void ImageView::slotBusyLevelChanged( BusyLevel level ) {
	bool resume = false;
	if( level <= BUSY_PAINTING
		&& !d->mPendingPaints.isEmpty() && !(*d->mPendingPaints.begin()).smooth ) {
		resume = true;
	} else if( level <= BUSY_SMOOTHING
			&& (( d->mPendingOperations & SMOOTH_PASS )
			|| ( !d->mPendingPaints.isEmpty() && (*d->mPendingPaints.begin()).smooth ))) {
		resume = true;
	}
	if( resume ) {
		d->mPendingPaintTimer.start( 0 );
	} else {
		d->mPendingPaintTimer.stop();
	}
}

// How to do painting:
// When something needs to be erased: QPainter on viewport and eraseRect()
// When whole picture needs to be repainted: fullRepaint()
// When a part of the picture needs to be updated: viewport()->repaint(area,false)
// All other paints will be changed to progressive painting.
void ImageView::fullRepaint() {
	if( !viewport()->isUpdatesEnabled()) return;
	cancelPending();
	viewport()->repaint(false);
}

void ImageView::cancelPending() {
	d->mPendingPaints.clear();
	d->mPendingNormalRegion = QRegion();
	d->mPendingSmoothRegion = QRegion();
	d->mPendingPaintTimer.stop();
	d->mPendingOperations = 0;
	updateBusyLevels();
}

//#define DEBUG_RECTS

// do the actual painting
void ImageView::performPaint( QPainter* painter, int clipx, int clipy, int clipw, int cliph, bool secondPass ) {
	#ifdef DEBUG_RECTS
	static QColor colors[4]={QColor(255,0,0),QColor(0,255,0),QColor(0,0,255),QColor(255,255,0) };
	static int numColor=0;
	#endif

	QTime t;
	t.start();

	if (d->mDocument->isNull()) {
		painter->eraseRect(clipx,clipy,clipw,cliph);
		return;
	}

	// True if another pass will follow
	bool fastpass = doDelayedSmoothing() && zoom() != 1.0 && !secondPass;

	ImageUtils::SmoothAlgorithm smoothAlgo = ImageUtils::SMOOTH_NONE;
	if( zoom() != 1.0 ) {
		if (doDelayedSmoothing() && !secondPass) {
			// Add a second, smoothing pass
			addPendingPaint( true, QRect( clipx, clipy, clipw, cliph ));
		} else {
			// We need to smooth now
			smoothAlgo = static_cast<ImageUtils::SmoothAlgorithm>( ImageViewConfig::smoothAlgorithm() );
		}
	}

	int extraPixels = ImageUtils::extraScalePixels( smoothAlgo, zoom());
	QRect imageRect = d->widgetToImageBounding( QRect(clipx,clipy,clipw,cliph), extraPixels );
	imageRect = imageRect.intersect( QRect( 0, 0, d->mDocument->width(), d->mDocument->height()));
	QMemArray< QRect > rects = d->mValidImageArea.intersect( imageRect ).rects();
	for( unsigned int i = 1; i < rects.count(); ++i ) {
		addPendingPaint( secondPass, d->imageToWidget( rects[ i ] ));
	}
	imageRect = rects.count() > 0 ? rects[ 0 ] : QRect();
	if (imageRect.isEmpty()) {
		painter->eraseRect(clipx,clipy,clipw,cliph);
		return;
	}
	QRect widgetRect = d->imageToWidget( imageRect );
	if (widgetRect.isEmpty() || imageRect.isEmpty()) {
		painter->eraseRect(clipx,clipy,clipw,cliph);
		return;
	}

// With very large images, just getting a subimage using QImage::copy( QRect ) takes a significant
// portion of time here (even though it's just copying of data - probably because it's a lot of data).
// So don't do any subimage copying but instead use CroppedQImage which just manipulates scanline
// pointers. Note however that it's a bit hackish and there may be trouble if any code accesses
// the image data directly as a whole. See CroppedQImage for details.

//	QImage image = d->mDocument->image().copy( imageRect );
	ImageUtils::CroppedQImage image( d->mDocument->image(), imageRect );

	if( zoom() != 1.0 ) {
		image=ImageUtils::scale(image,widgetRect.width(),widgetRect.height(), smoothAlgo );
	}

	if( d->mBrightness != 0 ) {
		image.normalize(); // needed, it will be modified
		image = ImageUtils::changeBrightness( image, d->mBrightness );
	}

	if( d->mContrast != 100 ) { // != 1.0
		image.normalize(); // needed, it will be modified
		image = ImageUtils::changeContrast( image, d->mContrast );
	}

	if( d->mGamma != 100 ) { // != 1.0
		image.normalize(); // needed, it will be modified
		image = ImageUtils::changeGamma( image, d->mGamma );
	}

// Calling normalize() here would make image to be a proper QImage without modified scanlines,
// so that even calling QImage::copy() would work. However, it seems it's not necessary to call
// it here. The code above checks that QImage::copy() or similar doesn't occur (that zoom() != 1.0
// is there primarily to avoid that). If any kind of redraw trouble occurs, try uncommenting this
// line below first.
//	image.normalize(); // make it use its own data, if needed

	if (image.hasAlphaBuffer()) {
		image.normalize(); // needed, it will be modified
		if (image.depth()!=32) {
			image=image.convertDepth(32);
		}

		bool light;

		int imageXOffset=widgetRect.x()-d->mXOffset;
		int imageYOffset=widgetRect.y()-d->mYOffset;
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

	QRect paintRect( clipx, clipy, clipw, cliph );
	QPixmap buffer( paintRect.size());
	{
		QPainter bufferPainter(&buffer);
		bufferPainter.setBackgroundColor(painter->backgroundColor());
		bufferPainter.eraseRect(0,0,paintRect.width(),paintRect.height());
		bufferPainter.drawImage(widgetRect.topLeft()-paintRect.topLeft(),image,
			fastpass?ThresholdDither:0);
	}
	painter->drawPixmap(paintRect.topLeft(),buffer);

	if( paintRect.width() * paintRect.height() >= 10000 ) { // ignore small repaints
		// try to do one step in 0.1sec
		int size = paintRect.width() * paintRect.height() * 100 / QMAX( t.elapsed(), 1 );
		
		int maxRepaintSize;
		if (zoom() == 1.0) {
			maxRepaintSize=ImageViewConfig::maxRepaintSize();
		} else {
			if (smoothAlgo!=ImageUtils::SMOOTH_NONE) {
				maxRepaintSize=ImageViewConfig::maxSmoothRepaintSize();
			} else {
				maxRepaintSize=ImageViewConfig::maxScaleRepaintSize();
			}
		}
		
		maxRepaintSize = KCLAMP(
			( size + maxRepaintSize ) / 2, 
			10000, LIMIT_MAX_REPAINT_SIZE);
		
		if (zoom() == 1.0) {
			ImageViewConfig::setMaxRepaintSize(maxRepaintSize);
		} else {
			if (smoothAlgo!=ImageUtils::SMOOTH_NONE) {
				ImageViewConfig::setMaxSmoothRepaintSize(maxRepaintSize);
			} else {
				ImageViewConfig::setMaxScaleRepaintSize(maxRepaintSize);
			}
		}
	}

	#ifdef DEBUG_RECTS
	painter->setPen(colors[numColor]);
	numColor=(numColor+1)%4;
	painter->drawRect(paintRect);
	#endif

	QApplication::flushX();
}


void ImageView::viewportMousePressEvent(QMouseEvent* event) {
	viewport()->setFocus();
	switch (event->button()) {
	case Qt::LeftButton:
		d->mTools[d->mToolID]->leftButtonPressEvent(event);
		break;
	case Qt::RightButton:
		d->mTools[d->mToolID]->rightButtonPressEvent(event);
		break;
	default: // Avoid compiler complain
		break;
	}
}


void ImageView::viewportMouseMoveEvent(QMouseEvent* event) {
	selectTool(event->state(), true);
	d->mTools[d->mToolID]->mouseMoveEvent(event);
}


void ImageView::viewportMouseReleaseEvent(QMouseEvent* event) {
	switch (event->button()) {
	case Qt::LeftButton:
		if (event->stateAfter() & Qt::RightButton) {
			d->mOperaLikePrevious=true;
			emit selectPrevious();
			return;
		}
		d->mTools[d->mToolID]->leftButtonReleaseEvent(event);
		break;

	case Qt::MidButton:
		d->mTools[d->mToolID]->midButtonReleaseEvent(event);
		break;

	case Qt::RightButton:
		if (event->stateAfter() & Qt::LeftButton) {
			emit selectNext();
			return;
		}

		if (d->mOperaLikePrevious) { // Avoid showing the popup menu after Opera like previous
			d->mOperaLikePrevious=false;
		} else {
			d->mTools[d->mToolID]->rightButtonReleaseEvent(event);
		}
		break;

	default: // Avoid compiler complain
		break;
	}
}


bool ImageView::eventFilter(QObject* obj, QEvent* event) {
	switch (event->type()) {
	case QEvent::KeyPress:
	case QEvent::KeyRelease:
	case QEvent::AccelOverride:
		return viewportKeyEvent(static_cast<QKeyEvent*>(event));

	case QEvent::MouseButtonDblClick:
		if (d->mToolID==ZOOM) return false;
		emit doubleClicked();		
		return true;

	// Getting/loosing focus causes repaints, but repainting here is expensive,
	// and there's no need to repaint on focus changes, as the focus is not
	// indicated.
	case QEvent::FocusIn:
	case QEvent::FocusOut:
		return true;

	case QEvent::Enter:
		selectTool( kapp->keyboardMouseState(), true );
		emitRequestHintDisplay();
		break;

	default:
		break;
	}
	return QScrollView::eventFilter(obj,event);
}


bool ImageView::viewportKeyEvent(QKeyEvent* event) {
	selectTool(event->stateAfter(), false);
	return false;
}


void ImageView::contentsDragEnterEvent(QDragEnterEvent* event) {
	event->accept( QUriDrag::canDecode( event ));
}

void ImageView::contentsDropEvent(QDropEvent* event) {
	KURL::List list;
	if( KURLDrag::decode( event, list )) {
		d->mDocument->setURL( list.first());
	}
}

void ImageView::keyPressEvent( QKeyEvent *event ) {
	QScrollView::keyPressEvent( event );
	int deltaX, deltaY;

	if (event->state() != Qt::NoButton) {
		return;
	}
	switch (event->key()) {
	case Key_Up:
		deltaX =  0;
		deltaY = -1;
		break;
	case Key_Down:
		deltaX =  0;
		deltaY =  1;
		break;
	case Key_Left:
		deltaX = -1;
		deltaY =  0;
		break;
	case Key_Right:
		deltaX =  1;
		deltaY =  0;
		break;
	default:
		return;
	}
	deltaX *= width() / 2;
	deltaY *= height() / 2;
	scrollBy (deltaX, deltaY);
}

/**
 * If force is set, the cursor will be updated even if the tool is not
 * different from the current one.
 */
void ImageView::selectTool(ButtonState state, bool force) {
	ToolID oldToolID=d->mToolID;
	if (state & ControlButton) {
		d->mToolID=ZOOM;
		if (d->mToolID!=oldToolID) {
			emitRequestHintDisplay();
		}
	} else {
		d->mToolID=SCROLL;
	}

	if (d->mToolID!=oldToolID || force) {
		d->mTools[d->mToolID]->updateCursor();
	}
}


void ImageView::wheelEvent(QWheelEvent* event) {
	d->mTools[d->mToolID]->wheelEvent(event);
}


//------------------------------------------------------------------------
//
// Slots
//
//------------------------------------------------------------------------
void ImageView::slotZoomIn() {
	updateZoom(ZOOM_FREE, computeZoom(true));
}


void ImageView::slotZoomOut() {
	updateZoom(ZOOM_FREE, computeZoom(false));
}


void ImageView::slotResetZoom() {
	updateZoom(ZOOM_FREE, 1.0);
}


void ImageView::slotSelectZoom() {
	int currentItem=d->mZoomCombo->currentItem();

	if (currentItem>=int(d->mZoomComboActions.count()) ) {
		QString txt=d->mZoomCombo->currentText();
		txt=txt.left(txt.find('%'));
		double value=KGlobal::locale()->readNumber(txt) / 100.0;
		updateZoom(ZOOM_FREE, value);
	} else {
		d->mZoomComboActions[currentItem]->activate();
	}
}


void ImageView::setZoomToFit(bool on) {
	if (on) {
		updateZoom(ZOOM_FIT);
	} else {
		updateZoom(ZOOM_FREE, d->mZoomBeforeAuto, d->mXCenterBeforeAuto, d->mYCenterBeforeAuto);
	}
}


void ImageView::setZoomToWidth(bool on) {
	if (on) {
		updateZoom(ZOOM_FIT_WIDTH);
	} else {
		updateZoom(ZOOM_FREE, d->mZoomBeforeAuto, d->mXCenterBeforeAuto, d->mYCenterBeforeAuto);
	}
}


void ImageView::setZoomToHeight(bool on) {
	if (on) {
		updateZoom(ZOOM_FIT_HEIGHT);
	} else {
		updateZoom(ZOOM_FREE, d->mZoomBeforeAuto, d->mXCenterBeforeAuto, d->mYCenterBeforeAuto);
	}
}


void ImageView::setLockZoom(bool value) {
	if( value ) {
		d->mZoomToFit->setChecked( false );
		d->mZoomToWidth->setChecked( false );
		d->mZoomToHeight->setChecked( false );
	}
	// don't change zoom here, keep it even if it was from some auto zoom mode
}


void ImageView::showBCGDialog() {
	if (!d->mBCGDialog) {
		d->mBCGDialog=new BCGDialog(this);
	}
	d->mBCGDialog->show();
}


int ImageView::brightness() const {
	return d->mBrightness;
}


void ImageView::setBrightness(int value) {
	d->mBrightness=value;
	fullRepaint();
}

void ImageView::increaseBrightness() {
	d->mBrightness = KCLAMP( d->mBrightness + 5, -100, 100 );
	emit bcgChanged();
	fullRepaint();
}

void ImageView::decreaseBrightness() {
	d->mBrightness = KCLAMP( d->mBrightness - 5, -100, 100 );
	emit bcgChanged();
	fullRepaint();
}



int ImageView::contrast() const {
	return d->mContrast - 100;
}

void ImageView::setContrast(int value) {
	d->mContrast=value + 100;
	fullRepaint();
}

void ImageView::increaseContrast() {
	d->mContrast = KCLAMP( d->mContrast + 10, 0, 500 );
	emit bcgChanged();
	fullRepaint();
}

void ImageView::decreaseContrast() {
	d->mContrast = KCLAMP( d->mContrast - 10, 0, 500 );
	emit bcgChanged();
	fullRepaint();
}



int ImageView::gamma() const {
	return d->mGamma - 100;
}


void ImageView::setGamma(int value) {
	d->mGamma=value + 100;
	fullRepaint();
}

void ImageView::increaseGamma() {
	d->mGamma = KCLAMP( d->mGamma + 10, 10, 500 );
	emit bcgChanged();
	fullRepaint();
}

void ImageView::decreaseGamma() {
	d->mGamma = KCLAMP( d->mGamma - 10, 10, 500 );
	emit bcgChanged();
	fullRepaint();
}
//------------------------------------------------------------------------
//
// Private
//
//------------------------------------------------------------------------
void ImageView::emitRequestHintDisplay() {
	if (d->mDocument->isNull()) return;

	emit requestHintDisplay( d->mTools[d->mToolID]->hint() );
}


void ImageView::slotImageSizeUpdated() {
	d->mXOffset=0;
	d->mYOffset=0;

	d->mValidImageArea = QRegion();
	if (d->mZoomMode!=ZOOM_FREE) {
		d->mXCenterBeforeAuto=0;
		d->mYCenterBeforeAuto=0;
	} else {
		horizontalScrollBar()->setValue(0);
		verticalScrollBar()->setValue(0);
	}
	if (d->mZoomMode!=ZOOM_FREE) {
		updateZoom(d->mZoomMode);
	} else {
		if( !d->mLockZoom->isChecked()) {
			setZoom( 1.0 );
		}
	}

	updateZoomActions();
	d->mAdjustBCG->setEnabled(!d->mDocument->isNull());
	d->mIncreaseGamma->setEnabled(!d->mDocument->isNull());
	d->mDecreaseGamma->setEnabled(!d->mDocument->isNull());
	d->mIncreaseBrightness->setEnabled(!d->mDocument->isNull());
	d->mDecreaseBrightness->setEnabled(!d->mDocument->isNull());
	d->mIncreaseContrast->setEnabled(!d->mDocument->isNull());
	d->mDecreaseContrast->setEnabled(!d->mDocument->isNull());

	updateContentSize();
	updateImageOffset();
	updateScrollBarMode();
	fullRepaint();
}

void ImageView::slotImageRectUpdated(const QRect& imageRect) {
	d->mValidImageArea += imageRect;
	viewport()->repaint( d->imageToWidget( imageRect ), false );
}


void ImageView::updateScrollBarMode() {
	if (d->mZoomMode==ZOOM_FIT || !ImageViewConfig::showScrollBars()) {
		setVScrollBarMode(AlwaysOff);
		setHScrollBarMode(AlwaysOff);
	} else {
		setVScrollBarMode(Auto);
		setHScrollBarMode(Auto);
	}
}


void ImageView::updateContentSize() {
	resizeContents(
		int(d->mDocument->width()*d->mZoom),
		int(d->mDocument->height()*d->mZoom) );
}

double ImageView::computeZoomToFit() const {
	if (d->mDocument->isNull()) {
		return 1.0;
	}
	QSize size=d->mDocument->image().size();
	size.scale(width(),height(),QSize::ScaleMin);

	double zoom=double(size.width())/d->mDocument->width();
	if (zoom>1.0 && !ImageViewConfig::enlargeSmallImages()) return 1.0;
	return zoom;
}

double ImageView::computeZoomToWidth() const {
	if (d->mDocument->isNull()) {
		return 1.0;
	}
	int sw = verticalScrollBar()->sizeHint().width(); // geometry is not valid before first show()
	int w = width();
	int dw = d->mDocument->width();
	switch( vScrollBarMode()) {
	case AlwaysOff:
		return double(w)/dw;
	case AlwaysOn:
		return double(w-sw)/dw;
	case Auto:
	default:
		// there will be a vertical scrollbar if the image's height will be too large
		if( d->mDocument->height() * (double(w)/dw) > height()) return double(w-sw)/dw;
		return double(w)/dw;
	}
}

double ImageView::computeZoomToHeight() const {
	if (d->mDocument->isNull()) {
		return 1.0;
	}
	int sh = horizontalScrollBar()->sizeHint().height();
	int h = height();
	int dh = d->mDocument->height();
	switch( vScrollBarMode()) {
	case AlwaysOff:
		return double(h)/dh;
	case AlwaysOn:
		return double(h-sh)/dh;
	case Auto:
	default:
		if( d->mDocument->width() * (double(h)/dh) > width()) return double(h-sh)/dh;
		return double(h)/dh;
	}
}

double ImageView::computeZoom(bool in) const {
	const double F = 0.5; // change in 0.5 steps
	double zoomtofit = computeZoomToFit();
	double zoomtowidth = computeZoomToWidth();
	double zoomtoheight = computeZoomToHeight();
	if (in) {
		double newzoom;
		if (d->mZoom>=1.0) {
			newzoom = (floor(d->mZoom/F)+1.0)*F;
		} else {
			newzoom = 1/(( ceil(1/d->mZoom/F)-1.0 )*F);
		}
		if( d->mZoom < zoomtofit && zoomtofit < newzoom ) newzoom = zoomtofit;
		if( d->mZoom < zoomtowidth && zoomtowidth < newzoom ) newzoom = zoomtowidth;
		if( d->mZoom < zoomtoheight && zoomtoheight < newzoom ) newzoom = zoomtoheight;
		return newzoom;
	} else {
		double newzoom;
		if (d->mZoom>1.0) {
			newzoom = (ceil(d->mZoom/F)-1.0)*F;
		} else {
			newzoom = 1/(( floor(1/d->mZoom/F)+1.0 )*F);
		}
		if( d->mZoom > zoomtofit && zoomtofit > newzoom ) newzoom = zoomtofit;
		if( d->mZoom > zoomtowidth && zoomtowidth > newzoom ) newzoom = zoomtowidth;
		if( d->mZoom > zoomtoheight && zoomtoheight > newzoom ) newzoom = zoomtoheight;
		return newzoom;
	}
}

void ImageView::updateImageOffset() {
	int viewWidth=width();
	int viewHeight=height();

	// Compute d->mXOffset and d->mYOffset in case the image does not fit
	// the view width or height
	int zpixWidth=int(d->mDocument->width() * d->mZoom);
	int zpixHeight=int(d->mDocument->height() * d->mZoom);

	if (zpixWidth>viewWidth && hScrollBarMode()!=AlwaysOff) {
		// use sizeHint() - geometry is not valid before first show()
		viewHeight-=horizontalScrollBar()->sizeHint().height();
	}
	if (zpixHeight>viewHeight && vScrollBarMode()!=AlwaysOff) {
		viewWidth-=verticalScrollBar()->sizeHint().width();
	}

	d->mXOffset=QMAX(0,(viewWidth-zpixWidth)/2);
	d->mYOffset=QMAX(0,(viewHeight-zpixHeight)/2);
}


void ImageView::updateZoomActions() {
	// Disable most actions if there's no image
	if (d->mDocument->isNull()) {
		d->mZoomComboAction->setEnabled(false);
		d->mZoomIn->setEnabled(false);
		d->mZoomOut->setEnabled(false);
		d->mResetZoom->setEnabled(false);
		return;
	}

	d->mZoomComboAction->setEnabled(true);
	d->mZoomToFit->setEnabled(true);
	d->mZoomToWidth->setEnabled(true);
	d->mZoomToHeight->setEnabled(true);
	d->mResetZoom->setEnabled(true);

	
	if (d->mZoomMode==ZOOM_FREE) {
		d->mZoomIn->setEnabled(d->mZoom<MAX_ZOOM);
		d->mZoomOut->setEnabled(d->mZoom>1/MAX_ZOOM);
		QString zoomText=QString("%1%").arg(int(d->mZoom*100));
		d->mZoomCombo->setCurrentText(zoomText);
	} else {
		d->mZoomIn->setEnabled(true);
		d->mZoomOut->setEnabled(true);
		d->mZoomCombo->setCurrentItem(d->mZoomMode);
	}
}

} // namespace
