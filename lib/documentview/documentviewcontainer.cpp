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
#include <QPointer>
#include <QPropertyAnimation>
#include <QTimer>
#include <QWidget>

// libc
#include <qmath.h>

namespace Gwenview {

//// ViewItem /////
class ViewItem {
public:
	ViewItem(DocumentView* view, DocumentViewContainer* container)
	: mView(view)
	, mPlaceholder(0)
	, mContainer(container)
	{}

	~ViewItem() {
		delete mPlaceholder;
	}

	Placeholder* createPlaceholder() {
		if (mPlaceholder) {
			delete mPlaceholder;
		}
		mPlaceholder = new Placeholder(this, mContainer);
		QObject::connect(mPlaceholder, SIGNAL(animationFinished(ViewItem*)),
			mContainer, SLOT(slotItemAnimationFinished(ViewItem*))
		);
		return mPlaceholder;
	}

	DocumentView* view() const {
		return mView;
	}

	Placeholder* placeholder() const {
		return mPlaceholder;
	}

private:
	Q_DISABLE_COPY(ViewItem)

	DocumentView* mView;
	QPointer<Placeholder> mPlaceholder;
	DocumentViewContainer* mContainer;
};

typedef QSet<ViewItem*> ViewItemSet;

//// Placeholder ////
Placeholder::Placeholder(ViewItem* item, QWidget* parent)
: QWidget(parent)
, mViewItem(item)
, mOpacity(1)
{
	setAttribute(Qt::WA_NoSystemBackground);
	setGeometry(item->view()->geometry());
	mPixmap = QPixmap::grabWidget(item->view());
}

void Placeholder::animate(QPropertyAnimation* anim) {
	connect(anim, SIGNAL(finished()),
		SLOT(emitAnimationFinished()));
	anim->setDuration(500);
	anim->start(QAbstractAnimation::DeleteWhenStopped);
	mViewItem->view()->hide();
	raise();
	show();
}

void Placeholder::paintEvent(QPaintEvent*) {
	QPainter painter(this);
	painter.setOpacity(mOpacity);
	painter.setCompositionMode(QPainter::CompositionMode_Source);
	QPixmap pix = mPixmap.scaled(size(), Qt::KeepAspectRatio);
	painter.drawPixmap((width() - pix.width()) / 2, (height() - pix.height())/ 2, pix);
}

void Placeholder::emitAnimationFinished() {
	animationFinished(mViewItem);
}

qreal Placeholder::opacity() const {
	return mOpacity;
}

void Placeholder::setOpacity(qreal opacity) {
	mOpacity = opacity;
	update();
}

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

	void moveView(ViewItem* item, const QRect& rect) {
		Placeholder* holder = item->createPlaceholder();
		QPropertyAnimation* anim = new QPropertyAnimation(holder, "geometry");
		anim->setStartValue(holder->geometry());
		anim->setEndValue(rect);
		holder->animate(anim);
	}

	void fadeInView(ViewItem* item) {
		Placeholder* holder = item->createPlaceholder();
		holder->setOpacity(0);
		QPropertyAnimation* anim = new QPropertyAnimation(holder, "opacity");
		anim->setStartValue(0.);
		anim->setEndValue(1.);
		holder->animate(anim);
	}

	void fadeOutView(ViewItem* item) {
		Placeholder* holder = item->createPlaceholder();
		QPropertyAnimation* anim = new QPropertyAnimation(holder, "opacity");
		anim->setStartValue(1.);
		anim->setEndValue(0.);
		holder->animate(anim);
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
					d->moveView(item, rect);
				}
			} else {
				item->view()->setGeometry(rect);
				d->fadeInView(item);
			}

			++col;
			if (col == colCount) {
				col = 0;
				++row;
			}
		}
	}

	Q_FOREACH(ViewItem* item, d->mRemovedViewItems) {
		d->fadeOutView(item);
	}
}

void DocumentViewContainer::slotItemAnimationFinished(ViewItem* item) {
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
	item->view()->setGeometry(item->placeholder()->geometry());
	item->view()->show();
	delete item->placeholder();
}

} // namespace
