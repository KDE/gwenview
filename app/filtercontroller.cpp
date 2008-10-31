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
#include "filtercontroller.h"

// Qt
#include <QAction>
#include <QLineEdit>
#include <QToolButton>

// KDE
#include <kicon.h>
#include <klocale.h>

// Local
#include <lib/flowlayout.h>
#include <lib/semanticinfo/sorteddirmodel.h>

namespace Gwenview {


class FilterWidgetContainer : public QWidget {
public:
	void setFilterWidget(QWidget* widget) {
		QToolButton* closeButton = new QToolButton;
		closeButton->setIcon(KIcon("window-close"));
		closeButton->setAutoRaise(true);
		connect(closeButton, SIGNAL(clicked()), SLOT(deleteLater()));
		QHBoxLayout* layout = new QHBoxLayout(this);
		layout->setMargin(0);
		layout->addWidget(widget);
		layout->addWidget(closeButton);
	}
};

typedef QLineEdit TagFilter;
typedef QLineEdit NameFilter;
typedef QLineEdit RatingFilter;


struct FilterControllerPrivate {
	FilterController* that;
	QFrame* mFrame;
	SortedDirModel* mDirModel;
	QList<QAction*> mActionList;

	void addAction(const QString& text, const char* slot) {
		QAction* action = new QAction(text, that);
		QObject::connect(action, SIGNAL(triggered()), that, slot);
		mActionList << action;
	}

	void addFilter(QWidget* widget) {
		if (mFrame->isHidden()) {
			mFrame->show();
		}
		FilterWidgetContainer* container = new FilterWidgetContainer;
		container->setFilterWidget(widget);
		mFrame->layout()->addWidget(container);
	}
};


FilterController::FilterController(QFrame* frame, SortedDirModel* dirModel)
: QObject(frame)
, d(new FilterControllerPrivate) {
	d->that = this;
	d->mFrame = frame;
	d->mDirModel = dirModel;

	d->mFrame->hide();
	new FlowLayout(d->mFrame);

	d->addAction(i18nc("@action:inmenu", "Filter by Name"), SLOT(addFilterByName()));
	d->addAction(i18nc("@action:inmenu", "Filter by Rating"), SLOT(addFilterByRating()));
	d->addAction(i18nc("@action:inmenu", "Filter by Tag"), SLOT(addFilterByTag()));
}


FilterController::~FilterController() {
	delete d;
}


QList<QAction*> FilterController::actionList() const {
	return d->mActionList;
}


void FilterController::addFilterByName() {
	d->addFilter(new NameFilter);
}


void FilterController::addFilterByRating() {
	d->addFilter(new RatingFilter);
}


void FilterController::addFilterByTag() {
	d->addFilter(new TagFilter);
}


} // namespace
