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
#include <kfileplacesmodel.h>

// Local
#include <lib/semanticinfo/sorteddirmodel.h>


namespace Gwenview {


struct Node {
	Node()
	: model(0) {}

	Node(SortedDirModel* _model, const KUrl& _url)
	: model(_model)
	, url(_url)
	{}

	SortedDirModel* model;
	KUrl url;
};

struct FolderModelPrivate {
	FolderModel* q;
	KFilePlacesModel* mPlacesModel;
	QList<SortedDirModel*> mDirModels;

	mutable QSet<Node*> mNodes;

	Node nodeForIndex(const QModelIndex& index) const {
		Q_ASSERT(index.isValid());
		if (!index.internalPointer()) {
			// Place node
			return Node(mDirModels[index.row()], KUrl());
		}

		Node* node = static_cast<Node*>(index.internalPointer());
		return *node;
	}


	QModelIndex createIndex(SortedDirModel* dirModel, const KUrl& url) const {
		if (!url.isValid()) {
			// Root index of dirModel == place node
			int row = mDirModels.indexOf(const_cast<SortedDirModel*>(dirModel));
			return row != -1 ? q->createIndex(row, 0) : QModelIndex();
		}
		Node* node = 0;
		// FIXME: Inefficient
		Q_FOREACH(Node* tmp, mNodes) {
			if (tmp->model == dirModel && tmp->url == url) {
				node = tmp;
				break;
			}
		}
		if (!node) {
			node = new Node(dirModel, url);
			mNodes.insert(node);
		}
		QModelIndex dirIndex = dirModel->indexForUrl(url);
		return q->createIndex(dirIndex.row(), dirIndex.column(), node);
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
	qDeleteAll(d->mNodes);
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
		const Node node = d->nodeForIndex(index);
		const QModelIndex dirIndex = node.model->indexForUrl(node.url);
		return node.model->data(dirIndex, role);
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

	Node parentNode = d->nodeForIndex(parent);
	SortedDirModel* dirModel = parentNode.model;
	QModelIndex parentDirIndex = dirModel->indexForUrl(parentNode.url);
	QModelIndex dirIndex = dirModel->index(row, column, parentDirIndex);
	KUrl url = dirModel->urlForIndex(dirIndex);
	return d->createIndex(dirModel, url);
}


QModelIndex FolderModel::parent(const QModelIndex& index) const {
	if (!index.isValid()) {
		return QModelIndex();
	}
	if (!index.internalPointer()) {
		// index is a place => parent is invisible root item
		return QModelIndex();
	}
	const Node node = d->nodeForIndex(index);
	// FIXME: Use KUrl::parent()?
	const QModelIndex dirIndex = node.model->indexForUrl(node.url);
	const QModelIndex parentDirIndex = node.model->parent(dirIndex);
	if (parentDirIndex.isValid()) {
		return d->createIndex(node.model, node.model->urlForIndex(parentDirIndex));
	} else {
		// index is a direct child of a place
		int row = d->mDirModels.indexOf(const_cast<SortedDirModel*>(node.model));
		return row != -1 ? createIndex(row, 0) : QModelIndex();
	}
}


int FolderModel::rowCount(const QModelIndex& parent) const {
	if (!parent.isValid()) {
		// This is the invisible root item
		return d->mDirModels.size();
	}

	// parent is a place or a dir
	const Node node = d->nodeForIndex(parent);
	const QModelIndex dirIndex = node.model->indexForUrl(node.url);
	return node.model->rowCount(dirIndex);
}


bool FolderModel::hasChildren(const QModelIndex& parent) const {
	if (!parent.isValid()) {
		return true;
	}
	if (!parent.internalPointer()) {
		// parent is a place node => always allow expanding
		return true;
	}
	const Node node = d->nodeForIndex(parent);
	const QModelIndex dirIndex = node.model->indexForUrl(node.url);
	return node.model->hasChildren(dirIndex);
}


bool FolderModel::canFetchMore(const QModelIndex& parent) const {
	if (!parent.isValid()) {
		return d->mPlacesModel->canFetchMore(QModelIndex());
	}
	const Node node = d->nodeForIndex(parent);
	if (!node.url.isValid() && !node.model->dirLister()->url().isValid()) {
		// Special case to avoid calling openUrl on all places at startup
		return true;
	}
	const QModelIndex dirIndex = node.model->indexForUrl(node.url);
	return node.model->canFetchMore(dirIndex);
}


void FolderModel::fetchMore(const QModelIndex& parent) {
	if (!parent.isValid()) {
		d->mPlacesModel->fetchMore(QModelIndex());
	}
	const Node node = d->nodeForIndex(parent);
	if (!node.url.isValid() && !node.model->dirLister()->url().isValid()) {
		KUrl url = d->mPlacesModel->url(d->mPlacesModel->index(parent.row(), 0));
		node.model->dirLister()->openUrl(url);
	}
	const QModelIndex dirIndex = node.model->indexForUrl(node.url);
	node.model->fetchMore(dirIndex);
}


void FolderModel::slotPlacesRowsInserted(const QModelIndex& /*parent*/, int start, int end) {
	beginInsertRows(QModelIndex(), start, end);
	for (int row=start; row<=end; ++row) {
		SortedDirModel* dirModel = new SortedDirModel(this);
		connect(dirModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)),
			SLOT(slotDirRowsInserted(const QModelIndex&, int, int)));
		connect(dirModel, SIGNAL(rowsAboutToBeRemoved(const QModelIndex&, int, int)),
			SLOT(slotDirRowsAboutToBeRemoved(const QModelIndex&, int, int)));

		d->mDirModels.insert(row, dirModel);
		KDirLister* lister = dirModel->dirLister();
		lister->setDirOnlyMode(true);
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


void FolderModel::slotDirRowsInserted(const QModelIndex& parentDirIndex, int start, int end) {
	SortedDirModel* model = static_cast<SortedDirModel*>(sender());
	KUrl url = model->urlForIndex(parentDirIndex);
	beginInsertRows(d->createIndex(model, url), start, end);
	endInsertRows();
}


void FolderModel::slotDirRowsAboutToBeRemoved(const QModelIndex& parentDirIndex, int start, int end) {
	SortedDirModel* model = static_cast<SortedDirModel*>(sender());
	KUrl url = model->urlForIndex(parentDirIndex);
	beginRemoveRows(d->createIndex(model, url), start, end);
	endRemoveRows();
}


KUrl FolderModel::urlForIndex(const QModelIndex& index) const {
	if (!index.internalPointer()) {
		return d->mPlacesModel->url(d->mPlacesModel->index(index.row(), 0));
	} else {
		const Node node = d->nodeForIndex(index);
		return node.url;
	}
}


} // namespace
