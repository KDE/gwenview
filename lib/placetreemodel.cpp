// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
#include "placetreemodel.h"

// Qt
#include <QUrl>

// KDE
#include <KDirLister>
#include <KFilePlacesModel>
#include <kio_version.h>

// Local
#include <lib/semanticinfo/sorteddirmodel.h>

namespace Gwenview
{

/**
 * Here is how the mapping work:
 *
 * Place1     Node(dirModel1, QUrl())
 *   Photos   Node(dirModel1, place1Url)
 *     2008   Node(dirModel1, place1Url/Photos)
 *     2009   Node(dirModel1, place1Url/Photos)
 *
 * Place2     Node(dirModel2, QUrl())
 * ...
 * Node contains the parent url, not the url of the node itself, because
 * for some unknown reason when accessing rows from a slot connected to
 * rowsInserted(), the rows are unsorted, they appear in KDirModel natural
 * order. Further access to the rows are correctly sorted! This confuses
 * QTreeView a lot (symptoms are mixed tooltips, filled nodes appearing
 * empty on first expand)
 *
 * I could not determine whether it's a bug or not, and if it's in my model
 * code, in QSortFilterProxyModel or somewhere else.
 */

struct Node
{
    Node()
        : model(0)
    {}

    Node(SortedDirModel* _model, const QUrl &_parentUrl)
        : model(_model)
        , parentUrl(_parentUrl)
    {}

    SortedDirModel* model;
    QUrl parentUrl;

    bool isPlace() const
    {
        return !parentUrl.isValid();
    }
};

typedef QHash<QUrl, Node*> NodeHash;
typedef QMap<SortedDirModel*, NodeHash*> NodeHashMap;

struct PlaceTreeModelPrivate
{
    PlaceTreeModel* q;
    KFilePlacesModel* mPlacesModel;
    QList<SortedDirModel*> mDirModels;
    mutable NodeHashMap mNodes;

    Node nodeForIndex(const QModelIndex& index) const
    {
        Q_ASSERT(index.isValid());
        Q_ASSERT(index.internalPointer());
        return *static_cast<Node*>(index.internalPointer());
    }

    Node* createNode(SortedDirModel* dirModel, const QUrl &parentUrl) const
    {
        NodeHashMap::iterator nhmIt = mNodes.find(dirModel);
        if (nhmIt == mNodes.end()) {
            nhmIt = mNodes.insert(dirModel, new NodeHash);
        }
        NodeHash* nodeHash = nhmIt.value();

        NodeHash::iterator nhIt = nodeHash->find(parentUrl);
        if (nhIt == nodeHash->end()) {
            nhIt = nodeHash->insert(parentUrl, new Node(dirModel, parentUrl));
        }
        return nhIt.value();
    }

    QModelIndex createIndexForDir(SortedDirModel* dirModel, const QUrl &url) const
    {
        QModelIndex dirIndex = dirModel->indexForUrl(url);
        QModelIndex parentDirIndex = dirIndex.parent();
        QUrl parentUrl;
        if (parentDirIndex.isValid()) {
            parentUrl = dirModel->urlForIndex(parentDirIndex);
        } else {
            parentUrl = dirModel->dirLister()->url();
        }
        return createIndexForDirChild(dirModel, parentUrl, dirIndex.row(), dirIndex.column());
    }

    QModelIndex createIndexForDirChild(SortedDirModel* dirModel, const QUrl &parentUrl, int row, int column) const
    {
        Q_ASSERT(parentUrl.isValid());
        Node* node = createNode(dirModel, parentUrl);
        return q->createIndex(row, column, node);
    }

    QModelIndex createIndexForPlace(SortedDirModel* dirModel) const
    {
        int row = mDirModels.indexOf(dirModel);
        Q_ASSERT(row != -1);
        Node* node = createNode(dirModel, QUrl());
        return q->createIndex(row, 0, node);
    }

    QModelIndex dirIndexForNode(const Node& node, const QModelIndex& index) const
    {
        if (node.isPlace()) {
            return QModelIndex();
        }
        Q_ASSERT(node.parentUrl.isValid());
        const QModelIndex parentDirIndex = node.model->indexForUrl(node.parentUrl);
        return node.model->index(index.row(), index.column(), parentDirIndex);
    }
};

PlaceTreeModel::PlaceTreeModel(QObject* parent)
: QAbstractItemModel(parent)
, d(new PlaceTreeModelPrivate)
{
    d->q = this;

    d->mPlacesModel = new KFilePlacesModel(this);
    connect(d->mPlacesModel, &KFilePlacesModel::rowsInserted, this, &PlaceTreeModel::slotPlacesRowsInserted);
    connect(d->mPlacesModel, &KFilePlacesModel::rowsAboutToBeRemoved, this, &PlaceTreeModel::slotPlacesRowsAboutToBeRemoved);

    // Bootstrap
    if (d->mPlacesModel->rowCount() > 0) {
        slotPlacesRowsInserted(QModelIndex(), 0, d->mPlacesModel->rowCount() - 1);
    }
}

PlaceTreeModel::~PlaceTreeModel()
{
    Q_FOREACH(NodeHash * nodeHash, d->mNodes) {
        qDeleteAll(*nodeHash);
    }
    qDeleteAll(d->mNodes);
    delete d;
}

int PlaceTreeModel::columnCount(const QModelIndex&) const
{
    return 1;
}

QVariant PlaceTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    QVariant value;
    const Node node = d->nodeForIndex(index);
    if (node.isPlace()) {
        const QModelIndex placesIndex = d->mPlacesModel->index(index.row(), index.column());
        value = d->mPlacesModel->data(placesIndex, role);
    } else {
        const QModelIndex dirIndex = d->dirIndexForNode(node, index);
        value = node.model->data(dirIndex, role);
    }
    return value;
}

QModelIndex PlaceTreeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (column != 0) {
        return QModelIndex();
    }
    if (!parent.isValid()) {
        // User wants to create a places index
        if (0 <= row && row < d->mDirModels.size()) {
            SortedDirModel* dirModel = d->mDirModels[row];
            return d->createIndexForPlace(dirModel);
        } else {
            return QModelIndex();
        }
    }

    Node parentNode = d->nodeForIndex(parent);
    QModelIndex parentDirIndex = d->dirIndexForNode(parentNode, parent);

    SortedDirModel* dirModel = parentNode.model;
    QUrl parentUrl = dirModel->urlForIndex(parentDirIndex);
    if (!parentUrl.isValid()) {
        // parent is a place
        parentUrl = dirModel->dirLister()->url();
        if (!parentUrl.isValid()) {
            return QModelIndex();
        }
    }
    return d->createIndexForDirChild(dirModel, parentUrl, row, column);
}

QModelIndex PlaceTreeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }
    const Node node = d->nodeForIndex(index);
    if (node.isPlace()) {
        return QModelIndex();
    }
    if (node.parentUrl == node.model->dirLister()->url()) {
        // index is a direct child of a place
        return d->createIndexForPlace(node.model);
    }
    return d->createIndexForDir(node.model, node.parentUrl);
}

int PlaceTreeModel::rowCount(const QModelIndex& index) const
{
    if (!index.isValid()) {
        // index is the invisible root item
        return d->mDirModels.size();
    }

    // index is a place or a dir
    const Node node = d->nodeForIndex(index);
    const QModelIndex dirIndex = d->dirIndexForNode(node, index);
    return node.model->rowCount(dirIndex);
}

bool PlaceTreeModel::hasChildren(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return true;
    }
    const Node node = d->nodeForIndex(index);
    if (node.isPlace()) {
        return true;
    }
    const QModelIndex dirIndex = d->dirIndexForNode(node, index);
    return node.model->hasChildren(dirIndex);
}

bool PlaceTreeModel::canFetchMore(const QModelIndex& parent) const
{
    if (!parent.isValid()) {
        return d->mPlacesModel->canFetchMore(QModelIndex());
    }
    const Node node = d->nodeForIndex(parent);
    if (!node.model->dirLister()->url().isValid()) {
        // Special case to avoid calling openUrl on all places at startup
        return true;
    }
    const QModelIndex dirIndex = d->dirIndexForNode(node, parent);
    return node.model->canFetchMore(dirIndex);
}

void PlaceTreeModel::fetchMore(const QModelIndex& parent)
{
    if (!parent.isValid()) {
        d->mPlacesModel->fetchMore(QModelIndex());
        return;
    }
    const Node node = d->nodeForIndex(parent);
    if (!node.model->dirLister()->url().isValid()) {
        QModelIndex placeIndex = d->mPlacesModel->index(parent.row(), parent.column());
#if KIO_VERSION >= QT_VERSION_CHECK(5, 41, 0)
        QUrl url = KFilePlacesModel::convertedUrl(d->mPlacesModel->url(placeIndex));
#else
        QUrl url = d->mPlacesModel->url(placeIndex);
#endif
        node.model->dirLister()->openUrl(url, KDirLister::Keep);
        return;
    }
    const QModelIndex dirIndex = d->dirIndexForNode(node, parent);
    node.model->fetchMore(dirIndex);
}

void PlaceTreeModel::slotPlacesRowsInserted(const QModelIndex& /*parent*/, int start, int end)
{
    beginInsertRows(QModelIndex(), start, end);
    for (int row = start; row <= end; ++row) {
        SortedDirModel* dirModel = new SortedDirModel(this);
        connect(dirModel, &SortedDirModel::rowsAboutToBeInserted, this, &PlaceTreeModel::slotDirRowsAboutToBeInserted);
        connect(dirModel, &SortedDirModel::rowsInserted, this, &PlaceTreeModel::slotDirRowsInserted);
        connect(dirModel, &SortedDirModel::rowsAboutToBeRemoved, this, &PlaceTreeModel::slotDirRowsAboutToBeRemoved);
        connect(dirModel, &SortedDirModel::rowsAboutToBeRemoved, this, &PlaceTreeModel::slotDirRowsRemoved);

        d->mDirModels.insert(row, dirModel);
        KDirLister* lister = dirModel->dirLister();
        lister->setDirOnlyMode(true);
    }
    endInsertRows();
}

void PlaceTreeModel::slotPlacesRowsAboutToBeRemoved(const QModelIndex&, int start, int end)
{
    beginRemoveRows(QModelIndex(), start, end);
    for (int row = end; row >= start; --row) {
        SortedDirModel* dirModel = d->mDirModels.takeAt(row);
        delete d->mNodes.take(dirModel);
        delete dirModel;
    }
    endRemoveRows();
}

void PlaceTreeModel::slotDirRowsAboutToBeInserted(const QModelIndex& parentDirIndex, int start, int end)
{
    SortedDirModel* dirModel = static_cast<SortedDirModel*>(sender());
    QModelIndex parentIndex;
    if (parentDirIndex.isValid()) {
        QUrl url = dirModel->urlForIndex(parentDirIndex);
        parentIndex = d->createIndexForDir(dirModel, url);
    } else {
        parentIndex = d->createIndexForPlace(dirModel);
    }
    beginInsertRows(parentIndex, start, end);
}

void PlaceTreeModel::slotDirRowsInserted(const QModelIndex&, int, int)
{
    endInsertRows();
}

void PlaceTreeModel::slotDirRowsAboutToBeRemoved(const QModelIndex& parentDirIndex, int start, int end)
{
    SortedDirModel* dirModel = static_cast<SortedDirModel*>(sender());
    QModelIndex parentIndex;
    if (parentDirIndex.isValid()) {
        QUrl url = dirModel->urlForIndex(parentDirIndex);
        parentIndex = d->createIndexForDir(dirModel, url);
    } else {
        parentIndex = d->createIndexForPlace(dirModel);
    }
    beginRemoveRows(parentIndex, start, end);
}

void PlaceTreeModel::slotDirRowsRemoved(const QModelIndex&, int, int)
{
    endRemoveRows();
}

QUrl PlaceTreeModel::urlForIndex(const QModelIndex& index) const
{
    const Node node = d->nodeForIndex(index);
    if (node.isPlace()) {
        QModelIndex placeIndex = d->mPlacesModel->index(index.row(), 0);
#if KIO_VERSION >= QT_VERSION_CHECK(5, 41, 0)
        return KFilePlacesModel::convertedUrl(d->mPlacesModel->url(placeIndex));
#else
        return d->mPlacesModel->url(placeIndex);
#endif
    }

    const QModelIndex parentDirIndex = node.model->indexForUrl(node.parentUrl);
    const QModelIndex dirIndex = node.model->index(index.row(), index.column(), parentDirIndex);
    return node.model->urlForIndex(dirIndex);
}

} // namespace
