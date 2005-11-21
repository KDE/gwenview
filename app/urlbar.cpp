// vim: set tabstop=4 shiftwidth=4 noexpandtab:
// kate: indent-mode csands; indent-width 4; replace-tabs-save off; replace-tabs off; replace-trailing-space-save off; space-indent off; tabs-indents on; tab-width 4;
/*
Gwenview - A simple image viewer for KDE
Copyright 2005 Aurelien Gateau

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
// Self
#include "urlbar.moc"

// Qt
#include <qcursor.h>
#include <qheader.h>
#include <qpopupmenu.h>

// KDE
#include <kconfig.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kurl.h>
#include <kurldrag.h>

// Local
#include "../gvcore/fileoperation.h"
#include "bookmark.h"
#include "bookmarkdialog.h"

namespace Gwenview {

const QString CONFIG_GROUP="KFileDialog Speedbar (Global)";
const QString CONFIG_ENTRY_COUNT="Number of Entries";

class URLBarItem : public QListViewItem {
public:
	URLBarItem(KListView* parent)
	: QListViewItem(parent)
	{}

	Bookmark* bookmark() { return &mBookmark; }

	void updateFromBookmark() {
		setText(0, mBookmark.title());
		setPixmap(0, SmallIcon(mBookmark.icon()) );
	}
	
private:
	Bookmark mBookmark;
};


struct URLBar::Private {
	KURL mDroppedURL;
	URLBar* mView;

	void addItem(const KURL& url) {
		URLBarItem* item=new URLBarItem(mView);
		item->bookmark()->initFromURL(url);
		item->updateFromBookmark();

		QListViewItem* last=mView->lastItem();
		if (last) {
			item->moveItem(last);
		}

		// Save now, so that KFileDialog displays an up-to-date list
		mView->writeConfig();
	}
};

URLBar::URLBar(QWidget* parent)
: KListView(parent)
{
	d=new Private;
	d->mView=this;

	setFocusPolicy(NoFocus);
	header()->hide();
	addColumn(QString::null);
	setFullWidth(true);
	setHScrollBarMode(QScrollView::AlwaysOff);
	setLineWidth(0);
	setSorting(-1);
	viewport()->setBackgroundMode(PaletteBackground);
	
	setDropVisualizer(true);
	setDropHighlighter(true);
	setAcceptDrops(true);
	
	connect(this, SIGNAL(clicked(QListViewItem*)),
		this, SLOT(slotClicked(QListViewItem*)) );
	
	connect(this, SIGNAL(contextMenu(KListView*, QListViewItem*, const QPoint&)),
		this, SLOT(slotContextMenu(KListView*, QListViewItem*, const QPoint&)) );

	readConfig();
}


URLBar::~URLBar() {
	delete d;
}


void URLBar::readConfig() {
	KConfig* config=KGlobal::config();
	config->setGroup(CONFIG_GROUP);
	int entryCount=config->readNumEntry(CONFIG_ENTRY_COUNT);
	for (int pos=0; pos<entryCount; ++pos) {
		URLBarItem* item=new URLBarItem(this);
		item->bookmark()->readConfig(config, pos);
		item->updateFromBookmark();
		QListViewItem* last=lastItem();
		if (last) {
			item->moveItem(last);
		}
	}
}


void URLBar::writeConfig() {
	KConfig* config=KGlobal::config();
	config->setGroup(CONFIG_GROUP);
	QListViewItem* item=firstChild();
	config->writeEntry(CONFIG_ENTRY_COUNT, childCount());
	for (int pos=0; item; item=item->nextSibling(), ++pos) {
		URLBarItem* urlItem=static_cast<URLBarItem*>(item);
		urlItem->bookmark()->writeConfig(config, pos);
	}
}


void URLBar::slotClicked(QListViewItem* item) {
	if (!item) return;

	URLBarItem* urlItem=static_cast<URLBarItem*>(item);
	emit activated(urlItem->bookmark()->url());
}


void URLBar::slotContextMenu(KListView*, QListViewItem* item, const QPoint& pos) {
	if (!item) return;
	QPopupMenu menu(this);
	menu.insertItem( SmallIcon("edit"), i18n("Edit..."),
		this, SLOT(editBookmark()) );
	menu.insertItem( SmallIcon("editdelete"), i18n("Delete"),
		this, SLOT(deleteBookmark()) );
	menu.exec(pos);
}


void URLBar::slotBookmarkDroppedURL() {
	d->addItem(d->mDroppedURL);
}


void URLBar::editBookmark() {
	URLBarItem* item=static_cast<URLBarItem*>( selectedItem() );
	if (!item) return;
	BookmarkDialog dlg(this, item->bookmark());
	if (dlg.exec()) {
		item->updateFromBookmark();
		writeConfig();
	}
}


void URLBar::deleteBookmark() {
	QListViewItem* item=selectedItem();
	if (!item) return;
	delete item;
	// Save now, so that KFileDialog displays an up-to-date list
	writeConfig();
}


void URLBar::contentsDragMoveEvent(QDragMoveEvent* event) {
	if (KURLDrag::canDecode(event)) {
		event->accept();
	} else {
		event->ignore();
	}
}


void URLBar::contentsDropEvent(QDropEvent* event) {
	KURL::List urls;
	if (!KURLDrag::decode(event, urls)) return;

	// Get a pointer to the drop item
	QPoint point(0,event->pos().y());
	URLBarItem* item=static_cast<URLBarItem*>( itemAt(contentsToViewport(point)) );
	
	QPopupMenu menu(this);
	int addBookmarkID=menu.insertItem( SmallIcon("bookmark_add"), i18n("&Add Bookmark"),
		this, SLOT(slotBookmarkDroppedURL()) );
	if (urls.count()==1) {
		d->mDroppedURL=*urls.begin();
	} else {
		menu.setItemEnabled(addBookmarkID, false);
	}

	if (item) {
		menu.insertSeparator();
		KURL dest=item->bookmark()->url();
		FileOperation::fillDropURLMenu(&menu, urls, dest);
	}
	menu.insertSeparator();
	menu.insertItem( SmallIcon("cancel"), i18n("Cancel") );
	menu.exec(QCursor::pos());
}

} // namespace
