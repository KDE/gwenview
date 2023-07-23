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
#include "kindproxymodel.h"

// Local

// KF
#include <KDirModel>
#include <KFileItem>

// Qt

namespace Gwenview
{
struct KindProxyModelPrivate {
    MimeTypeUtils::Kinds mKindFilter;
};

KindProxyModel::KindProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , d(new KindProxyModelPrivate)
{
}

KindProxyModel::~KindProxyModel()
{
    delete d;
}

void KindProxyModel::setKindFilter(MimeTypeUtils::Kinds filter)
{
    if (d->mKindFilter != filter) {
        d->mKindFilter = filter;
        invalidateFilter();
    }
}

MimeTypeUtils::Kinds KindProxyModel::kindFilter() const
{
    return d->mKindFilter;
}

bool KindProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (d->mKindFilter == MimeTypeUtils::Kinds()) {
        return true;
    }
    const QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
    const KFileItem fileItem = index.data(KDirModel::FileItemRole).value<KFileItem>();
    if (fileItem.isNull()) {
        return false;
    }
    const MimeTypeUtils::Kinds kind = MimeTypeUtils::fileItemKind(fileItem);
    return d->mKindFilter & kind;
}

} // namespace

#include "moc_kindproxymodel.cpp"
