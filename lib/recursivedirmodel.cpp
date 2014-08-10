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
#include <lib/gvdebug.h>

// KDE
#include <KDebug>
#include <KDirLister>
#include <KDirModel>

// Qt

namespace Gwenview
{

struct RecursiveDirModelPrivate {
    KDirLister* mDirLister;

    int rowForUrl(const KUrl &url) const
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
            KUrl url = mList.at(row).url();
            mRowForUrl[url]--;
        }
    }

    void addItem(const KFileItem& item)
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
    const KFileItemList& list() const
    {
        return mList;
    }

private:
    KFileItemList mList;
    QHash<KUrl, int> mRowForUrl;
};

RecursiveDirModel::RecursiveDirModel(QObject* parent)
: QAbstractListModel(parent)
, d(new RecursiveDirModelPrivate)
{
    d->mDirLister = new KDirLister(this);
    connect(d->mDirLister, SIGNAL(itemsAdded(KUrl, KFileItemList)),
        SLOT(slotItemsAdded(KUrl, KFileItemList)));
    connect(d->mDirLister, SIGNAL(itemsDeleted(KFileItemList)),
        SLOT(slotItemsDeleted(KFileItemList)));
    connect(d->mDirLister, SIGNAL(completed()),
        SIGNAL(completed()));
    connect(d->mDirLister, SIGNAL(clear()),
        SLOT(slotCleared()));
    connect(d->mDirLister, SIGNAL(clear(KUrl)),
        SLOT(slotDirCleared(KUrl)));
}

RecursiveDirModel::~RecursiveDirModel()
{
    delete d;
}

KUrl RecursiveDirModel::url() const
{
    return d->mDirLister->url();
}

void RecursiveDirModel::setUrl(const KUrl& url)
{
    beginResetModel();
    d->clear();
    endResetModel();
    d->mDirLister->openUrl(url);
}

int RecursiveDirModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    } else {
        return d->list().count();
    }
}

QVariant RecursiveDirModel::data(const QModelIndex& index, int role) const
{
    if (index.parent().isValid()) {
        return QVariant();
    }
    KFileItem item = d->list().value(index.row());
    if (item.isNull()) {
        kWarning() << "Invalid row" << index.row();
        return QVariant();
    }
    switch (role) {
    case Qt::DisplayRole:
        return item.text();
    case Qt::DecorationRole:
        return item.iconName();
    case KDirModel::FileItemRole:
        return QVariant(item);
    default:
        kWarning() << "Unhandled role" << role;
        break;
    }
    return QVariant();
}

void RecursiveDirModel::slotItemsAdded(const KUrl&, const KFileItemList& newList)
{
    QList<KUrl> dirUrls;
    KFileItemList fileList;
    Q_FOREACH(const KFileItem& item, newList) {
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
        Q_FOREACH(const KFileItem& item, fileList) {
            d->addItem(item);
        }
        endInsertRows();
    }

    Q_FOREACH(const KUrl& url, dirUrls) {
        d->mDirLister->openUrl(url, KDirLister::Keep);
    }
}

void RecursiveDirModel::slotItemsDeleted(const KFileItemList& list)
{
    Q_FOREACH(const KFileItem& item, list) {
        if (item.isDir()) {
            continue;
        }
        int row = d->rowForUrl(item.url());
        if (row == -1) {
            kWarning() << "Received itemsDeleted for an unknown item: this should not happen!";
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

void RecursiveDirModel::slotDirCleared(const KUrl& dirUrl)
{
    int row;
    for (row = d->list().count() - 1; row >= 0; --row) {
        const KUrl url = d->list().at(row).url();
        if (dirUrl.isParentOf(url)) {
            beginRemoveRows(QModelIndex(), row, row);
            d->removeAt(row);
            endRemoveRows();
        }
    }
}

} // namespace
