// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2012 Aurélien Gâteau <agateau@kde.org>

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
#include <serializedurlmap.h>

// Local

// KF
#include <KConfigGroup>

// Qt
#include <QUrl>

namespace Gwenview
{
static const char *KEY_SUFFIX = "key";
static const char *VALUE_SUFFIX = "value";

static QUrl stripPass(const QUrl &url_)
{
    QUrl url = url_;
    url.setPassword(QString());
    return url;
}

struct SerializedUrlMapPrivate {
    KConfigGroup mGroup;
    QMap<QUrl, QUrl> mMap;

    void read()
    {
        mMap.clear();
        for (int idx = 0;; ++idx) {
            const QString idxString = QString::number(idx);
            const QString key = idxString + QLatin1String(KEY_SUFFIX);
            if (!mGroup.hasKey(key)) {
                break;
            }
            const QVariant keyUrl = mGroup.readEntry(key, QVariant());
            const QVariant valueUrl = mGroup.readEntry(idxString + QLatin1String(VALUE_SUFFIX), QVariant());
            mMap.insert(keyUrl.toUrl(), valueUrl.toUrl());
        }
    }

    void write()
    {
        mGroup.deleteGroup();
        QMap<QUrl, QUrl>::ConstIterator it = mMap.constBegin(), end = mMap.constEnd();
        int idx = 0;
        for (; it != end; ++it, ++idx) {
            const QString idxString = QString::number(idx);
            mGroup.writeEntry(idxString + QLatin1String(KEY_SUFFIX), QVariant(it.key()));
            mGroup.writeEntry(idxString + QLatin1String(VALUE_SUFFIX), QVariant(it.value()));
        }
        mGroup.sync();
    }
};

SerializedUrlMap::SerializedUrlMap()
    : d(new SerializedUrlMapPrivate)
{
}

SerializedUrlMap::~SerializedUrlMap()
{
    delete d;
}

void SerializedUrlMap::setConfigGroup(const KConfigGroup &group)
{
    d->mGroup = group;
    d->read();
}

QUrl SerializedUrlMap::value(const QUrl &key_) const
{
    const QString pass = key_.password();
    const QUrl key = stripPass(key_);
    QUrl url = d->mMap.value(key);
    url.setPassword(pass);
    return url;
}

void SerializedUrlMap::insert(const QUrl &key, const QUrl &value)
{
    d->mMap.insert(stripPass(key), stripPass(value));
    d->write();
}

} // namespace
