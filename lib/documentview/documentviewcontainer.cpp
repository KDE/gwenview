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

// Qt
#include <QEvent>
#include <QWidget>

// libc
#include <qmath.h>

namespace Gwenview {

struct DocumentViewContainerPrivate {
	DocumentViewContainer* q;
	QList<DocumentView*> mItems;
};


DocumentViewContainer::DocumentViewContainer(QWidget* parent)
: QWidget(parent)
, d(new DocumentViewContainerPrivate) {
	d->q = this;
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}


DocumentViewContainer::~DocumentViewContainer() {
	delete d;
}


void DocumentViewContainer::addView(DocumentView* view) {
	d->mItems << view;
	view->setParent(this);
	view->installEventFilter(this);
	updateLayout();
}


bool DocumentViewContainer::eventFilter(QObject*, QEvent* event) {
	switch (event->type()) {
	case QEvent::Show:
	case QEvent::Hide:
		updateLayout();
		break;
	default:
		break;
	}
	return false;
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
	// List visible views
	QList<DocumentView*> visibleViews;
	Q_FOREACH(DocumentView* view, d->mItems) {
		if (view->isVisible()) {
			visibleViews << view;
		}
	}
	if (visibleViews.isEmpty()) {
		return;
	}

	// Compute column count
	int colCount;
	switch (visibleViews.count()) {
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

	int rowCount = qCeil(visibleViews.count() / qreal(colCount));
	Q_ASSERT(rowCount > 0);
	int viewWidth = width() / colCount;
	int viewHeight = height() / rowCount;

	int col = 0;
	int row = 0;

	Q_FOREACH(DocumentView* view, visibleViews) {
		QRect rect;
		rect.setLeft(col * viewWidth);
		rect.setTop(row * viewHeight);
		rect.setWidth(viewWidth);
		rect.setHeight(viewHeight);

		view->setGeometry(rect);

		++col;
		if (col == colCount) {
			col = 0;
			++row;
		}
	}
}

} // namespace
