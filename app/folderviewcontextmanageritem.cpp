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
#include <QDir>
#include <QDragEnterEvent>
#include <QHeaderView>
#include <QMimeData>
#include <QTreeView>

// KF
#include <KUrlMimeData>

// Local
#include "fileoperations.h"
#include "gwenview_app_debug.h"
#include "lib/touch/touch_helper.h"
#include <lib/contextmanager.h>
#include <lib/eventwatcher.h>
#include <lib/scrollerutils.h>

namespace Gwenview
{
/**
 * This treeview accepts url drops
 */
class UrlDropTreeView : public QTreeView
{
public:
    explicit UrlDropTreeView(QWidget *parent = nullptr)
        : QTreeView(parent)
    {
    }

protected:
    void dragEnterEvent(QDragEnterEvent *event) override
    {
        QAbstractItemView::dragEnterEvent(event);
        setDirtyRegion(mDropRect);
        if (event->mimeData()->hasUrls()) {
            event->acceptProposedAction();
        }
    }

    void dragMoveEvent(QDragMoveEvent *event) override
    {
        QAbstractItemView::dragMoveEvent(event);

        const QModelIndex index = indexAt(event->pos());

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

    void dropEvent(QDropEvent *event) override
    {
        const QList<QUrl> urlList = KUrlMimeData::urlsFromMimeData(event->mimeData());
        const QModelIndex index = indexAt(event->pos());
        if (!index.isValid()) {
            qCWarning(GWENVIEW_APP_LOG) << "Invalid index!";
            return;
        }
        const QUrl destUrl = static_cast<MODEL_CLASS *>(model())->urlForIndex(index);
        FileOperations::showMenuForDroppedUrls(this, urlList, destUrl);
    }

    bool viewportEvent(QEvent *event) override
    {
        if (event->type() == QEvent::TouchBegin) {
            return true;
        }
        const QPoint pos = Touch_Helper::simpleTapPosition(event);
        if (pos != QPoint(-1, -1)) {
            expand(indexAt(pos));
            Q_EMIT activated(indexAt(pos));
        }

        return QTreeView::viewportEvent(event);
    }

private:
    QRect mDropRect;
};

FolderViewContextManagerItem::FolderViewContextManagerItem(ContextManager *manager)
    : AbstractContextManagerItem(manager)
{
    mModel = nullptr;

    setupView();

    connect(contextManager(), &ContextManager::currentDirUrlChanged, this, &FolderViewContextManagerItem::slotCurrentDirUrlChanged);
}

void FolderViewContextManagerItem::slotCurrentDirUrlChanged(const QUrl &url)
{
    if (url.isValid() && mUrlToSelect != url) {
        mUrlToSelect = url.adjusted(QUrl::StripTrailingSlash | QUrl::NormalizePathSegments);
        mExpandingIndex = QModelIndex();
    }
    if (!mView->isVisible()) {
        return;
    }

    expandToSelectedUrl();
}

void FolderViewContextManagerItem::expandToSelectedUrl()
{
    if (!mUrlToSelect.isValid()) {
        return;
    }

    if (!mModel) {
        setupModel();
    }

    const QModelIndex index = findClosestIndex(mExpandingIndex, mUrlToSelect);
    if (!index.isValid()) {
        return;
    }
    mExpandingIndex = index;

    QUrl url = mModel->urlForIndex(mExpandingIndex);
    if (mUrlToSelect == url) {
        // We found our url
        QItemSelectionModel *selModel = mView->selectionModel();
        selModel->setCurrentIndex(mExpandingIndex, QItemSelectionModel::ClearAndSelect);
        mView->scrollTo(mExpandingIndex);
        mUrlToSelect = QUrl();
        mExpandingIndex = QModelIndex();
    } else {
        // We found a parent of our url
        mView->setExpanded(mExpandingIndex, true);
    }
}

void FolderViewContextManagerItem::slotRowsInserted(const QModelIndex &parentIndex, int /*start*/, int /*end*/)
{
    // Can't trigger the case where parentIndex is invalid, but it most
    // probably happen when root items are created. In this case we trigger
    // expandToSelectedUrl without checking the url.
    // See bug #191771
    if (!parentIndex.isValid() || mModel->urlForIndex(parentIndex).isParentOf(mUrlToSelect)) {
        mExpandingIndex = parentIndex;
        // Hack because otherwise indexes are not in correct order!
        QMetaObject::invokeMethod(this, &FolderViewContextManagerItem::expandToSelectedUrl, Qt::QueuedConnection);
    }
}

void FolderViewContextManagerItem::slotActivated(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    QUrl url = mModel->urlForIndex(index);
    Q_EMIT urlChanged(url);
}

void FolderViewContextManagerItem::setupModel()
{
    mModel = new MODEL_CLASS(this);
    mView->setModel(mModel);
#ifndef USE_PLACETREE
    for (int col = 1; col <= mModel->columnCount(); ++col) {
        mView->header()->setSectionHidden(col, true);
    }
    mModel->dirLister()->openUrl(QUrl("/"));
#endif
    QObject::connect(mModel, &MODEL_CLASS::rowsInserted, this, &FolderViewContextManagerItem::slotRowsInserted);
}

void FolderViewContextManagerItem::setupView()
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
    mView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    ScrollerUtils::setQScroller(mView->viewport());

    setWidget(mView);
    QObject::connect(mView, &QTreeView::activated, this, &FolderViewContextManagerItem::slotActivated);
    EventWatcher::install(mView, QEvent::Show, this, SLOT(expandToSelectedUrl()));
}

QModelIndex FolderViewContextManagerItem::findClosestIndex(const QModelIndex &parent, const QUrl &wantedUrl)
{
    Q_ASSERT(mModel);
    QModelIndex index = parent;
    if (!index.isValid()) {
        index = findRootIndex(wantedUrl);
        if (!index.isValid()) {
            return {};
        }
    }

    QUrl url = mModel->urlForIndex(index);
    if (!url.isParentOf(wantedUrl)) {
        qCWarning(GWENVIEW_APP_LOG) << url << "is not a parent of" << wantedUrl << "!";
        return {};
    }

    QString relativePath = QDir(url.path()).relativeFilePath(wantedUrl.path());
    QModelIndex lastFoundIndex = index;
    const QStringList relativePathList = relativePath.split(QDir::separator(), Qt::SkipEmptyParts);
    for (const QString &pathPart : relativePathList) {
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

QModelIndex FolderViewContextManagerItem::findRootIndex(const QUrl &wantedUrl)
{
    QModelIndex matchIndex;
    int matchUrlLength = 0;
    for (int row = 0; row < mModel->rowCount(); ++row) {
        QModelIndex index = mModel->index(row, 0);
        QUrl url = mModel->urlForIndex(index);
        int urlLength = url.url().length();
        if (url.isParentOf(wantedUrl) && urlLength > matchUrlLength) {
            matchIndex = index;
            matchUrlLength = urlLength;
        }
    }
    if (!matchIndex.isValid()) {
        qCWarning(GWENVIEW_APP_LOG) << "Found no root index for" << wantedUrl;
    }
    return matchIndex;
}

} // namespace
