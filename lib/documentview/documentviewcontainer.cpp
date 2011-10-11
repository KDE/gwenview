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
#include <QTimer>
#include <QWidget>

// libc
#include <qmath.h>

namespace Gwenview {

typedef QSet<DocumentView*> DocumentViewSet;

struct DocumentViewContainerPrivate {
	DocumentViewContainer* q;
	DocumentViewSet mViews;
	DocumentViewSet mAddedViews;
	DocumentViewSet mRemovedViews;
	QTimer* mLayoutUpdateTimer;

	void scheduleLayoutUpdate() {
		mLayoutUpdateTimer->start();
	}

	bool removeFromSet(DocumentView* view, DocumentViewSet* set) {
		DocumentViewSet::Iterator it = set->find(view);
		if (it == set->end()) {
			return false;
		}
		set->erase(it);
		mRemovedViews << *it;
		scheduleLayoutUpdate();
		return true;
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
	d->mAddedViews << view;
	view->setParent(this);
	connect(view, SIGNAL(animationFinished(DocumentView*)),
		SLOT(slotViewAnimationFinished(DocumentView*)));
	d->scheduleLayoutUpdate();
}


void DocumentViewContainer::removeView(DocumentView* view) {
	if (d->removeFromSet(view, &d->mViews)) {
		return;
	}
	d->removeFromSet(view, &d->mAddedViews);
}


void DocumentViewContainer::showEvent(QShowEvent* event) {
	QWidget::showEvent(event);
	updateLayout();
}


void DocumentViewContainer::resizeEvent(QResizeEvent* event) {
	QWidget::resizeEvent(event);
	updateLayout();
}

void DocumentViewContainer::updateLayout() {
	// Stop update timer: this is useful if updateLayout() is called directly
	// and not through scheduleLayoutUpdate()
	d->mLayoutUpdateTimer->stop();
	DocumentViewSet views = d->mViews | d->mAddedViews;

	if (!views.isEmpty()) {
		// Compute column count
		int colCount;
		switch (views.count()) {
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

		int rowCount = qCeil(views.count() / qreal(colCount));
		Q_ASSERT(rowCount > 0);
		int viewWidth = width() / colCount;
		int viewHeight = height() / rowCount;

		int col = 0;
		int row = 0;

		Q_FOREACH(DocumentView* view, views) {
			QRect rect;
			rect.setLeft(col * viewWidth);
			rect.setTop(row * viewHeight);
			rect.setWidth(viewWidth);
			rect.setHeight(viewHeight);

			if (d->mViews.contains(view)) {
				if (rect != view->geometry()) {
					if (d->mAddedViews.isEmpty() && d->mRemovedViews.isEmpty()) {
						// View moves because of a resize
						view->moveTo(rect);
					} else {
						// View moves because the number of views changed,
						// animate the change
						view->moveToAnimated(rect);
					}
				}
			} else {
				view->setGeometry(rect);
				view->fadeIn();
			}

			++col;
			if (col == colCount) {
				col = 0;
				++row;
			}
		}
	}

	Q_FOREACH(DocumentView* view, d->mRemovedViews) {
		view->fadeOut();
	}
}

void DocumentViewContainer::slotViewAnimationFinished(DocumentView* view) {
	if (d->mRemovedViews.contains(view)) {
		d->mRemovedViews.remove(view);
		delete view;
		return;
	}
	if (d->mAddedViews.contains(view)) {
		d->mAddedViews.remove(view);
		d->mViews.insert(view);
	}
}

} // namespace
