// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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
#include "semanticinfodirmodel.h"
#include <config-gwenview.h>

// Qt
#include <QHash>

// KDE
#include <KDebug>

// Local
#include "abstractsemanticinfobackend.h"
#include "../archiveutils.h"

#ifdef GWENVIEW_SEMANTICINFO_BACKEND_FAKE
#include "fakesemanticinfobackend.h"

#elif defined(GWENVIEW_SEMANTICINFO_BACKEND_BALOO)
#include "baloosemanticinfobackend.h"

#else
#ifdef __GNUC__
#error No metadata backend defined
#endif
#endif

namespace Gwenview
{

struct SemanticInfoCacheItem
{
    SemanticInfoCacheItem()
        : mValid(false)
        {}
    QPersistentModelIndex mIndex;
    bool mValid;
    SemanticInfo mInfo;
};

typedef QHash<KUrl, SemanticInfoCacheItem> SemanticInfoCache;

struct SemanticInfoDirModelPrivate
{
    SemanticInfoCache mSemanticInfoCache;
    AbstractSemanticInfoBackEnd* mBackEnd;
};

SemanticInfoDirModel::SemanticInfoDirModel(QObject* parent)
: KDirModel(parent)
, d(new SemanticInfoDirModelPrivate)
{
#ifdef GWENVIEW_SEMANTICINFO_BACKEND_FAKE
    d->mBackEnd = new FakeSemanticInfoBackEnd(this, FakeSemanticInfoBackEnd::InitializeRandom);
#elif defined(GWENVIEW_SEMANTICINFO_BACKEND_BALOO)
    d->mBackEnd = new BalooSemanticInfoBackend(this);
#endif

    connect(d->mBackEnd, SIGNAL(semanticInfoRetrieved(KUrl,SemanticInfo)),
            SLOT(slotSemanticInfoRetrieved(KUrl,SemanticInfo)),
            Qt::QueuedConnection);

    connect(this, SIGNAL(modelAboutToBeReset()),
            SLOT(slotModelAboutToBeReset()));

    connect(this, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
            SLOT(slotRowsAboutToBeRemoved(QModelIndex,int,int)));
}

SemanticInfoDirModel::~SemanticInfoDirModel()
{
    delete d;
}

void SemanticInfoDirModel::clearSemanticInfoCache()
{
    d->mSemanticInfoCache.clear();
}

bool SemanticInfoDirModel::semanticInfoAvailableForIndex(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return false;
    }
    KFileItem item = itemForIndex(index);
    if (item.isNull()) {
        return false;
    }
    SemanticInfoCache::const_iterator it = d->mSemanticInfoCache.constFind(item.targetUrl());
    if (it == d->mSemanticInfoCache.constEnd()) {
        return false;
    }
    return it.value().mValid;
}

SemanticInfo SemanticInfoDirModel::semanticInfoForIndex(const QModelIndex& index) const
{
    if (!index.isValid()) {
        kWarning() << "invalid index";
        return SemanticInfo();
    }
    KFileItem item = itemForIndex(index);
    if (item.isNull()) {
        kWarning() << "no item for index";
        return SemanticInfo();
    }
    return d->mSemanticInfoCache.value(item.targetUrl()).mInfo;
}

void SemanticInfoDirModel::retrieveSemanticInfoForIndex(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }
    KFileItem item = itemForIndex(index);
    if (item.isNull()) {
        kWarning() << "invalid item";
        return;
    }
    if (ArchiveUtils::fileItemIsDirOrArchive(item)) {
        return;
    }
    SemanticInfoCacheItem cacheItem;
    cacheItem.mIndex = QPersistentModelIndex(index);
    d->mSemanticInfoCache[item.targetUrl()] = cacheItem;
    d->mBackEnd->retrieveSemanticInfo(item.targetUrl());
}

QVariant SemanticInfoDirModel::data(const QModelIndex& index, int role) const
{
    if (role == RatingRole || role == DescriptionRole || role == TagsRole) {
        KFileItem item = itemForIndex(index);
        if (item.isNull()) {
            return QVariant();
        }
        SemanticInfoCache::ConstIterator it = d->mSemanticInfoCache.constFind(item.targetUrl());
        if (it != d->mSemanticInfoCache.constEnd()) {
            if (!it.value().mValid) {
                return QVariant();
            }
            const SemanticInfo& info = it.value().mInfo;
            if (role == RatingRole) {
                return info.mRating;
            } else if (role == DescriptionRole) {
                return info.mDescription;
            } else if (role == TagsRole) {
                return info.mTags.toVariant();
            } else {
                // We should never reach this part
                Q_ASSERT(0);
                return QVariant();
            }
        } else {
            const_cast<SemanticInfoDirModel*>(this)->retrieveSemanticInfoForIndex(index);
            return QVariant();
        }
    } else {
        return KDirModel::data(index, role);
    }
}

bool SemanticInfoDirModel::setData(const QModelIndex& index, const QVariant& data, int role)
{
    if (role == RatingRole || role == DescriptionRole || role == TagsRole) {
        KFileItem item = itemForIndex(index);
        if (item.isNull()) {
            kWarning() << "no item found for this index";
            return false;
        }
        KUrl url = item.targetUrl();
        SemanticInfoCache::iterator it = d->mSemanticInfoCache.find(url);
        if (it == d->mSemanticInfoCache.end()) {
            kWarning() << "No index for" << url;
            return false;
        }
        if (!it.value().mValid) {
            kWarning() << "Semantic info cache for" << url << "is invalid";
            return false;
        }
        SemanticInfo& semanticInfo = it.value().mInfo;
        if (role == RatingRole) {
            semanticInfo.mRating = data.toInt();
        } else if (role == DescriptionRole) {
            semanticInfo.mDescription = data.toString();
        } else if (role == TagsRole) {
            semanticInfo.mTags = TagSet::fromVariant(data);
        } else {
            // We should never reach this part
            Q_ASSERT(0);
        }
        emit dataChanged(index, index);

        d->mBackEnd->storeSemanticInfo(url, semanticInfo);
        return true;
    } else {
        return KDirModel::setData(index, data, role);
    }
}

void SemanticInfoDirModel::slotSemanticInfoRetrieved(const KUrl& url, const SemanticInfo& semanticInfo)
{
    SemanticInfoCache::iterator it = d->mSemanticInfoCache.find(url);
    if (it == d->mSemanticInfoCache.end()) {
        kWarning() << "No index for" << url;
        return;
    }
    SemanticInfoCacheItem& cacheItem = it.value();
    if (!cacheItem.mIndex.isValid()) {
        kWarning() << "Index for" << url << "is invalid";
        return;
    }
    cacheItem.mInfo = semanticInfo;
    cacheItem.mValid = true;
    emit dataChanged(cacheItem.mIndex, cacheItem.mIndex);
}

void SemanticInfoDirModel::slotRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
    for (int pos = start; pos <= end; ++pos) {
        QModelIndex idx = index(pos, 0, parent);
        KFileItem item = itemForIndex(idx);
        if (item.isNull()) {
            continue;
        }
        d->mSemanticInfoCache.remove(item.targetUrl());
    }
}

void SemanticInfoDirModel::slotModelAboutToBeReset()
{
    d->mSemanticInfoCache.clear();
}

AbstractSemanticInfoBackEnd* SemanticInfoDirModel::semanticInfoBackEnd() const
{
    return d->mBackEnd;
}

} // namespace
