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

typedef QHash<KUrl, MetaData> MetaDataCache;

struct MetaDataDirModelPrivate {
	MetaDataCache mMetaDataCache;
	AbstractMetaDataBackEnd* mBackEnd;
};


MetaDataDirModel::MetaDataDirModel(QObject* parent)
: KDirModel(parent)
, d(new MetaDataDirModelPrivate) {
#ifdef GWENVIEW_SEMANTICINFO_BACKEND_FAKE
	d->mBackEnd = new FakeMetaDataBackEnd(this, FakeMetaDataBackEnd::InitializeRandom);
#elif defined(GWENVIEW_SEMANTICINFO_BACKEND_NEPOMUK)
	d->mBackEnd = new NepomukMetaDataBackEnd(this);
#endif

	connect(d->mBackEnd, SIGNAL(metaDataRetrieved(const KUrl&, const MetaData&)),
		SLOT(storeRetrievedMetaData(const KUrl&, const MetaData&)),
		Qt::QueuedConnection);

	connect(this, SIGNAL(modelAboutToBeReset()),
		SLOT(slotModelAboutToBeReset()) );

	connect(this, SIGNAL(rowsAboutToBeRemoved(const QModelIndex&, int, int)),
		SLOT(slotRowsAboutToBeRemoved(const QModelIndex&, int, int)) );
}


MetaDataDirModel::~MetaDataDirModel() {
	delete d;
}


bool MetaDataDirModel::metaDataAvailableForIndex(const QModelIndex& index) const {
	if (!index.isValid()) {
		return false;
	}
	KFileItem item = itemForIndex(index);
	if (item.isNull()) {
		return false;
	}
	KUrl url = item.url();
	return d->mMetaDataCache.contains(url);
}


void MetaDataDirModel::retrieveMetaDataForIndex(const QModelIndex& index) {
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
	d->mBackEnd->retrieveMetaData(url);
}


QVariant MetaDataDirModel::data(const QModelIndex& index, int role) const {
	if (role == RatingRole || role == DescriptionRole || role == TagsRole) {
		KFileItem item = itemForIndex(index);
		if (item.isNull()) {
			return QVariant();
		}
		KUrl url = item.url();
		MetaDataCache::ConstIterator it = d->mMetaDataCache.find(url);
		if (it != d->mMetaDataCache.end()) {
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
			const_cast<MetaDataDirModel*>(this)->retrieveMetaDataForIndex(index);
			return QVariant();
		}
	} else {
		return KDirModel::data(index, role);
	}
}


bool MetaDataDirModel::setData(const QModelIndex& index, const QVariant& data, int role) {
	if (role == RatingRole || role == DescriptionRole || role == TagsRole) {
		KFileItem item = itemForIndex(index);
		if (item.isNull()) {
			kWarning() << "no item found for this index";
			return false;
		}
		KUrl url = item.url();
		MetaData metaData = d->mMetaDataCache[url];
		if (role == RatingRole) {
			metaData.mRating = data.toInt();
		} else if (role == DescriptionRole) {
			metaData.mDescription = data.toString();
		} else if (role == TagsRole) {
			metaData.mTags = TagSet::fromVariant(data);
		} else {
			// We should never reach this part
			Q_ASSERT(0);
		}
		d->mMetaDataCache[url] = metaData;
		emit dataChanged(index, index);

		d->mBackEnd->storeMetaData(url, metaData);
		return true;
	} else {
		return KDirModel::setData(index, data, role);
	}
}


void MetaDataDirModel::storeRetrievedMetaData(const KUrl& url, const MetaData& metaData) {
	QModelIndex index = indexForUrl(url);
	if (index.isValid()) {
		d->mMetaDataCache[url] = metaData;
		emit dataChanged(index, index);
	}
}


void MetaDataDirModel::slotRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) {
	for (int pos = start; pos <= end; ++pos) {
		QModelIndex idx = index(pos, 0, parent);
		KFileItem item = itemForIndex(idx);
		if (item.isNull()) {
			continue;
		}
		KUrl url = item.url();
		d->mMetaDataCache.remove(url);
	}
}


void MetaDataDirModel::slotModelAboutToBeReset() {
	d->mMetaDataCache.clear();
}


AbstractMetaDataBackEnd* MetaDataDirModel::metaDataBackEnd() const {
	return d->mBackEnd;
}


} // namespace
