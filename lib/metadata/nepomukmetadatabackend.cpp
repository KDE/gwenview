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
#include "nepomukmetadatabackend.h"

// Qt

// KDE
#include <kurl.h>

// Nepomuk
#include <nepomuk/global.h>
#include <nepomuk/resource.h>
#include <nepomuk/tag.h>

#include <Soprano/Vocabulary/Xesam>

// Local

namespace Gwenview {


struct NepomukMetaDataBackEndPrivate {
};


NepomukMetaDataBackEnd::NepomukMetaDataBackEnd(QObject* parent)
: AbstractMetaDataBackEnd(parent)
, d(new NepomukMetaDataBackEndPrivate) {
}


NepomukMetaDataBackEnd::~NepomukMetaDataBackEnd() {
	delete d;
}


void NepomukMetaDataBackEnd::storeMetaData(const KUrl& url, const MetaData& metaData) {
	QString urlString = url.url();
	Nepomuk::Resource resource(urlString, Soprano::Vocabulary::Xesam::File());
	resource.setRating(metaData.mRating);
}


void NepomukMetaDataBackEnd::retrieveMetaData(const KUrl& url) {
	QString urlString = url.url();
	Nepomuk::Resource resource(urlString, Soprano::Vocabulary::Xesam::File());

	MetaData metaData;
	metaData.mRating = resource.rating();
	emit metaDataRetrieved(url, metaData);
}


} // namespace
