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
#include <config-gwenview.h>

// Qt

// KDE
#include <kdebug.h>
#include <kdirlister.h>

// Local
#ifdef GWENVIEW_SEMANTICINFO_BACKEND_NONE
#include <kdirmodel.h>
#else
#include "abstractsemanticinfobackend.h"
#include "semanticinfodirmodel.h"
#endif

namespace Gwenview {


struct SortedDirModelPrivate {
#ifdef GWENVIEW_SEMANTICINFO_BACKEND_NONE
	KDirModel* mSourceModel;
#else
	SemanticInfoDirModel* mSourceModel;
	TagSet mTagSet;
#endif
	QStringList mBlackListedExtensions;
	QStringList mMimeExcludeFilter;
	int mMinimumRating;
};


SortedDirModel::SortedDirModel(QObject* parent)
: KDirSortFilterProxyModel(parent)
, d(new SortedDirModelPrivate)
{
#ifdef GWENVIEW_SEMANTICINFO_BACKEND_NONE
	d->mSourceModel = new KDirModel(this);
#else
	d->mSourceModel = new SemanticInfoDirModel(this);
#endif
	d->mMinimumRating = 0;
	setSourceModel(d->mSourceModel);
}


SortedDirModel::~SortedDirModel() {
	delete d;
}


KDirLister* SortedDirModel::dirLister() {
	return d->mSourceModel->dirLister();
}


void SortedDirModel::setBlackListedExtensions(const QStringList& list) {
	d->mBlackListedExtensions = list;
}


void SortedDirModel::setMinimumRating(int rating) {
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
	KFileItem fileItem = d->mSourceModel->itemForIndex(index);

	QString extension = fileItem.name().section('.', -1).toLower();
	if (d->mBlackListedExtensions.contains(extension)) {
		return false;
	}

	if (!d->mMimeExcludeFilter.isEmpty()) {
		if (d->mMimeExcludeFilter.contains(fileItem.mimetype())) {
			return false;
		}
	}
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
	if (d->mMinimumRating > 0 || !d->mTagSet.isEmpty()) {
		// Make sure we have metadata, otherwise retrieve it and return false,
		// we will be called again later when metadata is there.
		if (!d->mSourceModel->semanticInfoAvailableForIndex(index)) {
			d->mSourceModel->retrieveSemanticInfoForIndex(index);
			return false;
		}
	}

	if (d->mMinimumRating > 0) {
		int rating = d->mSourceModel->data(index, SemanticInfoDirModel::RatingRole).toInt();
		if (rating < d->mMinimumRating) {
			return false;
		}
	}

	if (!d->mTagSet.isEmpty()) {
		TagSet indexTagSet = TagSet::fromVariant(d->mSourceModel->data(index, SemanticInfoDirModel::TagsRole));
		TagSet commonSet = indexTagSet & d->mTagSet;
		if (commonSet.size() < d->mTagSet.size()) {
			return false;
		}
	}
#endif
	return KDirSortFilterProxyModel::filterAcceptsRow(row, parent);
}


AbstractSemanticInfoBackEnd* SortedDirModel::semanticInfoBackEnd() const {
#ifdef GWENVIEW_SEMANTICINFO_BACKEND_NONE
	return 0;
#else
	return d->mSourceModel->semanticInfoBackEnd();
#endif
}


void SortedDirModel::setTagSetFilter(const TagSet& tagSet) {
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
	kDebug() << tagSet;
	d->mTagSet = tagSet;
	invalidateFilter();
#endif
}


} //namespace
