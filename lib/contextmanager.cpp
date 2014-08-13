/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "contextmanager.h"
#include "contextmanager.h"

// Qt
#include <QItemSelectionModel>
#include <QTimer>
#include <QUndoGroup>

// KDE
#include <QDebug>
#include <KDirLister>
#include <KFileItem>
#include <QUrl>

// Local
#include <lib/document/documentfactory.h>
#include <lib/gvdebug.h>
#include <lib/semanticinfo/sorteddirmodel.h>

namespace Gwenview
{

struct ContextManagerPrivate
{
    SortedDirModel* mDirModel;
    QItemSelectionModel* mSelectionModel;
    QUrl mCurrentDirUrl;
    QUrl mCurrentUrl;

    QUrl mUrlToSelect;

    bool mSelectedFileItemListNeedsUpdate;
    QSet<QByteArray> mQueuedSignals;
    KFileItemList mSelectedFileItemList;

    QTimer* mQueuedSignalsTimer;

    void queueSignal(const QByteArray& signal)
    {
        mQueuedSignals << signal;
        mQueuedSignalsTimer->start();
    }

    void updateSelectedFileItemList()
    {
        if (!mSelectedFileItemListNeedsUpdate) {
            return;
        }
        mSelectedFileItemList.clear();
        QItemSelection selection = mSelectionModel->selection();
        Q_FOREACH(const QModelIndex & index, selection.indexes()) {
            mSelectedFileItemList << mDirModel->itemForIndex(index);
        }

        // At least add current url if it's valid (it may not be in
        // the list if we are viewing a non-browsable url, for example
        // using http protocol)
        if (mSelectedFileItemList.isEmpty() && mCurrentUrl.isValid()) {
            KFileItem item(KFileItem::Unknown, KFileItem::Unknown, mCurrentUrl);
            mSelectedFileItemList << item;
        }

        mSelectedFileItemListNeedsUpdate = false;
    }
};

ContextManager::ContextManager(SortedDirModel* dirModel, QObject* parent)
: QObject(parent)
, d(new ContextManagerPrivate)
{
    d->mQueuedSignalsTimer = new QTimer(this);
    d->mQueuedSignalsTimer->setInterval(100);
    d->mQueuedSignalsTimer->setSingleShot(true);
    connect(d->mQueuedSignalsTimer, SIGNAL(timeout()),
            SLOT(emitQueuedSignals()));

    d->mDirModel = dirModel;
    connect(d->mDirModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            SLOT(slotDirModelDataChanged(QModelIndex,QModelIndex)));

    /* HACK! In extended-selection mode, when the current index is removed,
     * QItemSelectionModel selects the previous index if there is one, if not it
     * selects the next index. This is not what we want: when the user removes
     * an image, he expects to go to the next one, not the previous one.
     *
     * To overcome this, we must connect to the mDirModel.rowsAboutToBeRemoved()
     * signal *before* QItemSelectionModel connects to it, so that our slot is
     * called before QItemSelectionModel slot. This allows us to pick a new
     * current index ourself, leaving QItemSelectionModel slot with nothing to
     * do.
     *
     * This is the reason ContextManager creates a QItemSelectionModel itself:
     * doing so ensures QItemSelectionModel cannot be connected to the
     * mDirModel.rowsAboutToBeRemoved() signal before us.
     */
    connect(d->mDirModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
            SLOT(slotRowsAboutToBeRemoved(QModelIndex,int,int)));

    connect(d->mDirModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
            SLOT(slotRowsInserted()));

    connect(d->mDirModel->dirLister(), SIGNAL(redirection(QUrl)),
            SLOT(slotDirListerRedirection(QUrl)));

    d->mSelectionModel = new QItemSelectionModel(d->mDirModel);

    connect(d->mSelectionModel, SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            SLOT(slotSelectionChanged()));
    connect(d->mSelectionModel, SIGNAL(currentChanged(QModelIndex,QModelIndex)),
            SLOT(slotCurrentChanged(QModelIndex)));

    d->mSelectedFileItemListNeedsUpdate = false;
}

ContextManager::~ContextManager()
{
    delete d;
}

QItemSelectionModel* ContextManager::selectionModel() const
{
    return d->mSelectionModel;
}

void ContextManager::setCurrentUrl(const QUrl &currentUrl)
{
    if (d->mCurrentUrl == currentUrl) {
        return;
    }

    d->mCurrentUrl = currentUrl;
    if (!d->mCurrentUrl.isEmpty()) {
        Document::Ptr doc = DocumentFactory::instance()->load(currentUrl);
        QUndoGroup* undoGroup = DocumentFactory::instance()->undoGroup();
        undoGroup->addStack(doc->undoStack());
        undoGroup->setActiveStack(doc->undoStack());
    }

    currentUrlChanged(currentUrl);
}

KFileItemList ContextManager::selectedFileItemList() const
{
    d->updateSelectedFileItemList();
    return d->mSelectedFileItemList;
}

void ContextManager::setCurrentDirUrl(const QUrl &url)
{
    if (url == d->mCurrentDirUrl) {
        return;
    }
    d->mCurrentDirUrl = url;
    if (url.isValid()) {
        d->mDirModel->dirLister()->openUrl(url);
    }
    currentDirUrlChanged(url);
}

QUrl ContextManager::currentDirUrl() const
{
    return d->mCurrentDirUrl;
}

QUrl ContextManager::currentUrl() const
{
    return d->mCurrentUrl;
}

SortedDirModel* ContextManager::dirModel() const
{
    return d->mDirModel;
}

void ContextManager::slotDirModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    // Data change can happen in the following cases:
    // - items have been renamed
    // - item bytes have been modified
    // - item meta info has been retrieved or modified
    //
    // If a selected item is affected, schedule emission of a
    // selectionDataChanged() signal. Don't emit it directly to avoid spamming
    // the context items in case of a mass change.
    QModelIndexList selectionList = d->mSelectionModel->selectedIndexes();
    if (selectionList.isEmpty()) {
        return;
    }

    QModelIndexList changedList;
    for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
        changedList << d->mDirModel->index(row, 0);
    }

    QModelIndexList& shortList = selectionList;
    QModelIndexList& longList = changedList;
    if (shortList.length() > longList.length()) {
        qSwap(shortList, longList);
    }
    Q_FOREACH(const QModelIndex & index, shortList) {
        if (longList.contains(index)) {
            d->mSelectedFileItemListNeedsUpdate = true;
            d->queueSignal("selectionDataChanged");
            return;
        }
    }
}

void ContextManager::slotSelectionChanged()
{
    d->mSelectedFileItemListNeedsUpdate = true;
    if (!d->mSelectionModel->hasSelection()) {
        setCurrentUrl(QUrl());
    }
    d->queueSignal("selectionChanged");
}

void Gwenview::ContextManager::slotCurrentChanged(const QModelIndex& index)
{
    QUrl url = d->mDirModel->urlForIndex(index);
    setCurrentUrl(url);
}

void ContextManager::emitQueuedSignals()
{
    Q_FOREACH(const QByteArray & signal, d->mQueuedSignals) {
        QMetaObject::invokeMethod(this, signal.data());
    }
    d->mQueuedSignals.clear();
}

void Gwenview::ContextManager::slotRowsAboutToBeRemoved(const QModelIndex& /*parent*/, int start, int end)
{
    QModelIndex oldCurrent = d->mSelectionModel->currentIndex();
    if (oldCurrent.row() < start || oldCurrent.row() > end) {
        // currentIndex has not been removed
        return;
    }
    QModelIndex newCurrent;
    if (end + 1 < d->mDirModel->rowCount()) {
        newCurrent = d->mDirModel->index(end + 1, 0);
    } else if (start > 0) {
        newCurrent = d->mDirModel->index(start - 1, 0);
    } else {
        // No index we can select, nothing to do
        return;
    }
    d->mSelectionModel->select(oldCurrent, QItemSelectionModel::Deselect);
    d->mSelectionModel->setCurrentIndex(newCurrent, QItemSelectionModel::Select);
}

bool ContextManager::currentUrlIsRasterImage() const
{
    return MimeTypeUtils::urlKind(currentUrl()) == MimeTypeUtils::KIND_RASTER_IMAGE;
}

QUrl ContextManager::urlToSelect() const
{
    return d->mUrlToSelect;
}

void ContextManager::setUrlToSelect(const QUrl &url)
{
    GV_RETURN_IF_FAIL(url.isValid());
    d->mUrlToSelect = url;
    setCurrentUrl(url);
    selectUrlToSelect();
}

void ContextManager::slotRowsInserted()
{
    // We reach this method when rows have been inserted in the model, but views
    // may not have been updated yet and thus do not have the matching items.
    // Delay the selection of mUrlToSelect so that the view items exist.
    //
    // Without this, when Gwenview is started with an image as argument and the
    // thumbnail bar is visible, the image will not be selected in the thumbnail
    // bar.
    if (d->mUrlToSelect.isValid()) {
        QMetaObject::invokeMethod(this, "selectUrlToSelect", Qt::QueuedConnection);
    }
}

void ContextManager::selectUrlToSelect()
{
    GV_RETURN_IF_FAIL(d->mUrlToSelect.isValid());
    QModelIndex index = d->mDirModel->indexForUrl(d->mUrlToSelect);
    if (index.isValid()) {
        d->mSelectionModel->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
        d->mUrlToSelect = QUrl();
    }
}

void ContextManager::slotDirListerRedirection(const QUrl &newUrl)
{
    setCurrentDirUrl(newUrl);
}


} // namespace
