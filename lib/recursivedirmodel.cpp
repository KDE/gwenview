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
#include "recursivedirmodel.moc"

// Local

// KDE
#include <KDebug>
#include <KDirLister>
#include <KDirModel>

// Qt

namespace Gwenview
{

struct RecursiveDirModelPrivate {
    KDirLister* mDirLister;

    int indexForUrl(const KUrl &url) const
    {
        int row = 0;
        KFileItemList::ConstIterator it = mList.begin(), end = mList.end();
        for (; it != end; ++it , ++row) {
            if (it->url() == url) {
                return row;
            }
        }
        return -1;
    }

    void removeAt(int row)
    {
        mList.removeAt(row);
    }

    void addItem(const KFileItem& item)
    {
        mList.append(item);
    }

    void clear()
    {
        mList.clear();
    }

    // Let the rest of the code access mList through this accessor to ensure it
    // cannot introduce inconsistencies with the hash
    const KFileItemList& list() const
    {
        return mList;
    }

private:
    KFileItemList mList;
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
    Q_FOREACH(const KFileItem& item, newList) {
        if (item.isFile()) {
            if (d->indexForUrl(item.url()) == -1) {
                beginInsertRows(QModelIndex(), d->list().count(), d->list().count());
                d->addItem(item);
                endInsertRows();
            }
        } else {
            dirUrls << item.url();
        }
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
        int row = d->indexForUrl(item.url());
        if (row == -1) {
            kWarning() << "Received itemsDeleted for an unknown item: this should not happen!";
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
