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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
#ifndef PREFERREDIMAGEMETAINFOMODEL_H
#define PREFERREDIMAGEMETAINFOMODEL_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QSortFilterProxyModel>

// Local
#include <lib/imagemetainfomodel.h>

namespace Gwenview
{

/**
 * This model uses an instance of ImageMetaInfoModel to make it possible to
 * select your preferred image metainfo keys by checking them.
 */
struct PreferredImageMetaInfoModelPrivate;
class GWENVIEWLIB_EXPORT PreferredImageMetaInfoModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    PreferredImageMetaInfoModel(ImageMetaInfoModel* model, const QStringList& list);
    ~PreferredImageMetaInfoModel();

    virtual QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role) Q_DECL_OVERRIDE;
    virtual Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;

Q_SIGNALS:
    void preferredMetaInfoKeyListChanged(const QStringList&);

protected:
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const Q_DECL_OVERRIDE;

private:
    PreferredImageMetaInfoModelPrivate* const d;
    friend struct PreferredImageMetaInfoModelPrivate;
};

} // namespace

#endif /* PREFERREDIMAGEMETAINFOMODEL_H */
