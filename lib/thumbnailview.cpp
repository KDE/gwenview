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

/**
 * Space between the item outer rect and the content, and between the
 * thumbnail and the caption
 */
const int ITEM_MARGIN = 5;

/** How darker is the border line around selection */
const int SELECTION_BORDER_DARKNESS = 140;

/** Radius of the selection rounded corners, in pixels */
const int SELECTION_RADIUS = 10;

/** Border around gadget icons */
const int GADGET_MARGIN = 2;

/** How many pixels between items */
const int SPACING = 11;

/** How dark is the shadow, 0 is invisible, 255 is as dark as possible */
const int SHADOW_STRENGTH = 128;

/** How many pixels around the thumbnail are shadowed */
const int SHADOW_SIZE = 4;


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


static QPixmap generateFuzzyRect(const QSize& size, const QColor& color, int radius) {
	QPixmap pix(size);
	const QColor transparent(0, 0, 0, 0);
	pix.fill(transparent);

	QPainter painter(&pix);
	painter.setRenderHint(QPainter::Antialiasing, true);

	// Fill middle
	painter.fillRect(pix.rect().adjusted(radius, radius, -radius, -radius), color);

	// Corners
	QRadialGradient gradient;
	gradient.setColorAt(0, color);
	gradient.setColorAt(1, transparent);
	gradient.setRadius(radius);
	QPoint center;

	// Top Left
	center = QPoint(radius, radius);
	gradient.setCenter(center);
	gradient.setFocalPoint(center);
	painter.fillRect(0, 0, radius, radius, gradient);

	// Top right
	center = QPoint(size.width() - radius, radius);
	gradient.setCenter(center);
	gradient.setFocalPoint(center);
	painter.fillRect(center.x(), 0, radius, radius, gradient);

	// Bottom left
	center = QPoint(radius, size.height() - radius);
	gradient.setCenter(center);
	gradient.setFocalPoint(center);
	painter.fillRect(0, center.y(), radius, radius, gradient);

	// Bottom right
	center = QPoint(size.width() - radius, size.height() - radius);
	gradient.setCenter(center);
	gradient.setFocalPoint(center);
	painter.fillRect(center.x(), center.y(), radius, radius, gradient);

	// Borders
	QLinearGradient linearGradient;
	linearGradient.setColorAt(0, color);
	linearGradient.setColorAt(1, transparent);

	// Top
	linearGradient.setStart(0, radius);
	linearGradient.setFinalStop(0, 0);
	painter.fillRect(radius, 0, size.width() - 2*radius, radius, linearGradient);

	// Bottom
	linearGradient.setStart(0, size.height() - radius);
	linearGradient.setFinalStop(0, size.height());
	painter.fillRect(radius, int(linearGradient.start().y()), size.width() - 2*radius, radius, linearGradient);

	// Left
	linearGradient.setStart(radius, 0);
	linearGradient.setFinalStop(0, 0);
	painter.fillRect(0, radius, radius, size.height() - 2*radius, linearGradient);

	// Right
	linearGradient.setStart(size.width() - radius, 0);
	linearGradient.setFinalStop(size.width(), 0);
	painter.fillRect(int(linearGradient.start().x()), radius, radius, size.height() - 2*radius, linearGradient);
	return pix;
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

		QColor bgColor = mView->palette().highlight().color();
		QColor borderColor = bgColor.dark(SELECTION_BORDER_DARKNESS);

		QString styleSheet =
			"QFrame {"
			"	background-color: %1;"
			"	border: 1px solid %1;"
			"	padding: 1px;"
			"	border-radius: 6px;"
			"}"

			"QToolButton {"
			"	padding: 2px;"
			"	border-radius: 4px;"
			"}"

			"QToolButton:hover {"
			"	border: 1px solid %2;"
			"}"

			"QToolButton:pressed {"
			"	background-color:"
			"		qlineargradient(x1:0, y1:0, x2:0, y2:1,"
			"		stop:0 %2, stop:1 %1);"
			"	border: 1px solid %2;"
			"}";
		styleSheet = styleSheet.arg(bgColor.name()).arg(borderColor.name());

		// Button frame
		mButtonFrame = new QFrame(mView->viewport());
		mButtonFrame->setStyleSheet(styleSheet);
		mButtonFrame->setAutoFillBackground(true);
		mButtonFrame->setBackgroundRole(QPalette::Button);
		mButtonFrame->hide();

		QToolButton* fullScreenButton = new QToolButton(mButtonFrame);
		fullScreenButton->setIconSize(mModifiedPixmap.size());
		fullScreenButton->setIcon(SmallIcon("view-fullscreen"));
		fullScreenButton->setAutoRaise(true);
		connect(fullScreenButton, SIGNAL(clicked()),
			mView, SLOT(slotFullScreenClicked()) );

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
		layout->addWidget(fullScreenButton);
		layout->addWidget(rotateLeftButton);
		layout->addWidget(rotateRightButton);

		// Save button frame
		mSaveButtonFrame = new QFrame(mView->viewport());
		mSaveButtonFrame->setStyleSheet(styleSheet);
		mSaveButtonFrame->setAutoFillBackground(true);
		mSaveButtonFrame->setBackgroundRole(QPalette::Button);
		mSaveButtonFrame->hide();

		mSaveButton = new QToolButton(mSaveButtonFrame);
		mSaveButton->setIconSize(mModifiedPixmap.size());
		mSaveButton->setIcon(mModifiedPixmap);
		mSaveButton->setAutoRaise(true);
		connect(mSaveButton, SIGNAL(clicked()),
			mView, SLOT(slotSaveClicked()) );

		layout = new QHBoxLayout(mSaveButtonFrame);
		layout->setMargin(0);
		layout->setSpacing(0);
		layout->addWidget(mSaveButton);
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

		bool showButtonFrames = false;
		if (mIndexUnderCursor.isValid()) {
			KFileItem item = fileItemForIndex(mIndexUnderCursor);
			showButtonFrames = !ArchiveUtils::fileItemIsDirOrArchive(item);
		}

		if (showButtonFrames) {
			QRect rect = mView->visualRect(mIndexUnderCursor);
			mButtonFrame->move(rect.x() + GADGET_MARGIN, rect.y() + GADGET_MARGIN);
			mButtonFrame->show();

			if (mView->isModified(mIndexUnderCursor)) {
				showSaveButtonFrame(rect);
			} else {
				mSaveButtonFrame->hide();
			}

		} else {
			mButtonFrame->hide();
			mSaveButtonFrame->hide();
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
			borderColor = bgColor.dark(SELECTION_BORDER_DARKNESS);
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
		bool isModified = mView->isModified(index);
		if (isModified) {
			// Draws the mModifiedPixmap pixmap at the exact same place where
			// it will be drawn when mSaveButtonFrame is shown.
			// This code assumes the save button pixmap is drawn in the middle
			// of the button. This should be the case unless the CSS is
			// changed.
			QPoint framePosition = saveButtonFramePosition(rect);
			QRect saveButtonRect(
				framePosition.x() + mSaveButton->x(),
				framePosition.y() + mSaveButton->y(),
				mSaveButton->sizeHint().width(),
				mSaveButton->sizeHint().height());

			int posX = saveButtonRect.x() + (saveButtonRect.width() - mModifiedPixmap.width()) / 2;
			int posY = saveButtonRect.y() + (saveButtonRect.height() - mModifiedPixmap.height()) / 2;

			painter->drawPixmap(posX, posY, mModifiedPixmap);
		}

		if (index == mIndexUnderCursor) {
			if (isModified) {
				// If we just rotated the image with the buttons from the
				// button frame, we need to show the save button frame right now.
				showSaveButtonFrame(rect);
			} else {
				mSaveButtonFrame->hide();
			}
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
	QPoint saveButtonFramePosition(const QRect& itemRect) const {
		QSize frameSize = mSaveButtonFrame->sizeHint();
		int textHeight = mView->fontMetrics().height();
		int posX = itemRect.right() - GADGET_MARGIN - frameSize.width();
		int posY = itemRect.bottom() - GADGET_MARGIN - textHeight - frameSize.height();

		return QPoint(posX, posY);
	}


	void showSaveButtonFrame(const QRect& itemRect) const {
		mSaveButtonFrame->move(saveButtonFramePosition(itemRect));
		mSaveButtonFrame->show();
	}


	void drawBackground(QPainter* painter, const QRect& rect, const QColor& bgColor, const QColor& borderColor) const {
		painter->setRenderHint(QPainter::Antialiasing);

		QRectF rectF = QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5);

		QPainterPath path = roundedRectangle(rectF, SELECTION_RADIUS);
		painter->fillPath(path, bgColor);
		painter->setPen(borderColor);
		painter->drawPath(path);
	}


	void drawShadow(QPainter* painter, const QRect& rect) const {
		const QPoint shadowOffset(-SHADOW_SIZE, -SHADOW_SIZE + 1);

		int key = rect.height() * 1000 + rect.width();

		ShadowCache::Iterator it = mShadowCache.find(key);
		if (it == mShadowCache.end()) {
			QSize size = QSize(rect.width() + 2*SHADOW_SIZE, rect.height() + 2*SHADOW_SIZE);
			QColor color(0, 0, 0, SHADOW_STRENGTH);
			QPixmap shadow = generateFuzzyRect(size, color, SHADOW_SIZE);
			it = mShadowCache.insert(key, shadow);
		}
		painter->drawPixmap(rect.topLeft() + shadowOffset, it.value());
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

	// Key is height * 1000 + width
	typedef QMap<int, QPixmap> ShadowCache;
	mutable ShadowCache mShadowCache;

	ThumbnailView* mView;
	QPixmap mModifiedPixmap;
	QToolButton* mSaveButton;
	QFrame* mButtonFrame;
	QFrame* mSaveButtonFrame;
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


void ThumbnailView::slotFullScreenClicked() {
	QModelIndex index = d->mItemDelegate->indexUnderCursor();
	showDocumentInFullScreenRequested(urlForIndex(index));
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
