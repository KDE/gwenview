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
#ifndef DOCUMENTONLYPROXYMODEL_H
#define DOCUMENTONLYPROXYMODEL_H

// Local
#include <lib/gwenviewlib_export.h>

// KF

// Qt
#include <QSortFilterProxyModel>

namespace Gwenview
{
struct DocumentOnlyProxyModelPrivate;
/**
 * A proxy model which lists items which are neither dirs nor archives.
 * Only works with models which expose a KDirModel::FileItemRole.
 */
class GWENVIEWLIB_EXPORT DocumentOnlyProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit DocumentOnlyProxyModel(QObject *parent = nullptr);
    ~DocumentOnlyProxyModel() override;

protected:
    bool filterAcceptsRow(int row, const QModelIndex &parent) const override;

private:
    DocumentOnlyProxyModelPrivate *const d;
};

} // namespace

#endif /* DOCUMENTONLYPROXYMODEL_H */
