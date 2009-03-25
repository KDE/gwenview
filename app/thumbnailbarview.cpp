// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>
Copyright 2008 Ilya Konkov <eruart@gmail.com>

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
#include "thumbnailbarview.moc"

// Qt
#include <QApplication>
#include <QHelpEvent>
#include <QScrollBar>
#include <QPainter>
#include <QTimeLine>
#include <QToolTip>
#include <QWindowsStyle>

// KDE

// Local
#include "lib/fullscreentheme.h"
#include "lib/paintutils.h"
#include "lib/thumbnailview/abstractthumbnailviewhelper.h"

namespace Gwenview {

/**
 * Duration in ms of the smooth scroll
 */
const int SMOOTH_SCROLL_DURATION = 250;

/**
 * Space between the item outer rect and the content, and between the
 * thumbnail and the caption
 */
const int ITEM_MARGIN = 5;

/** How dark is the shadow, 0 is invisible, 255 is as dark as possible */
const int SHADOW_STRENGTH = 127;

/** How many pixels around the thumbnail are shadowed */
const int SHADOW_SIZE = 4;


struct ThumbnailBarItemDelegatePrivate {
	// Key is height * 1000 + width
	typedef QMap<int, QPixmap> ShadowCache;
	mutable ShadowCache mShadowCache;

	ThumbnailBarItemDelegate* mDelegate;
	ThumbnailView* mView;
	QColor borderColor;

	void showToolTip(QHelpEvent* helpEvent) {
		QModelIndex index = mView->indexAt(helpEvent->pos());
		if (!index.isValid()) {
			return;
		}
		QString fullText = index.data().toString();
		QRect rect = mView->visualRect(index);
		QPoint pos = QCursor::pos();
		QToolTip::showText(pos, fullText, mView);
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
};


ThumbnailBarItemDelegate::ThumbnailBarItemDelegate(ThumbnailView* view)
: QAbstractItemDelegate(view)
, d(new ThumbnailBarItemDelegatePrivate) {
	d->mDelegate = this;
	d->mView = view;
	view->viewport()->installEventFilter(this);

	d->borderColor = PaintUtils::alphaAdjustedF(QColor(Qt::white), 0.65);
}


QSize ThumbnailBarItemDelegate::sizeHint( const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const {
	return d->mView->gridSize();
}


bool ThumbnailBarItemDelegate::eventFilter(QObject*, QEvent* event) {
	if (event->type() == QEvent::ToolTip) {
		QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
		d->showToolTip(helpEvent);
		return true;

	} else if (event->type() == QEvent::Wheel) {
		// FIXME
		QScrollBar* hsb = d->mView->verticalScrollBar();
		QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
		hsb->setValue(hsb->value() - wheelEvent->delta());
		return true;
	}

	return false;
}


void ThumbnailBarItemDelegate::paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const {
	QPixmap thumbnailPix = d->mView->thumbnailForIndex(index);
	QRect rect = option.rect;

	QStyleOptionViewItemV4 opt = option;
	const QWidget* widget = opt.widget;
	QStyle* style = widget ? widget->style() : QApplication::style();
	style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

	// Draw thumbnail
	if (!thumbnailPix.isNull()) {
		QRect thumbnailRect = QRect(
			rect.left() + (rect.width() - thumbnailPix.width())/2,
			rect.top() + (rect.height() - thumbnailPix.height())/2 - 1,
		    thumbnailPix.width(),
		    thumbnailPix.height());

		if (!thumbnailPix.hasAlphaChannel()) {
			d->drawShadow(painter, thumbnailRect);
			painter->setPen(d->borderColor);
			painter->setRenderHint(QPainter::Antialiasing, false);
			QRect borderRect = thumbnailRect.adjusted(-1, -1, 0, 0);
			painter->drawRect(borderRect);
		}
		painter->drawPixmap(thumbnailRect.left(), thumbnailRect.top(), thumbnailPix);
	}
}


ThumbnailBarItemDelegate::~ThumbnailBarItemDelegate() {
	delete d;
}


/**
 * This proxy style makes it possible to override the value returned by
 * styleHint() which leads to not-so-nice results with some styles.
 */
class ProxyStyle : public QWindowsStyle {
public:
	ProxyStyle(QStyle* baseStyle) : QWindowsStyle() {
		mBaseStyle = baseStyle;
	}

	void drawPrimitive(PrimitiveElement pe, const QStyleOption *opt, QPainter *p, const QWidget *w = 0) const {
		mBaseStyle->drawPrimitive(pe, opt, p, w);
	}

	void drawControl(ControlElement element, const QStyleOption *opt, QPainter *p, const QWidget *w = 0) const {
		mBaseStyle->drawControl(element, opt, p, w);
	}

	void drawComplexControl(ComplexControl cc, const QStyleOptionComplex *opt, QPainter *p, const QWidget *w = 0) const {
		mBaseStyle->drawComplexControl(cc, opt, p, w);
	}

	int styleHint(StyleHint sh, const QStyleOption *opt = 0, const QWidget *w = 0, QStyleHintReturn *shret = 0) const {
		switch (sh) {
		case SH_ItemView_ShowDecorationSelected:
			return true;
		case SH_ScrollView_FrameOnlyAroundContents:
			return false;
		default:
			return QWindowsStyle::styleHint(sh, opt, w, shret);
		}
	}

	void polish(QApplication* application) {
		mBaseStyle->polish(application);
	}

	void polish(QPalette& palette) {
		mBaseStyle->polish(palette);
	}

	void polish(QWidget* widget) {
		mBaseStyle->polish(widget);
	}

	void unpolish(QWidget* widget) {
		mBaseStyle->unpolish(widget);
	}

	void unpolish(QApplication* application) {
		mBaseStyle->unpolish(application);
	}

	int pixelMetric(PixelMetric pm, const QStyleOption* opt, const QWidget* widget) const {
		switch (pm) {
		case PM_MaximumDragDistance:
			return -1;
		default:
			return QWindowsStyle::pixelMetric(pm, opt, widget);
		}
	}

private:
	QStyle* mBaseStyle;
};



typedef int (QSize::*QSizeDimension)() const;

struct ThumbnailBarViewPrivate {
	ThumbnailBarView* q;
	QStyle* mStyle;
	QTimeLine* mTimeLine;

	Qt::Orientation mOrientation;


	QScrollBar* scrollBar() const {
		return mOrientation == Qt::Horizontal ? q->horizontalScrollBar() : q->verticalScrollBar();
	}


	QSizeDimension mainDimension() const {
		return mOrientation == Qt::Horizontal ? &QSize::width : &QSize::height;
	}


	QSizeDimension oppositeDimension() const {
		return mOrientation == Qt::Horizontal ? &QSize::height : &QSize::width;
	}


	void smoothScrollTo(const QModelIndex& index) {
		if (!index.isValid()) {
			return;
		}

		const QRect rect = q->visualRect(index);

		int oldValue = scrollBar()->value();
		int newValue = scrollToValue(rect);
		if (mTimeLine->state() == QTimeLine::Running) {
			mTimeLine->stop();
		}
		mTimeLine->setFrameRange(oldValue, newValue);
		mTimeLine->start();
	}


	int scrollToValue(const QRect& rect) {
		// This code is a much simplified version of
		// QListViewPrivate::horizontalScrollToValue()
		const QRect area = q->viewport()->rect();
		int value = scrollBar()->value();

		if (mOrientation == Qt::Horizontal) {
			if (q->isRightToLeft()) {
				value += (area.width() - rect.width()) / 2 - rect.left();
			} else {
				value += rect.left() - (area.width()- rect.width()) / 2;
			}
		} else {
			value += rect.top() - (area.height() - rect.height()) / 2;
		}
		return value;
	}
};



ThumbnailBarView::ThumbnailBarView(QWidget* parent)
: ThumbnailView(parent)
, d(new ThumbnailBarViewPrivate) {
	d->q = this;
	d->mOrientation = Qt::Vertical;
	d->mTimeLine = new QTimeLine(SMOOTH_SCROLL_DURATION, this);
	connect(d->mTimeLine, SIGNAL(frameChanged(int)),
		d->scrollBar(), SLOT(setValue(int)) );

	setObjectName("thumbnailBarView");
	setUniformItemSizes(true);
	setWrapping(false);
	if (d->mOrientation == Qt::Vertical) {
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		setMaximumWidth(256);
		setFlow(TopToBottom);
	} else {
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		setMaximumHeight(256);
	}

	d->mStyle = new ProxyStyle(style());
	setStyle(d->mStyle);
}


ThumbnailBarView::~ThumbnailBarView() {
	delete d->mStyle;
}



void ThumbnailBarView::paintEvent(QPaintEvent* event) {
	ThumbnailView::paintEvent(event);

	if (!d->scrollBar()->maximum()) {
		// Thumbnails doesn't fully cover viewport, draw a shadow after last item.
		QPainter painter(viewport());
		QLinearGradient linearGradient;
		linearGradient.setColorAt(0, QColor(0, 0, 0, 127));
		linearGradient.setColorAt(1, QColor(0, 0, 0, 0));

		QModelIndex index = model()->index(model()->rowCount() - 1, 0);
		QRect rect = rectForIndex(index);
		const int gradientExtent = 5;
		linearGradient.setStart(rect.bottomRight());
		if (d->mOrientation == Qt::Horizontal) {
			linearGradient.setFinalStop(rect.bottomRight() + QPoint(gradientExtent, 0));
			painter.fillRect(rect.right(), 0, gradientExtent, rect.height(), linearGradient);
		} else {
			linearGradient.setFinalStop(rect.bottomRight() + QPoint(0, gradientExtent));
			painter.fillRect(0, rect.bottom(), rect.width(), gradientExtent, linearGradient);
		}
	}
}


void ThumbnailBarView::resizeEvent(QResizeEvent *event) {
	ThumbnailView::resizeEvent(event);
	QSizeDimension dimension = d->oppositeDimension();

	int gridSize = (size().*dimension)() - ((d->scrollBar()->sizeHint().*dimension)() + 2 * frameWidth());
	setGridSize(QSize(gridSize, gridSize));
	setThumbnailSize(gridSize - ITEM_MARGIN * 2);
}


void ThumbnailBarView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) {
	QListView::selectionChanged(selected, deselected);

	QModelIndexList list = selected.indexes();
	if (list.count() == 1 && isVisible()) {
		d->smoothScrollTo(list.first());
	}
}


} // namespace
