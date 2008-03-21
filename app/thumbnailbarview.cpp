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
#include "thumbnailbarview.moc"

// Qt
#include <QApplication>
#include <QHelpEvent>
#include <QScrollBar>
#include <QPainter>
#include <QWindowsStyle>
#include <QToolTip>
// KDE

// Local
#include "lib/paintutils.h"
#include "lib/thumbnailview/abstractthumbnailviewhelper.h"

namespace Gwenview {

/**
 * Space between the item outer rect and the content, and between the
 * thumbnail and the caption
 */
const int ITEM_MARGIN = 5;

/** How dark is the shadow, 0 is invisible, 255 is as dark as possible */
const int SHADOW_STRENGTH = 127;

/** How many pixels around the thumbnail are shadowed */
const int SHADOW_SIZE = 3;


static QString rgba(const QColor &color) {
	QString rgba("rgba(%1, %2, %3, %4)");
	return rgba.arg(QString::number(color.red()), QString::number(color.green()), QString::number(color.red()), QString::number(color.alpha()));
}


struct ThumbnailBarItemDelegatePrivate {
	// Key is height * 1000 + width
	typedef QMap<int, QPixmap> ShadowCache;
	mutable ShadowCache mShadowCache;

	ThumbnailBarItemDelegate* mDelegate;
	ThumbnailView* mView;
	int mThumbnailSize;
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
: d(new ThumbnailBarItemDelegatePrivate) {
	d->mDelegate = this;
	d->mView = view;
	view->viewport()->installEventFilter(this);
	setThumbnailSize(view->thumbnailSize());

	QWidget* viewport = view->viewport();
	QColor bgColor = viewport->palette().color(viewport->backgroundRole());

	if (bgColor.value() < 128) {
		d->borderColor = bgColor.darker(200);
	} else {
		d->borderColor = bgColor.lighter(200);
	}
	d->borderColor = PaintUtils::adjustedHsv(d->borderColor, 0, - d->borderColor.saturation(), 0);

	connect(view, SIGNAL(thumbnailSizeChanged(int)),
		SLOT(setThumbnailSize(int)) );
}


QSize ThumbnailBarItemDelegate::sizeHint( const QStyleOptionViewItem & /*option*/, const QModelIndex & /*index*/) const {
	return d->mView->gridSize();
}


void ThumbnailBarItemDelegate::setThumbnailSize(int value) {
	d->mThumbnailSize = value;
	d->mView->setGridSize(QSize(value + ITEM_MARGIN*2 + 1, value + ITEM_MARGIN*2 + 1));
}


bool ThumbnailBarItemDelegate::eventFilter(QObject*, QEvent* event) {
	if (event->type() == QEvent::ToolTip) {
		QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
		d->showToolTip(helpEvent);
		return true;

	} else if (event->type() == QEvent::Wheel) {
		QScrollBar* hsb = d->mView->horizontalScrollBar();
		QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
		hsb->setValue(hsb->value() - wheelEvent->delta());
		return true;
	}

	return false;
}


void ThumbnailBarItemDelegate::paint( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const {
	ThumbnailView::Thumbnail thumbnail = d->mView->thumbnailForIndex(index);
	QPixmap thumbnailPix = thumbnail.mPixmap;
	QSize thumbnailSize = d->mView->gridSize() - QSize(ITEM_MARGIN*2 + 1, ITEM_MARGIN*3 + 1);
	if (thumbnailPix.width() > thumbnailSize.width() || thumbnailPix.height() > thumbnailSize.height()) {
		thumbnailPix = thumbnailPix.scaled(thumbnailSize, Qt::KeepAspectRatio);
	}
	QRect rect = option.rect;

	QStyleOptionViewItemV4 opt = option;
	const QWidget* widget = opt.widget;
	QStyle* style = widget ? widget->style() : QApplication::style();
	style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);

	// Draw thumbnail
	if (!thumbnailPix.isNull()) {
		QRect thumbnailRect = QRect(
			rect.left() + (rect.width() - thumbnailPix.width())/2,
			rect.top() + (rect.height() - thumbnailPix.height())/2 ,
		    thumbnailPix.width(),
		    thumbnailPix.height());

		if (/*!(option.state & QStyle::State_Selected) && */ thumbnail.mOpaque) {
			d->drawShadow(painter, thumbnailRect);
		}

		if (thumbnail.mOpaque) {
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


ThumbnailBarView::ThumbnailBarView(QWidget* parent)
: ThumbnailView(parent) {
	setObjectName("thumbnailBarView");
	setUniformItemSizes(true);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	setWrapping(false);
	setThumbnailSize(80);

	QStyle* defaultStyle = style();
	setStyle(mStyle = new QWindowsStyle);
	setStyleSheet(defaultStyleSheet());
	horizontalScrollBar()->setStyle(defaultStyle);

	// Since QWindowsStyle is used context menu won't be painted by system style. Avoid it by this hack:
	ThumbnailView::disconnect(SIGNAL(customContextMenuRequested(const QPoint&)));
	connect(this, SIGNAL(customContextMenuRequested(const QPoint&)),
	    SLOT(showContextMenu()) );
}


ThumbnailBarView::~ThumbnailBarView() {
	delete mStyle;
}


void ThumbnailBarView::showContextMenu() {
	// Using parentWidget as parent of menu to paint it by system style.
	thumbnailViewHelper()->showContextMenu(parentWidget());
}


QSize ThumbnailBarView::sizeHint() const {
	QSize hint = ThumbnailView::sizeHint();
	int size = gridSize().height();
	int hsbExt = horizontalScrollBar()->sizeHint().height();
	int f = 2 * frameWidth();
	hint.setHeight(size + f + hsbExt);
	return hint;
}


QString ThumbnailBarView::defaultStyleSheet() const {
	QColor bgColor, bgSelColor;
	bgSelColor = viewport()->palette().color(QPalette::Normal, QPalette::Highlight);
	bgColor = viewport()->palette().color(viewport()->backgroundRole());

	// Avoid dark and bright colors
	bgColor.setHsv(bgColor.hue(), bgColor.saturation(), (128 * 2 + bgColor.value()) / 3);
	bgSelColor.setHsv(bgSelColor.hue(), bgSelColor.saturation(), (128 * 2 + bgSelColor.value()) / 3);


	QString itemCss =
		"QListView::item {"
		"	background-color:"
		"		qlineargradient(x1:0, y1:0, x2:0, y2:1,"
		"		stop: 0 black, stop: 0.05 %1, stop:1 %2);"
		"	border-left: 1px solid "
		"		qlineargradient(x1:0, y1:0, x2:0, y2:1,"
		"		stop: 0 black, stop: 0.05 %3, stop:1 %4);"
		"	border-right: 1px solid "
		"		qlineargradient(x1:0, y1:0, x2:0, y2:1,"
		"		stop: 0 black, stop: 0.05 %4, stop:1 %5);"
		"}";
	itemCss = itemCss.arg(
		bgColor.lighter(125).name(),
		bgColor.darker(125).name(),
		bgColor.lighter(170).name(),
		bgColor.name(),
		bgColor.darker(170).name());

	QString viewCss =
		"#thumbnailBarView {"
		"	background-color:"
		"		qlineargradient(x1:0, y1:0, x2:0, y2:1,"
		"		stop: 0 black, stop: 0.05 %1, stop:1 %2);"
		"	border: 1px solid #444;"
		"}";
	viewCss = viewCss.arg(bgColor.darker(125).name(), bgColor.name());

	QColor borderSelColor(0, 0, 0, 127);
	QString itemSelCss =
		"QListView::item:selected {"
		"	background-color:"
		"		qlineargradient(x1:0, y1:0, x2:0, y2:1,"
		"		stop: 0 black, stop: 0.05 %1, stop:1 %2);"
		"	border-left: 1px solid %3;"
		"	border-right: 1px solid %3;"
		"}";
	itemSelCss = itemSelCss.arg(
	    bgSelColor.lighter(125).name(),
	    bgSelColor.darker(125).name(),
	    rgba(borderSelColor));

	QString scrollbarCss =
		"QScrollBar:horizontal {"
		"	background-color:"
		"		qlineargradient(x1:0, y1:0, x2:0, y2:1,"
		"		stop: 0 black, stop: 0.4 %1, stop: 1 %2);"
		"	height: 8px;"
		"	border-top: 1px solid black;"
		"}";
	scrollbarCss = scrollbarCss.arg(bgColor.darker(200).name(), bgColor.darker(175).name());

	QString handle =
		"background:"
		"	qlineargradient(x1:0, y1:0, x2:0, y2:1,"
		"	stop: 0 %1, stop: 0.17 %2,"
		"	stop: 0.2 %3, stop: 0.8 %4,"
		"	stop: 0.83 %2, stop: 1 %1);"
		"border: 1px solid"
		"	qlineargradient(x1:0, y1:0, x2:0, y2:1,"
		"	stop: 0 #242424, stop: 0.2 %2,"
		"	stop: 0.70 %2, stop: 1 #242424);"
		"border-top: 0px;"
		"border-bottom: 0px;"
		"}";

	QColor handleBase = QColor::fromHsv(bgColor.hue(), bgColor.saturation() / 2, (bgColor.value() + 255) / 2);
	QColor handleSelBase = handleBase.lighter(110);
	QString handleCss =
		"QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal, QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {"
		"	width: 0;"
		"	border: 0;"
		"}"
		"QScrollBar::handle:horizontal {"
		"	min-width: 20px;" +
		handle.arg(
			handleBase.darker(170).name(),
			handleBase.darker(125).name(),
			handleBase.darker(140).name(),
			handleBase.lighter(115).name()) +
		"QScrollBar::handle:hover {" +
		handle.arg(
			handleSelBase.darker(170).name(),
			handleSelBase.darker(125).name(),
			handleSelBase.darker(140).name(),
			handleSelBase.lighter(115).name());

	return viewCss + itemCss + itemSelCss + scrollbarCss + handleCss;
}


} // namespace
