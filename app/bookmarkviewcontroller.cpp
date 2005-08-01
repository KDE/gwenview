// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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

#include "bookmarkviewcontroller.moc"

// Qt
#include <qheader.h>
#include <qlistview.h>
#include <qpainter.h>

// KDE
#include <kdebug.h>
#include <kbookmarkmanager.h>
#include <kiconloader.h>
#include <kurl.h>

namespace Gwenview {

struct BookmarkItem : public QListViewItem {
	template <class ItemParent>
	BookmarkItem(ItemParent* parent, const KBookmark& bookmark)
	: QListViewItem(parent, bookmark.text())
	, mBookmark(bookmark)
	{
		setPixmap(0, SmallIcon(bookmark.icon()) );
	}

	KBookmark mBookmark;
};


struct BookmarkViewController::Private {
	QListView* mListView;
	KBookmarkManager* mManager;

	template <class ItemParent>
	void addGroup(ItemParent* itemParent, const KBookmarkGroup& group) {
		KBookmark bookmark=group.first();
		BookmarkItem* previousItem=0;
		BookmarkItem* item=0;
		for (;!bookmark.isNull(); bookmark=group.next(bookmark) ) {
			if (bookmark.isSeparator()) continue;
			
			// Create the item and make sure it's placed at the end
			previousItem=item;
			item=new BookmarkItem(itemParent, bookmark);
			if (previousItem) {
				item->moveItem(previousItem);
			}
			
			if (bookmark.isGroup()) {
				addGroup(item, static_cast<const KBookmarkGroup&>(bookmark) );
			}
		}
	}
};


BookmarkViewController::BookmarkViewController(QListView* listView, KBookmarkManager* manager)
: QObject(listView)
{
	d=new Private;
	d->mListView=listView;
	d->mManager=manager;

	d->mListView->header()->hide();
	d->mListView->setRootIsDecorated(true);
	d->mListView->addColumn(QString::null);
	d->mListView->setSorting(-1);

	connect(d->mListView, SIGNAL(clicked(QListViewItem*)),
		this, SLOT(slotOpenBookmark(QListViewItem*)) );
	connect(d->mListView, SIGNAL(returnPressed(QListViewItem*)),
		this, SLOT(slotOpenBookmark(QListViewItem*)) );

	// For now, we ignore the caller parameter and just refresh the full list on update
	connect(d->mManager, SIGNAL(changed(const QString&, const QString&)),
		this, SLOT(fill()) );
	
	fill();
}


BookmarkViewController::~BookmarkViewController() {
	delete d;
}


void BookmarkViewController::fill() {
	d->mListView->clear();
	KBookmarkGroup root=d->mManager->root();
	d->addGroup(d->mListView, root);
}
	

void BookmarkViewController::slotOpenBookmark(QListViewItem* item_) {
	if (!item_) return;
	BookmarkItem* item=static_cast<BookmarkItem*>(item_);
	const KURL& url=item->mBookmark.url();
	if (!url.isValid()) return;
	emit openURL(url);
}

} // namespace
