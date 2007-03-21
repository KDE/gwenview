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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "sorteddirmodel.h"

// KDE
#include <kdirmodel.h>

namespace Gwenview {

struct SortedDirModel::Private {
	KDirModel* mSourceModel;
};


SortedDirModel::SortedDirModel(QObject* parent)
: QSortFilterProxyModel(parent)
, d(new SortedDirModel::Private)
{
	d->mSourceModel = new KDirModel(this);
	setSourceModel(d->mSourceModel);
	setDynamicSortFilter(true);
    setSortRole(Qt::DisplayRole);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    sort(KDirModel::Name);
}


KDirLister* SortedDirModel::dirLister() {
	return d->mSourceModel->dirLister();
}


KFileItem* SortedDirModel::itemForIndex(const QModelIndex& index) const {
	if (!index.isValid()) {
		return 0;
	}

	QModelIndex sourceIndex = mapToSource(index);
	return d->mSourceModel->itemForIndex(sourceIndex);
}


QModelIndex SortedDirModel::indexForItem(const KFileItem* item) const {
	if (!item) {
		return QModelIndex();
	}

	QModelIndex sourceIndex = d->mSourceModel->indexForItem(*item);
	return mapFromSource(sourceIndex);
}

} //namespace
