// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2006 Aurélien Gâteau

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
#include <imageviewcontroller.moc>

// Qt
#include <qcursor.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpopupmenu.h>
#include <qwidgetstack.h>

// KDE
#include <kaction.h>
#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmediaplayer/player.h>
#include <kmimetype.h>
#include <ktoolbar.h>
#include <kuserprofile.h>
#include <kparts/componentfactory.h>

// Local
#include <captionformatterbase.h>
#include <document.h>
#include <externaltoolcontext.h>
#include <externaltoolmanager.h>
#include <fullscreenconfig.h>
#include <imageview.h>

namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

	
const int AUTO_HIDE_TIMEOUT=4000;

const int FULL_SCREEN_ICON_SIZE=32;

enum OSDMode { NONE, PATH, COMMENT, PATH_AND_COMMENT, FREE_OUTPUT };


//------------------------------------------------------------------------
//
// ImageViewController::Private
//
//------------------------------------------------------------------------
struct ImageViewController::Private {
	Document* mDocument;
	KActionCollection* mActionCollection;
	QWidget* mContainer;
	QWidgetStack* mStack;
	ImageView* mImageView;
	ImageViewController* mImageViewController;
	QTimer* mAutoHideTimer;
	
	QString mPlayerLibrary;
	KParts::ReadOnlyPart* mPlayerPart;
	
	// Fullscreen stuff
	bool mFullScreen;
	KToolBar* mFullScreenBar;
	QLabel* mFullScreenLabel;
	KActionPtrList mFullScreenActions;
	CaptionFormatterBase* mOSDFormatter;

	void createPlayerPart(void) {
		QString mimeType=KMimeType::findByURL(mDocument->url())->name();
		KService::Ptr service = KServiceTypeProfile::preferredService(mimeType, "KParts/ReadOnlyPart");
		if (!service) {
			mPlayerLibrary=QString::null;
			delete mPlayerPart;
			mPlayerPart=0;
			return;
		}

		QString library=service->library();
		Q_ASSERT(!library.isNull());
		LOG("Library:" << library);
		if (library==mPlayerLibrary) {
			LOG("Reusing library");
			return;
		}
		mPlayerLibrary=library;
		
		mPlayerPart = KParts::ComponentFactory::createPartInstanceFromService<KParts::ReadOnlyPart>(service, mStack, 0, mStack, 0);
		if (!mPlayerPart) return;
		mStack->addWidget(mPlayerPart->widget());
	}
	

	void showPlayerPart(void) {
		LOG("");
		createPlayerPart();
		if (!mPlayerPart) return;
		mStack->raiseWidget(mPlayerPart->widget());
		mPlayerPart->openURL(mDocument->url());

		// If the part implements the KMediaPlayer::Player interface, start
		// playing (needed for Kaboodle)
		KMediaPlayer::Player* player=dynamic_cast<KMediaPlayer::Player *>(mPlayerPart);
		if (player) {
			player->play();
		}
	}

	
	void showImageView(void) {
		LOG("");
		mStack->raiseWidget(mImageView);
	}


	void updateFullScreenLabel() {
		Q_ASSERT(mOSDFormatter);
		if (!mOSDFormatter) {
			kdWarning() << "mOSDFormatter is not set\n";
			return;
		}
		
		QString format;
		OSDMode osdMode=static_cast<OSDMode>( FullScreenConfig::osdMode() );
		switch (osdMode) {
		case FREE_OUTPUT:
			format = FullScreenConfig::freeOutputFormat();
			break;
		case PATH:
			format = "%p";
			break;
		case COMMENT:
			format = "%c";
			break;
		case PATH_AND_COMMENT:
			format = "%p\n%c";
			break;
		case NONE:
			break;
		}
		mFullScreenLabel->setText( (*mOSDFormatter)(format));
	}

	
	void restartAutoHideTimer() {
		mAutoHideTimer->start(AUTO_HIDE_TIMEOUT,true);
	}


	void updateFullScreenBarPosition() {
		int mouseY=mStack->mapFromGlobal(QCursor::pos()).y();
		bool visible = mFullScreenBar->y()==0;
		
		if (visible && mouseY>mFullScreenBar->height()) {
			mFullScreenBar->move(0, -mFullScreenBar->height());
		}

		if (!visible && mouseY<2) {
			mFullScreenBar->move(0, 0);
			mFullScreenBar->raise();
		}
	}

	
	/**
	 * This function creates the fullscreen toolbar.
	 * NOTE: It should not be called from/merged with setFullScreenActions,
	 * otherwise the toolbar will have a one pixel border which will prevent
	 * reaching easily buttons by pushing the mouse to the top edge of the
	 * screen.
	 * My guess is that instanciating the toolbar *before* the main
	 * window is shown causes the main window to tweak its bars. This happens
	 * with KDE 3.5.1.
	 */
	void initFullScreenBar() {
		Q_ASSERT(!mFullScreenBar);
		mFullScreenBar=new KToolBar(mContainer, "FullScreenToolBar");
		mFullScreenBar->setIconSize(FULL_SCREEN_ICON_SIZE);
		mFullScreenBar->setMovingEnabled(false);
		
		KActionPtrList::ConstIterator
			it=mFullScreenActions.begin(),
			end=mFullScreenActions.end();

		for (; it!=end; ++it) {
			(*it)->plug(mFullScreenBar);
		}
		
		mFullScreenLabel=new QLabel(mFullScreenBar);
		mFullScreenBar->insertWidget(12 /* id */, 0 /* width */, mFullScreenLabel);
	}
};


//------------------------------------------------------------------------
//
// ImageViewController
//
//------------------------------------------------------------------------
ImageViewController::ImageViewController(QWidget* parent, Document* document, KActionCollection* actionCollection)
: QObject(parent) {
	d=new Private;
	d->mImageViewController=this;
	d->mDocument=document;
	d->mActionCollection=actionCollection;
	d->mAutoHideTimer=new QTimer(this);

	d->mContainer=new QWidget(parent);
	
	QVBoxLayout* layout=new QVBoxLayout(d->mContainer);
	d->mStack=new QWidgetStack(d->mContainer);
	layout->add(d->mStack);
	
	d->mImageView=new ImageView(d->mStack, document, actionCollection);
	d->mStack->addWidget(d->mImageView);

	KApplication::kApplication()->installEventFilter(this);

	d->mPlayerPart=0;
	d->mFullScreen=false;
	d->mFullScreenBar=0;
	d->mFullScreenLabel=0;
	d->mOSDFormatter=0;
	
	connect(d->mDocument,SIGNAL(loaded(const KURL&)),
		this,SLOT(slotLoaded()) );

	connect(d->mImageView, SIGNAL(requestContextMenu(const QPoint&)),
		this, SLOT(openImageViewContextMenu(const QPoint&)) );
	
	connect(d->mImageView, SIGNAL(requestHintDisplay(const QString&)),
		this, SIGNAL(requestHintDisplay(const QString&)) );

	connect(d->mAutoHideTimer,SIGNAL(timeout()),
		this,SLOT(slotAutoHide()) );

	// Forward Image view signals
	connect(d->mImageView, SIGNAL(selectPrevious()), SIGNAL(selectPrevious()) );
	connect(d->mImageView, SIGNAL(selectNext()), SIGNAL(selectNext()) );
	connect(d->mImageView, SIGNAL(doubleClicked()), SIGNAL(doubleClicked()) );
}


ImageViewController::~ImageViewController() {
	delete d;
}

void ImageViewController::slotLoaded() {
	LOG("");
	if (d->mDocument->fileType()==Document::FILE_OTHER) {
		d->showPlayerPart();
	} else {
		d->showImageView();
	}
	if (d->mFullScreen) {
		d->updateFullScreenLabel();
	}
}


void ImageViewController::setOSDFormatter(CaptionFormatterBase* formatter) {
	d->mOSDFormatter=formatter;
}


void ImageViewController::setFullScreen(bool fullScreen) {
	d->mFullScreen=fullScreen;
	d->mImageView->setFullScreen(fullScreen);

	if (d->mFullScreen) {
		d->restartAutoHideTimer();
		if (!d->mFullScreenBar) {
			d->initFullScreenBar();
		}
		d->updateFullScreenLabel();
		
		int width=KGlobalSettings::desktopGeometry(d->mStack).width();
		int height=FULL_SCREEN_ICON_SIZE + 10;
        d->mFullScreenBar->setGeometry( 0, -height, width, height);
		d->mFullScreenBar->show();
	} else {
		d->mAutoHideTimer->stop();
		QApplication::restoreOverrideCursor();
		
		d->mFullScreenBar->hide();
	}
}


void ImageViewController::setFullScreenActions(const KActionPtrList& actions) {
	d->mFullScreenActions=actions;
}


void ImageViewController::slotAutoHide() {
	if (d->mFullScreenBar) {
		// Do not auto hide if the cursor is over the bar
		QPoint pos=d->mFullScreenBar->mapFromGlobal(QCursor::pos());
		if (d->mFullScreenBar->rect().contains(pos)) {
			d->restartAutoHideTimer();
			return;
		}
	}
	QApplication::setOverrideCursor(blankCursor);
}


QWidget* ImageViewController::widget() const {
	return d->mContainer;
}


void ImageViewController::updateFromSettings() {
	d->mImageView->updateFromSettings();
}


/**
 * This application eventFilter monitors mouse moves
 */
bool ImageViewController::eventFilter(QObject* object, QEvent* event) {
	if (!d->mFullScreen) return false;
	if (!event->type()==QEvent::MouseMove) return false;
			
	bool isAChildOfStack=false;
	QObject* parentObject;
	for (parentObject=object->parent(); parentObject; parentObject=parentObject->parent()) {
		if (parentObject==d->mStack) {
			isAChildOfStack=true;
			break;
		}
	}
	if (!isAChildOfStack) return false;

	d->updateFullScreenBarPosition();
	QApplication::restoreOverrideCursor();
	d->restartAutoHideTimer();
	return false;
}


/**
 * Little helper to plug an action if it exists
 */
inline void plugAction(QPopupMenu* menu, KActionCollection* actionCollection, const char* actionName) {
	KAction* action=actionCollection->action(actionName);
	if (action) action->plug(menu);
}


void ImageViewController::openImageViewContextMenu(const QPoint& pos) {
	QPopupMenu menu(d->mImageView);
	bool noImage=d->mDocument->filename().isEmpty();
	bool validImage=!d->mDocument->isNull();

	// The fullscreen item is always there, to be able to leave fullscreen mode
	// if necessary. But KParts may not have the action itself.
	plugAction(&menu, d->mActionCollection, "fullscreen");
	
	if (validImage) {
		menu.insertSeparator();

		plugAction(&menu, d->mActionCollection, "view_zoom_to_fit");
		plugAction(&menu, d->mActionCollection, "view_zoom_in");
		plugAction(&menu, d->mActionCollection, "view_zoom_out");
		plugAction(&menu, d->mActionCollection, "view_actual_size");
		plugAction(&menu, d->mActionCollection, "view_zoom_lock");
	}

	menu.insertSeparator();

	plugAction(&menu, d->mActionCollection, "first");
	plugAction(&menu, d->mActionCollection, "previous");
	plugAction(&menu, d->mActionCollection, "next");
	plugAction(&menu, d->mActionCollection, "last");

	if (validImage) {
		menu.insertSeparator();

		QPopupMenu* editMenu=new QPopupMenu(&menu);
		plugAction(editMenu, d->mActionCollection, "rotate_left");
		plugAction(editMenu, d->mActionCollection, "rotate_right");
		plugAction(editMenu, d->mActionCollection, "mirror");
		plugAction(editMenu, d->mActionCollection, "flip");
		menu.insertItem( i18n("Edit"), editMenu );

		ExternalToolContext* externalToolContext=
			ExternalToolManager::instance()->createContext(
			this, d->mDocument->url());

		menu.insertItem(
			i18n("External Tools"), externalToolContext->popupMenu());
	}

	if (!noImage) {
		menu.insertSeparator();

		plugAction(&menu, d->mActionCollection, "file_rename");
		plugAction(&menu, d->mActionCollection, "file_copy");
		plugAction(&menu, d->mActionCollection, "file_move");
		plugAction(&menu, d->mActionCollection, "file_link");
		plugAction(&menu, d->mActionCollection, "file_delete");

		menu.insertSeparator();

		plugAction(&menu, d->mActionCollection, "file_properties");
	}

	menu.exec(pos);
}



} // namespace
