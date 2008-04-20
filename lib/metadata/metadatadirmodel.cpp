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
#include "metadatadirmodel.moc"

// Qt
#include <QtConcurrentRun>

// KDE
#include <kdebug.h>

#define FAKE_METADATA_BACKEND

// Nepomuk
#ifndef FAKE_METADATA_BACKEND
#include <nepomuk/global.h>
#include <nepomuk/resource.h>
#include <nepomuk/tag.h>

#include <Soprano/Vocabulary/Xesam>
#endif

// Local
#include "abstractmetadatabackend.h"
#ifdef FAKE_METADATA_BACKEND
#include "fakemetadatabackend.h"
#endif

namespace Gwenview {

typedef QMap<QModelIndex, MetaData> MetaDataMap;

struct MetaDataDirModelPrivate {
	MetaDataMap mMetaDataForIndex;
	AbstractMetaDataBackEnd* mBackEnd;
};


/*
#ifdef FAKE_METADATA_BACKEND
#else
	QString urlString = url.url();
	Nepomuk::Resource resource(urlString, Soprano::Vocabulary::Xesam::File());
	resource.setRating(metaData.mRating);
#endif
}
*/


MetaDataDirModel::MetaDataDirModel(QObject* parent)
: KDirModel(parent)
, d(new MetaDataDirModelPrivate) {
#ifdef FAKE_METADATA_BACKEND
	d->mBackEnd = new FakeMetaDataBackEnd(this);
#else
	d->mBackEnd = new NepomukMetaDataBackEnd(this);
#endif

	connect(d->mBackEnd, SIGNAL(metaDataRetrieved(const KUrl&, const MetaData&)),
		SLOT(storeRetrievedMetaData(const KUrl&, const MetaData&)),
		Qt::QueuedConnection);
}


MetaDataDirModel::~MetaDataDirModel() {
	delete d;
}


bool MetaDataDirModel::metaDataAvailableForIndex(const QModelIndex& index) const {
	return d->mMetaDataForIndex.contains(index);
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
	KUrl url = item.url();
	d->mBackEnd->retrieveMetaData(url);
}


QVariant MetaDataDirModel::data(const QModelIndex& index, int role) const {
	if (role == RatingRole) {
		MetaDataMap::ConstIterator it = d->mMetaDataForIndex.find(index);
		if (it != d->mMetaDataForIndex.end()) {
			return it.value().mRating;
		} else {
			const_cast<MetaDataDirModel*>(this)->retrieveMetaDataForIndex(index);
			return QVariant();
		}
	} else {
		return KDirModel::data(index, role);
	}
}


bool MetaDataDirModel::setData(const QModelIndex& index, const QVariant& data, int role) {
	if (role == RatingRole) {
		int rating = data.toInt();
		MetaData metaData = d->mMetaDataForIndex[index];
		metaData.mRating = rating;
		d->mMetaDataForIndex[index] = metaData;
		emit dataChanged(index, index);

		KFileItem item = itemForIndex(index);
		Q_ASSERT(!item.isNull());
		KUrl url = item.url();
		d->mBackEnd->storeMetaData(url, metaData);
		return true;
	} else {
		return KDirModel::setData(index, data, role);
	}
}


/*
#else
	Nepomuk::Resource resource(urlString, Soprano::Vocabulary::Xesam::File());
	metaData.mRating = resource.rating();
#endif
*/


void MetaDataDirModel::storeRetrievedMetaData(const KUrl& url, const MetaData& metaData) {
	QModelIndex index = indexForUrl(url);
	if (index.isValid()) {
		d->mMetaDataForIndex[index] = metaData;
		emit dataChanged(index, index);
	}
}


} // namespace
