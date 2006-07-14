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
#include "dirviewcontroller.moc"

// Qt
#include <qpopupmenu.h>

// KDE
#include <kdebug.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kpropsdlg.h>

// Local
#include <treeview.h>
#include <gvcore/fileoperation.h>


namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

struct DirViewController::Private {
	TreeView* mTreeView;
};


DirViewController::DirViewController(QWidget* parent)
: QObject(parent)
{
	d=new Private;

	d->mTreeView=new TreeView(parent);

	connect(d->mTreeView, SIGNAL(selectionChanged(QListViewItem*)),
		this, SLOT(slotTreeViewSelectionChanged(QListViewItem*)) );

	connect(d->mTreeView, SIGNAL(contextMenu(KListView*, QListViewItem*, const QPoint&)),
		this, SLOT(slotTreeViewContextMenu(KListView*, QListViewItem*, const QPoint&)) );
}


DirViewController::~DirViewController() {
	delete d;
}


QWidget* DirViewController::widget() const {
	return d->mTreeView;
}


void DirViewController::setURL(const KURL& url) {
	d->mTreeView->setURL(url);
}


void DirViewController::slotTreeViewSelectionChanged(QListViewItem* item) {
	if (!item) return;
	LOG(d->mTreeView->currentURL());
	emit urlChanged(d->mTreeView->currentURL());
}


void DirViewController::slotTreeViewContextMenu(KListView*, QListViewItem*, const QPoint& pos) {
	QPopupMenu menu(d->mTreeView);
	menu.insertItem(SmallIcon("folder_new"),i18n("New Folder..."),this,SLOT(makeDir()));
	menu.insertSeparator();
	menu.insertItem(i18n("Rename..."),this,SLOT(renameDir()));
	menu.insertItem(SmallIcon("editdelete"),i18n("Delete"),this,SLOT(removeDir()));
	menu.insertSeparator();
	menu.insertItem(i18n("Properties"),this,SLOT(showPropertiesDialog()));
	
	menu.exec(pos);
}


void DirViewController::makeDir() {
	if (!d->mTreeView->currentItem()) return;
	FileOperation::makeDir(d->mTreeView->currentURL(), d->mTreeView, this, SLOT(slotDirMade()) );
}


void DirViewController::slotDirMade() {
	if (!d->mTreeView->currentItem()) return;
	d->mTreeView->currentItem()->setOpen(true);
}


void DirViewController::renameDir() {
	if (!d->mTreeView->currentItem()) return;
	FileOperation::rename(d->mTreeView->currentURL(), d->mTreeView);
}


void DirViewController::removeDir() {
	if (!d->mTreeView->currentItem()) return;

	KURL::List list;
	list << d->mTreeView->currentURL();
	FileOperation::del(list, d->mTreeView);
	
	QListViewItem* item=d->mTreeView->currentItem();
	if (!item) return;
	item=item->parent();
	if (!item) return;
	d->mTreeView->setCurrentItem(item);
}


void DirViewController::showPropertiesDialog() {
	(void)new KPropertiesDialog(d->mTreeView->currentURL(), d->mTreeView);
}

} // namespace
