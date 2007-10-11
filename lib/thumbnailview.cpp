/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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

#include <QHBoxLayout>
#include <QHelpEvent>
#include <QPainter>
#include <QPainterPath>
#include <QTimer>
#include <QToolButton>
#include <QToolTip>

// KDE
#include <kdebug.h>
#include <kdirmodel.h>

// Local
#include "archiveutils.h"
#include "abstractthumbnailviewhelper.h"
#include <lib/document/documentfactory.h>

namespace Gwenview {

/** Space between the item outer rect and the content */
const int ITEM_MARGIN = 5;

/** Border around gadget icons */
const int GADGET_MARGIN = 2;

const int SPACING = 11;

const int THUMBNAIL_GENERATION_TIMEOUT = 1000;


/**
 * An ItemDelegate which generates thumbnails for images. It also makes sure
 * all items are of the same size.
 */
class PreviewItemDelegate : public QAbstractItemDelegate {
public:
	PreviewItemDelegate(ThumbnailView* view)
	: QAbstractItemDelegate(view)
	, mView(view)
	{
		mModifiedPixmap = SmallIcon("document-save");

		mButtonFrame = new QFrame(mView->viewport());
		mButtonFrame->setStyleSheet(
			"QFrame {"
			"	background-color:"
			"		qlineargradient(x1:0, y1:0, x2:0, y2:1,"
			"		stop:0 #444, stop: 0.6 black, stop:1 black);"
			"	border: 1px solid #ccc;"
			"	padding: 2px;"
			"	border-radius: 4px;"
			"}"

			"QToolButton {"
			"	padding: 2px;"
			"	border-radius: 2px;"
			"}"

			"QToolButton:hover {"
			"	border: 1px solid #aaa;"
			"}"

			"QToolButton:pressed {"
			"	background-color:"
			"		qlineargradient(x1:0, y1:0, x2:0, y2:1,"
			"		stop:0 #222, stop: 0.6 black, stop:1 black);"
			"	border: 1px solid #444;"
			"}"
			);
		mButtonFrame->setAutoFillBackground(true);
		mButtonFrame->setBackgroundRole(QPalette::Button);
		mButtonFrame->hide();

		mSaveButton = new QToolButton(mButtonFrame);
		mSaveButton->setIconSize(mModifiedPixmap.size());
		mSaveButton->setIcon(mModifiedPixmap);
		mSaveButton->setAutoRaise(true);
		connect(mSaveButton, SIGNAL(clicked()),
			mView, SLOT(slotSaveClicked()) );

		QToolButton* rotateLeftButton = new QToolButton(mButtonFrame);
		rotateLeftButton->setIconSize(mModifiedPixmap.size());
		rotateLeftButton->setIcon(SmallIcon("object-rotate-left"));
		rotateLeftButton->setAutoRaise(true);
		connect(rotateLeftButton, SIGNAL(clicked()),
			mView, SLOT(slotRotateLeftClicked()) );

		QToolButton* rotateRightButton = new QToolButton(mButtonFrame);
		rotateRightButton->setIconSize(mModifiedPixmap.size());
		rotateRightButton->setIcon(SmallIcon("object-rotate-right"));
		rotateRightButton->setAutoRaise(true);
		connect(rotateRightButton, SIGNAL(clicked()),
			mView, SLOT(slotRotateRightClicked()) );

		QHBoxLayout* layout = new QHBoxLayout(mButtonFrame);
		layout->setMargin(0);
		layout->setSpacing(0);
		layout->addWidget(mSaveButton);
		layout->addWidget(rotateLeftButton);
		layout->addWidget(rotateRightButton);
	}


	void clearElidedTextMap() {
		mElidedTextMap.clear();
	}


	virtual QSize sizeHint( const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/ ) const {
		return QSize( mView->itemWidth(), mView->itemHeight() );
	}


	virtual bool eventFilter(QObject* object, QEvent* event) {
		if (event->type() == QEvent::ToolTip) {
			QAbstractItemView* view = static_cast<QAbstractItemView*>(object->parent());
			QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
			showToolTip(view, helpEvent);
			return true;

		} else if (event->type() == QEvent::HoverMove) {
			QHoverEvent* hoverEvent = static_cast<QHoverEvent*>(event);
			return hoverEventFilter(hoverEvent);
		}
		return false;
	}


	bool hoverEventFilter(QHoverEvent* event) {
		mIndexUnderCursor = mView->indexAt(event->pos());
		if (mIndexUnderCursor.isValid()) {
			QRect rect = mView->visualRect(mIndexUnderCursor);
			mButtonFrame->move(rect.x() + GADGET_MARGIN, rect.y() + GADGET_MARGIN);
			mButtonFrame->show();
		} else {
			mButtonFrame->hide();
		}
		return false;
	}


	virtual void paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const {
		int thumbnailSize = mView->thumbnailSize();
		QPixmap thumbnail = mView->thumbnailForIndex(index);
		if (thumbnail.width() > thumbnailSize || thumbnail.height() > thumbnailSize) {
			thumbnail = thumbnail.scaled(thumbnailSize, thumbnailSize, Qt::KeepAspectRatio);
		}
		QRect rect = option.rect;

#ifdef DEBUG_RECT
		painter->setPen(Qt::red);
		painter->setBrush(Qt::NoBrush);
		painter->drawRect(rect);
#endif

		// Crop text
		QString fullText = index.data(Qt::DisplayRole).toString();
		QString text;
		QMap<QString, QString>::const_iterator it = mElidedTextMap.find(fullText);
		if (it == mElidedTextMap.constEnd()) {
			text = elidedText(option.fontMetrics, rect.width() - 2*ITEM_MARGIN, Qt::ElideRight, fullText);
			mElidedTextMap[fullText] = text;
		} else {
			text = it.value();
		}

		int textWidth = option.fontMetrics.width(text);

		// Select color group
		QPalette::ColorGroup cg;

		if ( (option.state & QStyle::State_Enabled) && (option.state & QStyle::State_Active) ) {
			cg = QPalette::Normal;
		} else if ( (option.state & QStyle::State_Enabled)) {
			cg = QPalette::Inactive;
		} else {
			cg = QPalette::Disabled;
		}

		// Select colors
		QColor bgColor, borderColor, fgColor;
		if (option.state & QStyle::State_Selected) {
			bgColor = option.palette.color(cg, QPalette::Highlight);
			borderColor = bgColor.dark(140);
			fgColor = option.palette.color(cg, QPalette::HighlightedText);
		} else {
			QWidget* viewport = mView->viewport();
			bgColor = viewport->palette().color(viewport->backgroundRole());
			fgColor = viewport->palette().color(viewport->foregroundRole());
			borderColor = fgColor;
		}
		painter->setPen(borderColor);

		// Draw background
		if (option.state & QStyle::State_Selected) {
			painter->fillRect(rect, bgColor);
			QRect borderRect = rect;
			borderRect.adjust(0, 0, -1, -1);
			painter->drawRect(borderRect);
		}

		// Draw thumbnail
		QRect thumbnailRect = QRect(
			rect.left() + (rect.width() - thumbnail.width())/2,
			rect.top() + (thumbnailSize - thumbnail.height())/2 + ITEM_MARGIN,
			thumbnail.width(),
			thumbnail.height());

		if (!thumbnail.hasAlphaChannel()) {
			QRect borderRect = thumbnailRect.adjusted(-1, -1, 0, 0);
			painter->drawRect(borderRect);
		}
		painter->drawPixmap(
			thumbnailRect.left() + (thumbnailRect.width() - thumbnail.width()) / 2,
			thumbnailRect.top() + (thumbnailRect.height() - thumbnail.height()) / 2,
			thumbnail);

		// Draw modified indicator
		if (mView->isModified(index)) {
			QRect saveRect(
				rect.x() + GADGET_MARGIN,
				rect.y() + GADGET_MARGIN,
				mSaveButton->sizeHint().width(),
				mSaveButton->sizeHint().height());

			painter->drawPixmap(
				saveRect.x() + (saveRect.width() - mModifiedPixmap.width()) / 2,
				saveRect.y() + (saveRect.height() - mModifiedPixmap.height()) / 2,
				mModifiedPixmap);
		}

		// Draw text
		painter->setPen(fgColor);

		painter->drawText(
			rect.left() + (rect.width() - textWidth) / 2,
			rect.top() + ITEM_MARGIN + thumbnailSize + ITEM_MARGIN + option.fontMetrics.ascent(),
			text);
	}


	QModelIndex indexUnderCursor() const {
		return mIndexUnderCursor;
	}


private:
	/**
	 * Show a tooltip only if the item has been elided.
	 * This function places the tooltip over the item text.
	 */
	void showToolTip(QAbstractItemView* view, QHelpEvent* helpEvent) {
		QModelIndex index = view->indexAt(helpEvent->pos());
		if (!index.isValid()) {
			return;
		}

		QString fullText = index.data().toString();
		QMap<QString, QString>::const_iterator it = mElidedTextMap.find(fullText);
		if (it == mElidedTextMap.constEnd()) {
			return;
		}
		QString elidedText = it.value();
		if (elidedText.length() == fullText.length()) {
			// text and tooltip are the same, don't show tooltip
			fullText = QString();
		}
		QRect rect = view->visualRect(index);
		QPoint pos(rect.left() + ITEM_MARGIN, rect.top() + mView->thumbnailSize() + ITEM_MARGIN);
		QToolTip::showText(view->mapToGlobal(pos), fullText, view);
		return;
	}


	/**
	 * Maps full text to elided text.
	 */
	mutable QMap<QString, QString> mElidedTextMap;

	ThumbnailView* mView;
	QPixmap mModifiedPixmap;
	QToolButton* mSaveButton;
	QFrame* mButtonFrame;
	QModelIndex mIndexUnderCursor;
};


struct ThumbnailViewPrivate {
	int mThumbnailSize;
	PreviewItemDelegate* mItemDelegate;
	AbstractThumbnailViewHelper* mThumbnailViewHelper;
	QMap<QUrl, QPixmap> mThumbnailForUrl;
	QMap<QUrl, QPersistentModelIndex> mPersistentIndexForUrl;
};


ThumbnailView::ThumbnailView(QWidget* parent)
: QListView(parent)
, d(new ThumbnailViewPrivate) {
	setViewMode(QListView::IconMode);
	setResizeMode(QListView::Adjust);
	setMovement(QListView::Static);
	setSpacing(SPACING);

	d->mItemDelegate = new PreviewItemDelegate(this);
	setItemDelegate(d->mItemDelegate);
	viewport()->installEventFilter(d->mItemDelegate);

	viewport()->setMouseTracking(true);
	// Set this attribute, otherwise the item delegate won't get the
	// State_MouseOver state
	viewport()->setAttribute(Qt::WA_Hover);

	setVerticalScrollMode(ScrollPerPixel);
	setHorizontalScrollMode(ScrollPerPixel);

	d->mThumbnailViewHelper = 0;
	setThumbnailSize(128);

	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
		SLOT(showContextMenu()) );
}


ThumbnailView::~ThumbnailView() {
	delete d;
}


void ThumbnailView::setThumbnailSize(int value) {
	d->mThumbnailSize = value;
	d->mItemDelegate->clearElidedTextMap();
	setSpacing(SPACING);
}


int ThumbnailView::thumbnailSize() const {
	return d->mThumbnailSize;
}

int ThumbnailView::itemWidth() const {
	return d->mThumbnailSize + 2 * ITEM_MARGIN;
}

int ThumbnailView::itemHeight() const {
	return d->mThumbnailSize + fontMetrics().height() + 3*ITEM_MARGIN;
}

void ThumbnailView::setThumbnailViewHelper(AbstractThumbnailViewHelper* helper) {
	d->mThumbnailViewHelper = helper;
	connect(helper, SIGNAL(thumbnailLoaded(const KFileItem&, const QPixmap&)),
		SLOT(setThumbnail(const KFileItem&, const QPixmap&)) );
}

AbstractThumbnailViewHelper* ThumbnailView::thumbnailViewHelper() const {
	return d->mThumbnailViewHelper;
}


void ThumbnailView::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end) {
	QListView::rowsAboutToBeRemoved(parent, start, end);

	KFileItemList itemList;
	for (int pos=start; pos<=end; ++pos) {
		QModelIndex index = model()->index(pos, 0, parent);

		QVariant data = index.data(KDirModel::FileItemRole);
		KFileItem item = qvariant_cast<KFileItem>(data);

		QUrl url = item.url();
		d->mThumbnailForUrl.remove(url);
		d->mPersistentIndexForUrl.remove(url);

		itemList.append(item);
	}

	Q_ASSERT(d->mThumbnailViewHelper);
	d->mThumbnailViewHelper->abortThumbnailGenerationForItems(itemList);
}


void ThumbnailView::showContextMenu() {
	d->mThumbnailViewHelper->showContextMenu(this);
}


void ThumbnailView::setThumbnail(const KFileItem& item, const QPixmap& pixmap) {
	QUrl url = item.url();
	QPersistentModelIndex persistentIndex = d->mPersistentIndexForUrl[url];
	if (!persistentIndex.isValid()) {
		return;
	}
	d->mThumbnailForUrl[url] = pixmap;

	QRect rect = visualRect(persistentIndex);
	update(rect);
	viewport()->update(rect);
}


QPixmap ThumbnailView::thumbnailForIndex(const QModelIndex& index) {
	QVariant data = index.data(KDirModel::FileItemRole);
	KFileItem item = qvariant_cast<KFileItem>(data);

	QUrl url = item.url();
	QMap<QUrl, QPixmap>::ConstIterator it = d->mThumbnailForUrl.find(url);
	if (it != d->mThumbnailForUrl.constEnd()) {
		return it.value();
	}

	if (ArchiveUtils::fileItemIsDirOrArchive(item)) {
		return item.pixmap(128);
	}

	KFileItemList list;
	list << item;
	d->mPersistentIndexForUrl[url] = QPersistentModelIndex(index);
	d->mThumbnailViewHelper->generateThumbnailsForItems(list);
	return QPixmap();
}


static KUrl urlForIndex(const QModelIndex& index) {
	Q_ASSERT(index.isValid());
	QVariant data = index.data(KDirModel::FileItemRole);
	KFileItem item = qvariant_cast<KFileItem>(data);
	return item.url();
}


bool ThumbnailView::isModified(const QModelIndex& index) const {
	QVariant data = index.data(KDirModel::FileItemRole);
	KFileItem item = qvariant_cast<KFileItem>(data);
	QUrl url = item.url();
	DocumentFactory* factory = DocumentFactory::instance();

	if (factory->hasUrl(url)) {
		Document::Ptr doc = factory->load(item.url());
		return doc->isLoaded() && doc->isModified();
	} else {
		return false;
	}
}


void ThumbnailView::slotSaveClicked() {
	QModelIndex index = d->mItemDelegate->indexUnderCursor();
	saveDocumentRequested(urlForIndex(index));
}


void ThumbnailView::slotRotateLeftClicked() {
	QModelIndex index = d->mItemDelegate->indexUnderCursor();
	rotateDocumentLeftRequested(urlForIndex(index));
}


void ThumbnailView::slotRotateRightClicked() {
	QModelIndex index = d->mItemDelegate->indexUnderCursor();
	rotateDocumentRightRequested(urlForIndex(index));
}


} // namespace
