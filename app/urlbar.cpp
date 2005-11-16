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
#include <qheader.h>

// KDE
#include <kiconloader.h>
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
	void setURL(KURL& url) { mURL=url; }
	
private:
	KURL mURL;
};


URLBar::URLBar(QWidget* parent)
: KListView(parent)
{
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
}


URLBar::~URLBar() {
}


void URLBar::readConfig(KConfig* config, const QString& group) {
	config->setGroup(group);
	int entryCount=config->readNumEntry("Number of Entries");
	for (int pos=0; pos<entryCount; ++pos) {
		KURL url( config->readPathEntry(QString("URL_%1").arg(pos)) );
		QString description=config->readEntry( QString("Description_%1").arg(pos) );
		if (description.isEmpty()) {
			description=url.fileName();
		}
		QString icon=config->readEntry( QString("Icon_%1").arg(pos) );
		if (icon.isEmpty()) {
			icon=KMimeType::iconForURL(url);
		}
		QPixmap pix=SmallIcon(icon);

		URLBarItem* item=new URLBarItem(this, description);
		QListViewItem* last=lastItem();
		if (last) {
			item->moveItem(last);
		}
		item->setPixmap(0, pix);
		item->setURL(url);
	}
}


void URLBar::writeConfig(KConfig*, const QString& group) {
}


void URLBar::slotClicked(QListViewItem* item) {
	if (!item) return;

	URLBarItem* urlItem=static_cast<URLBarItem*>(item);
	emit activated(urlItem->url());
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
	if (!KURLDrag::decode(event,urls)) return;

	// Get a pointer to the drop item
	QPoint point(0,event->pos().y());
	URLBarItem* item=static_cast<URLBarItem*>( itemAt(contentsToViewport(point)) );
	if (!item) return;
	KURL dest=item->url();
	
	FileOperation::openDropURLMenu(this, urls, dest);
}

} // namespace
