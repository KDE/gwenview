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
#include <kdatetime.h>
#include <kdirlister.h>
#include <kfilemetainfo.h>
#include <kfilemetainfoitem.h>

// Local
#include <lib/archiveutils.h>
#ifdef GWENVIEW_SEMANTICINFO_BACKEND_NONE
#include <kdirmodel.h>
#else
#include "abstractsemanticinfobackend.h"
#include "semanticinfodirmodel.h"
#endif

namespace Gwenview {

AbstractSortedDirModelFilter::AbstractSortedDirModelFilter(SortedDirModel* model)
: QObject(model)
, mModel(model)
{
	if (mModel) {
		mModel->addFilter(this);
	}
}

AbstractSortedDirModelFilter::~AbstractSortedDirModelFilter()
{
	if (mModel) {
		mModel->removeFilter(this);
	}
}


struct SortedDirModelPrivate {
#ifdef GWENVIEW_SEMANTICINFO_BACKEND_NONE
	KDirModel* mSourceModel;
#else
	SemanticInfoDirModel* mSourceModel;
#endif
	QStringList mBlackListedExtensions;
	QStringList mMimeExcludeFilter;
	QList<AbstractSortedDirModelFilter*> mFilters;
	QTimer mDelayedApplyFiltersTimer;
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
	setSourceModel(d->mSourceModel);
	d->mDelayedApplyFiltersTimer.setInterval(0);
	d->mDelayedApplyFiltersTimer.setSingleShot(true);
	connect(&d->mDelayedApplyFiltersTimer, SIGNAL(timeout()), SLOT(doApplyFilters()));
}


SortedDirModel::~SortedDirModel() {
	delete d;
}


void SortedDirModel::addFilter(AbstractSortedDirModelFilter* filter) {
	d->mFilters << filter;
	applyFilters();
}


void SortedDirModel::removeFilter(AbstractSortedDirModelFilter* filter) {
	d->mFilters.removeAll(filter);
	applyFilters();
}


KDirLister* SortedDirModel::dirLister() {
	return d->mSourceModel->dirLister();
}


void SortedDirModel::reload() {
	#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
	d->mSourceModel->clearSemanticInfoCache();
	#endif
	dirLister()->updateDirectory(dirLister()->url());
}


void SortedDirModel::setBlackListedExtensions(const QStringList& list) {
	d->mBlackListedExtensions = list;
}


KFileItem SortedDirModel::itemForIndex(const QModelIndex& index) const {
	if (!index.isValid()) {
		return KFileItem();
	}

	QModelIndex sourceIndex = mapToSource(index);
	return d->mSourceModel->itemForIndex(sourceIndex);
}


KDateTime SortedDirModel::dateTimeForSourceIndex(const QModelIndex& sourceIndex) const {
	if (!sourceIndex.isValid()) {
		return KDateTime();
	}

	const KFileItem item = d->mSourceModel->itemForIndex(sourceIndex);
	const KFileMetaInfo info = item.metaInfo();
	if (info.isValid()) {
		const KFileMetaInfoItem& mii = info.item("http://freedesktop.org/standards/xesam/1.0/core#contentCreated");
		KDateTime dt(mii.value().toDateTime(), KDateTime::LocalZone);
		if (dt.isValid()) {
			return dt;
		}
	}
	return item.time(KFileItem::ModificationTime);
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
	applyFilters();
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
	if (!ArchiveUtils::fileItemIsDirOrArchive(fileItem)) {
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
		if (!d->mSourceModel->semanticInfoAvailableForIndex(index)) {
			Q_FOREACH(const AbstractSortedDirModelFilter* filter, d->mFilters) {
				// Make sure we have semanticinfo, otherwise retrieve it and
				// return false, we will be called again later when it is
				// there.
				if (filter->needsSemanticInfo()) {
					d->mSourceModel->retrieveSemanticInfoForIndex(index);
					return false;
				}
			}
		}
#endif

		Q_FOREACH(const AbstractSortedDirModelFilter* filter, d->mFilters) {
			if (!filter->acceptsIndex(index)) {
				return false;
			}
		}
	}
	return KDirSortFilterProxyModel::filterAcceptsRow(row, parent);
}


AbstractSemanticInfoBackEnd* SortedDirModel::semanticInfoBackEnd() const {
#ifdef GWENVIEW_SEMANTICINFO_BACKEND_NONE
	return 0;
#else
	return d->mSourceModel->semanticInfoBackEnd();
#endif
}


#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
SemanticInfo SortedDirModel::semanticInfoForSourceIndex(const QModelIndex& sourceIndex) const {
	return d->mSourceModel->semanticInfoForIndex(sourceIndex);
}
#endif


void SortedDirModel::applyFilters() {
	d->mDelayedApplyFiltersTimer.start();
}


void SortedDirModel::doApplyFilters() {
	QSortFilterProxyModel::invalidateFilter();
}


bool SortedDirModel::lessThan(const QModelIndex& left, const QModelIndex& right) const {
	if (sortRole() != KDirModel::ModifiedTime) {
		return KDirSortFilterProxyModel::lessThan(left, right);
	}

	const KDateTime leftDate = dateTimeForSourceIndex(left);
	const KDateTime rightDate = dateTimeForSourceIndex(right);

	return leftDate < rightDate;
}


} //namespace
