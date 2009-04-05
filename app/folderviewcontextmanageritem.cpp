// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <aurelien.gateau@free.fr>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "folderviewcontextmanageritem.h"

// Qt
#include <QDir>
#include <QTreeView>

// KDE
#include <kdebug.h>
#include <kfileitem.h>
#include <kdirlister.h>
#include <kdirmodel.h>
#include <klocale.h>

// Local
#include "contextmanager.h"
#include "sidebar.h"

namespace Gwenview {

struct FolderViewContextManagerItemPrivate {
	FolderViewContextManagerItem* q;
	SideBar* mSideBar;
	SideBarGroup* mGroup;
	KDirModel* mModel;
	QTreeView* mView;

	void setupModel() {
		mModel = new KDirModel(q);
		KDirLister* lister = mModel->dirLister();
		lister->setDirOnlyMode(true);
		lister->openUrl(QDir::homePath());
	}

	void setupView() {
		mView = new QTreeView;
		mView->setModel(mModel);
		mView->setEditTriggers(QAbstractItemView::NoEditTriggers);
		mGroup->addWidget(mView);
		QObject::connect(mView, SIGNAL(activated(const QModelIndex&)),
			q, SLOT(slotActivated(const QModelIndex&)));
	}
};


FolderViewContextManagerItem::FolderViewContextManagerItem(ContextManager* manager)
: AbstractContextManagerItem(manager)
, d(new FolderViewContextManagerItemPrivate) {
	d->q = this;
	d->mSideBar = 0;
	d->mGroup = 0;
	d->mModel = 0;
	d->mView = 0;

	connect(contextManager(), SIGNAL(currentDirUrlChanged()),
		SLOT(updateSideBarContent()) );
}


FolderViewContextManagerItem::~FolderViewContextManagerItem() {
	delete d;
}


void FolderViewContextManagerItem::setSideBar(SideBar* sideBar) {
	d->mSideBar = sideBar;
	connect(sideBar, SIGNAL(aboutToShow()),
		SLOT(updateSideBarContent()) );

	d->mGroup = sideBar->createGroup(i18n("Folders"));
}


void FolderViewContextManagerItem::updateSideBarContent() {
	if (!d->mSideBar || !d->mSideBar->isVisible()) {
		return;
	}

	if (!d->mModel) {
		d->setupModel();
		d->setupView();
	}

	/* FIXME: Expand to url
	KUrl url = contextManager()->currentDirUrl();
	if (url.isValid()) {
		d->mModel->dirLister()->openUrl(url);
	}
	*/
}


void FolderViewContextManagerItem::slotActivated(const QModelIndex& index) {
	if (!index.isValid()) {
		return;
	}

	KFileItem item = d->mModel->itemForIndex(index);
	emit urlChanged(item.url());
}


} // namespace
