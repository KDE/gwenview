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
#include <qiconset.h>
#include <qlabel.h>
#include <qpainter.h>
#include <qpopupmenu.h>
#include <qtimer.h>

// KDE includes
#include <kaction.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kiconeffect.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kpropsdlg.h>
#include <kstdaction.h>

// Our includes
#include "fileoperation.h"
#include "gvfitpixmapview.h"
#include "gvpixmap.h"
#include "gvscrollpixmapview.h"

#include "gvpixmapviewstack.moc"


const char* CONFIG_SHOW_PATH="show path";
const char* CONFIG_LOCK_ZOOM="lock zoom";
const char* CONFIG_AUTO_ZOOM="auto zoom";
const char* CONFIG_WHEEL_BEHAVIOUR_NONE=   "wheel behaviour none";
const char* CONFIG_WHEEL_BEHAVIOUR_SHIFT=  "wheel behaviour shift";
const char* CONFIG_WHEEL_BEHAVIOUR_CONTROL="wheel behaviour control";
const char* CONFIG_WHEEL_BEHAVIOUR_ALT=    "wheel behaviour alt";

const int AUTO_HIDE_TIMEOUT=1000;

GVPixmapViewStack::GVPixmapViewStack(QWidget* parent,GVPixmap* pixmap,KActionCollection* actionCollection)
: QWidgetStack(parent), mGVPixmap(pixmap), mShowPathInFullScreen(false),
mFullScreen(false), mOperaLikePrevious(false), mActionCollection(actionCollection)
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
	mGVScrollPixmapView=new GVScrollPixmapView(this,pixmap,true);
	mGVScrollPixmapView->viewport()->installEventFilter(this);
	mGVFitPixmapView=new GVFitPixmapView(this,pixmap,false);
	mGVFitPixmapView->installEventFilter(this);

	// Create actions
	mAutoZoom=new KToggleAction(i18n("&Auto Zoom"),"viewmagfit",0,this,SLOT(slotAutoZoom()),mActionCollection,"autozoom");

	mZoomIn=KStdAction::zoomIn(mGVScrollPixmapView,SLOT(slotZoomIn()),mActionCollection);
	
	mZoomOut=KStdAction::zoomOut(mGVScrollPixmapView,SLOT(slotZoomOut()),mActionCollection);
	
	mResetZoom=KStdAction::actualSize(mGVScrollPixmapView,SLOT(slotResetZoom()),mActionCollection);
    mResetZoom->setIcon("viewmag1");

	/* Experimental code to generate the lock zoom icon on the fly
	 * Unfortunately this does not work when resizing the toolbar buttons
	QImage icon=MainBarIcon("viewmag").convertToImage();
	KIconTheme* theme=KGlobal::instance()->iconLoader()->theme();
	QImage overlay=MainBarIcon(theme->lockOverlay()).convertToImage();
	KIconEffect::overlay(icon,overlay);
	QPixmap result=QPixmap(icon);
	QIconSet iconSet(result);
	mLockZoom=new KToggleAction(i18n("&Lock Zoom"),iconSet,0,mActionCollection,"lockzoom");
	*/
	mLockZoom=new KToggleAction(i18n("&Lock Zoom"),"lockzoom",0,mActionCollection,"lockzoom");
	connect(mLockZoom,SIGNAL(toggled(bool)),
		mGVScrollPixmapView,SLOT(setLockZoom(bool)) );

	// Connect to some interesting signals
	connect(mGVPixmap,SIGNAL(urlChanged(const KURL&,const QString&)),
		this,SLOT(slotUpdateView()) );
	connect(mGVScrollPixmapView,SIGNAL(zoomChanged(double)),
		this,SLOT(updateZoomActions()) );
	connect(mGVFitPixmapView,SIGNAL(zoomChanged(double)),
		this,SLOT(updateZoomActions()) );

	// Propagate child view signals
	connect(mGVScrollPixmapView,SIGNAL(zoomChanged(double)),
		this,SIGNAL(zoomChanged(double)) );
	connect(mGVFitPixmapView,SIGNAL(zoomChanged(double)),
		this,SIGNAL(zoomChanged(double)) );

	// Add children to stack
	addWidget(mGVScrollPixmapView,0);
	addWidget(mGVFitPixmapView,1);

}


GVPixmapViewStack::~GVPixmapViewStack() {
}


void GVPixmapViewStack::plugActionsToAccel(KAccel* accel) {
	mAutoZoom->plugAccel(accel);
	mZoomIn->plugAccel(accel);
	mZoomOut->plugAccel(accel);
	mResetZoom->plugAccel(accel);
}


//------------------------------------------------------------------------
//
// Config
//
//------------------------------------------------------------------------
void GVPixmapViewStack::readConfig(KConfig* config, const QString& group) {
	mGVScrollPixmapView->readConfig(config,group);
	mGVFitPixmapView->readConfig(config,group);
	config->setGroup(group);
	mShowPathInFullScreen=config->readBoolEntry(CONFIG_SHOW_PATH,true);
	mAutoZoom->setChecked(config->readBoolEntry(CONFIG_AUTO_ZOOM,false));
	slotAutoZoom(); // Call the auto zoom slot to initialise all settings
	mLockZoom->setChecked(config->readBoolEntry(CONFIG_LOCK_ZOOM,false));
	mGVScrollPixmapView->setLockZoom(mLockZoom->isChecked());

	mWheelBehaviours[NoButton]=     WheelBehaviour( config->readNumEntry(CONFIG_WHEEL_BEHAVIOUR_NONE,Scroll) );
	mWheelBehaviours[ControlButton]=WheelBehaviour( config->readNumEntry(CONFIG_WHEEL_BEHAVIOUR_CONTROL,Zoom) );
	mWheelBehaviours[ShiftButton]=  WheelBehaviour( config->readNumEntry(CONFIG_WHEEL_BEHAVIOUR_SHIFT,Browse) );
	mWheelBehaviours[AltButton]=    WheelBehaviour( config->readNumEntry(CONFIG_WHEEL_BEHAVIOUR_ALT,None) );
}


void GVPixmapViewStack::writeConfig(KConfig* config, const QString& group) const {
	mGVScrollPixmapView->writeConfig(config,group);
	mGVFitPixmapView->writeConfig(config,group);
	config->setGroup(group);
	config->writeEntry(CONFIG_SHOW_PATH,mShowPathInFullScreen);
	config->writeEntry(CONFIG_AUTO_ZOOM,mAutoZoom->isChecked());
	config->writeEntry(CONFIG_LOCK_ZOOM,mLockZoom->isChecked());
	
	config->writeEntry(CONFIG_WHEEL_BEHAVIOUR_NONE   , int(mWheelBehaviours[NoButton]) );
	config->writeEntry(CONFIG_WHEEL_BEHAVIOUR_CONTROL, int(mWheelBehaviours[ControlButton]) );
	config->writeEntry(CONFIG_WHEEL_BEHAVIOUR_SHIFT  , int(mWheelBehaviours[ShiftButton]) );
	config->writeEntry(CONFIG_WHEEL_BEHAVIOUR_ALT    , int(mWheelBehaviours[AltButton]) );
}


//------------------------------------------------------------------------
//
// Properties
// 
//------------------------------------------------------------------------
void GVPixmapViewStack::setFullScreen(bool fullScreen) {
	mFullScreen=fullScreen;

	if (mFullScreen) {
		mGVScrollPixmapView->viewport()->setMouseTracking(true);
		mGVFitPixmapView->setMouseTracking(true);
		setCursor(blankCursor);
	} else {
		mAutoHideTimer->stop();
		mGVScrollPixmapView->viewport()->setMouseTracking(false);
		mGVFitPixmapView->setMouseTracking(false);
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


double GVPixmapViewStack::zoom() const {
	return currentView()->zoom();
}


void GVPixmapViewStack::setShowPathInFullScreen(bool value) {
	mShowPathInFullScreen=value;
}


//------------------------------------------------------------------------
//
// Overloaded methods
//
//------------------------------------------------------------------------
bool GVPixmapViewStack::eventFilter(QObject* object,QEvent* event) {
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


bool GVPixmapViewStack::mouseMoveEventFilter(QObject* object,QMouseEvent* event) {
	if (mFullScreen) {
		setCursor(arrowCursor);
		mAutoHideTimer->start(AUTO_HIDE_TIMEOUT,true);
	}
	return QWidgetStack::eventFilter(object,event);
}


bool GVPixmapViewStack::mouseReleaseEventFilter(QObject* object,QMouseEvent* event) {
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
			if ( !mGVPixmap->isNull()) {
				openContextMenu(event->globalPos());
				return true;
			}
		}
		break;
	
	default: // Avoid compiler complain
		break;
	}
	return QWidgetStack::eventFilter(object,event);
}


bool GVPixmapViewStack::wheelEventFilter(QObject* object,QWheelEvent* event) {

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


//------------------------------------------------------------------------
//
// Private slots
// 
//------------------------------------------------------------------------
void GVPixmapViewStack::openContextMenu(const QPoint& pos) {
	if (mGVPixmap->isNull()) return;

	QPopupMenu menu(this);

	mActionCollection->action("view_fullscreen")->plug(&menu);
	menu.insertSeparator();
	
	mAutoZoom->plug(&menu);
	mZoomIn->plug(&menu);
	mZoomOut->plug(&menu);
	mResetZoom->plug(&menu);
	mLockZoom->plug(&menu);

	menu.insertSeparator();
	
	mActionCollection->action("first")->plug(&menu);
	mActionCollection->action("previous")->plug(&menu);
	mActionCollection->action("next")->plug(&menu);
	mActionCollection->action("last")->plug(&menu);
	
	menu.insertSeparator();
	
	menu.connectItem(
		menu.insertItem( i18n("Open With &Editor") ),
		this,SLOT(openWithEditor()) );
	
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
		menu.insertItem( i18n("&Delete...") ),
		this,SLOT(deleteFile()) );
	
	menu.insertSeparator();
	
	menu.connectItem(
		menu.insertItem( i18n("Properties") ),
		this,SLOT(showFileProperties()) );
	
	menu.exec(pos);
}


void GVPixmapViewStack::slotAutoZoom() {
	if (mAutoZoom->isChecked()) {
		mGVScrollPixmapView->enableView(false);
		raiseWidget(mGVFitPixmapView);
		mGVFitPixmapView->enableView(true);
		mGVFitPixmapView->updateView();
	} else {
		mGVFitPixmapView->enableView(false);
		raiseWidget(mGVScrollPixmapView);
		mGVScrollPixmapView->enableView(true);
		mGVScrollPixmapView->updateView();
	}
	updateZoomActions();
	setFullScreen(mFullScreen); // Apply fullscreen state to the new view
}


/**
 * Update the path label when necessary in fullscreen mode
 */
void GVPixmapViewStack::slotUpdateView() {
	if (mFullScreen && mShowPathInFullScreen) updatePathLabel();
	updateZoomActions();
}


void GVPixmapViewStack::hideCursor() {
	setCursor(blankCursor);
}


//------------------------------------------------------------------------
//
// File operations
//
//------------------------------------------------------------------------
void GVPixmapViewStack::showFileProperties() {
	(void)new KPropertiesDialog(mGVPixmap->url());
}


void GVPixmapViewStack::openWithEditor() {
	FileOperation::openWithEditor(mGVPixmap->url());
}


void GVPixmapViewStack::renameFile() {
	FileOperation::rename(mGVPixmap->url(),this);
}


void GVPixmapViewStack::copyFile() {
	KURL::List list;
	list << mGVPixmap->url();
	FileOperation::copyTo(list,this);
}


void GVPixmapViewStack::moveFile() {
	KURL::List list;
	list << mGVPixmap->url();
	FileOperation::moveTo(list,this);
}


void GVPixmapViewStack::deleteFile() {
	KURL::List list;
	list << mGVPixmap->url();
	FileOperation::del(list,this);
}


//------------------------------------------------------------------------
//
// Private
//
//------------------------------------------------------------------------
void GVPixmapViewStack::updateZoomActions() {
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
		mZoomIn->setEnabled(!mGVScrollPixmapView->isZoomSetToMax());
		mZoomOut->setEnabled(!mGVScrollPixmapView->isZoomSetToMin());
	}
}


GVPixmapViewBase* GVPixmapViewStack::currentView() const {
	if (mAutoZoom->isChecked()) {
		return mGVFitPixmapView;
	} else {
		return mGVScrollPixmapView;
	}
}


void GVPixmapViewStack::updatePathLabel() {
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
