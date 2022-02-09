/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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

// Qt
#include <QTimer>
#include <QUrl>

// KF
#include <KDirLister>
#ifdef GWENVIEW_SEMANTICINFO_BACKEND_NONE
#include <KDirModel>
#endif
// Local
#include "gwenview_lib_debug.h"
#include <lib/archiveutils.h>
#include <lib/timeutils.h>
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
#include "abstractsemanticinfobackend.h"
#include "semanticinfodirmodel.h"
#include <lib/sorting.h>
#endif

namespace Gwenview
{
AbstractSortedDirModelFilter::AbstractSortedDirModelFilter(SortedDirModel *model)
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
    KDirModel *mSourceModel;
#else
    SemanticInfoDirModel *mSourceModel;
#endif
    QStringList mBlackListedExtensions;
    QList<AbstractSortedDirModelFilter *> mFilters;
    QTimer mDelayedApplyFiltersTimer;
    MimeTypeUtils::Kinds mKindFilter;
};

SortedDirModel::SortedDirModel(QObject *parent)
    : KDirSortFilterProxyModel(parent)
    , d(new SortedDirModelPrivate)
{
#ifdef GWENVIEW_SEMANTICINFO_BACKEND_NONE
    d->mSourceModel = new KDirModel(this);
#else
    d->mSourceModel = new SemanticInfoDirModel(this);
#endif
    setSourceModel(d->mSourceModel);

#ifdef KIO_REQUEST_MIMETYPE
    d->mSourceModel->dirLister()->setRequestMimeTypeWhileListing(true);
#endif

    d->mDelayedApplyFiltersTimer.setInterval(0);
    d->mDelayedApplyFiltersTimer.setSingleShot(true);
    connect(&d->mDelayedApplyFiltersTimer, &QTimer::timeout, this, &SortedDirModel::doApplyFilters);
}

SortedDirModel::~SortedDirModel()
{
    delete d;
}

MimeTypeUtils::Kinds SortedDirModel::kindFilter() const
{
    return d->mKindFilter;
}

void SortedDirModel::setKindFilter(MimeTypeUtils::Kinds kindFilter)
{
    if (d->mKindFilter == kindFilter) {
        return;
    }
    d->mKindFilter = kindFilter;
    applyFilters();
}

void SortedDirModel::adjustKindFilter(MimeTypeUtils::Kinds kinds, bool set)
{
    MimeTypeUtils::Kinds kindFilter = d->mKindFilter;
    if (set) {
        kindFilter |= kinds;
    } else {
        kindFilter &= ~kinds;
    }
    setKindFilter(kindFilter);
}

void SortedDirModel::addFilter(AbstractSortedDirModelFilter *filter)
{
    d->mFilters << filter;
    applyFilters();
}

void SortedDirModel::removeFilter(AbstractSortedDirModelFilter *filter)
{
    d->mFilters.removeAll(filter);
    applyFilters();
}

KDirLister *SortedDirModel::dirLister() const
{
    return d->mSourceModel->dirLister();
}

void SortedDirModel::reload()
{
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
    d->mSourceModel->clearSemanticInfoCache();
#endif
    dirLister()->updateDirectory(dirLister()->url());
}

void SortedDirModel::setBlackListedExtensions(const QStringList &list)
{
    d->mBlackListedExtensions = list;
}

KFileItem SortedDirModel::itemForIndex(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return {};
    }

    QModelIndex sourceIndex = mapToSource(index);
    return d->mSourceModel->itemForIndex(sourceIndex);
}

QUrl SortedDirModel::urlForIndex(const QModelIndex &index) const
{
    KFileItem item = itemForIndex(index);
    return item.isNull() ? QUrl() : item.url();
}

KFileItem SortedDirModel::itemForSourceIndex(const QModelIndex &sourceIndex) const
{
    if (!sourceIndex.isValid()) {
        return {};
    }
    return d->mSourceModel->itemForIndex(sourceIndex);
}

QModelIndex SortedDirModel::indexForItem(const KFileItem &item) const
{
    if (item.isNull()) {
        return {};
    }

    QModelIndex sourceIndex = d->mSourceModel->indexForItem(item);
    return mapFromSource(sourceIndex);
}

QModelIndex SortedDirModel::indexForUrl(const QUrl &url) const
{
    if (!url.isValid()) {
        return {};
    }
    QModelIndex sourceIndex = d->mSourceModel->indexForUrl(url);
    return mapFromSource(sourceIndex);
}

bool SortedDirModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    QModelIndex index = d->mSourceModel->index(row, 0, parent);
    KFileItem fileItem = d->mSourceModel->itemForIndex(index);

    MimeTypeUtils::Kinds kind = MimeTypeUtils::fileItemKind(fileItem);
    if (d->mKindFilter != MimeTypeUtils::Kinds() && !(d->mKindFilter & kind)) {
        return false;
    }

    if (kind != MimeTypeUtils::KIND_DIR && kind != MimeTypeUtils::KIND_ARCHIVE) {
        int dotPos = fileItem.name().lastIndexOf(QLatin1Char('.'));
        if (dotPos >= 1) {
            QString extension = fileItem.name().mid(dotPos + 1).toLower();
            if (d->mBlackListedExtensions.contains(extension)) {
                return false;
            }
        }
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
        if (!d->mSourceModel->semanticInfoAvailableForIndex(index)) {
            for (const AbstractSortedDirModelFilter *filter : qAsConst(d->mFilters)) {
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

        for (const AbstractSortedDirModelFilter *filter : qAsConst(d->mFilters)) {
            if (!filter->acceptsIndex(index)) {
                return false;
            }
        }
    }
    return KDirSortFilterProxyModel::filterAcceptsRow(row, parent);
}

AbstractSemanticInfoBackEnd *SortedDirModel::semanticInfoBackEnd() const
{
#ifdef GWENVIEW_SEMANTICINFO_BACKEND_NONE
    return 0;
#else
    return d->mSourceModel->semanticInfoBackEnd();
#endif
}

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
SemanticInfo SortedDirModel::semanticInfoForSourceIndex(const QModelIndex &sourceIndex) const
{
    return d->mSourceModel->semanticInfoForIndex(sourceIndex);
}
#endif

void SortedDirModel::applyFilters()
{
    d->mDelayedApplyFiltersTimer.start();
}

void SortedDirModel::doApplyFilters()
{
    QSortFilterProxyModel::invalidateFilter();
}

bool SortedDirModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const KFileItem leftItem = itemForSourceIndex(left);
    const KFileItem rightItem = itemForSourceIndex(right);

    const bool leftIsDirOrArchive = ArchiveUtils::fileItemIsDirOrArchive(leftItem);
    const bool rightIsDirOrArchive = ArchiveUtils::fileItemIsDirOrArchive(rightItem);

    if (leftIsDirOrArchive != rightIsDirOrArchive) {
        return sortOrder() == Qt::AscendingOrder ? leftIsDirOrArchive : rightIsDirOrArchive;
    }

    // Apply special sort handling only to images. For folders/archives or when
    // a secondary criterion is needed, delegate sorting to the parent class.
    if (!leftIsDirOrArchive) {
        if (sortColumn() == KDirModel::ModifiedTime) {
            const QDateTime leftDate = TimeUtils::dateTimeForFileItem(leftItem);
            const QDateTime rightDate = TimeUtils::dateTimeForFileItem(rightItem);

            if (leftDate != rightDate) {
                return leftDate < rightDate;
            }
        }
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
        if (sortRole() == SemanticInfoDirModel::RatingRole) {
            const int leftRating = d->mSourceModel->data(left, SemanticInfoDirModel::RatingRole).toInt();
            const int rightRating = d->mSourceModel->data(right, SemanticInfoDirModel::RatingRole).toInt();

            if (leftRating != rightRating) {
                return leftRating < rightRating;
            }
        }
#endif
    }

    return KDirSortFilterProxyModel::lessThan(left, right);
}

bool SortedDirModel::hasDocuments() const
{
    const int count = rowCount();
    if (count == 0) {
        return false;
    }
    for (int row = 0; row < count; ++row) {
        const QModelIndex idx = index(row, 0);
        const KFileItem item = itemForIndex(idx);
        if (!ArchiveUtils::fileItemIsDirOrArchive(item)) {
            return true;
        }
    }
    return false;
}

void SortedDirModel::setDirLister(KDirLister *dirLister)
{
    d->mSourceModel->setDirLister(dirLister);
}

} // namespace
