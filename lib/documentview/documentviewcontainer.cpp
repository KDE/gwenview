// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2011 Aurélien Gâteau <agateau@kde.org>

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
#include "documentviewcontainer.moc"

// Local
#include <lib/documentview/documentview.h>

// KDE
#include <kdebug.h>
#include <kurl.h>

// Qt
#include <QEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QTimer>
#include <QWidget>

// libc
#include <qmath.h>

namespace Gwenview {

//// ViewItem /////
ViewItem::ViewItem(DocumentView* view, DocumentViewContainer* container)
: QObject(container)
, mView(view)
, mContainer(container)
{}

ViewItem::~ViewItem() {
}

void ViewItem::moveTo(const QRect& rect) {
	QPropertyAnimation* anim = new QPropertyAnimation(mView, "geometry");
	anim->setStartValue(mView->geometry());
	anim->setEndValue(rect);
	animate(anim);
}

void ViewItem::fadeIn() {
	mView->setOpacity(0);
	mView->show();
	QPropertyAnimation* anim = new QPropertyAnimation(mView, "opacity");
	anim->setStartValue(0.);
	anim->setEndValue(1.);
	animate(anim);
}

void ViewItem::fadeOut() {
	QPropertyAnimation* anim = new QPropertyAnimation(mView, "opacity");
	anim->setStartValue(1.);
	anim->setEndValue(0.);
	animate(anim);
}

void ViewItem::animate(QPropertyAnimation* anim) {
	connect(anim, SIGNAL(finished()),
		SLOT(slotAnimationFinished()));
	anim->setDuration(500);
	anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void ViewItem::slotAnimationFinished() {
	mContainer->onItemAnimationFinished(this);
}

typedef QSet<ViewItem*> ViewItemSet;

//// DocumentViewContainer ////
struct DocumentViewContainerPrivate {
	DocumentViewContainer* q;
	ViewItemSet mViewItems;
	ViewItemSet mAddedViewItems;
	ViewItemSet mRemovedViewItems;
	QTimer* mLayoutUpdateTimer;

	void scheduleLayoutUpdate() {
		mLayoutUpdateTimer->start();
	}

	bool removeFromSet(DocumentView* view, ViewItemSet* set) {
		ViewItemSet::Iterator it = set->begin(), end = set->end();
		for (; it != end; ++it) {
			if ((*it)->view() == view) {
				set->erase(it);
				mRemovedViewItems << *it;
				scheduleLayoutUpdate();
				return true;
			}
		}
		return false;
	}
};


DocumentViewContainer::DocumentViewContainer(QWidget* parent)
: QWidget(parent)
, d(new DocumentViewContainerPrivate) {
	d->q = this;
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	d->mLayoutUpdateTimer = new QTimer(this);
	d->mLayoutUpdateTimer->setInterval(0);
	d->mLayoutUpdateTimer->setSingleShot(true);
	connect(d->mLayoutUpdateTimer, SIGNAL(timeout()), SLOT(updateLayout()));
}


DocumentViewContainer::~DocumentViewContainer() {
	delete d;
}


void DocumentViewContainer::addView(DocumentView* view) {
	d->mAddedViewItems << new ViewItem(view, this);
	view->setParent(this);
	d->scheduleLayoutUpdate();
}


void DocumentViewContainer::removeView(DocumentView* view) {
	if (d->removeFromSet(view, &d->mViewItems)) {
		return;
	}
	if (d->removeFromSet(view, &d->mAddedViewItems)) {
		return;
	}
}


void DocumentViewContainer::showEvent(QShowEvent* event) {
	QWidget::showEvent(event);
	d->scheduleLayoutUpdate();
}


void DocumentViewContainer::resizeEvent(QResizeEvent* event) {
	QWidget::resizeEvent(event);
	d->scheduleLayoutUpdate();
}

void DocumentViewContainer::updateLayout() {
	ViewItemSet items = d->mViewItems | d->mAddedViewItems;

	if (!items.isEmpty()) {
		// Compute column count
		int colCount;
		switch (items.count()) {
		case 1:
			colCount = 1;
			break;
		case 2:
			colCount = 2;
			break;
		case 3:
			colCount = 3;
			break;
		case 4:
			colCount = 2;
			break;
		case 5:
			colCount = 3;
			break;
		case 6:
			colCount = 3;
			break;
		default:
			colCount = 3;
			break;
		}

		int rowCount = qCeil(items.count() / qreal(colCount));
		Q_ASSERT(rowCount > 0);
		int viewWidth = width() / colCount;
		int viewHeight = height() / rowCount;

		int col = 0;
		int row = 0;

		Q_FOREACH(ViewItem* item, items) {
			QRect rect;
			rect.setLeft(col * viewWidth);
			rect.setTop(row * viewHeight);
			rect.setWidth(viewWidth);
			rect.setHeight(viewHeight);

			if (d->mViewItems.contains(item)) {
				if (rect != item->view()->geometry()) {
					item->moveTo(rect);
				}
			} else {
				item->view()->setGeometry(rect);
				item->fadeIn();;
			}

			++col;
			if (col == colCount) {
				col = 0;
				++row;
			}
		}
	}

	Q_FOREACH(ViewItem* item, d->mRemovedViewItems) {
		item->fadeOut();
	}
}

void DocumentViewContainer::onItemAnimationFinished(ViewItem* item) {
	if (d->mRemovedViewItems.contains(item)) {
		d->mRemovedViewItems.remove(item);
		delete item->view();
		delete item;
		return;
	}
	if (d->mAddedViewItems.contains(item)) {
		d->mAddedViewItems.remove(item);
		d->mViewItems.insert(item);
	}
}

} // namespace
