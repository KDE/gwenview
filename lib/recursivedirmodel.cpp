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
#include "recursivedirmodel.h"

// Local
#include "gwenview_lib_debug.h"
#include <lib/gvdebug.h>

// KF
#include <KDirLister>
#include <KDirModel>
#include <kio_version.h>

// Qt

namespace Gwenview
{
struct RecursiveDirModelPrivate {
    KDirLister *mDirLister = nullptr;

    int rowForUrl(const QUrl &url) const
    {
        return mRowForUrl.value(url, -1);
    }

    void removeAt(int row)
    {
        KFileItem item = mList.takeAt(row);
        mRowForUrl.remove(item.url());

        // Decrease row value for all urls after the one we removed
        // ("row" now points to the item after the one we removed since we used takeAt)
        const int count = mList.count();
        for (; row < count; ++row) {
            QUrl url = mList.at(row).url();
            mRowForUrl[url]--;
        }
    }

    void addItem(const KFileItem &item)
    {
        mRowForUrl.insert(item.url(), mList.count());
        mList.append(item);
    }

    void clear()
    {
        mRowForUrl.clear();
        mList.clear();
    }

    // RecursiveDirModel can only access mList through this read-only getter.
    // This ensures it cannot introduce inconsistencies between mList and mRowForUrl.
    const KFileItemList &list() const
    {
        return mList;
    }

private:
    KFileItemList mList;
    QHash<QUrl, int> mRowForUrl;
};

RecursiveDirModel::RecursiveDirModel(QObject *parent)
    : QAbstractListModel(parent)
    , d(new RecursiveDirModelPrivate)
{
    d->mDirLister = new KDirLister(this);
    connect(d->mDirLister, &KDirLister::itemsAdded, this, &RecursiveDirModel::slotItemsAdded);
    connect(d->mDirLister, &KDirLister::itemsDeleted, this, &RecursiveDirModel::slotItemsDeleted);
    connect(d->mDirLister, QOverload<>::of(&KDirLister::completed), this, &RecursiveDirModel::completed);
    connect(d->mDirLister, QOverload<>::of(&KDirLister::clear), this, &RecursiveDirModel::slotCleared);

    connect(d->mDirLister, &KDirLister::clearDir, this, &RecursiveDirModel::slotDirCleared);
}

RecursiveDirModel::~RecursiveDirModel()
{
    delete d;
}

QUrl RecursiveDirModel::url() const
{
    return d->mDirLister->url();
}

void RecursiveDirModel::setUrl(const QUrl &url)
{
    beginResetModel();
    d->clear();
    endResetModel();
    d->mDirLister->openUrl(url);
}

int RecursiveDirModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    } else {
        return d->list().count();
    }
}

QVariant RecursiveDirModel::data(const QModelIndex &index, int role) const
{
    if (index.parent().isValid()) {
        return {};
    }
    KFileItem item = d->list().value(index.row());
    if (item.isNull()) {
        qCWarning(GWENVIEW_LIB_LOG) << "Invalid row" << index.row();
        return {};
    }
    switch (role) {
    case Qt::DisplayRole:
        return item.text();
    case Qt::DecorationRole:
        return item.iconName();
    case KDirModel::FileItemRole:
        return QVariant(item);
    default:
        qCWarning(GWENVIEW_LIB_LOG) << "Unhandled role" << role;
        break;
    }
    return {};
}

void RecursiveDirModel::slotItemsAdded(const QUrl &, const KFileItemList &newList)
{
    QList<QUrl> dirUrls;
    KFileItemList fileList;
    for (const KFileItem &item : newList) {
        if (item.isFile()) {
            if (d->rowForUrl(item.url()) == -1) {
                fileList << item;
            }
        } else {
            dirUrls << item.url();
        }
    }

    if (!fileList.isEmpty()) {
        beginInsertRows(QModelIndex(), d->list().count(), d->list().count() + fileList.count());
        for (const KFileItem &item : qAsConst(fileList)) {
            d->addItem(item);
        }
        endInsertRows();
    }

    for (const QUrl &url : qAsConst(dirUrls)) {
        d->mDirLister->openUrl(url, KDirLister::Keep);
    }
}

void RecursiveDirModel::slotItemsDeleted(const KFileItemList &list)
{
    for (const KFileItem &item : list) {
        if (item.isDir()) {
            continue;
        }
        int row = d->rowForUrl(item.url());
        if (row == -1) {
            qCWarning(GWENVIEW_LIB_LOG) << "Received itemsDeleted for an unknown item: this should not happen!";
            GV_FATAL_FAILS;
            continue;
        }
        beginRemoveRows(QModelIndex(), row, row);
        d->removeAt(row);
        endRemoveRows();
    }
}

void RecursiveDirModel::slotCleared()
{
    if (d->list().isEmpty()) {
        return;
    }
    beginResetModel();
    d->clear();
    endResetModel();
}

void RecursiveDirModel::slotDirCleared(const QUrl &dirUrl)
{
    int row;
    for (row = d->list().count() - 1; row >= 0; --row) {
        const QUrl url = d->list().at(row).url();
        if (dirUrl.isParentOf(url)) {
            beginRemoveRows(QModelIndex(), row, row);
            d->removeAt(row);
            endRemoveRows();
        }
    }
}

} // namespace
