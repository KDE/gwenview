// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2013 Aurélien Gâteau <agateau@kde.org>

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
#include <documentonlyproxymodel.h>

// Local
#include <lib/archiveutils.h>

// KF
#include <KDirModel>

// Qt

namespace Gwenview
{
struct DocumentOnlyProxyModelPrivate {
};

DocumentOnlyProxyModel::DocumentOnlyProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
    , d(new DocumentOnlyProxyModelPrivate)
{
    setDynamicSortFilter(true);
}

DocumentOnlyProxyModel::~DocumentOnlyProxyModel()
{
    delete d;
}

bool DocumentOnlyProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    const QModelIndex index = sourceModel()->index(row, 0, parent);
    const KFileItem fileItem = index.data(KDirModel::FileItemRole).value<KFileItem>();
    return !ArchiveUtils::fileItemIsDirOrArchive(fileItem);
}

} // namespace

#include "moc_documentonlyproxymodel.cpp"
