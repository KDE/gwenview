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

// Qt
#include <QItemSelectionModel>
#include <QTimer>
#include <QUndoGroup>

// KF
#include <KDirLister>
#include <KProtocolManager>

// Local
#include <lib/document/documentfactory.h>
#include <lib/gvdebug.h>
#include <lib/gwenviewconfig.h>
#include <lib/semanticinfo/sorteddirmodel.h>

namespace Gwenview
{
struct ContextManagerPrivate {
    SortedDirModel *mDirModel = nullptr;
    QItemSelectionModel *mSelectionModel = nullptr;
    QUrl mCurrentDirUrl;
    QUrl mCurrentUrl;

    QUrl mUrlToSelect;
    QUrl mTargetDirUrl;

    bool mSelectedFileItemListNeedsUpdate;
    using Signal = void (ContextManager::*)();
    QVector<Signal> mQueuedSignals;
    KFileItemList mSelectedFileItemList;

    bool mDirListerFinished = false;
    QTimer *mQueuedSignalsTimer = nullptr;

    void queueSignal(Signal signal)
    {
        if (!mQueuedSignals.contains(signal)) {
            mQueuedSignals << signal;
        }
        mQueuedSignalsTimer->start();
    }

    void updateSelectedFileItemList()
    {
        if (!mSelectedFileItemListNeedsUpdate) {
            return;
        }
        mSelectedFileItemList.clear();
        const QItemSelection selection = mSelectionModel->selection();
        for (const QModelIndex &index : selection.indexes()) {
            mSelectedFileItemList << mDirModel->itemForIndex(index);
        }

        // At least add current url if it's valid (it may not be in
        // the list if we are viewing a non-browsable url, for example
        // using http protocol)
        if (mSelectedFileItemList.isEmpty() && mCurrentUrl.isValid()) {
            KFileItem item(mCurrentUrl);
            mSelectedFileItemList << item;
        }

        mSelectedFileItemListNeedsUpdate = false;
    }
};

ContextManager::ContextManager(SortedDirModel *dirModel, QObject *parent)
    : QObject(parent)
    , d(new ContextManagerPrivate)
{
    d->mQueuedSignalsTimer = new QTimer(this);
    d->mQueuedSignalsTimer->setInterval(100);
    d->mQueuedSignalsTimer->setSingleShot(true);
    connect(d->mQueuedSignalsTimer, &QTimer::timeout, this, &ContextManager::emitQueuedSignals);

    d->mDirModel = dirModel;
    connect(d->mDirModel, &SortedDirModel::dataChanged, this, &ContextManager::slotDirModelDataChanged);

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
    connect(d->mDirModel, &SortedDirModel::rowsAboutToBeRemoved, this, &ContextManager::slotRowsAboutToBeRemoved);

    connect(d->mDirModel, &SortedDirModel::rowsInserted, this, &ContextManager::slotRowsInserted);

    connect(d->mDirModel->dirLister(), QOverload<const QUrl &, const QUrl &>::of(&KDirLister::redirection), this, [this](const QUrl &, const QUrl &newUrl) {
        setCurrentDirUrl(newUrl);
    });

    connect(d->mDirModel->dirLister(), QOverload<>::of(&KDirLister::completed), this, &ContextManager::slotDirListerCompleted);

    d->mSelectionModel = new QItemSelectionModel(d->mDirModel);

    connect(d->mSelectionModel, &QItemSelectionModel::selectionChanged, this, &ContextManager::slotSelectionChanged);
    connect(d->mSelectionModel, &QItemSelectionModel::currentChanged, this, &ContextManager::slotCurrentChanged);

    d->mSelectedFileItemListNeedsUpdate = false;

    connect(DocumentFactory::instance(), &DocumentFactory::readyForDirListerStart, this, [this](const QUrl &urlReady) {
        setCurrentDirUrl(urlReady.adjusted(QUrl::RemoveFilename));
    });
}

ContextManager::~ContextManager()
{
    delete d;
}

void ContextManager::loadConfig()
{
    setTargetDirUrl(QUrl(GwenviewConfig::lastTargetDir()));
}

void ContextManager::saveConfig() const
{
    GwenviewConfig::setLastTargetDir(targetDirUrl().toString());
}

QItemSelectionModel *ContextManager::selectionModel() const
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
        QUndoGroup *undoGroup = DocumentFactory::instance()->undoGroup();
        undoGroup->addStack(doc->undoStack());
        undoGroup->setActiveStack(doc->undoStack());
    }

    d->mSelectedFileItemListNeedsUpdate = true;
    Q_EMIT currentUrlChanged(currentUrl);
}

KFileItemList ContextManager::selectedFileItemList() const
{
    d->updateSelectedFileItemList();
    return d->mSelectedFileItemList;
}

void ContextManager::setCurrentDirUrl(const QUrl &_url)
{
    const QUrl url = _url.adjusted(QUrl::StripTrailingSlash);
    if (url == d->mCurrentDirUrl) {
        return;
    }

    if (url.isValid() && KProtocolManager::supportsListing(url)) {
        d->mCurrentDirUrl = url;
        d->mDirModel->dirLister()->openUrl(url);
        d->mDirListerFinished = false;
    } else {
        d->mCurrentDirUrl.clear();
        Q_EMIT d->mDirModel->dirLister()->clear();
    }
    Q_EMIT currentDirUrlChanged(d->mCurrentDirUrl);
}

QUrl ContextManager::currentDirUrl() const
{
    return d->mCurrentDirUrl;
}

QUrl ContextManager::currentUrl() const
{
    return d->mCurrentUrl;
}

SortedDirModel *ContextManager::dirModel() const
{
    return d->mDirModel;
}

void ContextManager::slotDirModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight)
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

    QModelIndexList &shortList = selectionList;
    QModelIndexList &longList = changedList;
    if (shortList.length() > longList.length()) {
        qSwap(shortList, longList);
    }
    for (const QModelIndex &index : qAsConst(shortList)) {
        if (longList.contains(index)) {
            d->mSelectedFileItemListNeedsUpdate = true;
            d->queueSignal(&ContextManager::selectionDataChanged);
            return;
        }
    }
}

void ContextManager::slotSelectionChanged()
{
    d->mSelectedFileItemListNeedsUpdate = true;
    if (!d->mSelectionModel->hasSelection()) {
        // There is a chance that the URL that has been passed in from the command
        // line is not shown by the thumbnail view. In that case, we will not have
        // a selection but we also do not want to clear the current URL, as that
        // would hide the image that was requested to be shown. So check to see if
        // the current URL is in the thumbnail view, and only if it is, deselect
        // it.
        if (d->mDirModel->indexForUrl(d->mCurrentUrl).isValid()) {
            setCurrentUrl(QUrl());
        }
    }
    d->queueSignal(&ContextManager::selectionChanged);
}

void Gwenview::ContextManager::slotCurrentChanged(const QModelIndex &index)
{
    const QUrl url = d->mDirModel->urlForIndex(index);
    setCurrentUrl(url);
}

void ContextManager::emitQueuedSignals()
{
    for (ContextManagerPrivate::Signal signal : qAsConst(d->mQueuedSignals)) {
        Q_EMIT(this->*signal)();
    }
    d->mQueuedSignals.clear();
}

void Gwenview::ContextManager::slotRowsAboutToBeRemoved(const QModelIndex & /*parent*/, int start, int end)
{
    const QModelIndex oldCurrent = d->mSelectionModel->currentIndex();
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

QUrl ContextManager::targetDirUrl() const
{
    return d->mTargetDirUrl;
}

void ContextManager::setTargetDirUrl(const QUrl &url)
{
    GV_RETURN_IF_FAIL(url.isEmpty() || url.isValid());
    d->mTargetDirUrl = GwenviewConfig::historyEnabled() ? url : QUrl();
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
        QMetaObject::invokeMethod(this, &ContextManager::selectUrlToSelect, Qt::QueuedConnection);
    }
}

void ContextManager::selectUrlToSelect()
{
    // Because of the queued connection above we might be called several times in a row
    // In this case we don't want the warning below
    if (d->mUrlToSelect.isEmpty()) {
        return;
    }

    GV_RETURN_IF_FAIL(d->mUrlToSelect.isValid());
    QModelIndex index = d->mDirModel->indexForUrl(d->mUrlToSelect);
    if (index.isValid()) {
        d->mSelectionModel->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
        d->mUrlToSelect = QUrl();
    } else if (d->mDirListerFinished) {
        // Desired URL cannot be found in the directory
        // Clear the selection to avoid dragging any local files into context
        // and manually set current URL
        d->mSelectionModel->clearSelection();
        setCurrentUrl(d->mUrlToSelect);
    }
}

void ContextManager::slotDirListerCompleted()
{
    d->mDirListerFinished = true;
}

} // namespace
