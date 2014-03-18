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
#ifndef KINDPROXYMODEL_H
#define KINDPROXYMODEL_H

#include <lib/gwenviewlib_export.h>

// Local
#include <lib/mimetypeutils.h>

// KDE

// Qt
#include <QSortFilterProxyModel>

namespace Gwenview
{

struct KindProxyModelPrivate;
/**
 * A simple proxy model allowing only objects of a certain kind
 */
class GWENVIEWLIB_EXPORT KindProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    KindProxyModel(QObject* parent = 0);
    ~KindProxyModel();

    void setKindFilter(MimeTypeUtils::Kinds);
    MimeTypeUtils::Kinds kindFilter() const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const; // reimp

private:
    KindProxyModelPrivate* const d;
};

} // namespace

#endif /* KINDPROXYMODEL_H */
