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
// Self
#include "preferredimagemetainfomodel.h"

// Qt
#include <QStringList>

// KDE
#include <KDebug>

namespace Gwenview
{

struct PreferredImageMetaInfoModelPrivate
{
    const ImageMetaInfoModel* mModel;
    QStringList mPreferredMetaInfoKeyList;

    QVariant checkStateData(const QModelIndex& sourceIndex) const
    {
        if (sourceIndex.parent().isValid() && sourceIndex.column() == 0) {
            QString key = mModel->keyForIndex(sourceIndex);
            bool checked = mPreferredMetaInfoKeyList.contains(key);
            return QVariant(checked ? Qt::Checked : Qt::Unchecked);
        } else {
            return QVariant();
        }
    }

    void sortPreferredMetaInfoKeyList()
    {
        QStringList sortedList;
        int groupCount = mModel->rowCount();
        for (int groupRow = 0; groupRow < groupCount; ++groupRow) {
            QModelIndex groupIndex = mModel->index(groupRow, 0);
            int keyCount = mModel->rowCount(groupIndex);
            for (int keyRow = 0; keyRow < keyCount; ++keyRow) {
                QModelIndex keyIndex = mModel->index(keyRow, 0, groupIndex);
                QString key = mModel->keyForIndex(keyIndex);
                if (mPreferredMetaInfoKeyList.contains(key)) {
                    sortedList << key;
                }
            }
        }
        mPreferredMetaInfoKeyList = sortedList;
    }
};

PreferredImageMetaInfoModel::PreferredImageMetaInfoModel(ImageMetaInfoModel* model, const QStringList& list)
: d(new PreferredImageMetaInfoModelPrivate)
{
    d->mModel = model;
    setSourceModel(model);
    sort(0);
    setDynamicSortFilter(true);
    d->mPreferredMetaInfoKeyList = list;
}

PreferredImageMetaInfoModel::~PreferredImageMetaInfoModel()
{
    delete d;
}

Qt::ItemFlags PreferredImageMetaInfoModel::flags(const QModelIndex& index) const
{
    QModelIndex sourceIndex = mapToSource(index);
    Qt::ItemFlags fl = d->mModel->flags(sourceIndex);
    if (sourceIndex.parent().isValid() && sourceIndex.column() == 0) {
        fl |= Qt::ItemIsUserCheckable;
    }
    return fl;
}

QVariant PreferredImageMetaInfoModel::data(const QModelIndex& index, int role) const
{
    QModelIndex sourceIndex = mapToSource(index);
    if (!sourceIndex.isValid()) {
        return QVariant();
    }

    switch (role) {
    case Qt::CheckStateRole:
        return d->checkStateData(sourceIndex);

    default:
        return d->mModel->data(sourceIndex, role);
    }
}

bool PreferredImageMetaInfoModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    QModelIndex sourceIndex = mapToSource(index);
    if (role != Qt::CheckStateRole) {
        return false;
    }

    if (!sourceIndex.parent().isValid()) {
        return false;
    }

    QString key = d->mModel->keyForIndex(sourceIndex);
    if (value == Qt::Checked) {
        d->mPreferredMetaInfoKeyList << key;
        d->sortPreferredMetaInfoKeyList();
    } else {
        d->mPreferredMetaInfoKeyList.removeAll(key);
    }
    emit preferredMetaInfoKeyListChanged(d->mPreferredMetaInfoKeyList);
    emit dataChanged(index, index);
    return true;
}

bool PreferredImageMetaInfoModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    if (!left.parent().isValid()) {
        // Keep root entries in insertion order
        return left.row() < right.row();
    } else {
        // Sort leaf entries alphabetically
        return QSortFilterProxyModel::lessThan(left, right);
    }
}

} // namespace
