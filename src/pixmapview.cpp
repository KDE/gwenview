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

// Qt includes
#include <qbitmap.h>
#include <qcursor.h>
#include <qlabel.h>
#include <qpainter.h>
#include <qpopupmenu.h>
#include <qtimer.h>

// KDE includes
#include <kaction.h>
#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>
#include <kstdaction.h>

// Our includes
#include "fitpixmapview.h"
#include "gvpixmap.h"
#include "scrollpixmapview.h"

#include "pixmapview.moc"


const char* CONFIG_SHOW_PATH="show path";
const char* CONFIG_LOCK_ZOOM="lock zoom";
const char* CONFIG_AUTO_ZOOM="auto zoom";
const char* CONFIG_WHEEL_BEHAVIOUR_NONE=   "wheel behaviour none";
const char* CONFIG_WHEEL_BEHAVIOUR_SHIFT=  "wheel behaviour shift";
const char* CONFIG_WHEEL_BEHAVIOUR_CONTROL="wheel behaviour control";
const char* CONFIG_WHEEL_BEHAVIOUR_ALT=    "wheel behaviour alt";

const int AUTO_HIDE_TIMEOUT=1000;

PixmapView::PixmapView(QWidget* parent,GVPixmap* pixmap,KActionCollection* actionCollection)
: QWidgetStack(parent), mPopupMenu(0L), mGVPixmap(pixmap), mShowPathInFullScreen(false), mFullScreen(false), mOperaLikePrevious(false)
{
	mAutoHideTimer=new QTimer(this);
	connect(mAutoHideTimer,SIGNAL(timeout()),
		this,SLOT(hideCursor()) );

// Create path label
	mPathLabel=new QLabel(parent);
	mPathLabel->setBackgroundColor(white);
	QFont font=mPathLabel->font();
	font.setWeight(QFont::Bold);
	mPathLabel->setFont(font);
	mPathLabel->hide();
	mPathLabel->installEventFilter(this);

// Create child widgets
	mScrollPixmapView=new ScrollPixmapView(this,pixmap,true);
	mScrollPixmapView->viewport()->installEventFilter(this);
	mFitPixmapView=new FitPixmapView(this,pixmap,false);
	mFitPixmapView->installEventFilter(this);

// Create actions
	mAutoZoom=new KToggleAction(i18n("&Auto Zoom"),"viewmagfit",0,this,SLOT(slotAutoZoom()),actionCollection,"autozoom");

	mZoomIn=KStdAction::zoomIn(mScrollPixmapView,SLOT(slotZoomIn()),actionCollection);
	
	mZoomOut=KStdAction::zoomOut(mScrollPixmapView,SLOT(slotZoomOut()),actionCollection);
	
	mResetZoom=KStdAction::actualSize(mScrollPixmapView,SLOT(slotResetZoom()),actionCollection);
	
	mLockZoom=new KToggleAction(i18n("&Lock Zoom"),"lockzoom",0,actionCollection,"lockzoom");
	connect(mLockZoom,SIGNAL(toggled(bool)),
		mScrollPixmapView,SLOT(setLockZoom(bool)) );

// Connect to some interesting signals
	connect(mGVPixmap,SIGNAL(urlChanged(const KURL&,const QString&)),
		this,SLOT(slotUpdateView()) );
	connect(mScrollPixmapView,SIGNAL(zoomChanged(double)),
		this,SLOT(updateZoomActions()) );
	connect(mFitPixmapView,SIGNAL(zoomChanged(double)),
		this,SLOT(updateZoomActions()) );

// Propagate child view signals
	connect(mScrollPixmapView,SIGNAL(zoomChanged(double)),
		this,SIGNAL(zoomChanged(double)) );
	connect(mFitPixmapView,SIGNAL(zoomChanged(double)),
		this,SIGNAL(zoomChanged(double)) );

// Add children to stack
	addWidget(mScrollPixmapView,0);
	addWidget(mFitPixmapView,1);

}


PixmapView::~PixmapView() {
}


void PixmapView::plugActionsToAccel(KAccel* accel) {
	mAutoZoom->plugAccel(accel);
	mZoomIn->plugAccel(accel);
	mZoomOut->plugAccel(accel);
	mResetZoom->plugAccel(accel);
}


void PixmapView::installRBPopup(QPopupMenu* menu) {
	mPopupMenu=menu;
}


//- Config -------------------------------------------------------------------------------
void PixmapView::readConfig(KConfig* config, const QString& group) {
	mScrollPixmapView->readConfig(config,group);
	mFitPixmapView->readConfig(config,group);
	config->setGroup(group);
	mShowPathInFullScreen=config->readBoolEntry(CONFIG_SHOW_PATH,true);
	mAutoZoom->setChecked(config->readBoolEntry(CONFIG_AUTO_ZOOM,false));
	slotAutoZoom(); // Call the auto zoom slot to initialise all settings
	mLockZoom->setChecked(config->readBoolEntry(CONFIG_LOCK_ZOOM,false));
	mScrollPixmapView->setLockZoom(mLockZoom->isChecked());

	mWheelBehaviours[NoButton]=     WheelBehaviour( config->readNumEntry(CONFIG_WHEEL_BEHAVIOUR_NONE,Scroll) );
	mWheelBehaviours[ControlButton]=WheelBehaviour( config->readNumEntry(CONFIG_WHEEL_BEHAVIOUR_CONTROL,Zoom) );
	mWheelBehaviours[ShiftButton]=  WheelBehaviour( config->readNumEntry(CONFIG_WHEEL_BEHAVIOUR_SHIFT,Browse) );
	mWheelBehaviours[AltButton]=    WheelBehaviour( config->readNumEntry(CONFIG_WHEEL_BEHAVIOUR_ALT,None) );
}


void PixmapView::writeConfig(KConfig* config, const QString& group) const {
	mScrollPixmapView->writeConfig(config,group);
	mFitPixmapView->writeConfig(config,group);
	config->setGroup(group);
	config->writeEntry(CONFIG_SHOW_PATH,mShowPathInFullScreen);
	config->writeEntry(CONFIG_AUTO_ZOOM,mAutoZoom->isChecked());
	config->writeEntry(CONFIG_LOCK_ZOOM,mLockZoom->isChecked());
	
	config->writeEntry(CONFIG_WHEEL_BEHAVIOUR_NONE   , int(mWheelBehaviours[NoButton]) );
	config->writeEntry(CONFIG_WHEEL_BEHAVIOUR_CONTROL, int(mWheelBehaviours[ControlButton]) );
	config->writeEntry(CONFIG_WHEEL_BEHAVIOUR_SHIFT  , int(mWheelBehaviours[ShiftButton]) );
	config->writeEntry(CONFIG_WHEEL_BEHAVIOUR_ALT    , int(mWheelBehaviours[AltButton]) );
}


//- Properties ---------------------------------------------------------------------------
void PixmapView::setFullScreen(bool fullScreen) {
	mFullScreen=fullScreen;

	if (mFullScreen) {
		mScrollPixmapView->viewport()->setMouseTracking(true);
		mFitPixmapView->setMouseTracking(true);
		setCursor(blankCursor);
	} else {
		mAutoHideTimer->stop();
		mScrollPixmapView->viewport()->setMouseTracking(false);
		mFitPixmapView->setMouseTracking(false);
		setCursor(arrowCursor);
	}

	if (mFullScreen && mShowPathInFullScreen) {
		updatePathLabel();
		mPathLabel->show();
	} else {
		mPathLabel->hide();
	}
	currentView()->setFullScreen(mFullScreen);
}


double PixmapView::zoom() const {
	return currentView()->zoom();
}


void PixmapView::setShowPathInFullScreen(bool value) {
	mShowPathInFullScreen=value;
}


//- Overloaded methods -------------------------------------------------------------------
bool PixmapView::eventFilter(QObject* object,QEvent* event) {
	switch (event->type()) {
	case QEvent::MouseMove:
		return mouseMoveEventFilter(object,static_cast<QMouseEvent*>(event));
	case QEvent::MouseButtonRelease:
		return mouseReleaseEventFilter(object,static_cast<QMouseEvent*>(event));
	case QEvent::Wheel:
		return wheelEventFilter(object,static_cast<QWheelEvent*>(event));
	default:
		return QWidgetStack::eventFilter(object,event);
	}
}


bool PixmapView::mouseMoveEventFilter(QObject* object,QMouseEvent* event) {
	if (mFullScreen) {
		setCursor(arrowCursor);
		mAutoHideTimer->start(AUTO_HIDE_TIMEOUT,true);
	}
	return QWidgetStack::eventFilter(object,event);
}


bool PixmapView::mouseReleaseEventFilter(QObject* object,QMouseEvent* event) {
	switch (event->button()) {
	case Qt::LeftButton:
		if (event->stateAfter() & Qt::RightButton) {
			mOperaLikePrevious=true;
			emit selectPrevious();
			return true;
		}
		break;
			
	case Qt::RightButton:
		if (event->stateAfter() & Qt::LeftButton) {
			emit selectNext();
			return true;
		}

		if (mOperaLikePrevious) { // Avoid showing the popup menu after Opera like previous
			mOperaLikePrevious=false;
		} else {
			if ( !mGVPixmap->isNull() && mPopupMenu) {
				mPopupMenu->popup(event->globalPos());
				return true;
			}
		}
		break;
	
	default: // Avoid compiler complain
		break;
	}
	return QWidgetStack::eventFilter(object,event);
}


bool PixmapView::wheelEventFilter(QObject* object,QWheelEvent* event) {

	if (mAutoZoom->isChecked()) {
		if (event->delta()<0) {
			emit selectNext();
		} else {
			emit selectPrevious();
		}
		event->accept();
		return true;
	} else {
		ButtonState modifier=ButtonState( event->state() & (ShiftButton | ControlButton | AltButton) );
		switch (mWheelBehaviours[modifier]) {
		case Browse:
			if (event->delta()<0) {
				emit selectNext();
			} else {
				emit selectPrevious();
			}
			event->accept();
			return true;
	
		case Scroll:
			// Do nothing, this will pass the event to the child scrollview,
			// which will handle the scroll
			break;
	
		case Zoom:
			if (event->delta()<0) {
				if (mZoomIn->isEnabled()) mZoomIn->activate();
			} else {
				if (mZoomOut->isEnabled()) mZoomOut->activate();
			}
			event->accept();
			return true;

		case None:
			event->accept();
			return true;
		}
	}
	return QWidgetStack::eventFilter(object,event);
}


//- Private slots ------------------------------------------------------------------------
void PixmapView::slotAutoZoom() {
	if (mAutoZoom->isChecked()) {
		mScrollPixmapView->enableView(false);
		raiseWidget(mFitPixmapView);
		mFitPixmapView->enableView(true);
		mFitPixmapView->updateView();
	} else {
		mFitPixmapView->enableView(false);
		raiseWidget(mScrollPixmapView);
		mScrollPixmapView->enableView(true);
		mScrollPixmapView->updateView();
	}
	updateZoomActions();
	setFullScreen(mFullScreen); // Apply fullscreen state to the new view
}


/**
 * Update the path label when necessary in fullscreen mode
 */
void PixmapView::slotUpdateView() {
	if (mFullScreen && mShowPathInFullScreen) updatePathLabel();
	updateZoomActions();
}


void PixmapView::hideCursor() {
	setCursor(blankCursor);
}


//- Private ------------------------------------------------------------------------------
void PixmapView::updateZoomActions() {
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
		mZoomIn->setEnabled(false);
		mZoomOut->setEnabled(false);
	} else {
		mZoomIn->setEnabled(!mScrollPixmapView->isZoomSetToMax());
		mZoomOut->setEnabled(!mScrollPixmapView->isZoomSetToMin());
	}
}


PixmapViewInterface* PixmapView::currentView() const {
	if (mAutoZoom->isChecked()) {
		return mFitPixmapView;
	} else {
		return mScrollPixmapView;
	}
}


void PixmapView::updatePathLabel() {
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
