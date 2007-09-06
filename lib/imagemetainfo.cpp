// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
#include "imagemetainfo.h"

// Qt

// KDE
#include <kfileitem.h>
#include <kglobal.h>
#include <klocale.h>

// Exiv2
#include <exiv2/exif.hpp>
#include <exiv2/image.hpp>

// Local


namespace Gwenview {


struct ImageMetaInfoPrivate {
	KFileItem mFileItem;
	const Exiv2::Image* mExiv2Image;

	void getFileItemInfoForKey(const QString& key, QString* label, QString* value) {
		if (key == "KFileItem.Name") {
			*label = i18n("Name");
			*value = mFileItem.name();
		} else if (key == "KFileItem.Size") {
			*label = i18n("File Size");
			*value = KGlobal::locale()->formatByteSize(mFileItem.size());
		} else if (key == "KFileItem.Time") {
			*label = i18n("File Time");
			*value = mFileItem.timeString();
		} else {
			kWarning() << "Unknown KFileItem metainfo key" << key;
		}
	}


	void getExifInfoForKey(const QString& keyName, QString* label, QString* value) {
		if (!mExiv2Image) {
			return;
		}

		if (!mExiv2Image->supportsMetadata(Exiv2::mdExif)) {
			return;
		}
		const Exiv2::ExifData& exifData = mExiv2Image->exifData();

		Exiv2::ExifKey key(keyName.toAscii().data());
		Exiv2::ExifData::const_iterator it = exifData.findKey(key);

		if (it == exifData.end()) {
			return;
		}

		*label = QString::fromUtf8(it->tagLabel().c_str());
		std::ostringstream stream;
		stream << *it;
		*value = QString::fromUtf8(stream.str().c_str());
	}
};


ImageMetaInfo::ImageMetaInfo()
: d(new ImageMetaInfoPrivate) {
}


ImageMetaInfo::~ImageMetaInfo() {
	delete d;
}


void ImageMetaInfo::setFileItem(const KFileItem& item) {
	d->mFileItem = item;
}


void ImageMetaInfo::setExiv2Image(const Exiv2::Image* image) {
	d->mExiv2Image = image;
}


void ImageMetaInfo::getInfoForKey(const QString& key, QString* label, QString* value) const {
	if (key.startsWith("KFileItem")) {
		d->getFileItemInfoForKey(key, label, value);
	} else if (key.startsWith("Exif")) {
		d->getExifInfoForKey(key, label, value);
	} else {
		kWarning() << "Unknown metainfo key" << key;
	}
}


} // namespace
