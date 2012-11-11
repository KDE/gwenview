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

// KDE
#include <KConfigGroup>
#include <KUrl>

// Qt

namespace Gwenview
{

static const char* KEY_SUFFIX = "key";
static const char* VALUE_SUFFIX = "value";

static KUrl stripPass(const KUrl &url_)
{
    KUrl url = url_;
    url.setPass(QString());
    return url;
}

struct SerializedUrlMapPrivate
{
    KConfigGroup mGroup;
    QMap<KUrl, KUrl> mMap;

    void read()
    {
        mMap.clear();
        for (int idx=0;; ++idx) {
            QString idxString = QString::number(idx);
            QString key = idxString + QLatin1String(KEY_SUFFIX);
            if (!mGroup.hasKey(key)) {
                break;
            }
            QString keyUrl = mGroup.readEntry(key);
            QString valueUrl = mGroup.readEntry(idxString + QLatin1String(VALUE_SUFFIX));
            mMap.insert(keyUrl, valueUrl);
        }
    }

    void write()
    {
        mGroup.deleteGroup();
        QMap<KUrl, KUrl>::ConstIterator it = mMap.constBegin(), end = mMap.constEnd();
        int idx = 0;
        for (; it != end; ++it, ++idx) {
            QString idxString = QString::number(idx);
            mGroup.writeEntry(idxString + QLatin1String(KEY_SUFFIX), it.key().url());
            mGroup.writeEntry(idxString + QLatin1String(VALUE_SUFFIX), it.value().url());
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

void SerializedUrlMap::setConfigGroup(const KConfigGroup& group)
{
    d->mGroup = group;
    d->read();
}

KUrl SerializedUrlMap::value(const KUrl& key_) const
{
    QString pass = key_.pass();
    KUrl key = stripPass(key_);
    KUrl url = d->mMap.value(key);
    url.setPass(pass);
    return url;
}

void SerializedUrlMap::insert(const KUrl& key, const KUrl& value)
{
    d->mMap.insert(stripPass(key), stripPass(value));
    d->write();
}

} // namespace
