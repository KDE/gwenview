// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
#include "foldermodel.moc"

// Qt

// KDE
#include <kdebug.h>
#include <kdirlister.h>
#include <kdirmodel.h>
#include <kfileplacesmodel.h>

// Local

/*
 * Home          0
 *   photos      KDirModel*
 *     2008      KDirModel*
 *     2009
 *   documents
 *   music
 *
 *
 *
 */

namespace Gwenview {

typedef QPair<KDirModel*, QPersistentModelIndex> DirModelIndexPair;

struct FolderModelPrivate {
	FolderModel* q;
	KFilePlacesModel* mPlacesModel;
	QList<KDirModel*> mDirModels;

	mutable QSet<DirModelIndexPair*> mPairs;

	DirModelIndexPair pairForIndex(const QModelIndex& index) const {
		Q_ASSERT(index.isValid());
		if (!index.internalPointer()) {
			// Place node
			return DirModelIndexPair(mDirModels[index.row()], QModelIndex());
		}

		DirModelIndexPair* pair = static_cast<DirModelIndexPair*>(index.internalPointer());
		return *pair;
	}


	QModelIndex createIndexForDirModelAndIndex(KDirModel* dirModel, const QModelIndex& dirIndex) const {
		DirModelIndexPair* pair = 0;
		// FIXME: Inefficient
		Q_FOREACH(DirModelIndexPair* tmp, mPairs) {
			if (tmp->first == dirModel && tmp->second == dirIndex) {
				pair = tmp;
				break;
			}
		}
		if (!pair) {
			pair = new DirModelIndexPair(dirModel, dirIndex);
			mPairs.insert(pair);
		}
		return q->createIndex(dirIndex.row(), dirIndex.column(), pair);
	}
};


FolderModel::FolderModel(QObject* parent)
: QAbstractItemModel(parent)
, d(new FolderModelPrivate) {
	d->q = this;

	d->mPlacesModel = new KFilePlacesModel(this);
	connect(d->mPlacesModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)),
		SLOT(slotPlacesRowsInserted(const QModelIndex&, int, int)));
	connect(d->mPlacesModel, SIGNAL(rowsAboutToBeRemoved(const QModelIndex&, int, int)),
		SLOT(slotPlacesRowsAboutToBeRemoved(const QModelIndex&, int, int)));

	// Bootstrap
	slotPlacesRowsInserted(QModelIndex(), 0, d->mPlacesModel->rowCount() - 1);
}


FolderModel::~FolderModel() {
	qDeleteAll(d->mPairs);
	delete d;
}


int FolderModel::columnCount(const QModelIndex&) const {
	return 1;
}


QVariant FolderModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid()) {
		return QVariant();
	}
	if (!index.internalPointer()) {
		const QModelIndex placesIndex = d->mPlacesModel->index(index.row(), index.column());
		return d->mPlacesModel->data(placesIndex, role);
	} else {
		const DirModelIndexPair pair = d->pairForIndex(index);
		return pair.first->data(pair.second, role);
	}
}


QModelIndex FolderModel::index(int row, int column, const QModelIndex& parent) const {
	if (column != 0) {
		return QModelIndex();
	}
	if (!parent.isValid()) {
		// User wants to create a places index
		if (row < d->mDirModels.size()) {
			return createIndex(row, column);
		} else {
			return QModelIndex();
		}
	}

	DirModelIndexPair pair = d->pairForIndex(parent);
	KDirModel* dirModel = pair.first;
	const QModelIndex dirIndex = dirModel->index(row, column, pair.second);
	return d->createIndexForDirModelAndIndex(dirModel, dirIndex);
}


QModelIndex FolderModel::parent(const QModelIndex& index) const {
	if (!index.isValid()) {
		return QModelIndex();
	}
	if (!index.internalPointer()) {
		// index is a place
		return QModelIndex();
	} else {
		const DirModelIndexPair pair = d->pairForIndex(index);
		const QModelIndex parentDirIndex = pair.first->parent(pair.second);
		if (parentDirIndex.isValid()) {
			return d->createIndexForDirModelAndIndex(pair.first, parentDirIndex);
		} else {
			// index is a direct child of a place
			int row = d->mDirModels.indexOf(const_cast<KDirModel*>(pair.first));
			return row != -1 ? createIndex(row, 0) : QModelIndex();
		}
	}
}


int FolderModel::rowCount(const QModelIndex& parent) const {
	if (!parent.isValid()) {
		// This is the invisible root item
		return d->mDirModels.size();
	}

	// parent is a place or a dir
	const DirModelIndexPair pair = d->pairForIndex(parent);
	return pair.first->rowCount(pair.second);
}


bool FolderModel::hasChildren(const QModelIndex& parent) const {
	return QAbstractItemModel::hasChildren(parent);
	/*
	if (!parent.isValid()) {
		return true;
	}
	if (!parent.internalPointer()) {
		// parent is a place node always allow expanding
		return true;
	}
	const DirModelIndexPair pair = d->pairForIndex(parent);
	return pair.first->hasChildren(pair.second);
	*/
}


bool FolderModel::canFetchMore(const QModelIndex& parent) const {
	return QAbstractItemModel::canFetchMore(parent);
	/*
	if (!parent.isValid()) {
		return d->mPlacesModel->canFetchMore();
	}
	*/
}


void FolderModel::slotPlacesRowsInserted(const QModelIndex& parent, int start, int end) {
	beginInsertRows(QModelIndex(), start, end);
	for (int row=start; row<=end; ++row) {
		KDirModel* dirModel = new KDirModel(this);
		d->mDirModels.insert(row, dirModel);
		KDirLister* lister = dirModel->dirLister();
		lister->setDirOnlyMode(true);
		KUrl url = d->mPlacesModel->url(d->mPlacesModel->index(row, 0, parent));
		lister->openUrl(url);
	}
	endInsertRows();
}


void FolderModel::slotPlacesRowsAboutToBeRemoved(const QModelIndex&, int start, int end) {
	beginRemoveRows(QModelIndex(), start, end);
	for (int row=end; row>=start; --row) {
		delete d->mDirModels.takeAt(row);
	}
	endRemoveRows();
}


KUrl FolderModel::urlForIndex(const QModelIndex& index) const {
	if (!index.internalPointer()) {
		return d->mPlacesModel->url(d->mPlacesModel->index(index.row(), 0));
	} else {
		const DirModelIndexPair pair = d->pairForIndex(index);
		KFileItem item = pair.first->itemForIndex(pair.second);
		return item.isNull() ? KUrl() : item.url();
	}
}


} // namespace
