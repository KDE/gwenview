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
    d->mDirLister->openUrl(url);
}

int RecursiveDirModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    } else {
        return d->mList.count();
    }
}

QVariant RecursiveDirModel::data(const QModelIndex& index, int role) const
{
    if (index.parent().isValid()) {
        return QVariant();
    }
    KFileItem item = d->mList.value(index.row());
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
            beginInsertRows(QModelIndex(), d->mList.count(), d->mList.count());
            d->mList << item;
            endInsertRows();
        } else {
            dirUrls << item.url();
        }
    }
    Q_FOREACH(const KUrl& url, dirUrls) {
        d->mDirLister->openUrl(url, KDirLister::Keep);
    }
}

void RecursiveDirModel::slotItemsDeleted(const KFileItemList&)
{
}

} // namespace
