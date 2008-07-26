/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "contextmanager.moc"

// Qt
#include <QTimer>

// KDE
#include <kfileitem.h>

// Local
#include "sidebar.h"
#include "abstractcontextmanageritem.h"
#include <lib/metadata/sorteddirmodel.h>

namespace Gwenview {

struct ContextManagerPrivate {
	QList<AbstractContextManagerItem*> mList;
	KFileItemList mSelection;
	SideBar* mSideBar;
	SortedDirModel* mDirModel;
	KUrl mCurrentDirUrl;
	KUrl mCurrentUrl;

	QTimer* mSelectionDataChangedTimer;

	void scheduleEmittingSelectionDataChanged() {
		mSelectionDataChangedTimer->start();
	}
};


ContextManager::ContextManager(QObject* parent)
: QObject(parent)
, d(new ContextManagerPrivate)
{
	d->mSelectionDataChangedTimer = new QTimer(this);
	d->mSelectionDataChangedTimer->setInterval(500);
	d->mSelectionDataChangedTimer->setSingleShot(true);
	connect(d->mSelectionDataChangedTimer, SIGNAL(timeout()),
		SIGNAL(selectionDataChanged()) );

	d->mSideBar = 0;
	d->mDirModel = 0;
}


ContextManager::~ContextManager() {
	qDeleteAll(d->mList);
	delete d;
}


void ContextManager::setSideBar(SideBar* sideBar) {
	d->mSideBar = sideBar;
}


void ContextManager::addItem(AbstractContextManagerItem* item) {
	Q_ASSERT(d->mSideBar);
	d->mList << item;
	item->setSideBar(d->mSideBar);
}


void ContextManager::setContext(const KUrl& currentUrl, const KFileItemList& selection) {
	d->mCurrentUrl = currentUrl;
	d->mSelection = selection;
	selectionChanged();
}


KFileItemList ContextManager::selection() const {
	return d->mSelection;
}


void ContextManager::setCurrentDirUrl(const KUrl& url) {
	if (url.equals(d->mCurrentDirUrl, KUrl::CompareWithoutTrailingSlash)) {
		return;
	}
	d->mCurrentDirUrl = url;
	currentDirUrlChanged();
}


KUrl ContextManager::currentDirUrl() const {
	return d->mCurrentDirUrl;
}


KUrl ContextManager::currentUrl() const {
	return d->mCurrentUrl;
}


QString ContextManager::currentUrlMimeType() const {
	/*
	if (d->mDocumentPanel->isVisible() && !d->mDocumentPanel->isEmpty()) {
		return MimeTypeUtils::urlMimeType(d->mDocumentPanel->url());
	} else {
		QModelIndex index = d->mThumbnailView->currentIndex();
		if (!index.isValid()) {
			return QString();
		}
		KFileItem item = d->mDirModel->itemForIndex(index);
		Q_ASSERT(!item.isNull());
		return item.mimetype();
	}
	*/
	// FIXME
	Q_FOREACH(const KFileItem& item, d->mSelection) {
		if (item.url() == d->mCurrentUrl) {
			return item.mimetype();
		}
	}
	return QString();
}


SortedDirModel* ContextManager::dirModel() const {
	return d->mDirModel;
}


void ContextManager::setDirModel(SortedDirModel* dirModel) {
	d->mDirModel = dirModel;

	connect(d->mDirModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)),
		SLOT(slotDirModelDataChanged(const QModelIndex&, const QModelIndex&)) );
}


void ContextManager::slotDirModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight) {
	// Look if a selected item has changed, if there is one, schedule emission
	// of a selectionDataChanged() signal. Don't emit it directly to avoid
	// spamming the context items in case of a mass change.
	for (int row=topLeft.row(); row <= bottomRight.row(); ++row) {
		const QModelIndex index = d->mDirModel->index(row, 0);
		const KFileItem item = d->mDirModel->itemForIndex(index);
		if (d->mSelection.contains(item)) {
			d->scheduleEmittingSelectionDataChanged();
			return;
		}
	}
}


} // namespace
