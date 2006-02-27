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
#include <qpopupmenu.h>
#include <qwidgetstack.h>

// KDE
#include <kaction.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmediaplayer/player.h>
#include <kmimetype.h>
#include <kuserprofile.h>
#include <kparts/componentfactory.h>

// Local
#include <document.h>
#include <externaltoolcontext.h>
#include <externaltoolmanager.h>
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

struct ImageViewController::Private {
	Document* mDocument;
	KActionCollection* mActionCollection;
	QWidgetStack* mStack;
	ImageView* mImageView;
	
	QString mPlayerLibrary;
	KParts::ReadOnlyPart* mPlayerPart;

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
};


ImageViewController::ImageViewController(QWidget* parent, Document* document, KActionCollection* actionCollection)
: QObject(parent) {
	d=new Private;
	d->mDocument=document;
	d->mActionCollection=actionCollection;

	d->mStack=new QWidgetStack(parent);
	d->mImageView=new ImageView(d->mStack, document, actionCollection);
	d->mStack->addWidget(d->mImageView);
	d->mPlayerPart=0;
	
	connect(d->mDocument,SIGNAL(loaded(const KURL&)),
		this,SLOT(slotLoaded()) );

	connect(d->mImageView, SIGNAL(requestContextMenu(const QPoint&)),
		this, SLOT(openImageViewContextMenu(const QPoint&)) );
	
	connect(d->mImageView, SIGNAL(requestHintDisplay(const QString&)),
		this, SIGNAL(requestHintDisplay(const QString&)) );
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
}

QWidget* ImageViewController::widget() const {
	return d->mStack;
}

void ImageViewController::updateFromSettings() {
	d->mImageView->updateFromSettings();
}

ImageView* ImageViewController::imageView() const {
	return d->mImageView;
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
	
	// The action which causes the fullscreen toolbar to slide in or out. Not
	// sure it should be there, but at least it makes the shortcut key for this
	// action more discoverable.
	plugAction(&menu, d->mActionCollection, "toggle_bar");

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
