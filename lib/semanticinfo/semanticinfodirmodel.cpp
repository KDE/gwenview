// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#include "semanticinfodirmodel.moc"
#include <config-gwenview.h>

// Qt
#include <QHash>

// KDE
#include <kdebug.h>

// Local
#include "abstractsemanticinfobackend.h"
#include "../archiveutils.h"

#ifdef GWENVIEW_SEMANTICINFO_BACKEND_FAKE
#include "fakesemanticinfobackend.h"

#elif defined(GWENVIEW_SEMANTICINFO_BACKEND_NEPOMUK)
#include "nepomuksemanticinfobackend.h"

#else
#ifdef __GNUC__
	#error No metadata backend defined
#endif
#endif

namespace Gwenview {

typedef QHash<KUrl, SemanticInfo> SemanticInfoCache;

struct SemanticInfoDirModelPrivate {
	SemanticInfoCache mSemanticInfoCache;
	AbstractSemanticInfoBackEnd* mBackEnd;
};


SemanticInfoDirModel::SemanticInfoDirModel(QObject* parent)
: KDirModel(parent)
, d(new SemanticInfoDirModelPrivate) {
#ifdef GWENVIEW_SEMANTICINFO_BACKEND_FAKE
	d->mBackEnd = new FakeSemanticInfoBackEnd(this, FakeSemanticInfoBackEnd::InitializeRandom);
#elif defined(GWENVIEW_SEMANTICINFO_BACKEND_NEPOMUK)
	d->mBackEnd = new NepomukSemanticInfoBackEnd(this);
#endif

	connect(d->mBackEnd, SIGNAL(semanticInfoRetrieved(const KUrl&, const SemanticInfo&)),
		SLOT(storeRetrievedSemanticInfo(const KUrl&, const SemanticInfo&)),
		Qt::QueuedConnection);

	connect(this, SIGNAL(modelAboutToBeReset()),
		SLOT(slotModelAboutToBeReset()) );

	connect(this, SIGNAL(rowsAboutToBeRemoved(const QModelIndex&, int, int)),
		SLOT(slotRowsAboutToBeRemoved(const QModelIndex&, int, int)) );
}


SemanticInfoDirModel::~SemanticInfoDirModel() {
	delete d;
}


void SemanticInfoDirModel::clearSemanticInfoCache() {
	d->mSemanticInfoCache.clear();
}


bool SemanticInfoDirModel::semanticInfoAvailableForIndex(const QModelIndex& index) const {
	if (!index.isValid()) {
		return false;
	}
	KFileItem item = itemForIndex(index);
	if (item.isNull()) {
		return false;
	}
	KUrl url = item.url();
	return d->mSemanticInfoCache.contains(url);
}


void SemanticInfoDirModel::retrieveSemanticInfoForIndex(const QModelIndex& index) {
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
	KUrl url = item.url();
	d->mBackEnd->retrieveSemanticInfo(url);
}


QVariant SemanticInfoDirModel::data(const QModelIndex& index, int role) const {
	if (role == RatingRole || role == DescriptionRole || role == TagsRole) {
		KFileItem item = itemForIndex(index);
		if (item.isNull()) {
			return QVariant();
		}
		KUrl url = item.url();
		SemanticInfoCache::ConstIterator it = d->mSemanticInfoCache.find(url);
		if (it != d->mSemanticInfoCache.end()) {
			if (role == RatingRole) {
				return it.value().mRating;
			} else if (role == DescriptionRole) {
				return it.value().mDescription;
			} else if (role == TagsRole) {
				return it.value().mTags.toVariant();
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


bool SemanticInfoDirModel::setData(const QModelIndex& index, const QVariant& data, int role) {
	if (role == RatingRole || role == DescriptionRole || role == TagsRole) {
		KFileItem item = itemForIndex(index);
		if (item.isNull()) {
			kWarning() << "no item found for this index";
			return false;
		}
		KUrl url = item.url();
		SemanticInfo semanticInfo = d->mSemanticInfoCache[url];
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
		d->mSemanticInfoCache[url] = semanticInfo;
		emit dataChanged(index, index);

		d->mBackEnd->storeSemanticInfo(url, semanticInfo);
		return true;
	} else {
		return KDirModel::setData(index, data, role);
	}
}


void SemanticInfoDirModel::storeRetrievedSemanticInfo(const KUrl& url, const SemanticInfo& semanticInfo) {
	QModelIndex index = indexForUrl(url);
	if (index.isValid()) {
		d->mSemanticInfoCache[url] = semanticInfo;
		emit dataChanged(index, index);
	}
}


void SemanticInfoDirModel::slotRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) {
	for (int pos = start; pos <= end; ++pos) {
		QModelIndex idx = index(pos, 0, parent);
		KFileItem item = itemForIndex(idx);
		if (item.isNull()) {
			continue;
		}
		KUrl url = item.url();
		d->mSemanticInfoCache.remove(url);
	}
}


void SemanticInfoDirModel::slotModelAboutToBeReset() {
	d->mSemanticInfoCache.clear();
}


AbstractSemanticInfoBackEnd* SemanticInfoDirModel::semanticInfoBackEnd() const {
	return d->mBackEnd;
}


} // namespace
