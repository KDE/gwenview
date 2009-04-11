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
#include <QTreeView>

// KDE
#include <kdebug.h>
#include <klocale.h>

// Local
#include "contextmanager.h"
#include "placetreemodel.h"
#include "sidebar.h"

namespace Gwenview {

struct FolderViewContextManagerItemPrivate {
	FolderViewContextManagerItem* q;
	PlaceTreeModel* mModel;
	QTreeView* mView;

	void setupModel() {
		mModel = new PlaceTreeModel(q);
		mView->setModel(mModel);
	}

	void setupView() {
		mView = new QTreeView;
		mView->setEditTriggers(QAbstractItemView::NoEditTriggers);
		mView->setHeaderHidden(true);
		q->setWidget(mView);
		QObject::connect(mView, SIGNAL(activated(const QModelIndex&)),
			q, SLOT(slotActivated(const QModelIndex&)));
		new AboutToShowHelper(mView, q, SLOT(updateContent()));
	}
};


FolderViewContextManagerItem::FolderViewContextManagerItem(ContextManager* manager)
: AbstractContextManagerItem(manager)
, d(new FolderViewContextManagerItemPrivate) {
	d->q = this;
	d->mModel = 0;

	d->setupView();

	connect(contextManager(), SIGNAL(currentDirUrlChanged()),
		SLOT(updateContent()) );
}


FolderViewContextManagerItem::~FolderViewContextManagerItem() {
	delete d;
}


void FolderViewContextManagerItem::updateContent() {
	if (!d->mView->isVisible()) {
		return;
	}

	if (!d->mModel) {
		d->setupModel();
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

	KUrl url = d->mModel->urlForIndex(index);
	emit urlChanged(url);
}


} // namespace
