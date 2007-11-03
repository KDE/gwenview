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

// Std
#include <math.h>

// Qt
#include <QDragEnterEvent>
#include <QDropEvent>
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

namespace Gwenview {

/** Space between the item outer rect and the content */
const int ITEM_MARGIN = 5;

/** Border around gadget icons */
const int GADGET_MARGIN = 2;

const int SPACING = 11;

const int ROUND_RECT_RADIUS = 10;

const int THUMBNAIL_GENERATION_TIMEOUT = 1000;


// Copied from KFileItemDelegate
static QPainterPath roundedRectangle(const QRectF &rect, qreal radius) {
	QPainterPath path(QPointF(rect.left(), rect.top() + radius));
	path.quadTo(rect.left(), rect.top(), rect.left() + radius, rect.top());         // Top left corner
	path.lineTo(rect.right() - radius, rect.top());                                 // Top side
	path.quadTo(rect.right(), rect.top(), rect.right(), rect.top() + radius);       // Top right corner
	path.lineTo(rect.right(), rect.bottom() - radius);                              // Right side
	path.quadTo(rect.right(), rect.bottom(), rect.right() - radius, rect.bottom()); // Bottom right corner
	path.lineTo(rect.left() + radius, rect.bottom());                               // Bottom side
	path.quadTo(rect.left(), rect.bottom(), rect.left(), rect.bottom() - radius);   // Bottom left corner
	path.closeSubpath();

	return path;
}


static KFileItem fileItemForIndex(const QModelIndex& index) {
	Q_ASSERT(index.isValid());
	QVariant data = index.data(KDirModel::FileItemRole);
	return qvariant_cast<KFileItem>(data);
}


static KUrl urlForIndex(const QModelIndex& index) {
	KFileItem item = fileItemForIndex(index);
	return item.url();
}


ThumbnailView::Thumbnail::Thumbnail(const QPixmap& pixmap)
: mPixmap(pixmap) {
	if (mPixmap.isNull()) {
		mOpaque = true;
		return;
	}
	QImage img = mPixmap.toImage();
	int a1 = qAlpha(img.pixel(0, 0));
	int a2 = qAlpha(img.pixel(img.width() - 1, 0));
	int a3 = qAlpha(img.pixel(0, img.height() - 1));
	int a4 = qAlpha(img.pixel(img.width() - 1, img.height() - 1));
	mOpaque = a1 + a2 + a3 + a4 == 4*255;
}


ThumbnailView::Thumbnail::Thumbnail() {}


ThumbnailView::Thumbnail::Thumbnail(const ThumbnailView::Thumbnail& other)
: mPixmap(other.mPixmap)
, mOpaque(other.mOpaque) {}


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
		QModelIndex index = mView->indexAt(event->pos());
		if (index == mIndexUnderCursor) {
			// Same index, nothing to do
			return false;
		}
		mIndexUnderCursor = index;

		bool showButtonFrame = false;
		if (mIndexUnderCursor.isValid()) {
			KFileItem item = fileItemForIndex(mIndexUnderCursor);
			showButtonFrame = !ArchiveUtils::fileItemIsDirOrArchive(item);
		}

		if (showButtonFrame) {
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
		ThumbnailView::Thumbnail thumbnail = mView->thumbnailForIndex(index);
		QPixmap thumbnailPix = thumbnail.mPixmap;
		if (thumbnailPix.width() > thumbnailSize || thumbnailPix.height() > thumbnailSize) {
			thumbnailPix = thumbnailPix.scaled(thumbnailSize, thumbnailSize, Qt::KeepAspectRatio);
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

			if (bgColor.value() < 128) {
				borderColor = bgColor.dark(200);
			} else {
				borderColor = bgColor.light(200);
			}
			//borderColor = fgColor;
		}

		// Draw background
		if (option.state & QStyle::State_Selected) {
			drawBackground(painter, rect, bgColor, borderColor);
		}

		// Draw thumbnail
		if (!thumbnailPix.isNull()) {
			QRect thumbnailRect = QRect(
				rect.left() + (rect.width() - thumbnailPix.width())/2,
				rect.top() + (thumbnailSize - thumbnailPix.height())/2 + ITEM_MARGIN,
				thumbnailPix.width(),
				thumbnailPix.height());

			if (!(option.state & QStyle::State_Selected) && thumbnail.mOpaque) {
				drawShadow(painter, thumbnailRect);
			}

			if (thumbnail.mOpaque) {
				painter->setPen(borderColor);
				painter->setRenderHint(QPainter::Antialiasing, false);
				QRect borderRect = thumbnailRect.adjusted(-1, -1, 0, 0);
				painter->drawRect(borderRect);
			}
			painter->drawPixmap(thumbnailRect.left(), thumbnailRect.top(), thumbnailPix);
		}

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
	void drawBackground(QPainter* painter, const QRect& rect, const QColor& bgColor, const QColor& borderColor) const {
		painter->setRenderHint(QPainter::Antialiasing);

		QRectF rectF = QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5);

		QPainterPath path = roundedRectangle(rectF, ROUND_RECT_RADIUS);
		painter->fillPath(path, bgColor);
		painter->setPen(borderColor);
		painter->drawPath(path);
	}


	void drawShadow(QPainter* painter, const QRect& rect) const {
		const int shadowStrength = 96;
		const int shadowSize = 4;
		const int shadowOffsetX = 0;
		const int shadowOffsetY = 1;
		QImage image(rect.width() + 2*shadowSize, rect.height() + 2*shadowSize, QImage::Format_ARGB32);
		image.fill(0);
		{
			QPainter imgPainter(&image);
			QColor color = QColor(0, 0, 0, shadowStrength);
			imgPainter.fillRect(shadowSize, shadowSize, rect.width(), rect.height(), color);

			for (int pos = 0; pos < shadowSize; ++pos) {
				double delta = pos / double(shadowSize);
				int alpha = int(shadowStrength * delta);
				color = QColor(0, 0, 0, alpha);
				imgPainter.setPen(color);
				imgPainter.drawLine(shadowSize, pos, image.width() - 1 - shadowSize, pos);
				imgPainter.drawLine(shadowSize, image.height() - 1 - pos, image.width() - 1 - shadowSize, image.height() - 1 - pos);
				imgPainter.drawLine(pos, shadowSize, pos, image.height() - 1 - shadowSize);
				imgPainter.drawLine(image.width() - 1 - pos, shadowSize, image.width() - 1 - pos, image.height() - 1 - shadowSize);
			}
		}
		for (int posY = 0; posY < shadowSize; ++posY) {
			for (int posX = 0; posX < shadowSize; ++posX) {
				double dX = (shadowSize - posX) / double(shadowSize);
				double dY = (shadowSize - posY) / double(shadowSize);
				double delta = 1 - sqrt(dX*dX + dY*dY);
				if (delta < 0.) {
					continue;
				}
				int alpha = int(shadowStrength * delta);
				QRgb rgb = qRgba(0, 0, 0, alpha);
				image.setPixel(posX, posY, rgb);
				image.setPixel(image.width() - 1 - posX, posY, rgb);
				image.setPixel(posX, image.height() - 1 - posY, rgb);
				image.setPixel(image.width() - 1 - posX, image.height() - 1 - posY, rgb);
			}
		}
		painter->drawImage(rect.left() - shadowSize + shadowOffsetX, rect.top() - shadowSize + shadowOffsetY, image);
	}


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
	QMap<QUrl, ThumbnailView::Thumbnail> mThumbnailForUrl;
	QMap<QUrl, QPersistentModelIndex> mPersistentIndexForUrl;
};


ThumbnailView::ThumbnailView(QWidget* parent)
: QListView(parent)
, d(new ThumbnailViewPrivate) {
	setViewMode(QListView::IconMode);
	setResizeMode(QListView::Adjust);
	setSpacing(SPACING);
	setDragEnabled(true);
	setAcceptDrops(true);
	setDropIndicatorShown(true);

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

	// Alpha check

	d->mThumbnailForUrl[url] = Thumbnail(pixmap);

	QRect rect = visualRect(persistentIndex);
	update(rect);
	viewport()->update(rect);
}


ThumbnailView::Thumbnail ThumbnailView::thumbnailForIndex(const QModelIndex& index) {
	QVariant data = index.data(KDirModel::FileItemRole);
	KFileItem item = qvariant_cast<KFileItem>(data);

	QUrl url = item.url();
	QMap<QUrl, Thumbnail>::ConstIterator it = d->mThumbnailForUrl.find(url);
	if (it != d->mThumbnailForUrl.constEnd()) {
		return it.value();
	}

	if (ArchiveUtils::fileItemIsDirOrArchive(item)) {
		return Thumbnail(item.pixmap(128));
	}

	KFileItemList list;
	list << item;
	d->mPersistentIndexForUrl[url] = QPersistentModelIndex(index);
	d->mThumbnailViewHelper->generateThumbnailsForItems(list);
	return Thumbnail(QPixmap());
}


bool ThumbnailView::isModified(const QModelIndex& index) const {
	KUrl url = urlForIndex(index);
	return d->mThumbnailViewHelper->isDocumentModified(url);
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


} // namespace
