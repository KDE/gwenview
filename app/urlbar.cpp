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
#include <kiconloader.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kurl.h>
#include <kurldrag.h>

// Local
#include "../gvcore/fileoperation.h"

namespace Gwenview {


class URLBarItem : public QListViewItem {
public:
	URLBarItem(KListView* parent, const QString& text)
	: QListViewItem(parent, text)
	{}

	KURL url() const { return mURL; }
	void setURL(const KURL& url) { mURL=url; }
	
private:
	KURL mURL;
};


struct URLBar::Private {
	KURL mDroppedURL;
	URLBar* mView;

	void addItem(const KURL& url, QString description, QString icon) {
		if (description.isEmpty()) {
			description=url.fileName();
		}
		
		if (icon.isEmpty()) {
			icon=KMimeType::iconForURL(url);
		}
		QPixmap pix=SmallIcon(icon);

		URLBarItem* item=new URLBarItem(mView, description);
		QListViewItem* last=mView->lastItem();
		if (last) {
			item->moveItem(last);
		}
		item->setPixmap(0, pix);
		item->setURL(url);
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
	setSorting(-1);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	
	setDropVisualizer(true);
	setDropHighlighter(true);
	setAcceptDrops(true);
	
	connect(this, SIGNAL(clicked(QListViewItem*)),
		this, SLOT(slotClicked(QListViewItem*)) );
	
	connect(this, SIGNAL(contextMenu(KListView*, QListViewItem*, const QPoint&)),
		this, SLOT(slotContextMenu(KListView*, QListViewItem*, const QPoint&)) );
}


URLBar::~URLBar() {
	delete d;
}


void URLBar::readConfig(KConfig* config, const QString& group) {
	config->setGroup(group);
	int entryCount=config->readNumEntry("Number of Entries");
	for (int pos=0; pos<entryCount; ++pos) {
		KURL url( config->readPathEntry(QString("URL_%1").arg(pos)) );
		QString description=config->readEntry( QString("Description_%1").arg(pos) );
		QString icon=config->readEntry( QString("Icon_%1").arg(pos) );
		
		d->addItem(url, description, icon);
	}
}


void URLBar::writeConfig(KConfig*, const QString& /*group*/) {
	/* @todo implement */
}


void URLBar::slotClicked(QListViewItem* item) {
	if (!item) return;

	URLBarItem* urlItem=static_cast<URLBarItem*>(item);
	emit activated(urlItem->url());
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
	d->addItem(d->mDroppedURL, QString::null, QString::null);
}


void URLBar::editBookmark() {
	/* @todo implement */
}


void URLBar::deleteBookmark() {
	QListViewItem* item=selectedItem();
	if (!item) return;
	delete item;
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
		KURL dest=item->url();
		FileOperation::fillDropURLMenu(&menu, urls, dest);
	}
	menu.insertSeparator();
	menu.insertItem( SmallIcon("cancel"), i18n("Cancel") );
	menu.exec(QCursor::pos());
}

} // namespace
