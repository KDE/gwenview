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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "folderviewcontextmanageritem.h"

// Qt
#include <QDragEnterEvent>
#include <QHeaderView>
#include <QTreeView>

// KDE
#include <KDebug>
#include <KLocale>

// Local
#include <lib/contextmanager.h>
#include <lib/eventwatcher.h>
#include "sidebar.h"
#include "fileoperations.h"

#define USE_PLACETREE
#ifdef USE_PLACETREE
#include <lib/placetreemodel.h>
#define MODEL_CLASS PlaceTreeModel
#else
#include <KDirLister>
#include <lib/semanticinfo/sorteddirmodel.h>
#define MODEL_CLASS SortedDirModel
#endif

namespace Gwenview
{

/**
 * This treeview accepts url drops
 */
class UrlDropTreeView : public QTreeView
{
public:
    UrlDropTreeView(QWidget* parent = 0)
        : QTreeView(parent)
        {}

protected:
    void dragEnterEvent(QDragEnterEvent* event)
    {
        QAbstractItemView::dragEnterEvent(event);
        setDirtyRegion(mDropRect);
        if (event->mimeData()->hasUrls()) {
            event->acceptProposedAction();
        }
    }

    void dragMoveEvent(QDragMoveEvent* event)
    {
        QAbstractItemView::dragMoveEvent(event);

        QModelIndex index = indexAt(event->pos());

        // This code has been copied from Dolphin
        // (panels/folders/paneltreeview.cpp)
        setDirtyRegion(mDropRect);
        mDropRect = visualRect(index);
        setDirtyRegion(mDropRect);

        if (index.isValid()) {
            event->acceptProposedAction();
        } else {
            event->ignore();
        }
    }

    void dropEvent(QDropEvent* event)
    {
        const KUrl::List urlList = KUrl::List::fromMimeData(event->mimeData());
        const QModelIndex index = indexAt(event->pos());
        if (!index.isValid()) {
            kWarning() << "Invalid index!";
            return;
        }
        const KUrl destUrl = static_cast<MODEL_CLASS*>(model())->urlForIndex(index);
        FileOperations::showMenuForDroppedUrls(this, urlList, destUrl);
    }

private:
    QRect mDropRect;
};

struct FolderViewContextManagerItemPrivate
{
    FolderViewContextManagerItem* q;
    MODEL_CLASS* mModel;
    QTreeView* mView;

    KUrl mUrlToSelect;
    QPersistentModelIndex mExpandingIndex;

    void setupModel()
    {
        mModel = new MODEL_CLASS(q);
        mView->setModel(mModel);
#ifndef USE_PLACETREE
        for (int col = 1; col <= mModel->columnCount(); ++col) {
            mView->header()->setSectionHidden(col, true);
        }
        mModel->dirLister()->openUrl(KUrl("/"));
#endif
        QObject::connect(mModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
                         q, SLOT(slotRowsInserted(QModelIndex,int,int)));
    }

    void setupView()
    {
        mView = new UrlDropTreeView;
        mView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        mView->setAcceptDrops(true);
        mView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        mView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

        // Necessary to get the drop target highlighted
        mView->viewport()->setAttribute(Qt::WA_Hover);

        mView->setHeaderHidden(true);

        // This is tricky: QTreeView header has stretchLastSection set to true.
        // In this configuration, the header gets quite wide and cause an
        // horizontal scrollbar to appear.
        // To avoid this, set stretchLastSection to false and resizeMode to
        // Stretch (we still want the column to take the full width of the
        // widget).
        mView->header()->setStretchLastSection(false);
        mView->header()->setResizeMode(QHeaderView::ResizeToContents);

        q->setWidget(mView);
        QObject::connect(mView, SIGNAL(activated(QModelIndex)),
                         q, SLOT(slotActivated(QModelIndex)));
        EventWatcher::install(mView, QEvent::Show, q, SLOT(expandToSelectedUrl()));
    }

    QModelIndex findClosestIndex(const QModelIndex& parent, const KUrl& wantedUrl)
    {
        Q_ASSERT(mModel);
        QModelIndex index = parent;
        if (!index.isValid()) {
            index = findRootIndex(wantedUrl);
            if (!index.isValid()) {
                return QModelIndex();
            }
        }

        bool isParent;
        KUrl url = mModel->urlForIndex(index);
        QString relativePath = KUrl::relativePath(url.path(), wantedUrl.path(), &isParent);
        if (!isParent) {
            kWarning() << url << "is not a parent of" << wantedUrl << "!";
            return QModelIndex();
        }

        QModelIndex lastFoundIndex = index;
        Q_FOREACH(const QString & pathPart, relativePath.mid(1).split('/', QString::SkipEmptyParts)) {
            bool found = false;
            for (int row = 0; row < mModel->rowCount(lastFoundIndex); ++row) {
                QModelIndex index = mModel->index(row, 0, lastFoundIndex);
                if (index.data().toString() == pathPart) {
                    // FIXME: Check encoding
                    found = true;
                    lastFoundIndex = index;
                    break;
                }
            }
            if (!found) {
                break;
            }
        }
        return lastFoundIndex;
    }

    QModelIndex findRootIndex(const KUrl& wantedUrl)
    {
        QModelIndex matchIndex;
        int matchUrlLength = 0;
        for (int row = 0; row < mModel->rowCount(); ++row) {
            QModelIndex index = mModel->index(row, 0);
            KUrl url = mModel->urlForIndex(index);
            int urlLength = url.url().length();
            if (url.isParentOf(wantedUrl) && urlLength > matchUrlLength) {
                matchIndex = index;
                matchUrlLength = urlLength;
            }
        }
        if (!matchIndex.isValid()) {
            kWarning() << "Found no root index for" << wantedUrl;
        }
        return matchIndex;
    }
};

FolderViewContextManagerItem::FolderViewContextManagerItem(ContextManager* manager)
: AbstractContextManagerItem(manager)
, d(new FolderViewContextManagerItemPrivate)
{
    d->q = this;
    d->mModel = 0;

    d->setupView();

    connect(contextManager(), SIGNAL(currentDirUrlChanged(KUrl)),
            SLOT(slotCurrentDirUrlChanged(KUrl)));
}

FolderViewContextManagerItem::~FolderViewContextManagerItem()
{
    delete d;
}

void FolderViewContextManagerItem::slotCurrentDirUrlChanged(const KUrl& url)
{
    if (url.isValid() && d->mUrlToSelect != url) {
        d->mUrlToSelect = url;
        d->mUrlToSelect.cleanPath();
        d->mUrlToSelect.adjustPath(KUrl::RemoveTrailingSlash);
        d->mExpandingIndex = QModelIndex();
    }
    if (!d->mView->isVisible()) {
        return;
    }

    expandToSelectedUrl();
}

void FolderViewContextManagerItem::expandToSelectedUrl()
{
    if (!d->mUrlToSelect.isValid()) {
        return;
    }

    if (!d->mModel) {
        d->setupModel();
    }

    QModelIndex index = d->findClosestIndex(d->mExpandingIndex, d->mUrlToSelect);
    if (!index.isValid()) {
        return;
    }
    d->mExpandingIndex = index;

    KUrl url = d->mModel->urlForIndex(d->mExpandingIndex);
    if (d->mUrlToSelect == url) {
        // We found our url
        QItemSelectionModel* selModel = d->mView->selectionModel();
        selModel->setCurrentIndex(d->mExpandingIndex, QItemSelectionModel::ClearAndSelect);
        d->mView->scrollTo(d->mExpandingIndex);
        d->mUrlToSelect = KUrl();
        d->mExpandingIndex = QModelIndex();
    } else {
        // We found a parent of our url
        d->mView->setExpanded(d->mExpandingIndex, true);
    }
}

void FolderViewContextManagerItem::slotRowsInserted(const QModelIndex& parentIndex, int /*start*/, int /*end*/)
{
    // Can't trigger the case where parentIndex is invalid, but it most
    // probably happen when root items are created. In this case we trigger
    // expandToSelectedUrl without checking the url.
    // See bug #191771
    if (!parentIndex.isValid() || d->mModel->urlForIndex(parentIndex).isParentOf(d->mUrlToSelect)) {
        d->mExpandingIndex = parentIndex;
        // Hack because otherwise indexes are not in correct order!
        QMetaObject::invokeMethod(this, "expandToSelectedUrl", Qt::QueuedConnection);
    }
}

void FolderViewContextManagerItem::slotActivated(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }

    KUrl url = d->mModel->urlForIndex(index);
    emit urlChanged(url);
}

} // namespace
