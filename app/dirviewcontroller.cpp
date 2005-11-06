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
#include <qhbox.h>
#include <qheader.h>

// KDE
#include <kurlbar.h>

// Local
#include <treeview.h>


namespace Gwenview {


struct DirViewController::Private {
	QHBox* mBox;
	KURLBar* mURLBar;
	TreeView* mTreeView;
};


DirViewController::DirViewController(QWidget* parent)
: QObject(parent)
{
	d=new Private;

	d->mBox=new QHBox(parent);
	
	d->mURLBar=new KURLBar(true, d->mBox);
	d->mURLBar->readConfig(KGlobal::config(), "KFileDialog Speedbar");
	d->mURLBar->setIconSize(KIcon::SizeSmall);

	d->mTreeView=new TreeView(d->mBox);
	d->mTreeView->addColumn(QString::null);
	d->mTreeView->header()->hide();
	d->mTreeView->setAllColumnsShowFocus(true);
	d->mTreeView->setRootIsDecorated(false);
	d->mTreeView->setFullWidth(true);
	
	connect(d->mURLBar, SIGNAL(activated(const KURL&)),
		d->mTreeView, SLOT(createBranch(const KURL&)) );
	
	connect(d->mURLBar, SIGNAL(activated(const KURL&)),
		this, SIGNAL(urlChanged(const KURL&)) );

	connect(d->mTreeView, SIGNAL(selectionChanged(QListViewItem*)),
		this, SLOT(slotTreeViewSelectionChanged(QListViewItem*)) );
}

DirViewController::~DirViewController() {
	delete d;
}


QWidget* DirViewController::widget() const {
	return d->mBox;
}


void DirViewController::setURL(const KURL& url) {
	d->mTreeView->setURL(url);
}


void DirViewController::slotTreeViewSelectionChanged(QListViewItem* item) {
	if (!item) return;
	emit urlChanged(d->mTreeView->currentURL());
}


} // namespace
