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

// Local
#include "branchpropertiesdialog.h"

namespace Gwenview {

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
	KListView* mListView;
	KBookmarkManager* mManager;
	KURL mCurrentURL;
	std::auto_ptr<BookmarkToolTip> mToolTip;
	KActionCollection* mActionCollection;

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
};


BookmarkViewController::BookmarkViewController(KListView* listView, KToolBar* toolbar, KBookmarkManager* manager)
: QObject(listView)
{
	d=new Private;
	d->mListView=listView;
	d->mManager=manager;
	d->mToolTip.reset(new BookmarkToolTip(listView) );
	d->mActionCollection=new KActionCollection(listView);

	d->mListView->header()->hide();
	d->mListView->setRootIsDecorated(true);
	d->mListView->addColumn(QString::null);
	d->mListView->setSorting(-1);
	d->mListView->setShowToolTips(false);

	connect(d->mListView, SIGNAL(executed(QListViewItem*)),
		this, SLOT(slotOpenBookmark(QListViewItem*)) );
	connect(d->mListView, SIGNAL(returnPressed(QListViewItem*)),
		this, SLOT(slotOpenBookmark(QListViewItem*)) );
	connect(d->mListView, SIGNAL(contextMenuRequested(QListViewItem*, const QPoint&, int)),
		this, SLOT(slotContextMenu(QListViewItem*)) );

	// For now, we ignore the caller parameter and just refresh the full list on update
	connect(d->mManager, SIGNAL(changed(const QString&, const QString&)),
		this, SLOT(fill()) );

	KAction* action;
	toolbar->setIconText(KToolBar::IconTextRight);
	action=new KAction(i18n("Add a bookmark (keep it short)", "Add"), "bookmark_add", 0, 
			this, SLOT(addBookmark()), d->mActionCollection);
	action->plug(toolbar);
	action=new KAction(i18n("Remove a bookmark (keep it short)", "Remove"), "editdelete", 0,
			this, SLOT(deleteCurrentBookmark()), d->mActionCollection);
	action->plug(toolbar);
	
	fill();
}


BookmarkViewController::~BookmarkViewController() {
	delete d;
}


void BookmarkViewController::setURL(const KURL& url) {
	d->mCurrentURL=url;
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


void BookmarkViewController::slotContextMenu(QListViewItem* item_) {
	BookmarkItem* item=static_cast<BookmarkItem*>(item_);
	QPopupMenu menu(d->mListView);
	menu.insertItem(SmallIcon("bookmark_add"), i18n("Add Bookmark..."),
		this, SLOT(addBookmark()));
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


void BookmarkViewController::addBookmark() {
	BranchPropertiesDialog dialog(d->mListView, BranchPropertiesDialog::BOOKMARK);
	dialog.setTitle(d->mCurrentURL.fileName());
	dialog.setURL(d->mCurrentURL.prettyURL());
	dialog.setIcon(KMimeType::iconForURL(d->mCurrentURL));
	if (dialog.exec()==QDialog::Rejected) return;

	KBookmarkGroup parentGroup=d->findBestParentGroup();
	parentGroup.addBookmark(d->mManager, dialog.title(), dialog.url(), dialog.icon());
	d->mManager->emitChanged(parentGroup);
}


void BookmarkViewController::addBookmarkGroup() {
	BranchPropertiesDialog dialog(d->mListView, BranchPropertiesDialog::BOOKMARK_GROUP);
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
	
	BranchPropertiesDialog dialog(d->mListView,
		isGroup ? BranchPropertiesDialog::BOOKMARK_GROUP : BranchPropertiesDialog::BOOKMARK);

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
