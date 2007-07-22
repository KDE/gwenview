// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "fileopscontextmanageritem.h"

// Qt
#include <QAction>

// KDE
#include <kfileitem.h>
#include <klocale.h>
#include <kpropertiesdialog.h>

// Local
#include "contextmanager.h"
#include "sidebar.h"

namespace Gwenview {


struct FileOpsContextManagerItemPrivate {
	SideBar* mSideBar;
	SideBarGroup* mGroup;
	QAction* mShowPropertiesAction;
};


FileOpsContextManagerItem::FileOpsContextManagerItem(ContextManager* manager)
: AbstractContextManagerItem(manager)
, d(new FileOpsContextManagerItemPrivate) {
	connect(contextManager(), SIGNAL(selectionChanged()),
		SLOT(updateSideBarContent()) );
	
	d->mShowPropertiesAction = new QAction(this);
	d->mShowPropertiesAction->setText(i18n("Properties"));
	d->mShowPropertiesAction->setIcon(KIcon("file-properties"));
	connect(d->mShowPropertiesAction, SIGNAL(triggered()),
		SLOT(showProperties()) );
}


FileOpsContextManagerItem::~FileOpsContextManagerItem() {
	delete d;
}


void FileOpsContextManagerItem::setSideBar(SideBar* sideBar) {
	d->mSideBar = sideBar;
	connect(sideBar, SIGNAL(aboutToShow()),
		SLOT(updateSideBarContent()) );

	d->mGroup = sideBar->createGroup(i18n("File Operations"));
	d->mGroup->hide();
}


void FileOpsContextManagerItem::updateSideBarContent() {
	if (!d->mSideBar->isVisible()) {
		return;
	}

	QList<KFileItem> list = contextManager()->selection();
	if (list.count() == 0) {
		d->mGroup->hide();
		return;
	}

	d->mGroup->show();
	d->mGroup->clear();
	d->mGroup->addAction(d->mShowPropertiesAction);
}


void FileOpsContextManagerItem::showProperties() {
	QList<KFileItem> list = contextManager()->selection();
	KFileItemList itemList;
	Q_FOREACH(KFileItem item, list) {
		itemList << &item;
	}
	KPropertiesDialog::showDialog(itemList, d->mSideBar);
}


} // namespace
