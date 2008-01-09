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
#include "sorteddirmodel.moc"

// Qt

// KDE
#include <kdirlister.h>
#include <kdirmodel.h>

// Local

namespace Gwenview {

struct SortedDirModelPrivate {
	KDirModel* mSourceModel;
};


SortedDirModel::SortedDirModel(QObject* parent)
: KDirSortFilterProxyModel(parent)
, d(new SortedDirModelPrivate)
{
	d->mSourceModel = new KDirModel(this);
	setSourceModel(d->mSourceModel);
}


SortedDirModel::~SortedDirModel() {
	delete d;
}


KDirLister* SortedDirModel::dirLister() {
	return d->mSourceModel->dirLister();
}


KFileItem SortedDirModel::itemForIndex(const QModelIndex& index) const {
	if (!index.isValid()) {
		return KFileItem();
	}

	QModelIndex sourceIndex = mapToSource(index);
	return d->mSourceModel->itemForIndex(sourceIndex);
}


QModelIndex SortedDirModel::indexForItem(const KFileItem& item) const {
	if (item.isNull()) {
		return QModelIndex();
	}

	QModelIndex sourceIndex = d->mSourceModel->indexForItem(item);
	return mapFromSource(sourceIndex);
}


QModelIndex SortedDirModel::indexForUrl(const KUrl& url) const {
	if (!url.isValid()) {
		return QModelIndex();
	}
	QModelIndex sourceIndex = d->mSourceModel->indexForUrl(url);
	return mapFromSource(sourceIndex);
}


} //namespace
