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

// STL
#include <memory>
#include <sstream>
#include <vector>

// Qt
#include <QDateTime>
#include <QTimeZone>

// KF
#include <KFileItem>

// Exiv2
#include <exiv2/exiv2.hpp>

// Local
#include "gwenview_lib_debug.h"
#include <lib/exiv2imageloader.h>
#include <lib/urlutils.h>

namespace Gwenview
{
namespace TimeUtils
{
// The date found in the exif data, and the key of the offset from UTC that goes with that date
struct DateTimeEntry {
    Exiv2::ExifData::const_iterator dateTime;
    Exiv2::ExifKey offset;
};

static DateTimeEntry findDateTimeKey(const Exiv2::ExifData &exifData)
{
    // Ordered list of keys to try, each with its offset key
    static const std::vector<std::pair<Exiv2::ExifKey, Exiv2::ExifKey>> keys = {
        {Exiv2::ExifKey("Exif.Photo.DateTimeOriginal"), Exiv2::ExifKey("Exif.Photo.OffsetTimeOriginal")},
        {Exiv2::ExifKey("Exif.Image.DateTimeOriginal"), Exiv2::ExifKey("Exif.Photo.OffsetTimeOriginal")},
        {Exiv2::ExifKey("Exif.Photo.DateTimeDigitized"), Exiv2::ExifKey("Exif.Photo.OffsetTimeDigitized")},
        {Exiv2::ExifKey("Exif.Image.DateTime"), Exiv2::ExifKey("Exif.Photo.OffsetTime")},
    };

    for (const auto &[dateTimeKey, offsetKey] : keys) {
        if (auto it = exifData.findKey(dateTimeKey); it != exifData.end()) {
            return {it, offsetKey};
        }
    }
    return {exifData.end(), keys.front().second};
}

static QString exifValue(const Exiv2::Exifdatum &datum)
{
    std::ostringstream stream;
    stream << datum;
    return QString::fromLocal8Bit(stream.str().c_str());
}

struct CacheItem {
    QDateTime fileMTime;
    QDateTime realTime;

    void update(const KFileItem &fileItem)
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
        const QString path = url.path();
        Exiv2ImageLoader loader;

        if (!loader.load(path)) {
            return false;
        }
        std::unique_ptr<Exiv2::Image> img(loader.popImage().release());
        try {
            Exiv2::ExifData exifData = img->exifData();
            if (exifData.empty()) {
                return false;
            }
            const DateTimeEntry entry = findDateTimeKey(exifData);
            if (entry.dateTime == exifData.end()) {
                qCWarning(GWENVIEW_LIB_LOG) << "No date in exif header of" << path;
                return false;
            }

            QDateTime dt = QDateTime::fromString(exifValue(*entry.dateTime), QStringLiteral("yyyy:MM:dd hh:mm:ss"));
            if (!dt.isValid()) {
                qCWarning(GWENVIEW_LIB_LOG) << "Invalid date in exif header of" << path;
                return false;
            }

            // The date is the time of the clock of the camera. With its offset from UTC, for example
            // "+02:00", it is a point in time that compares with the time of a file. Without it, the
            // date is read in the time zone of this computer.
            if (auto it = exifData.findKey(entry.offset); it != exifData.end()) {
                const QDateTime offset = QDateTime::fromString(exifValue(*it).trimmed(), QStringLiteral("ttt"));
                if (offset.isValid()) {
                    dt.setTimeZone(offset.timeZone());
                } else {
                    qCWarning(GWENVIEW_LIB_LOG) << "Invalid offset from UTC in exif header of" << path;
                }
            }

            realTime = dt;
            return true;
        } catch (const Exiv2::Error &error) {
            qCWarning(GWENVIEW_LIB_LOG) << "Failed to read date from exif header of" << path << ". Error:" << error.what();
            return false;
        }
    }
};

using Cache = QHash<QUrl, CacheItem>;

QDateTime dateTimeForFileItem(const KFileItem &fileItem, CachePolicy cachePolicy)
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
