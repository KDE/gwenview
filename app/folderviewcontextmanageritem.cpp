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
#include <QHeaderView>
#include <QTreeView>

// KDE
#include <kdebug.h>
#include <klocale.h>

// Local
#include "contextmanager.h"
#include "sidebar.h"
#include "fileoperations.h"

#define USE_PLACETREE
#ifdef USE_PLACETREE
#include "placetreemodel.h"
#define MODEL_CLASS PlaceTreeModel
#else
#include <kdirlister.h>
#include <lib/semanticinfo/sorteddirmodel.h>
#define MODEL_CLASS SortedDirModel
#endif


namespace Gwenview {

struct FolderViewContextManagerItemPrivate {
	FolderViewContextManagerItem* q;
	MODEL_CLASS* mModel;
	QTreeView* mView;

	void setupModel() {
		mModel = new MODEL_CLASS(q);
		mView->setModel(mModel);
		#ifndef USE_PLACETREE
		for (int col = 1; col <= mModel->columnCount(); ++col) {
			mView->header()->setSectionHidden(col, true);
		}
		mModel->dirLister()->openUrl(KUrl("/"));
		#endif
	}

	void setupView() {
		mView = new QTreeView;
		mView->setEditTriggers(QAbstractItemView::NoEditTriggers);
		mView->setHeaderHidden(true);
		mView->setFrameStyle(QFrame::NoFrame);

		// This is tricky: QTreeView header has stretchLastSection set to true.
		// In this configuration, the header gets quite wide and cause an
		// horizontal scrollbar to appear.
		// To avoid this, set stretchLastSection to false and resizeMode to
		// Stretch (we still want the column to take the full width of the
		// widget).
		mView->header()->setStretchLastSection(false);
		mView->header()->setResizeMode(QHeaderView::Stretch);

		QPalette p = mView->palette();
		p.setColor(QPalette::Active,   QPalette::Text, p.color(QPalette::Active,   QPalette::WindowText));
		p.setColor(QPalette::Inactive, QPalette::Text, p.color(QPalette::Inactive, QPalette::WindowText));
		p.setColor(QPalette::Disabled, QPalette::Text, p.color(QPalette::Disabled, QPalette::WindowText));
		mView->setPalette(p);
		mView->viewport()->setAutoFillBackground(false);

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
