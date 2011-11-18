// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
#include <QHash>

// KDE
#include <kdatetime.h>
#include <kdebug.h>
#include <kfileitem.h>
#include <kfilemetainfo.h>
#include <kfilemetainfoitem.h>

namespace Gwenview
{

namespace TimeUtils
{

struct CacheItem {
    KDateTime fileMTime;
    KDateTime realTime;

    void update(const KFileItem& fileItem)
    {
        KDateTime time = fileItem.time(KFileItem::ModificationTime);
        if (fileMTime == time) {
            return;
        }

        fileMTime = time;
        const KFileMetaInfo info = fileItem.metaInfo();
        if (info.isValid()) {
            const KFileMetaInfoItem& mii = info.item("http://www.semanticdesktop.org/ontologies/2007/01/19/nie#contentCreated");
            KDateTime dt(mii.value().toDateTime(), KDateTime::LocalZone);
            if (dt.isValid()) {
                realTime = dt;
                return;
            }
        }

        realTime = time;
    }
};

typedef QHash<KUrl, CacheItem> Cache;

KDateTime dateTimeForFileItem(const KFileItem& fileItem)
{
    static Cache cache;
    const KUrl url = fileItem.targetUrl();

    Cache::iterator it = cache.find(url);
    if (it == cache.end()) {
        it = cache.insert(url, CacheItem());
    }

    it.value().update(fileItem);
    return it.value().realTime;
}

} // namespace

} // namespace
