// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "splittercollapser.h"

// Qt
#include <QEvent>
#include <QSplitter>
#include <QStyleOptionToolButton>
#include <QStylePainter>

// KDE

// Local

namespace Gwenview {


enum Direction {
	LTR      = 1 << 0,
	RTL      = 1 << 1,
	Vertical = 1 << 2,
	TTB      = Vertical + (1 << 0),
	BTT      = Vertical + (1 << 1)
};


struct ArrowTypes {
	ArrowTypes() {}

	ArrowTypes(Qt::ArrowType t1, Qt::ArrowType t2)
	: visible(t1), notVisible(t2) {}

	Qt::ArrowType visible,
	notVisible;

	Qt::ArrowType get(bool isVisible) {
		return isVisible ? visible : notVisible;
	}
};


struct SplitterCollapserPrivate {
	SplitterCollapser* q;
	QSplitter* mSplitter;
	QWidget* mWidget;
	Direction mDirection;

	bool isVertical() const {
		return mDirection & Vertical;
	}

	void updatePosition() {
		int x, y;
		QRect widgetRect = mWidget->geometry();
		int splitterWidth = mSplitter->width();
		int handleWidth = mSplitter->handleWidth();
		int width = q->width();

		if (!isVertical()) {
			y = (mSplitter->height() - q->height()) / 2;
			if (mDirection == LTR) {
				if (mWidget->isVisible()) {
					x = widgetRect.right() + handleWidth + 1;
				} else {
					x = 0;
				}
			} else { // RTL
				if (mWidget->isVisible()) {
					x = widgetRect.left() - handleWidth - width;
				} else {
					x = splitterWidth - handleWidth - width;
				}
			}
		} else {
			// FIXME
			x = 0;
			y = 0;
		}
		q->move(x, y);
	}


	void updateArrow() {
		static QMap<Direction, ArrowTypes> arrowForDirection;
		if (arrowForDirection.isEmpty()) {
			arrowForDirection[LTR] = ArrowTypes(Qt::LeftArrow,  Qt::RightArrow);
			arrowForDirection[RTL] = ArrowTypes(Qt::RightArrow, Qt::LeftArrow);
			arrowForDirection[TTB] = ArrowTypes(Qt::UpArrow,    Qt::DownArrow);
			arrowForDirection[BTT] = ArrowTypes(Qt::DownArrow,  Qt::UpArrow);
		}
		q->setArrowType(arrowForDirection[mDirection].get(mWidget->isVisible()));
	}
};


SplitterCollapser::SplitterCollapser(QSplitter* splitter, QWidget* widget)
: QToolButton()
, d(new SplitterCollapserPrivate) {
	d->q = this;

	// We do not want our collapser to be added as a regular widget in the
	// splitter!
	setAttribute(Qt::WA_NoChildEventsForParent);

	d->mWidget = widget;
	d->mWidget->installEventFilter(this);

	d->mSplitter = splitter;
	setParent(d->mSplitter);

	if (splitter->indexOf(widget) < splitter->count() / 2) {
		d->mDirection = LTR;
	} else {
		d->mDirection = RTL;
	}
	if (splitter->orientation() == Qt::Vertical) {
		// FIXME: Ugly!
		d->mDirection = static_cast<Direction>(int(d->mDirection) + int(TTB));
	}

	connect(this, SIGNAL(clicked()), SLOT(slotClicked()));

	show();
}


SplitterCollapser::~SplitterCollapser() {
	delete d;
}


bool SplitterCollapser::eventFilter(QObject*, QEvent* event) {
	switch (event->type()) {
	case QEvent::Resize:
		d->updatePosition();
		break;

	case QEvent::Show:
	case QEvent::Hide:
		d->updatePosition();
		d->updateArrow();
		break;

	default:
		break;
	}
	return false;
}


QSize SplitterCollapser::sizeHint() const {
	int extent = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
	QSize sh(extent, extent * 3 / 2);
	if (d->isVertical()) {
		sh.transpose();
	}
	return sh;
}


void SplitterCollapser::slotClicked() {
	d->mWidget->setVisible(!d->mWidget->isVisible());
}


void SplitterCollapser::paintEvent(QPaintEvent*) {
	QStylePainter painter(this);

	painter.setOpacity(0.3);

	QStyleOptionToolButton opt;
	initStyleOption(&opt);
	if (d->mDirection == LTR) {
		opt.rect.setLeft(-width());
	} else {
		opt.rect.setWidth(width() * 2);
	}
	painter.drawPrimitive(QStyle::PE_PanelButtonTool, opt);

	QStyleOptionToolButton opt2;
	initStyleOption(&opt2);
	painter.drawControl(QStyle::CE_ToolButtonLabel, opt2);
}


} // namespace
