/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#include "thumbnailview.moc"

// Std
#include <math.h>

// Qt
#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QPainter>
#include <QPointer>
#include <QQueue>
#include <QTimer>

// KDE
#include <kdebug.h>
#include <kdirmodel.h>
#include <kglobalsettings.h>

// Local
#include "abstractthumbnailviewhelper.h"
#include "archiveutils.h"
#include "mimetypeutils.h"
#include "thumbnailloadjob.h"

namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << x
#else
#define LOG(x) ;
#endif

/** How many pixels between items */
const int SPACING = 11;

/** How many msec to wait before starting to smooth thumbnails */
const int SMOOTH_DELAY = 500;

static KFileItem fileItemForIndex(const QModelIndex& index) {
	if (!index.isValid()) {
		LOG("Invalid index");
		return KFileItem();
	}
	QVariant data = index.data(KDirModel::FileItemRole);
	return qvariant_cast<KFileItem>(data);
}


static KUrl urlForIndex(const QModelIndex& index) {
	KFileItem item = fileItemForIndex(index);
	return item.isNull() ? KUrl() : item.url();
}

struct Thumbnail {
	/// The pix loaded from .thumbnails/{large,normal}
	QPixmap groupPix;
	/// Scaled version of groupPix, adjusted to ThumbnailView::thumbnailSize
	QPixmap adjustedPix;
	/// Size of the full image
	QSize fullSize;

	/// True if adjustedPix has been scaled with FastTransform instead of
	//Smooth
	bool rough;

	Thumbnail() : rough(true) {}

	bool isGroupPixAdaptedForSize(int size) const {
		if (groupPix.isNull()) {
			return false;
		}
		const int groupSize = qMax(groupPix.width(), groupPix.height());
		if (groupSize >= size) {
			return true;
		}

		// groupSize is less than size, but this may be because the full image
		// is the same size as groupSize
		return groupSize == qMax(fullSize.width(), fullSize.height());
	}
};

typedef QMap<QUrl, Thumbnail> ThumbnailForUrlMap;
typedef QQueue<KUrl> UrlQueue;

struct ThumbnailViewPrivate {
	ThumbnailView* that;
	int mThumbnailSize;
	AbstractThumbnailViewHelper* mThumbnailViewHelper;
	ThumbnailForUrlMap mThumbnailForUrl;
	QMap<QUrl, QPersistentModelIndex> mPersistentIndexForUrl;
	QTimer mScheduledThumbnailGenerationTimer;

	UrlQueue mSmoothThumbnailQueue;
	QTimer mSmoothThumbnailTimer;;

	QPixmap mWaitingThumbnail;
	QPointer<ThumbnailLoadJob> mThumbnailLoadJob;

	void scheduleThumbnailGenerationForVisibleItems() {
		if (mThumbnailLoadJob) {
			mThumbnailLoadJob->removeItems(mThumbnailLoadJob->pendingItems());
		}
		mSmoothThumbnailQueue.clear();
		mScheduledThumbnailGenerationTimer.start();
	}

	void updateThumbnailForModifiedDocument(const QModelIndex& index) {
		KFileItem item = fileItemForIndex(index);
		KUrl url = item.url();
		ThumbnailGroup::Enum group = ThumbnailGroup::fromPixelSize(mThumbnailSize);
		QPixmap pix;
		QSize fullSize;
		mThumbnailViewHelper->thumbnailForDocument(url, group, &pix, &fullSize);
		mPersistentIndexForUrl[url] = QPersistentModelIndex(index);
		that->setThumbnail(item, pix, fullSize);
	}

	void generateThumbnailsForItems(const KFileItemList& list) {
		ThumbnailGroup::Enum group = ThumbnailGroup::fromPixelSize(mThumbnailSize);
		if (!mThumbnailLoadJob) {
			mThumbnailLoadJob = new ThumbnailLoadJob(list, group);
			QObject::connect(mThumbnailLoadJob, SIGNAL(thumbnailLoaded(const KFileItem&, const QPixmap&, const QSize&)),
				that, SLOT(setThumbnail(const KFileItem&, const QPixmap&, const QSize&)));
			mThumbnailLoadJob->start();
		} else {
			mThumbnailLoadJob->setThumbnailGroup(group);
			Q_FOREACH(const KFileItem& item, list) {
				mThumbnailLoadJob->appendItem(item);
			}
		}
	}

	void roughAdjustThumbnail(Thumbnail* thumbnail, const KUrl& url) {
		const QPixmap& groupPix = thumbnail->groupPix;
		const int groupSize = qMax(groupPix.width(), groupPix.height());
		const int fullSize = qMax(thumbnail->fullSize.width(), thumbnail->fullSize.height());
		if (fullSize == groupSize && groupSize <= mThumbnailSize) {
			thumbnail->adjustedPix = groupPix;
			thumbnail->rough = false;
		} else {
			thumbnail->adjustedPix = groupPix.scaled(mThumbnailSize, mThumbnailSize, Qt::KeepAspectRatio);
			thumbnail->rough = true;
			if (!mSmoothThumbnailQueue.contains(url)) {
				mSmoothThumbnailQueue.enqueue(url);
				if (!mSmoothThumbnailTimer.isActive()) {
					mSmoothThumbnailTimer.start(SMOOTH_DELAY);
				}
			}
		}
	}
};


ThumbnailView::ThumbnailView(QWidget* parent)
: QListView(parent)
, d(new ThumbnailViewPrivate) {
	d->that = this;
	d->mThumbnailViewHelper = 0;

	setFrameShape(QFrame::NoFrame);
	setViewMode(QListView::IconMode);
	setResizeMode(QListView::Adjust);
	setSpacing(SPACING);
	setDragEnabled(true);
	setAcceptDrops(true);
	setDropIndicatorShown(true);

	viewport()->setMouseTracking(true);
	// Set this attribute, otherwise the item delegate won't get the
	// State_MouseOver state
	viewport()->setAttribute(Qt::WA_Hover);

	setVerticalScrollMode(ScrollPerPixel);
	setHorizontalScrollMode(ScrollPerPixel);

	// Make sure mThumbnailSize is initialized before calling setThumbnailSize,
	// since it will compare the new size with the old one
	d->mThumbnailSize = 0;
	setThumbnailSize(128);

	d->mScheduledThumbnailGenerationTimer.setSingleShot(true);
	d->mScheduledThumbnailGenerationTimer.setInterval(500);
	connect(&d->mScheduledThumbnailGenerationTimer, SIGNAL(timeout()),
		SLOT(generateThumbnailsForVisibleItems()) );

	d->mSmoothThumbnailTimer.setSingleShot(true);
	connect(&d->mSmoothThumbnailTimer, SIGNAL(timeout()),
		SLOT(smoothNextThumbnail()) );

	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(showContextMenu()) );

	if (KGlobalSettings::singleClick()) {
		connect(this, SIGNAL(clicked(const QModelIndex&)),
			SLOT(emitIndexActivatedIfNoModifiers(const QModelIndex&)) );
	} else {
		connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
			SLOT(emitIndexActivatedIfNoModifiers(const QModelIndex&)) );
	}
}


ThumbnailView::~ThumbnailView() {
	delete d;
}


void ThumbnailView::setThumbnailSize(int value) {
	if (d->mThumbnailSize == value) {
		return;
	}
	d->mThumbnailSize = value;

	// mWaitingThumbnail
	int waitingThumbnailSize;
	if (value > 64) {
		waitingThumbnailSize = 48;
	} else {
		waitingThumbnailSize = 32;
	}
	if (d->mWaitingThumbnail.width() != waitingThumbnailSize) {
		QPixmap icon = DesktopIcon("chronometer", waitingThumbnailSize);
		QPixmap pix(icon.size());
		pix.fill(Qt::transparent);
		QPainter painter(&pix);
		painter.setOpacity(0.5);
		painter.drawPixmap(0, 0, icon);
		painter.end();
		d->mWaitingThumbnail = pix;
	}

	// Stop smoothing
	d->mSmoothThumbnailTimer.stop();
	d->mSmoothThumbnailQueue.clear();

	// Clear adjustedPixes
	ThumbnailForUrlMap::iterator
		it = d->mThumbnailForUrl.begin(),
		end = d->mThumbnailForUrl.end();
	for (; it!=end; ++it) {
		it.value().adjustedPix = QPixmap();
	}

	thumbnailSizeChanged(value);
	setSpacing(SPACING);
	d->scheduleThumbnailGenerationForVisibleItems();
}


int ThumbnailView::thumbnailSize() const {
	return d->mThumbnailSize;
}


void ThumbnailView::setThumbnailViewHelper(AbstractThumbnailViewHelper* helper) {
	d->mThumbnailViewHelper = helper;
}

AbstractThumbnailViewHelper* ThumbnailView::thumbnailViewHelper() const {
	return d->mThumbnailViewHelper;
}


void ThumbnailView::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) {
	QListView::rowsAboutToBeRemoved(parent, start, end);

	// Remove references to removed items
	KFileItemList itemList;
	for (int pos=start; pos<=end; ++pos) {
		QModelIndex index = model()->index(pos, 0, parent);
		KFileItem item = fileItemForIndex(index);
		if (item.isNull()) {
			kDebug() << "Skipping invalid item!" << index.data().toString();
			continue;
		}

		QUrl url = item.url();
		d->mThumbnailForUrl.remove(url);
		d->mPersistentIndexForUrl.remove(url);
		d->mSmoothThumbnailQueue.removeAll(url);

		itemList.append(item);
	}

	if (d->mThumbnailLoadJob) {
		d->mThumbnailLoadJob->removeItems(itemList);
	}

	// Update current index if it is among the deleted rows
	const int row = currentIndex().row();
	if (start <= row && row <= end) {
		QModelIndex index;
		if (end < model()->rowCount() - 1) {
			index = model()->index(end + 1, 0);
		} else if (start > 0) {
			index = model()->index(start - 1, 0);
		}
		setCurrentIndex(index);
	}
}


void ThumbnailView::rowsInserted(const QModelIndex& parent, int start, int end) {
	QListView::rowsInserted(parent, start, end);
	d->mScheduledThumbnailGenerationTimer.start();
}


void ThumbnailView::showContextMenu() {
	d->mThumbnailViewHelper->showContextMenu(this);
}


void ThumbnailView::emitIndexActivatedIfNoModifiers(const QModelIndex& index) {
	if (QApplication::keyboardModifiers() == Qt::NoModifier) {
		emit indexActivated(index);
	}
}


void ThumbnailView::setThumbnail(const KFileItem& item, const QPixmap& pixmap, const QSize& size) {
	QUrl url = item.url();
	QPersistentModelIndex persistentIndex = d->mPersistentIndexForUrl[url];
	if (!persistentIndex.isValid()) {
		return;
	}

	Thumbnail& thumbnail = d->mThumbnailForUrl[url];
	thumbnail.groupPix = pixmap;
	thumbnail.adjustedPix = QPixmap();
	thumbnail.fullSize = size;

	QRect rect = visualRect(persistentIndex);
	update(rect);
	viewport()->update(rect);
}


QPixmap ThumbnailView::thumbnailForIndex(const QModelIndex& index) {
	KFileItem item = fileItemForIndex(index);
	if (item.isNull()) {
		LOG("Invalid item");
		return QPixmap();
	}
	KUrl url = item.url();


	ThumbnailForUrlMap::Iterator it = d->mThumbnailForUrl.find(url);
	if (it == d->mThumbnailForUrl.end()) {
		if (ArchiveUtils::fileItemIsDirOrArchive(item)) {
			QPixmap pix = item.pixmap(128);
			Thumbnail& thumbnail = d->mThumbnailForUrl[url];
			thumbnail.groupPix = pix;
			thumbnail.fullSize = QSize(128, 128);
			d->roughAdjustThumbnail(&thumbnail, url);
			return pix;
		} else {
			generateThumbnailForIndex(index);
			return d->mWaitingThumbnail;
		}
	}

	Thumbnail& thumbnail = it.value();
	if (thumbnail.adjustedPix.isNull()) {
		d->roughAdjustThumbnail(&thumbnail, url);
	}
	return thumbnail.adjustedPix;
}


bool ThumbnailView::isModified(const QModelIndex& index) const {
	KUrl url = urlForIndex(index);
	return d->mThumbnailViewHelper->isDocumentModified(url);
}


void ThumbnailView::dragEnterEvent(QDragEnterEvent* event) {
	if (event->mimeData()->hasUrls()) {
		event->acceptProposedAction();
	}
}


void ThumbnailView::dragMoveEvent(QDragMoveEvent* event) {
	// Necessary, otherwise we don't reach dropEvent()
	event->acceptProposedAction();
}


void ThumbnailView::dropEvent(QDropEvent* event) {
	const KUrl::List urlList = KUrl::List::fromMimeData(event->mimeData());
	if (urlList.isEmpty()) {
		return;
	}

	QModelIndex destIndex = indexAt(event->pos());
	if (destIndex.isValid()) {
		KFileItem item = fileItemForIndex(destIndex);
		if (item.isDir()) {
			KUrl destUrl = item.url();
			d->mThumbnailViewHelper->showMenuForUrlDroppedOnDir(this, urlList, destUrl);
			return;
		}
	}

	d->mThumbnailViewHelper->showMenuForUrlDroppedOnViewport(this, urlList);

	event->acceptProposedAction();
}


void ThumbnailView::keyPressEvent(QKeyEvent* event) {
	QListView::keyPressEvent(event);
	if (event->key() == Qt::Key_Return) {
		const QModelIndex index = selectionModel()->currentIndex();
		if (index.isValid() && selectionModel()->selectedIndexes().count() == 1) {
			emit indexActivated(index);
		}
	}
}


void ThumbnailView::resizeEvent(QResizeEvent* event) {
	QListView::resizeEvent(event);
	d->scheduleThumbnailGenerationForVisibleItems();
}


void ThumbnailView::showEvent(QShowEvent* event) {
	QListView::showEvent(event);
	d->scheduleThumbnailGenerationForVisibleItems();
}


void ThumbnailView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
	QListView::selectionChanged(selected, deselected);
	emit selectionChangedSignal(selected, deselected);
}


void ThumbnailView::scrollContentsBy(int dx, int dy) {
	QListView::scrollContentsBy(dx, dy);
	d->scheduleThumbnailGenerationForVisibleItems();
}


void ThumbnailView::generateThumbnailsForVisibleItems() {
	if (!isVisible()) {
		return;
	}
	KFileItemList list;
	QRect viewportRect = viewport()->rect();
	for (int row=0; row < model()->rowCount(); ++row) {
		QModelIndex index = model()->index(row, 0);
		KFileItem item = fileItemForIndex(index);
		QUrl url = item.url();

		// Filter out invisible items
		QRect rect = visualRect(index);
		if (!viewportRect.intersects(rect)) {
			continue;
		}

		// Filter out non documents
		MimeTypeUtils::Kind kind = MimeTypeUtils::fileItemKind(item);
		if (kind == MimeTypeUtils::KIND_DIR || kind == MimeTypeUtils::KIND_ARCHIVE) {
			continue;
		}

		// Immediately update modified items
		if (d->mThumbnailViewHelper->isDocumentModified(url)) {
			d->updateThumbnailForModifiedDocument(index);
			continue;
		}

		// Filter out items which already have a thumbnail
		ThumbnailForUrlMap::ConstIterator it = d->mThumbnailForUrl.constFind(url);
		if (it != d->mThumbnailForUrl.constEnd() && it.value().isGroupPixAdaptedForSize(d->mThumbnailSize)) {
			continue;
		}

		// Add the item to our list and to mPersistentIndexForUrl, so that
		// setThumbnail() can find the item to update
		list << item;
		d->mPersistentIndexForUrl[url] = QPersistentModelIndex(index);
	}

	if (!list.empty()) {
		d->generateThumbnailsForItems(list);
	}
}


void ThumbnailView::generateThumbnailForIndex(const QModelIndex& index) {
	KFileItem item = fileItemForIndex(index);
	KUrl url = item.url();
	if (d->mThumbnailViewHelper->isDocumentModified(url)) {
		d->updateThumbnailForModifiedDocument(index);
	} else {
		KFileItemList list;
		list << item;
		d->generateThumbnailsForItems(list);
	}
}


void ThumbnailView::smoothNextThumbnail() {
	if (d->mSmoothThumbnailQueue.isEmpty()) {
		return;
	}

	if (d->mThumbnailLoadJob) {
		// give mThumbnailLoadJob priority over smoothing
		d->mSmoothThumbnailTimer.start(SMOOTH_DELAY);
		return;
	}

	KUrl url = d->mSmoothThumbnailQueue.dequeue();
	ThumbnailForUrlMap::Iterator it = d->mThumbnailForUrl.find(url);
	if (it == d->mThumbnailForUrl.end()) {
		kWarning() <<  url << " not in mThumbnailForUrl. This should not happen!";
		return;
	}

	Thumbnail& thumbnail = it.value();
	thumbnail.adjustedPix = thumbnail.groupPix.scaled(d->mThumbnailSize, d->mThumbnailSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	thumbnail.rough = false;

	QPersistentModelIndex persistentIndex = d->mPersistentIndexForUrl.value(url);
	viewport()->update(visualRect(persistentIndex));

	if (!d->mSmoothThumbnailQueue.isEmpty()) {
		d->mSmoothThumbnailTimer.start(0);
	}
}


} // namespace
