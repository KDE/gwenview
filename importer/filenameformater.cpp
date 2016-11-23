// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
#include "filenameformater.h"

// Qt
#include <QDateTime>
#include <QFileInfo>
#include <QUrl>

// KDE
#include <KLocale>

// Local

namespace Gwenview
{

typedef QHash<QString, QString> Dict;

struct FileNameFormaterPrivate
{
    QString mFormat;
};

FileNameFormater::FileNameFormater(const QString& format)
: d(new FileNameFormaterPrivate)
{
    d->mFormat = format;
}

FileNameFormater::~FileNameFormater()
{
    delete d;
}

QString FileNameFormater::format(const QUrl& url, const QDateTime& dateTime)
{
    QFileInfo info(url.fileName());

    // Keep in sync with helpMap()
    Dict dict;
    dict["date"]       = dateTime.toString("yyyy-MM-dd");
    dict["time"]       = dateTime.toString("HH-mm-ss");
    dict["ext"]        = info.suffix();
    dict["ext.lower"]  = info.suffix().toLower();
    dict["name"]       = info.completeBaseName();
    dict["name.lower"] = info.completeBaseName().toLower();

    QString name;
    int length = d->mFormat.length();
    for (int pos = 0; pos < length; ++pos) {
        QChar ch = d->mFormat[pos];
        if (ch == '{') {
            if (pos == length - 1) {
                // We are at the end, ignore this
                break;
            }
            if (d->mFormat[pos + 1] == '{') {
                // This is an escaped '{', skip one
                name += '{';
                ++pos;
                continue;
            }
            int end = d->mFormat.indexOf('}', pos + 1);
            if (end == -1) {
                // No '}' found, stop here
                return name;
            }
            // Replace keyword with its value
            QString keyword = d->mFormat.mid(pos + 1, end - pos - 1);
            name += dict.value(keyword);
            pos = end;
        } else {
            name += ch;
        }
    }
    return name;
}

FileNameFormater::HelpMap FileNameFormater::helpMap()
{
    // Keep in sync with dict in format()
    static HelpMap map;
    if (map.isEmpty()) {
        map["date"]       = i18n("Shooting date");
        map["time"]       = i18n("Shooting time");
        map["ext"]        = i18n("Original extension");
        map["ext.lower"]  = i18n("Original extension, in lower case");
        map["name"]       = i18n("Original filename");
        map["name.lower"] = i18n("Original filename, in lower case");
    }
    return map;
}

} // namespace
