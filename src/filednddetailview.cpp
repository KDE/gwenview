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

// KDE includes
#include <kdebug.h>
#include <kurldrag.h>

// Our includes
#include "filednddetailview.h"


FileDnDDetailView::FileDnDDetailView(QWidget *parent, const char *name)
: KFileDetailView(parent,name) {
	setDragEnabled(true);
	KFileDetailView::setSelectionMode(KFile::Extended);
}


void FileDnDDetailView::startDrag(){
	KFileItem* item=KFileView::selectedItems()->getFirst();
	if (!item) {
		kdWarning() << "No item to drag\n";
		return;
	}

	KURL::List urls;
	urls.append(item->url());

	QDragObject* drag = KURLDrag::newDrag( urls, this );
	drag->setPixmap( item->pixmap(0) );

	drag->dragCopy();
}


void FileDnDDetailView::updateView(const KFileItem* fileItem) {
	if (!fileItem) return;

	KFileDetailView::updateView(fileItem);

// WARNING : This code makes a lot of assumption on the way KFileView works
	KFileListViewItem* listItem=static_cast<KFileListViewItem*>(const_cast<void*>( fileItem->extraData(this) ) );
	if (!listItem) return;
	
	listItem->setText(0,fileItem->text());
	listItem->setKey(fileItem->text());
	sort(); // FIXME : Find out why this doesn't really sort the list on rename
}

