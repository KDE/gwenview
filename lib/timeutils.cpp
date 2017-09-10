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
#include "timeutils.h"

// Qt
#include <QFile>
#include <QDateTime>
#include <QDebug>

// KDE
#include <KFileItem>

// Exiv2
#include <exiv2/exif.hpp>
#include <exiv2/image.hpp>

// Local
#include <lib/exiv2imageloader.h>
#include <lib/urlutils.h>

namespace Gwenview
{

namespace TimeUtils
{

static Exiv2::ExifData::const_iterator findDateTimeKey(const Exiv2::ExifData& exifData)
{
    // Ordered list of keys to try
    static QList<Exiv2::ExifKey> lst = QList<Exiv2::ExifKey>()
        << Exiv2::ExifKey("Exif.Photo.DateTimeOriginal")
        << Exiv2::ExifKey("Exif.Image.DateTimeOriginal")
        << Exiv2::ExifKey("Exif.Photo.DateTimeDigitized")
        << Exiv2::ExifKey("Exif.Image.DateTime");

    Exiv2::ExifData::const_iterator it, end = exifData.end();
    Q_FOREACH(const Exiv2::ExifKey& key, lst) {
        it = exifData.findKey(key);
        if (it != end) {
            return it;
        }
    }
    return end;
}

struct CacheItem
{
    QDateTime fileMTime;
    QDateTime realTime;

    void update(const KFileItem& fileItem)
    {
        QDateTime time = fileItem.time(KFileItem::ModificationTime);
        if (fileMTime == time) {
            return;
        }

        fileMTime = time;

        if (!updateFromExif(fileItem.url())) {
            realTime = time;
        }
    }

    bool updateFromExif(const QUrl &url)
    {
        if (!UrlUtils::urlIsFastLocalFile(url)) {
            return false;
        }
        QString path = url.path();
        Exiv2ImageLoader loader;

        if (!loader.load(path)) {
            return false;
        }
        Exiv2::Image::AutoPtr img = loader.popImage();
        try {
            Exiv2::ExifData exifData = img->exifData();
            if (exifData.empty()) {
                return false;
            }
            Exiv2::ExifData::const_iterator it = findDateTimeKey(exifData);
            if (it == exifData.end()) {
                qWarning() << "No date in exif header of" << path;
                return false;
            }

            std::ostringstream stream;
            stream << *it;
            QString value = QString::fromLocal8Bit(stream.str().c_str());

            QDateTime dt = QDateTime::fromString(value, "yyyy:MM:dd hh:mm:ss");
            if (!dt.isValid()) {
                qWarning() << "Invalid date in exif header of" << path;
                return false;
            }

            realTime = dt;
            return true;
        } catch (const Exiv2::Error& error) {
            qWarning() << "Failed to read date from exif header of" << path << ". Error:" << error.what();
            return false;
        }
    }
};

typedef QHash<QUrl, CacheItem> Cache;

QDateTime dateTimeForFileItem(const KFileItem& fileItem, CachePolicy cachePolicy)
{
    if (cachePolicy == SkipCache) {
        CacheItem item;
        item.update(fileItem);
        return item.realTime;
    }

    static Cache cache;
    const QUrl url = fileItem.targetUrl();

    Cache::iterator it = cache.find(url);
    if (it == cache.end()) {
        it = cache.insert(url, CacheItem());
    }

    it.value().update(fileItem);
    return it.value().realTime;
}

} // namespace

} // namespace
