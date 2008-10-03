// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#include "previewitemdelegate.moc"
#include <config-gwenview.h>

// Qt
#include <QHBoxLayout>
#include <QHelpEvent>
#include <QMap>
#include <QPainter>
#include <QPainterPath>
#include <QToolButton>
#include <QToolTip>

// KDE
#include <kdebug.h>
#include <kdirmodel.h>
#include <kglobalsettings.h>
#include <kurl.h>
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
#include <nepomuk/kratingpainter.h>
#endif

// Local
#include "archiveutils.h"
#include "paintutils.h"
#include "thumbnailview.h"
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
#include "../semanticinfo/semanticinfodirmodel.h"
#endif

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

/** Radius of the gadget frame, in pixels */
const int GADGET_RADIUS = 6;

/** How dark is the shadow, 0 is invisible, 255 is as dark as possible */
const int SHADOW_STRENGTH = 128;

/** How many pixels around the thumbnail are shadowed */
const int SHADOW_SIZE = 4;


static KFileItem fileItemForIndex(const QModelIndex& index) {
	Q_ASSERT(index.isValid());
	QVariant data = index.data(KDirModel::FileItemRole);
	return qvariant_cast<KFileItem>(data);
}


static KUrl urlForIndex(const QModelIndex& index) {
	KFileItem item = fileItemForIndex(index);
	return item.url();
}


/**
 * A frame with a rounded semi-opaque background. Since it's not possible (yet)
 * to define non-opaque colors in Qt stylesheets, we do it the old way: by
 * reimplementing paintEvent().
 * FIXME: Found out it is possible in fact, code should be updated.
 */
class GlossyFrame : public QFrame {
public:
	GlossyFrame(QWidget* parent = 0)
	: QFrame(parent)
	, mOpaque(false)
	{}

	void setOpaque(bool value) {
		if (value != mOpaque) {
			mOpaque = value;
			update();
		}
	}

	void setBackgroundColor(const QColor& color) {
		QPalette pal = palette();
		pal.setColor(backgroundRole(), color);
		setPalette(pal);
	}

protected:
	virtual void paintEvent(QPaintEvent* /*event*/) {
		QColor color = palette().color(backgroundRole());
		QColor borderColor;
		QRectF rectF = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
		QPainterPath path = PaintUtils::roundedRectangle(rectF, GADGET_RADIUS);

		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);

		if (mOpaque) {
			painter.fillPath(path, color);
			borderColor = color;
		} else {
			QLinearGradient gradient(rect().topLeft(), rect().bottomLeft());
			gradient.setColorAt(0, PaintUtils::alphaAdjustedF(color, 0.9));
			gradient.setColorAt(1, PaintUtils::alphaAdjustedF(color, 0.7));
			painter.fillPath(path, gradient);
			borderColor = color.dark(SELECTION_BORDER_DARKNESS);
		}
		painter.setPen(borderColor);
		painter.drawPath(path);
	}

private:
	bool mOpaque;
};


static QToolButton* createFrameButton(QWidget* parent, const char* iconName) {
	int size = KIconLoader::global()->currentSize(KIconLoader::Small);
	QToolButton* button = new QToolButton(parent);
	button->setIcon(SmallIcon(iconName));
	button->setIconSize(QSize(size, size));
	button->setAutoRaise(true);

	return button;
}


struct PreviewItemDelegatePrivate {
	/**
	 * Maps full text to elided text.
	 */
	mutable QMap<QString, QString> mElidedTextMap;

	// Key is height * 1000 + width
	typedef QMap<int, QPixmap> ShadowCache;
	mutable ShadowCache mShadowCache;

	PreviewItemDelegate* mDelegate;
	ThumbnailView* mView;
	GlossyFrame* mButtonFrame;
	GlossyFrame* mSaveButtonFrame;
	QPixmap mSaveButtonFramePixmap;
	KRatingPainter mRatingPainter;

	QModelIndex mIndexUnderCursor;
	int mThumbnailSize;

	QPoint mToolTipOffset;


	void initSaveButtonFramePixmap() {
		// Necessary otherwise we won't see the save button itself
		mSaveButtonFrame->adjustSize();

		// This is hackish.
		// Show/hide the frame to make sure mSaveButtonFrame->render produces
		// something coherent.
		mSaveButtonFrame->show();
		mSaveButtonFrame->repaint();
		mSaveButtonFrame->hide();

		mSaveButtonFramePixmap = QPixmap(mSaveButtonFrame->size());
		mSaveButtonFramePixmap.fill(Qt::transparent);
		mSaveButtonFrame->render(&mSaveButtonFramePixmap, QPoint(), QRegion(), QWidget::DrawChildren);
	}


	/*
	 * mToolTipOffset is here to compensate QToolTip offset so that the text
	 * inside the tooltip appears exactly over the thumbnail text.
	 * The offset values have been copied from QTipLabel code in qtooltip.cpp.
	 * Let's hope they do not change.
	 */
	void initToolTipOffset() {
		mToolTipOffset = QPoint(2,
			#ifdef Q_WS_WIN
				21
			#else
				16
			#endif
			);

		const int margin = 1 + mView->style()->pixelMetric(QStyle::PM_ToolTipLabelFrameWidth);
		mToolTipOffset += QPoint(margin, margin);
	}


	bool hoverEventFilter(QHoverEvent* event) {
		QModelIndex index = mView->indexAt(event->pos());
		if (mIndexUnderCursor.isValid()) {
			mView->update(mIndexUnderCursor);
		}
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
			mButtonFrame->adjustSize();
			mDelegate->updateButtonFrameOpacity();
			int posX = rect.x() + (rect.width() - mButtonFrame->width()) / 2;
			int posY = rect.y() + GADGET_MARGIN;
			mButtonFrame->move(posX, posY);
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

	QRect ratingRectFromIndexRect(const QRect& rect) const {
		return QRect(
			rect.left(),
			rect.bottom() - ratingRowHeight() - ITEM_MARGIN,
			rect.width(),
			ratingRowHeight());
	}

	int ratingFromCursorPosition(const QRect& ratingRect) const {
		const QPoint pos = mView->viewport()->mapFromGlobal(QCursor::pos());
		const int hoverRating = mRatingPainter.ratingFromPosition(ratingRect, pos);
		if (hoverRating == -1) {
			return -1;
		}

		return hoverRating & 1 ? hoverRating + 1 : hoverRating;
	}

	bool mouseReleaseEventFilter(QMouseEvent* event) {
	#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
		const QRect rect = ratingRectFromIndexRect(mView->visualRect(mIndexUnderCursor));
		const int rating = ratingFromCursorPosition(rect);
		if (rating == -1) {
			return false;
		}
		kDebug() << "Set rating to:" << rating;
		return true;
	#else
		return false;
	#endif
	}


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

		QPainterPath path = PaintUtils::roundedRectangle(rectF, SELECTION_RADIUS);
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
			QPixmap shadow = PaintUtils::generateFuzzyRect(size, color, SHADOW_SIZE);
			it = mShadowCache.insert(key, shadow);
		}
		painter->drawPixmap(rect.topLeft() + shadowOffset, it.value());
	}


	void drawText(QPainter* painter, const QRect& rect, const QColor& fgColor, const QString& fullText) const {
		QFontMetrics fm = mView->fontMetrics();

		// Elide text
		QString text;
		QMap<QString, QString>::const_iterator it = mElidedTextMap.find(fullText);
		if (it == mElidedTextMap.constEnd()) {
			text = fm.elidedText(fullText, Qt::ElideRight, rect.width() - 2*ITEM_MARGIN);
			mElidedTextMap[fullText] = text;
		} else {
			text = it.value();
		}

		// Compute x pos
		int posX;
		if (text.length() == fullText.length()) {
			// Not elided, center text
			posX = (rect.width() - fm.width(text)) / 2;
		} else {
			// Elided, left align
			posX = ITEM_MARGIN;
		}

		// Draw text
		painter->setPen(fgColor);
		painter->drawText(
			rect.left() + posX,
			rect.top() + ITEM_MARGIN + mThumbnailSize + ITEM_MARGIN + fm.ascent(),
			text);
	}


	void drawRating(QPainter* painter, const QRect& rect, const QVariant& value) {
	#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
		const int rating = value.toInt() * 2;
		const QRect ratingRect = ratingRectFromIndexRect(rect);
		const int hoverRating = ratingFromCursorPosition(ratingRect);
		mRatingPainter.paint(painter, ratingRect, rating, hoverRating);
	#endif
	}


	/**
	 * Show a tooltip only if the item has been elided.
	 * This function places the tooltip over the item text.
	 */
	void showToolTip(QHelpEvent* helpEvent) {
		QModelIndex index = mView->indexAt(helpEvent->pos());
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
			return;
		}

		// Compute tip position
		QRect rect = mView->visualRect(index);
		const int textX = ITEM_MARGIN;
		const int textY = ITEM_MARGIN + mThumbnailSize + ITEM_MARGIN;
		const QPoint tipPosition = rect.topLeft() + QPoint(textX, textY) - mToolTipOffset;

		// Compute visibility rect
		// We do not include the text line to avoid flicker:
		// When the mouse is over the tooltip, it's hidden, but the view then
		// receives a QHelpEvent which causes the tooltip to show again...
		QRect visibilityRect = rect;
		visibilityRect.setHeight(textY);

		// Show tip
		if (visibilityRect.contains(helpEvent->pos())) {
			QToolTip::showText(mView->mapToGlobal(tipPosition), fullText, mView, visibilityRect);
		}
	}

	int itemWidth() const {
		return mThumbnailSize + 2 * ITEM_MARGIN;
	}

	int ratingRowHeight() const {
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
		return mView->fontMetrics().height();
#endif
		return 0;
	}

	int itemHeight() const {
		return mThumbnailSize
			+ mView->fontMetrics().height()
			+ ratingRowHeight()
			+ 3*ITEM_MARGIN;
	}

	void selectIndexUnderCursorIfNoMultiSelection() {
		if (mView->selectionModel()->selectedIndexes().size() <= 1) {
			mView->setCurrentIndex(mIndexUnderCursor);
		}
	}
};


PreviewItemDelegate::PreviewItemDelegate(ThumbnailView* view)
: QAbstractItemDelegate(view)
, d(new PreviewItemDelegatePrivate) {
	d->mDelegate = this;
	d->mView = view;
	view->viewport()->installEventFilter(this);
	d->mThumbnailSize = view->thumbnailSize();

	d->mRatingPainter.setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
	d->mRatingPainter.setLayoutDirection(view->layoutDirection());

	connect(view, SIGNAL(thumbnailSizeChanged(int)),
		SLOT(setThumbnailSize(int)) );
	connect(view, SIGNAL(selectionChangedSignal(const QItemSelection&, const QItemSelection&)),
		SLOT(updateButtonFrameOpacity()) );

	QColor bgColor = d->mView->palette().highlight().color();
	QColor borderColor = bgColor.dark(SELECTION_BORDER_DARKNESS);

	QString styleSheet =
		"QFrame {"
		"	padding: 1px;"
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
	d->mButtonFrame = new GlossyFrame(d->mView->viewport());
	d->mButtonFrame->setStyleSheet(styleSheet);
	d->mButtonFrame->setBackgroundColor(bgColor);
	d->mButtonFrame->hide();

	QToolButton* fullScreenButton = createFrameButton(d->mButtonFrame, "view-fullscreen");
	connect(fullScreenButton, SIGNAL(clicked()),
		SLOT(slotFullScreenClicked()) );

	QToolButton* rotateLeftButton = createFrameButton(d->mButtonFrame, "object-rotate-left");
	connect(rotateLeftButton, SIGNAL(clicked()),
		SLOT(slotRotateLeftClicked()) );

	QToolButton* rotateRightButton = createFrameButton(d->mButtonFrame, "object-rotate-right");
	connect(rotateRightButton, SIGNAL(clicked()),
		SLOT(slotRotateRightClicked()) );

	QHBoxLayout* layout = new QHBoxLayout(d->mButtonFrame);
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addWidget(fullScreenButton);
	layout->addWidget(rotateLeftButton);
	layout->addWidget(rotateRightButton);

	// Save button frame
	d->mSaveButtonFrame = new GlossyFrame(d->mView->viewport());
	d->mSaveButtonFrame->setStyleSheet(styleSheet);
	d->mSaveButtonFrame->setBackgroundColor(bgColor);
	d->mSaveButtonFrame->hide();

	QToolButton* saveButton = createFrameButton(d->mSaveButtonFrame, "document-save");
	connect(saveButton, SIGNAL(clicked()),
		SLOT(slotSaveClicked()) );

	layout = new QHBoxLayout(d->mSaveButtonFrame);
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addWidget(saveButton);

	d->initSaveButtonFramePixmap();
	d->initToolTipOffset();
}


PreviewItemDelegate::~PreviewItemDelegate() {
	delete d;
}


QSize PreviewItemDelegate::sizeHint( const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/ ) const {
	return QSize( d->itemWidth(), d->itemHeight() );
}


bool PreviewItemDelegate::eventFilter(QObject*, QEvent* event) {
	switch (event->type()) {
	case QEvent::ToolTip:
		d->showToolTip(static_cast<QHelpEvent*>(event));
		return true;

	case QEvent::HoverMove:
		return d->hoverEventFilter(static_cast<QHoverEvent*>(event));

	case QEvent::MouseButtonRelease:
		return d->mouseReleaseEventFilter(static_cast<QMouseEvent*>(event));

	default:
		return false;
	}
}


void PreviewItemDelegate::paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const {
	int thumbnailSize = d->mThumbnailSize;
	QPixmap thumbnailPix = d->mView->thumbnailForIndex(index);
	const bool opaque = !thumbnailPix.hasAlphaChannel();
	const bool isDirOrArchive = ArchiveUtils::fileItemIsDirOrArchive(fileItemForIndex(index));
	QRect rect = option.rect;

#ifdef DEBUG_RECT
	painter->setPen(Qt::red);
	painter->setBrush(Qt::NoBrush);
	painter->drawRect(rect);
#endif

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
		QWidget* viewport = d->mView->viewport();
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
		d->drawBackground(painter, rect, bgColor, borderColor);
	}

	// Draw thumbnail
	if (!thumbnailPix.isNull()) {
		QRect thumbnailRect = QRect(
			rect.left() + (rect.width() - thumbnailPix.width())/2,
			rect.top() + (thumbnailSize - thumbnailPix.height())/2 + ITEM_MARGIN,
			thumbnailPix.width(),
			thumbnailPix.height());

		if (!(option.state & QStyle::State_Selected) && opaque) {
			d->drawShadow(painter, thumbnailRect);
		}

		if (opaque) {
			painter->setPen(borderColor);
			painter->setRenderHint(QPainter::Antialiasing, false);
			QRect borderRect = thumbnailRect.adjusted(-1, -1, 0, 0);
			painter->drawRect(borderRect);
		}
		painter->drawPixmap(thumbnailRect.left(), thumbnailRect.top(), thumbnailPix);
	}

	// Draw modified indicator
	bool isModified = d->mView->isModified(index);
	if (isModified) {
		// Draws a pixmap of the save button frame, as an indicator that
		// the image has been modified
		QPoint framePosition = d->saveButtonFramePosition(rect);
		painter->drawPixmap(framePosition, d->mSaveButtonFramePixmap);
	}

	if (index == d->mIndexUnderCursor) {
		if (isModified) {
			// If we just rotated the image with the buttons from the
			// button frame, we need to show the save button frame right now.
			d->showSaveButtonFrame(rect);
		} else {
			d->mSaveButtonFrame->hide();
		}
	}

	d->drawText(painter, rect, fgColor, index.data(Qt::DisplayRole).toString());

	if (!isDirOrArchive) {
		d->drawRating(painter, rect, index.data(SemanticInfoDirModel::RatingRole));
	}
}


void PreviewItemDelegate::updateButtonFrameOpacity() {
	bool isSelected = d->mView->selectionModel()->isSelected(d->mIndexUnderCursor);
	d->mButtonFrame->setOpaque(isSelected);
	d->mSaveButtonFrame->setOpaque(isSelected);
}


void PreviewItemDelegate::setThumbnailSize(int value) {
	d->mThumbnailSize = value;
	d->mElidedTextMap.clear();
}


void PreviewItemDelegate::slotSaveClicked() {
	saveDocumentRequested(urlForIndex(d->mIndexUnderCursor));
}


void PreviewItemDelegate::slotRotateLeftClicked() {
	d->selectIndexUnderCursorIfNoMultiSelection();
	rotateDocumentLeftRequested(urlForIndex(d->mIndexUnderCursor));
}


void PreviewItemDelegate::slotRotateRightClicked() {
	d->selectIndexUnderCursorIfNoMultiSelection();
	rotateDocumentRightRequested(urlForIndex(d->mIndexUnderCursor));
}


void PreviewItemDelegate::slotFullScreenClicked() {
	showDocumentInFullScreenRequested(urlForIndex(d->mIndexUnderCursor));
}


} // namespace
