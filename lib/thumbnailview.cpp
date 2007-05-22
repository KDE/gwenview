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

#include <QtGui/QHelpEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QToolTip>

// KDE
#include <kdirmodel.h>

// Local
#include "abstractthumbnailviewhelper.h"

namespace Gwenview {

/** Space between the item outer rect and the content */
const int ITEM_MARGIN = 11;

const int DEFAULT_COLUMN_COUNT = 4;
const int DEFAULT_ROW_COUNT = 3;

const int SELECTION_CORNER_RADIUS = 10;
const int SELECTION_BORDER_SIZE = 2;
const int SELECTION_ALPHA1 = 32;
const int SELECTION_ALPHA2 = 128;


static QPainterPath createRoundRectPainterPath(const QSize& size, int radius) {
	int width = size.width();
	int height = size.height();
	QRect rect(0, 0, width, height);

	QPainterPath roundRectPath;
	roundRectPath.moveTo(radius, 0);
	roundRectPath.arcTo(0, 0, radius, radius, 90.0, 90.0);

	roundRectPath.lineTo(0, height - radius - 1);

	roundRectPath.arcTo(0, height - radius - 1, radius, radius, 180.0, 90.0);

	roundRectPath.lineTo(width - radius - 1, height - 1);

	roundRectPath.arcTo(width - radius - 1, height - radius -1 , radius, radius, 270.0, 90.0);

	roundRectPath.lineTo(width - 1, radius);

	roundRectPath.arcTo(width - radius - 1, 0, radius, radius, 0.0, 90.0);

	roundRectPath.closeSubpath();

	return roundRectPath;
}


/**
 * An ItemDelegate which generates thumbnails for images. It also makes sure
 * all items are of the same size.
 */
class PreviewItemDelegate : public QAbstractItemDelegate {
public:
	PreviewItemDelegate(ThumbnailView* view)
	: QAbstractItemDelegate(view)
	, mView(view)
	{}


	virtual QSize sizeHint( const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/ ) const {
		return QSize( mView->itemWidth(), mView->itemHeight() );
	}


	virtual bool eventFilter(QObject* object, QEvent* event) {
		if (event->type() == QEvent::ToolTip) {
			QAbstractItemView* view = static_cast<QAbstractItemView*>(object->parent());
			QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
			showToolTip(view, helpEvent);
			return true;
		}
		return false;
	}


	virtual void paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const {
		QVariant value = index.data(Qt::DecorationRole);
		QIcon icon = qvariant_cast<QIcon>(value);
		QPixmap thumbnail = icon.pixmap(mView->thumbnailSize(), mView->thumbnailSize());
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

		// Draw selection
		if (option.state & QStyle::State_Selected) {
			int width = mView->itemWidth();
			QRect selectionRect(
				rect.left() + (rect.width() - width) / 2,
				rect.top(),
				width,
				rect.height());
			QColor borderColor = option.palette.color(cg, QPalette::Highlight);
			QColor fillColor = borderColor;
			fillColor.setAlpha(SELECTION_ALPHA1);
			drawSelectionRect(painter, selectionRect, borderColor, fillColor);
		}

		// Draw thumbnail
		painter->drawPixmap(
			rect.left() + (rect.width() - thumbnail.width())/2,
			rect.top() + (mView->thumbnailSize() - thumbnail.height())/2 + ITEM_MARGIN,
			thumbnail);

		// Draw text
		painter->setPen(option.palette.color(cg, QPalette::Text));

		painter->drawText(
			rect.left() + (rect.width() - textWidth) / 2,
			rect.top() + ITEM_MARGIN + mView->thumbnailSize() + ITEM_MARGIN + option.fontMetrics.ascent(),
			text);
	}


private:
	void drawSelectionRect(QPainter * painter, QRect rect, const QColor & borderColor, const QColor & fillColor) const {

		painter->setRenderHint(QPainter::Antialiasing);

		rect.adjust(SELECTION_BORDER_SIZE, SELECTION_BORDER_SIZE, 0, 0);

		QPainterPath roundRectPath = createRoundRectPainterPath(rect.size(), SELECTION_CORNER_RADIUS);

		QLinearGradient gradient(0, 0, 0, rect.height());
		gradient.setColorAt(0.0, fillColor);
		QColor fillColor2 = fillColor;
		fillColor2.setAlpha(SELECTION_ALPHA2);
		gradient.setColorAt(1.0, fillColor2);
		painter->setBrush(gradient);
		painter->setPen(QPen(borderColor, SELECTION_BORDER_SIZE));

		painter->translate(rect.left(), rect.top());
		painter->drawPath(roundRectPath);
		painter->translate(-rect.left(), -rect.top());
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
			fullText = QString::null;
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
};


struct ThumbnailViewPrivate {
	int mThumbnailSize;
	AbstractThumbnailViewHelper* mThumbnailViewHelper;
};


ThumbnailView::ThumbnailView(QWidget* parent)
: QListView(parent)
, d(new ThumbnailViewPrivate) {
	setViewMode(QListView::IconMode);
	setResizeMode(QListView::Adjust);
	setMovement(QListView::Static);

	PreviewItemDelegate* delegate = new PreviewItemDelegate(this);
	setItemDelegate(delegate);
	viewport()->installEventFilter(delegate);

	setVerticalScrollMode(ScrollPerPixel);
	setHorizontalScrollMode(ScrollPerPixel);

	d->mThumbnailViewHelper = 0;
	setThumbnailSize(128);
}


ThumbnailView::~ThumbnailView() {
	delete d;
}


void ThumbnailView::setThumbnailSize(int value) {
	d->mThumbnailSize = value;
}

int ThumbnailView::thumbnailSize() const {
	return d->mThumbnailSize;
}

int ThumbnailView::itemWidth() const {
	return int(d->mThumbnailSize * 1.4);
}

int ThumbnailView::itemHeight() const {
	return d->mThumbnailSize + fontMetrics().height() + 3*ITEM_MARGIN;
}

void ThumbnailView::setThumbnailViewHelper(AbstractThumbnailViewHelper* helper) {
	d->mThumbnailViewHelper = helper;
}

void ThumbnailView::rowsInserted(const QModelIndex& parent, int start, int end) {
	QListView::rowsInserted(parent, start, end);

	QList<KFileItem> itemsToPreview;
	for (int pos=start; pos<=end; ++pos) {
		QModelIndex index = model()->index(pos, 0, parent);
		QVariant data = index.data(KDirModel::FileItemRole);
		KFileItem item = qvariant_cast<KFileItem>(data);
		itemsToPreview.append(item);
	}

	Q_ASSERT(d->mThumbnailViewHelper);
	d->mThumbnailViewHelper->generateThumbnailsForItems(itemsToPreview, d->mThumbnailSize);
}

} // namespace
