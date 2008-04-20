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
#include <kdebug.h>
#include <kdirlister.h>

// Local
#include "metadatadirmodel.h"

namespace Gwenview {


struct SortedDirModelPrivate {
	MetaDataDirModel* mSourceModel;
	QStringList mMimeExcludeFilter;
	int mMinimumRating;
};


SortedDirModel::SortedDirModel(QObject* parent)
: KDirSortFilterProxyModel(parent)
, d(new SortedDirModelPrivate)
{
	d->mSourceModel = new MetaDataDirModel(this);
	d->mMinimumRating = 0;
	setSourceModel(d->mSourceModel);
}


SortedDirModel::~SortedDirModel() {
	delete d;
}


KDirLister* SortedDirModel::dirLister() {
	return d->mSourceModel->dirLister();
}


void SortedDirModel::setMinimumRating(int rating) {
	kDebug() << "rating=" << rating;
	d->mMinimumRating = rating;
	invalidateFilter();
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


void SortedDirModel::setMimeExcludeFilter(const QStringList &mimeList) {
	if (d->mMimeExcludeFilter == mimeList) {
		return;
	}
	d->mMimeExcludeFilter = mimeList;
	invalidateFilter();
}


bool SortedDirModel::filterAcceptsRow(int row, const QModelIndex& parent) const {
	QModelIndex index = d->mSourceModel->index(row, 0, parent);
	if (!d->mMimeExcludeFilter.isEmpty()) {
		QString mimeType = d->mSourceModel->itemForIndex(index).mimetype();
		if (d->mMimeExcludeFilter.contains(mimeType)) {
			return false;
		}
	}
	if (d->mMinimumRating > 0) {
		if (d->mSourceModel->metaDataAvailableForIndex(index)) {
			int rating = d->mSourceModel->data(index, MetaDataDirModel::RatingRole).toInt();
			if (rating < d->mMinimumRating) {
				return false;
			}
		} else {
			d->mSourceModel->retrieveMetaDataForIndex(index);
			return false;
		}
	}
	return KDirSortFilterProxyModel::filterAcceptsRow(row, parent);
}


} //namespace
