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

#include <memory>

// Qt
#include <qcursor.h>
#include <qheader.h>
#include <qpopupmenu.h>
#include <qtooltip.h>
#include <qvbox.h>

// KDE
#include <kaction.h>
#include <kactioncollection.h>
#include <kbookmarkmanager.h>
#include <kdebug.h>
#include <kdeversion.h>
#include <kiconloader.h>
#include <klistview.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetype.h>
#include <ktoolbar.h>
#include <kurl.h>
#include <kurldrag.h>

// Local
#include "bookmarkdialog.h"
#include "../gvcore/fileoperation.h"

namespace Gwenview {

// URLDropListView	
URLDropListView::URLDropListView(QWidget* parent)
: KListView(parent) {
	setAcceptDrops(true);
}


void URLDropListView::contentsDragMoveEvent(QDragMoveEvent* event) {
	if (KURLDrag::canDecode(event)) {
		event->accept();
	} else {
		event->ignore();
	}
}




struct BookmarkItem : public KListViewItem {
	template <class ItemParent>
	BookmarkItem(ItemParent* parent, const KBookmark& bookmark)
	: KListViewItem(parent)
	, mBookmark(bookmark)
	{
		refresh();
	}

	void refresh() {
		setText(0, mBookmark.text() );
		setPixmap(0, SmallIcon(mBookmark.icon()) );
	}

	KBookmark mBookmark;
};


class BookmarkToolTip : public QToolTip {
public:
	BookmarkToolTip(KListView* lv)
	: QToolTip(lv->viewport())
	, mListView(lv) {}

	void maybeTip(const QPoint& pos) {
		BookmarkItem *item = static_cast<BookmarkItem*>( mListView->itemAt(pos) );
		if ( !item) return;
		if (item->mBookmark.isGroup()) return;
		
		QRect rect=mListView->itemRect(item);
		tip(rect, item->mBookmark.url().prettyURL());
	};
	
	KListView* mListView;
};


struct BookmarkViewController::Private {
	QVBox* mBox;
	KListView* mListView;
	KBookmarkManager* mManager;
	KURL mCurrentURL;
	std::auto_ptr<BookmarkToolTip> mToolTip;
	KActionCollection* mActionCollection;
	KURL mDroppedURL;

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

	KBookmarkGroup findBestParentGroup() {
		KBookmarkGroup parentGroup;
		BookmarkItem* item=static_cast<BookmarkItem*>( mListView->currentItem() );
		if (item) {
			if (item->mBookmark.isGroup()) {
				parentGroup=item->mBookmark.toGroup();
			} else {
				parentGroup=item->mBookmark.parentGroup();
			}
		} else {
			parentGroup=mManager->root();
		}

		return parentGroup;
	}

	void bookmarkURL(const KURL& url) {
		BookmarkDialog dialog(mListView, BookmarkDialog::BOOKMARK);
		dialog.setTitle(url.fileName());
		dialog.setURL(url.prettyURL());
		dialog.setIcon(KMimeType::iconForURL(url));
		if (dialog.exec()==QDialog::Rejected) return;

		KBookmarkGroup parentGroup=findBestParentGroup();
		parentGroup.addBookmark(mManager, dialog.title(), dialog.url(), dialog.icon());
		mManager->emitChanged(parentGroup);
	}
};


void URLDropListView::contentsDropEvent(QDropEvent* event) {
	KURL::List urls;
	if (!KURLDrag::decode(event, urls)) return;
	emit urlDropped(event, urls);
}


BookmarkViewController::BookmarkViewController(QWidget* parent)
: QObject(parent)
{
	d=new Private;
	d->mManager=0;

	d->mBox=new QVBox(parent);
	
	// Init listview
	d->mListView=new URLDropListView(d->mBox);
	d->mToolTip.reset(new BookmarkToolTip(d->mListView) );
	d->mActionCollection=new KActionCollection(d->mListView);

	d->mListView->header()->hide();
	d->mListView->setRootIsDecorated(true);
	d->mListView->addColumn(QString::null);
	d->mListView->setSorting(-1);
	d->mListView->setShowToolTips(false);
	d->mListView->setFullWidth(true);

	connect(d->mListView, SIGNAL(clicked(QListViewItem*)),
		this, SLOT(slotOpenBookmark(QListViewItem*)) );
	connect(d->mListView, SIGNAL(returnPressed(QListViewItem*)),
		this, SLOT(slotOpenBookmark(QListViewItem*)) );
	connect(d->mListView, SIGNAL(contextMenuRequested(QListViewItem*, const QPoint&, int)),
		this, SLOT(slotContextMenu(QListViewItem*)) );
	connect(d->mListView, SIGNAL(urlDropped(QDropEvent*, const KURL::List&)),
		this, SLOT(slotURLDropped(QDropEvent*, const KURL::List&)) );

	// Init toolbar
	KToolBar* toolbar=new KToolBar(d->mBox, "", true);
	KAction* action;
	toolbar->setIconText(KToolBar::IconTextRight);
	action=new KAction(i18n("Add a bookmark (keep it short)", "Add"), "bookmark_add", 0, 
			this, SLOT(bookmarkCurrentURL()), d->mActionCollection);
	action->plug(toolbar);
	action=new KAction(i18n("Remove a bookmark (keep it short)", "Remove"), "editdelete", 0,
			this, SLOT(deleteCurrentBookmark()), d->mActionCollection);
	action->plug(toolbar);
}


BookmarkViewController::~BookmarkViewController() {
	delete d;
}


void BookmarkViewController::init(KBookmarkManager* manager) {
	// This method should not be called twice
	Q_ASSERT(!d->mManager);
	
	d->mManager=manager;
	// For now, we ignore the caller parameter and just refresh the full list on update
	connect(d->mManager, SIGNAL(changed(const QString&, const QString&)),
		this, SLOT(fill()) );
	fill();
}


void BookmarkViewController::setURL(const KURL& url) {
	d->mCurrentURL=url;
}


QWidget* BookmarkViewController::widget() const {
	return d->mBox;
}


void BookmarkViewController::fill() {
	d->mListView->clear();
	KBookmarkGroup root=d->mManager->root();
	d->addGroup(d->mListView, root);
}


void BookmarkViewController::slotURLDropped(QDropEvent* event, const KURL::List& urls) {
	// Get a pointer to the drop item
	QPoint point(0,event->pos().y());
	KListView* lst=d->mListView;
	BookmarkItem* item=static_cast<BookmarkItem*>( lst->itemAt(lst->contentsToViewport(point)) );
	
	QPopupMenu menu(lst);
	int addBookmarkID=menu.insertItem( SmallIcon("bookmark_add"), i18n("&Add Bookmark"),
		this, SLOT(slotBookmarkDroppedURL()) );
	if (urls.count()==1) {
		d->mDroppedURL=*urls.begin();
	} else {
		menu.setItemEnabled(addBookmarkID, false);
	}

	if (item) {
		menu.insertSeparator();
		KURL dest=item->mBookmark.url();
		FileOperation::fillDropURLMenu(&menu, urls, dest);
	}

	menu.insertSeparator();
	menu.insertItem( SmallIcon("cancel"), i18n("Cancel") );
	menu.exec(QCursor::pos());
}


void BookmarkViewController::slotBookmarkDroppedURL() {
	d->bookmarkURL(d->mDroppedURL);
}


void BookmarkViewController::slotOpenBookmark(QListViewItem* item_) {
	if (!item_) return;
	BookmarkItem* item=static_cast<BookmarkItem*>(item_);
	const KURL& url=item->mBookmark.url();
	if (!url.isValid()) return;
	emit openURL(url);
}


void BookmarkViewController::slotContextMenu(QListViewItem* item_) {
	BookmarkItem* item=static_cast<BookmarkItem*>(item_);
	QPopupMenu menu(d->mListView);
	menu.insertItem(SmallIcon("bookmark_add"), i18n("Add Bookmark..."),
		this, SLOT(bookmarkCurrentURL()));
	menu.insertItem(SmallIcon("bookmark_folder"), i18n("Add Bookmark Folder..."),
		this, SLOT(addBookmarkGroup()));

	if (item) {
		menu.insertSeparator();
		menu.insertItem(SmallIcon("edit"), i18n("Edit..."), 
			this, SLOT(editCurrentBookmark()));
		menu.insertItem(SmallIcon("editdelete"), i18n("Delete"),
			this, SLOT(deleteCurrentBookmark()));
	}
	menu.exec(QCursor::pos());
}


void BookmarkViewController::bookmarkCurrentURL() {
	d->bookmarkURL(d->mCurrentURL);
}


void BookmarkViewController::addBookmarkGroup() {
	BookmarkDialog dialog(d->mListView, BookmarkDialog::BOOKMARK_GROUP);
	if (dialog.exec()==QDialog::Rejected) return;

	KBookmarkGroup parentGroup=d->findBestParentGroup();
	KBookmarkGroup newGroup=parentGroup.createNewFolder(d->mManager, dialog.title());
	newGroup.internalElement().setAttribute("icon", dialog.icon());
	d->mManager->emitChanged(parentGroup);
	QListViewItem* item=d->mListView->currentItem();
	if (item) {
		item->setOpen(true);
	}
}


void BookmarkViewController::editCurrentBookmark() {
	BookmarkItem* item=static_cast<BookmarkItem*>( d->mListView->currentItem() );
	Q_ASSERT(item);
	if (!item) return;
	KBookmark bookmark=item->mBookmark;
	bool isGroup=bookmark.isGroup();
	
	BookmarkDialog dialog(d->mListView,
		isGroup ? BookmarkDialog::BOOKMARK_GROUP : BookmarkDialog::BOOKMARK);

	dialog.setIcon(bookmark.icon());
	dialog.setTitle(bookmark.text());
	if (!isGroup) {
		dialog.setURL(bookmark.url().prettyURL());
	}
	if (dialog.exec()==QDialog::Rejected) return;

	QDomElement element=bookmark.internalElement();
	element.setAttribute("icon", dialog.icon());
	if (!isGroup) {
		element.setAttribute("href", dialog.url());
	}

	// Find title element (or create it if it does not exist)
	QDomElement titleElement;
	QDomNode tmp=element.namedItem("title");
	if (tmp.isNull()) {
		titleElement=element.ownerDocument().createElement("title");
		element.appendChild(titleElement);
	} else {
		titleElement=tmp.toElement();
	}
	Q_ASSERT(!titleElement.isNull());

	// Get title element content (or create)
	QDomText titleText;
	tmp=titleElement.firstChild();
	if (tmp.isNull()) {
		titleText=element.ownerDocument().createTextNode("");
		titleElement.appendChild(titleText);
	} else {
		titleText=tmp.toText();
	}
	Q_ASSERT(!titleText.isNull());

	// Set title (at last!)
	titleText.setData(dialog.title());
	
	KBookmarkGroup group=bookmark.parentGroup();
	d->mManager->emitChanged(group);
}


void BookmarkViewController::deleteCurrentBookmark() {
	BookmarkItem* item=static_cast<BookmarkItem*>( d->mListView->currentItem() );
	Q_ASSERT(item);
	if (!item) return;
	KBookmark bookmark=item->mBookmark;

	QString msg;
	QString title;
	if (bookmark.isGroup()) {
		msg=i18n("Are you sure you want to delete the bookmark folder <b>%1</b>?<br>This will delete the folder and all the bookmarks in it.")
			.arg(bookmark.text());
		title=i18n("Delete Bookmark &Folder");
	} else {
		msg=i18n("Are you sure you want to delete the bookmark <b>%1</b>?")
			.arg(bookmark.text());
		title=i18n("Delete &Bookmark");
	}

	int response=KMessageBox::warningContinueCancel(d->mListView,
		"<qt>" + msg + "</qt>", title,
		KGuiItem(title, "editdelete")
		);
	if (response==KMessageBox::Cancel) return;
	
	KBookmarkGroup group=bookmark.parentGroup();
	group.deleteBookmark(bookmark);
	d->mManager->emitChanged(group);
}


} // namespace
