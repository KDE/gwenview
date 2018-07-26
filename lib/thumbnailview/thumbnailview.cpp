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
#include "thumbnailview.h"

// Std
#include <math.h>

// Qt
#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QPainter>
#include <QPointer>
#include <QQueue>
#include <QScrollBar>
#include <QTimeLine>
#include <QTimer>
#include <QDrag>
#include <QMimeData>
#include <QDebug>
#include <QDateTime>

// KDE
#include <KDirModel>
#include <KIconLoader>
#include <KPixmapSequence>
#include <KUrlMimeData>

// Local
#include "abstractdocumentinfoprovider.h"
#include "abstractthumbnailviewhelper.h"
#include "archiveutils.h"
#include "dragpixmapgenerator.h"
#include "mimetypeutils.h"
#include "urlutils.h"
#include <lib/gvdebug.h>
#include <lib/thumbnailprovider/thumbnailprovider.h>

namespace Gwenview
{

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) //qDebug() << x
#else
#define LOG(x) ;
#endif

/** How many msec to wait before starting to smooth thumbnails */
const int SMOOTH_DELAY = 500;

const int WHEEL_ZOOM_MULTIPLIER = 4;

static KFileItem fileItemForIndex(const QModelIndex& index)
{
    if (!index.isValid()) {
        LOG("Invalid index");
        return KFileItem();
    }
    QVariant data = index.data(KDirModel::FileItemRole);
    return qvariant_cast<KFileItem>(data);
}

static QUrl urlForIndex(const QModelIndex& index)
{
    KFileItem item = fileItemForIndex(index);
    return item.isNull() ? QUrl() : item.url();
}

struct Thumbnail
{
    Thumbnail(const QPersistentModelIndex& index_, const QDateTime& mtime)
        : mIndex(index_)
        , mModificationTime(mtime)
        , mFileSize(0)
        , mRough(true)
        , mWaitingForThumbnail(true) {}

    Thumbnail()
        : mFileSize(0)
        , mRough(true)
        , mWaitingForThumbnail(true) {}

    /**
     * Init the thumbnail based on a icon
     */
    void initAsIcon(const QPixmap& pix)
    {
        mGroupPix = pix;
        int largeGroupSize = ThumbnailGroup::pixelSize(ThumbnailGroup::Large);
        mFullSize = QSize(largeGroupSize, largeGroupSize);
    }

    bool isGroupPixAdaptedForSize(int size) const
    {
        if (mWaitingForThumbnail) {
            return false;
        }
        if (mGroupPix.isNull()) {
            return false;
        }
        const int groupSize = qMax(mGroupPix.width(), mGroupPix.height());
        if (groupSize >= size) {
            return true;
        }

        // groupSize is less than size, but this may be because the full image
        // is the same size as groupSize
        return groupSize == qMax(mFullSize.width(), mFullSize.height());
    }

    void prepareForRefresh(const QDateTime& mtime)
    {
        mModificationTime = mtime;
        mFileSize = 0;
        mGroupPix = QPixmap();
        mAdjustedPix = QPixmap();
        mFullSize = QSize();
        mRealFullSize = QSize();
        mRough = true;
        mWaitingForThumbnail = true;
    }

    QPersistentModelIndex mIndex;
    QDateTime mModificationTime;
    /// The pix loaded from .thumbnails/{large,normal}
    QPixmap mGroupPix;
    /// Scaled version of mGroupPix, adjusted to ThumbnailView::thumbnailSize
    QPixmap mAdjustedPix;
    /// Size of the full image
    QSize mFullSize;
    /// Real size of the full image, invalid unless the thumbnail
    /// represents a raster image (not an icon)
    QSize mRealFullSize;
    /// File size of the full image
    KIO::filesize_t mFileSize;
    /// Whether mAdjustedPix represents has been scaled using fast or smooth
    /// transformation
    bool mRough;
    /// Set to true if mGroupPix should be replaced with a real thumbnail
    bool mWaitingForThumbnail;
};

typedef QHash<QUrl, Thumbnail> ThumbnailForUrl;
typedef QQueue<QUrl> UrlQueue;
typedef QSet<QPersistentModelIndex> PersistentModelIndexSet;

struct ThumbnailViewPrivate
{
    ThumbnailView* q;
    ThumbnailView::ThumbnailScaleMode mScaleMode;
    QSize mThumbnailSize;
    qreal mThumbnailAspectRatio;
    AbstractDocumentInfoProvider* mDocumentInfoProvider;
    AbstractThumbnailViewHelper* mThumbnailViewHelper;
    ThumbnailForUrl mThumbnailForUrl;
    QTimer mScheduledThumbnailGenerationTimer;

    UrlQueue mSmoothThumbnailQueue;
    QTimer mSmoothThumbnailTimer;

    QPixmap mWaitingThumbnail;
    QPointer<ThumbnailProvider> mThumbnailProvider;

    PersistentModelIndexSet mBusyIndexSet;
    KPixmapSequence mBusySequence;
    QTimeLine* mBusyAnimationTimeLine;

    bool mCreateThumbnailsForRemoteUrls;

    void setupBusyAnimation()
    {
        mBusySequence = KIconLoader::global()->loadPixmapSequence(QStringLiteral("process-working"), 22);
        mBusyAnimationTimeLine = new QTimeLine(100 * mBusySequence.frameCount(), q);
        mBusyAnimationTimeLine->setCurveShape(QTimeLine::LinearCurve);
        mBusyAnimationTimeLine->setEndFrame(mBusySequence.frameCount() - 1);
        mBusyAnimationTimeLine->setLoopCount(0);
        QObject::connect(mBusyAnimationTimeLine, &QTimeLine::frameChanged, q, &ThumbnailView::updateBusyIndexes);
    }

    void scheduleThumbnailGeneration()
    {
        if (mThumbnailProvider) {
            mThumbnailProvider->removePendingItems();
        }
        mSmoothThumbnailQueue.clear();
        mScheduledThumbnailGenerationTimer.start();
    }

    void updateThumbnailForModifiedDocument(const QModelIndex& index)
    {
        Q_ASSERT(mDocumentInfoProvider);
        KFileItem item = fileItemForIndex(index);
        QUrl url = item.url();
        ThumbnailGroup::Enum group = ThumbnailGroup::fromPixelSize(mThumbnailSize.width());
        QPixmap pix;
        QSize fullSize;
        mDocumentInfoProvider->thumbnailForDocument(url, group, &pix, &fullSize);
        mThumbnailForUrl[url] = Thumbnail(QPersistentModelIndex(index), QDateTime::currentDateTime());
        q->setThumbnail(item, pix, fullSize, 0);
    }

    void appendItemsToThumbnailProvider(const KFileItemList& list)
    {
        if (mThumbnailProvider) {
            ThumbnailGroup::Enum group = ThumbnailGroup::fromPixelSize(mThumbnailSize.width());
            mThumbnailProvider->setThumbnailGroup(group);
            mThumbnailProvider->appendItems(list);
        }
    }

    void roughAdjustThumbnail(Thumbnail* thumbnail)
    {
        const QPixmap& mGroupPix = thumbnail->mGroupPix;
        const int groupSize = qMax(mGroupPix.width(), mGroupPix.height());
        const int fullSize = qMax(thumbnail->mFullSize.width(), thumbnail->mFullSize.height());
        if (fullSize == groupSize && mGroupPix.height() <= mThumbnailSize.height() && mGroupPix.width() <= mThumbnailSize.width()) {
            thumbnail->mAdjustedPix = mGroupPix;
            thumbnail->mRough = false;
        } else {
            thumbnail->mAdjustedPix = scale(mGroupPix, Qt::FastTransformation);
            thumbnail->mRough = true;
        }
    }

    void initDragPixmap(QDrag* drag, const QModelIndexList& indexes)
    {
        const int thumbCount = qMin(indexes.count(), int(DragPixmapGenerator::MaxCount));
        QList<QPixmap> lst;
        for (int row = 0; row < thumbCount; ++row) {
            const QUrl url = urlForIndex(indexes[row]);
            lst << mThumbnailForUrl.value(url).mAdjustedPix;
        }
        DragPixmapGenerator::DragPixmap dragPixmap = DragPixmapGenerator::generate(lst, indexes.count());
        drag->setPixmap(dragPixmap.pix);
        drag->setHotSpot(dragPixmap.hotSpot);
    }

    QPixmap scale(const QPixmap& pix, Qt::TransformationMode transformationMode)
    {
        switch (mScaleMode) {
        case ThumbnailView::ScaleToFit:
            return pix.scaled(mThumbnailSize.width(), mThumbnailSize.height(), Qt::KeepAspectRatio, transformationMode);
        case ThumbnailView::ScaleToSquare: {
            int minSize = qMin(pix.width(), pix.height());
            QPixmap pix2 = pix.copy((pix.width() - minSize) / 2, (pix.height() - minSize) / 2, minSize, minSize);
            return pix2.scaled(mThumbnailSize.width(), mThumbnailSize.height(), Qt::KeepAspectRatio, transformationMode);
        }
        case ThumbnailView::ScaleToHeight:
            return pix.scaledToHeight(mThumbnailSize.height(), transformationMode);
        case ThumbnailView::ScaleToWidth:
            return pix.scaledToWidth(mThumbnailSize.width(), transformationMode);
        }
        // Keep compiler happy
        Q_ASSERT(0);
        return QPixmap();
    }
};

ThumbnailView::ThumbnailView(QWidget* parent)
: QListView(parent)
, d(new ThumbnailViewPrivate)
{
    d->q = this;
    d->mScaleMode = ScaleToFit;
    d->mThumbnailViewHelper = nullptr;
    d->mDocumentInfoProvider = nullptr;
    d->mThumbnailProvider = nullptr;
    // Init to some stupid value so that the first call to setThumbnailSize()
    // is not ignored (do not use 0 in case someone try to divide by
    // mThumbnailSize...)
    d->mThumbnailSize = QSize(1, 1);
    d->mThumbnailAspectRatio = 1;
    d->mCreateThumbnailsForRemoteUrls = true;

    setFrameShape(QFrame::NoFrame);
    setViewMode(QListView::IconMode);
    setResizeMode(QListView::Adjust);
    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setUniformItemSizes(true);
    setEditTriggers(QAbstractItemView::EditKeyPressed);

    d->setupBusyAnimation();

    setVerticalScrollMode(ScrollPerPixel);
    setHorizontalScrollMode(ScrollPerPixel);

    d->mScheduledThumbnailGenerationTimer.setSingleShot(true);
    d->mScheduledThumbnailGenerationTimer.setInterval(500);
    connect(&d->mScheduledThumbnailGenerationTimer, &QTimer::timeout, this, &ThumbnailView::generateThumbnailsForItems);

    d->mSmoothThumbnailTimer.setSingleShot(true);
    connect(&d->mSmoothThumbnailTimer, &QTimer::timeout, this, &ThumbnailView::smoothNextThumbnail);

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &ThumbnailView::customContextMenuRequested, this, &ThumbnailView::showContextMenu);

    connect(this, &ThumbnailView::activated, this, &ThumbnailView::emitIndexActivatedIfNoModifiers);
}

ThumbnailView::~ThumbnailView()
{
    delete d;
}

ThumbnailView::ThumbnailScaleMode ThumbnailView::thumbnailScaleMode() const
{
    return d->mScaleMode;
}

void ThumbnailView::setThumbnailScaleMode(ThumbnailScaleMode mode)
{
    d->mScaleMode = mode;
    setUniformItemSizes(mode == ScaleToFit || mode == ScaleToSquare);
}

void ThumbnailView::setModel(QAbstractItemModel* newModel)
{
    if (model()) {
        disconnect(model(), nullptr, this, nullptr);
    }
    QListView::setModel(newModel);
    connect(model(), SIGNAL(rowsRemoved(QModelIndex,int,int)),
            SIGNAL(rowsRemovedSignal(QModelIndex,int,int)));
}

void ThumbnailView::setThumbnailProvider(ThumbnailProvider* thumbnailProvider)
{
    GV_RETURN_IF_FAIL(d->mThumbnailProvider != thumbnailProvider);
    if (thumbnailProvider) {
        connect(thumbnailProvider, SIGNAL(thumbnailLoaded(KFileItem,QPixmap,QSize,qulonglong)),
                         SLOT(setThumbnail(KFileItem,QPixmap,QSize,qulonglong)));
        connect(thumbnailProvider, SIGNAL(thumbnailLoadingFailed(KFileItem)),
                         SLOT(setBrokenThumbnail(KFileItem)));
    } else {
        disconnect(d->mThumbnailProvider, nullptr , this, nullptr);
    }
    d->mThumbnailProvider = thumbnailProvider;
}

void ThumbnailView::updateThumbnailSize()
{
    QSize value = d->mThumbnailSize;
    // mWaitingThumbnail
    int waitingThumbnailSize;
    if (value.width() > 64) {
        waitingThumbnailSize = 48;
    } else {
        waitingThumbnailSize = 32;
    }
    QPixmap icon = DesktopIcon(QStringLiteral("chronometer"), waitingThumbnailSize);
    QPixmap pix(value);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.setOpacity(0.5);
    painter.drawPixmap((value.width() - icon.width()) / 2, (value.height() - icon.height()) / 2, icon);
    painter.end();
    d->mWaitingThumbnail = pix;

    // Stop smoothing
    d->mSmoothThumbnailTimer.stop();
    d->mSmoothThumbnailQueue.clear();

    // Clear adjustedPixes
    ThumbnailForUrl::iterator
    it = d->mThumbnailForUrl.begin(),
    end = d->mThumbnailForUrl.end();
    for (; it != end; ++it) {
        it.value().mAdjustedPix = QPixmap();
    }

    thumbnailSizeChanged(value);
    thumbnailWidthChanged(value.width());
    if (d->mScaleMode != ScaleToFit) {
        scheduleDelayedItemsLayout();
    }
    d->scheduleThumbnailGeneration();
}

void ThumbnailView::setThumbnailWidth(int width)
{
    if(d->mThumbnailSize.width() == width) {
        return;
    }
    int height = round((qreal)width / d->mThumbnailAspectRatio);
    d->mThumbnailSize = QSize(width, height);
    updateThumbnailSize();
}

void ThumbnailView::setThumbnailAspectRatio(qreal ratio)
{
    if(d->mThumbnailAspectRatio == ratio) {
        return;
    }
    d->mThumbnailAspectRatio = ratio;
    int width = d->mThumbnailSize.width();
    int height = round((qreal)width / d->mThumbnailAspectRatio);
    d->mThumbnailSize = QSize(width, height);
    updateThumbnailSize();
}

qreal ThumbnailView::thumbnailAspectRatio() const
{
    return d->mThumbnailAspectRatio;
}

QSize ThumbnailView::thumbnailSize() const
{
    return d->mThumbnailSize;
}

void ThumbnailView::setThumbnailViewHelper(AbstractThumbnailViewHelper* helper)
{
    d->mThumbnailViewHelper = helper;
}

AbstractThumbnailViewHelper* ThumbnailView::thumbnailViewHelper() const
{
    return d->mThumbnailViewHelper;
}

void ThumbnailView::setDocumentInfoProvider(AbstractDocumentInfoProvider* provider)
{
    d->mDocumentInfoProvider = provider;
    if (provider) {
        connect(provider, &AbstractDocumentInfoProvider::busyStateChanged,
                this, &ThumbnailView::updateThumbnailBusyState);
        connect(provider, &AbstractDocumentInfoProvider::documentChanged,
                this, &ThumbnailView::updateThumbnail);
    }
}

AbstractDocumentInfoProvider* ThumbnailView::documentInfoProvider() const
{
    return d->mDocumentInfoProvider;
}

void ThumbnailView::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
    QListView::rowsAboutToBeRemoved(parent, start, end);

    // Remove references to removed items
    KFileItemList itemList;
    for (int pos = start; pos <= end; ++pos) {
        QModelIndex index = model()->index(pos, 0, parent);
        KFileItem item = fileItemForIndex(index);
        if (item.isNull()) {
            //qDebug() << "Skipping invalid item!" << index.data().toString();
            continue;
        }

        QUrl url = item.url();
        d->mThumbnailForUrl.remove(url);
        d->mSmoothThumbnailQueue.removeAll(url);

        itemList.append(item);
    }

    if (d->mThumbnailProvider) {
        d->mThumbnailProvider->removeItems(itemList);
    }

    // Removing rows might make new images visible, make sure their thumbnail
    // is generated
    d->mScheduledThumbnailGenerationTimer.start();
}

void ThumbnailView::rowsInserted(const QModelIndex& parent, int start, int end)
{
    QListView::rowsInserted(parent, start, end);
    d->mScheduledThumbnailGenerationTimer.start();
    rowsInsertedSignal(parent, start, end);
}

void ThumbnailView::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int> &roles)
{
    QListView::dataChanged(topLeft, bottomRight, roles);
    bool thumbnailsNeedRefresh = false;
    for (int row = topLeft.row(); row <= bottomRight.row(); ++row) {
        QModelIndex index = model()->index(row, 0);
        KFileItem item = fileItemForIndex(index);
        if (item.isNull()) {
            qWarning() << "Invalid item for index" << index << ". This should not happen!";
            GV_FATAL_FAILS;
            continue;
        }

        ThumbnailForUrl::Iterator it = d->mThumbnailForUrl.find(item.url());
        if (it != d->mThumbnailForUrl.end()) {
            // All thumbnail views are connected to the model, so
            // ThumbnailView::dataChanged() is called for all of them. As a
            // result this method will also be called for views which are not
            // currently visible, and do not yet have a thumbnail for the
            // modified url.
            QDateTime mtime = item.time(KFileItem::ModificationTime);
            if (it->mModificationTime != mtime || it->mFileSize != item.size()) {
                // dataChanged() is called when the file changes but also when
                // the model fetched additional data such as semantic info. To
                // avoid needless refreshes, we only trigger a refresh if the
                // modification time changes.
                thumbnailsNeedRefresh = true;
                it->prepareForRefresh(mtime);
            }
        }
    }
    if (thumbnailsNeedRefresh) {
        d->mScheduledThumbnailGenerationTimer.start();
    }
}

void ThumbnailView::showContextMenu()
{
    d->mThumbnailViewHelper->showContextMenu(this);
}

void ThumbnailView::emitIndexActivatedIfNoModifiers(const QModelIndex& index)
{
    if (QApplication::keyboardModifiers() == Qt::NoModifier) {
        emit indexActivated(index);
    }
}

void ThumbnailView::setThumbnail(const KFileItem& item, const QPixmap& pixmap, const QSize& size, qulonglong fileSize)
{
    ThumbnailForUrl::iterator it = d->mThumbnailForUrl.find(item.url());
    if (it == d->mThumbnailForUrl.end()) {
        return;
    }
    Thumbnail& thumbnail = it.value();
    thumbnail.mGroupPix = pixmap;
    thumbnail.mAdjustedPix = QPixmap();
    int largeGroupSize = ThumbnailGroup::pixelSize(ThumbnailGroup::Large);
    thumbnail.mFullSize = size.isValid() ? size : QSize(largeGroupSize, largeGroupSize);
    thumbnail.mRealFullSize = size;
    thumbnail.mWaitingForThumbnail = false;
    thumbnail.mFileSize = fileSize;

    update(thumbnail.mIndex);
    if (d->mScaleMode != ScaleToFit) {
        scheduleDelayedItemsLayout();
    }
}

void ThumbnailView::setBrokenThumbnail(const KFileItem& item)
{
    ThumbnailForUrl::iterator it = d->mThumbnailForUrl.find(item.url());
    if (it == d->mThumbnailForUrl.end()) {
        return;
    }
    Thumbnail& thumbnail = it.value();
    MimeTypeUtils::Kind kind = MimeTypeUtils::fileItemKind(item);
    if (kind == MimeTypeUtils::KIND_VIDEO) {
        // Special case for videos because our kde install may come without
        // support for video thumbnails so we show the mimetype icon instead of
        // a broken image icon
        const QPixmap pix = KIconLoader::global()->loadIcon(item.iconName(), KIconLoader::Desktop, d->mThumbnailSize.height());
        thumbnail.initAsIcon(pix);
    } else if (kind == MimeTypeUtils::KIND_DIR) {
        // Special case for folders because ThumbnailProvider does not return a
        // thumbnail if there is no images
        thumbnail.mWaitingForThumbnail = false;
        return;
    } else {
        thumbnail.initAsIcon(DesktopIcon(QStringLiteral("image-missing"), 48));
        thumbnail.mFullSize = thumbnail.mGroupPix.size();
    }
    update(thumbnail.mIndex);
}

QPixmap ThumbnailView::thumbnailForIndex(const QModelIndex& index, QSize* fullSize)
{
    KFileItem item = fileItemForIndex(index);
    if (item.isNull()) {
        LOG("Invalid item");
        if (fullSize) {
            *fullSize = QSize();
        }
        return QPixmap();
    }
    QUrl url = item.url();

    // Find or create Thumbnail instance
    ThumbnailForUrl::Iterator it = d->mThumbnailForUrl.find(url);
    if (it == d->mThumbnailForUrl.end()) {
        Thumbnail thumbnail = Thumbnail(QPersistentModelIndex(index), item.time(KFileItem::ModificationTime));
        it = d->mThumbnailForUrl.insert(url, thumbnail);
    }
    Thumbnail& thumbnail = it.value();

    // If dir or archive, generate a thumbnail from fileitem pixmap
    MimeTypeUtils::Kind kind = MimeTypeUtils::fileItemKind(item);
    if (kind == MimeTypeUtils::KIND_ARCHIVE || kind == MimeTypeUtils::KIND_DIR) {
        int groupSize = ThumbnailGroup::pixelSize(ThumbnailGroup::fromPixelSize(d->mThumbnailSize.height()));
        if (thumbnail.mGroupPix.isNull() || thumbnail.mGroupPix.height() < groupSize) {
            const QPixmap pix = KIconLoader::global()->loadIcon(item.iconName(), KIconLoader::Desktop, d->mThumbnailSize.height());

            thumbnail.initAsIcon(pix);
            if (kind == MimeTypeUtils::KIND_ARCHIVE) {
                // No thumbnails for archives
                thumbnail.mWaitingForThumbnail = false;
            } else if (!d->mCreateThumbnailsForRemoteUrls && !UrlUtils::urlIsFastLocalFile(url)) {
                // If we don't want thumbnails for remote urls, use
                // "folder-remote" icon for remote folders, so that they do
                // not look like regular folders
                thumbnail.mWaitingForThumbnail = false;
                thumbnail.initAsIcon(DesktopIcon(QStringLiteral("folder-remote"), groupSize));
            } else {
                // set mWaitingForThumbnail to true (necessary in the case
                // 'thumbnail' already existed before, but with a too small
                // mGroupPix)
                thumbnail.mWaitingForThumbnail = true;
            }
        }
    }

    if (thumbnail.mGroupPix.isNull()) {
        if (fullSize) {
            *fullSize = QSize();
        }
        return d->mWaitingThumbnail;
    }

    // Adjust thumbnail
    if (thumbnail.mAdjustedPix.isNull()) {
        d->roughAdjustThumbnail(&thumbnail);
    }
    if (thumbnail.mRough && !d->mSmoothThumbnailQueue.contains(url)) {
        d->mSmoothThumbnailQueue.enqueue(url);
        if (!d->mSmoothThumbnailTimer.isActive()) {
            d->mSmoothThumbnailTimer.start(SMOOTH_DELAY);
        }
    }
    if (fullSize) {
        *fullSize = thumbnail.mRealFullSize;
    }
    return thumbnail.mAdjustedPix;
}

bool ThumbnailView::isModified(const QModelIndex& index) const
{
    if (!d->mDocumentInfoProvider) {
        return false;
    }
    QUrl url = urlForIndex(index);
    return d->mDocumentInfoProvider->isModified(url);
}

bool ThumbnailView::isBusy(const QModelIndex& index) const
{
    if (!d->mDocumentInfoProvider) {
        return false;
    }
    QUrl url = urlForIndex(index);
    return d->mDocumentInfoProvider->isBusy(url);
}

void ThumbnailView::startDrag(Qt::DropActions)
{
    const QModelIndexList indexes = selectionModel()->selectedIndexes();
    if (indexes.isEmpty()) {
        return;
    }

    KFileItemList selectedFiles;
    for (const auto &index : indexes) {
        selectedFiles << fileItemForIndex(index);
    }

    QDrag* drag = new QDrag(this);
    drag->setMimeData(MimeTypeUtils::selectionMimeData(selectedFiles));
    d->initDragPixmap(drag, indexes);
    drag->exec(Qt::MoveAction | Qt::CopyAction | Qt::LinkAction, Qt::CopyAction);
}

void ThumbnailView::dragEnterEvent(QDragEnterEvent* event)
{
    QAbstractItemView::dragEnterEvent(event);
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void ThumbnailView::dragMoveEvent(QDragMoveEvent* event)
{
    // Necessary, otherwise we don't reach dropEvent()
    QAbstractItemView::dragMoveEvent(event);
    event->acceptProposedAction();
}

void ThumbnailView::dropEvent(QDropEvent* event)
{
    const QList<QUrl> urlList = KUrlMimeData::urlsFromMimeData(event->mimeData());
    if (urlList.isEmpty()) {
        return;
    }

    QModelIndex destIndex = indexAt(event->pos());
    if (destIndex.isValid()) {
        KFileItem item = fileItemForIndex(destIndex);
        if (item.isDir()) {
            QUrl destUrl = item.url();
            d->mThumbnailViewHelper->showMenuForUrlDroppedOnDir(this, urlList, destUrl);
            return;
        }
    }

    d->mThumbnailViewHelper->showMenuForUrlDroppedOnViewport(this, urlList);

    event->acceptProposedAction();
}

void ThumbnailView::keyPressEvent(QKeyEvent* event)
{
    QListView::keyPressEvent(event);
    if (event->key() == Qt::Key_Return) {
        const QModelIndex index = selectionModel()->currentIndex();
        if (index.isValid() && selectionModel()->selectedIndexes().count() == 1) {
            emit indexActivated(index);
        }
    }
}

void ThumbnailView::resizeEvent(QResizeEvent* event)
{
    QListView::resizeEvent(event);
    d->scheduleThumbnailGeneration();
}

void ThumbnailView::showEvent(QShowEvent* event)
{
    QListView::showEvent(event);
    d->scheduleThumbnailGeneration();
    QTimer::singleShot(0, this, SLOT(scrollToSelectedIndex()));
}

void ThumbnailView::wheelEvent(QWheelEvent* event)
{
    // If we don't adjust the single step, the wheel scroll exactly one item up
    // and down, giving the impression that the items do not move but only
    // their label changes.
    // For some reason it is necessary to set the step here: setting it in
    // setThumbnailSize() does not work
    //verticalScrollBar()->setSingleStep(d->mThumbnailSize / 5);
    if (event->modifiers() == Qt::ControlModifier) {
        int width = d->mThumbnailSize.width() + (event->delta() > 0 ? 1 : -1) * WHEEL_ZOOM_MULTIPLIER;
        width = qMax(int(MinThumbnailSize), qMin(width, int(MaxThumbnailSize)));
        setThumbnailWidth(width);
    } else {
        QListView::wheelEvent(event);
    }
}

void ThumbnailView::scrollToSelectedIndex()
{
    QModelIndexList list = selectedIndexes();
    if (list.count() >= 1) {
        scrollTo(list.first(), PositionAtCenter);
    }
}

void ThumbnailView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    QListView::selectionChanged(selected, deselected);
    emit selectionChangedSignal(selected, deselected);
}

void ThumbnailView::scrollContentsBy(int dx, int dy)
{
    QListView::scrollContentsBy(dx, dy);
    d->scheduleThumbnailGeneration();
}

void ThumbnailView::generateThumbnailsForItems()
{
    if (!isVisible() || !model()) {
        return;
    }
    const QRect visibleRect = viewport()->rect();
    const int visibleSurface = visibleRect.width() * visibleRect.height();
    const QPoint origin = visibleRect.center();

    // distance => item
    QMultiMap<int, KFileItem> itemMap;

    for (int row = 0; row < model()->rowCount(); ++row) {
        QModelIndex index = model()->index(row, 0);
        KFileItem item = fileItemForIndex(index);
        QUrl url = item.url();

        // Filter out remote items if necessary
        if (!d->mCreateThumbnailsForRemoteUrls && !url.isLocalFile()) {
            continue;
        }

        // Filter out archives
        MimeTypeUtils::Kind kind = MimeTypeUtils::fileItemKind(item);
        if (kind == MimeTypeUtils::KIND_ARCHIVE) {
            continue;
        }

        // Immediately update modified items
        if (d->mDocumentInfoProvider && d->mDocumentInfoProvider->isModified(url)) {
            d->updateThumbnailForModifiedDocument(index);
            continue;
        }

        // Filter out items which already have a thumbnail
        ThumbnailForUrl::ConstIterator it = d->mThumbnailForUrl.constFind(url);
        if (it != d->mThumbnailForUrl.constEnd() && it.value().isGroupPixAdaptedForSize(d->mThumbnailSize.height())) {
            continue;
        }

        // Compute distance
        int distance;
        const QRect itemRect = visualRect(index);
        const qreal itemSurface = itemRect.width() * itemRect.height();
        const QRect visibleItemRect = visibleRect.intersected(itemRect);
        qreal visibleItemFract = 0;
        if (itemSurface > 0) {
            visibleItemFract = visibleItemRect.width() * visibleItemRect.height() / itemSurface;
        }
        if (visibleItemFract > 0.7) {
            // Item is visible, order thumbnails from left to right, top to bottom
            // Distance is computed so that it is between 0 and visibleSurface
            distance = itemRect.top() * visibleRect.width() + itemRect.left();
            // Make sure directory thumbnails are generated after image thumbnails:
            // Distance is between visibleSurface and 2 * visibleSurface
            if (kind == MimeTypeUtils::KIND_DIR) {
                distance = distance + visibleSurface;
            }
        } else {
            // Item is not visible, order thumbnails according to distance
            // Start at 2 * visibleSurface to ensure invisible thumbnails are
            // generated *after* visible thumbnails
            distance = 2 * visibleSurface + (itemRect.center() - origin).manhattanLength();
        }

        // Add the item to our map
        itemMap.insert(distance, item);

        // Insert the thumbnail in mThumbnailForUrl, so that
        // setThumbnail() can find the item to update
        if (it == d->mThumbnailForUrl.constEnd()) {
            Thumbnail thumbnail = Thumbnail(QPersistentModelIndex(index), item.time(KFileItem::ModificationTime));
            d->mThumbnailForUrl.insert(url, thumbnail);
        }
    }

    if (!itemMap.isEmpty()) {
        d->appendItemsToThumbnailProvider(itemMap.values());
    }
}

void ThumbnailView::updateThumbnail(const QUrl& url)
{
    const ThumbnailForUrl::Iterator it = d->mThumbnailForUrl.find(url);
    if (it == d->mThumbnailForUrl.end()) {
        return;
    }

    if (d->mDocumentInfoProvider) {
        d->updateThumbnailForModifiedDocument(it->mIndex);
    } else {
        const KFileItem item = fileItemForIndex(it->mIndex);
        d->appendItemsToThumbnailProvider(KFileItemList({ item }));
    }
}

void ThumbnailView::updateThumbnailBusyState(const QUrl& url, bool busy)
{
    const ThumbnailForUrl::Iterator it = d->mThumbnailForUrl.find(url);
    if (it == d->mThumbnailForUrl.end()) {
        return;
    }

    QPersistentModelIndex index(it->mIndex);
    if (busy && !d->mBusyIndexSet.contains(index)) {
        d->mBusyIndexSet << index;
        update(index);
        if (d->mBusyAnimationTimeLine->state() != QTimeLine::Running) {
            d->mBusyAnimationTimeLine->start();
        }
    } else if (!busy && d->mBusyIndexSet.remove(index)) {
        update(index);
        if (d->mBusyIndexSet.isEmpty()) {
            d->mBusyAnimationTimeLine->stop();
        }
    }
}

void ThumbnailView::updateBusyIndexes()
{
    Q_FOREACH(const QPersistentModelIndex & index, d->mBusyIndexSet) {
        update(index);
    }
}

QPixmap ThumbnailView::busySequenceCurrentPixmap() const
{
    return d->mBusySequence.frameAt(d->mBusyAnimationTimeLine->currentFrame());
}

void ThumbnailView::smoothNextThumbnail()
{
    if (d->mSmoothThumbnailQueue.isEmpty()) {
        return;
    }

    if (d->mThumbnailProvider && d->mThumbnailProvider->isRunning()) {
        // give mThumbnailProvider priority over smoothing
        d->mSmoothThumbnailTimer.start(SMOOTH_DELAY);
        return;
    }

    QUrl url = d->mSmoothThumbnailQueue.dequeue();
    ThumbnailForUrl::Iterator it = d->mThumbnailForUrl.find(url);
    GV_RETURN_IF_FAIL2(it != d->mThumbnailForUrl.end(), url << "not in mThumbnailForUrl.");

    Thumbnail& thumbnail = it.value();
    thumbnail.mAdjustedPix = d->scale(thumbnail.mGroupPix, Qt::SmoothTransformation);
    thumbnail.mRough = false;

    GV_RETURN_IF_FAIL2(thumbnail.mIndex.isValid(), "index for" << url << "is invalid.");
    update(thumbnail.mIndex);

    if (!d->mSmoothThumbnailQueue.isEmpty()) {
        d->mSmoothThumbnailTimer.start(0);
    }
}

void ThumbnailView::reloadThumbnail(const QModelIndex& index)
{
    QUrl url = urlForIndex(index);
    if (!url.isValid()) {
        qWarning() << "Invalid url for index" << index;
        return;
    }
    ThumbnailProvider::deleteImageThumbnail(url);
    ThumbnailForUrl::Iterator it = d->mThumbnailForUrl.find(url);
    if (it == d->mThumbnailForUrl.end()) {
        return;
    }
    d->mThumbnailForUrl.erase(it);
    generateThumbnailsForItems();
}

void ThumbnailView::setCreateThumbnailsForRemoteUrls(bool createRemoteThumbs)
{
    d->mCreateThumbnailsForRemoteUrls = createRemoteThumbs;
}

} // namespace
