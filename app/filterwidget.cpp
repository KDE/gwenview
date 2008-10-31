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
#include "filterwidget.moc"

#include <config-gwenview.h>

// Qt
#include <QHBoxLayout>
#include <QListView>
#include <QPointer>
#include <QStackedWidget>
#include <QTimer>

// KDE
#include <kcombobox.h>
#include <kdebug.h>
#include <klineedit.h>
#include <klocale.h>

// KDE
#include <lib/semanticinfo/sorteddirmodel.h>

#ifdef GWENVIEW_SEMANTICINFO_BACKEND_NONE
// Qt
#include <QLabel>

#else

// KDE
#include <nepomuk/kratingwidget.h>

// Local
#include <lib/semanticinfo/abstractsemanticinfobackend.h>
#include <lib/semanticinfo/tagmodel.h>
#endif // GWENVIEW_SEMANTICINFO_BACKEND_NONE

namespace Gwenview {

enum FilterType {
	FilterByName = 0,
	FilterByTag,
	FilterByRating
};


//// AbstractFilterController ////
AbstractFilterController::AbstractFilterController(QObject* parent)
: QObject(parent)
, mDirModel(0) {}

void AbstractFilterController::setDirModel(SortedDirModel* model) {
	mDirModel = model;
}

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
#endif // GWENVIEW_SEMANTICINFO_BACKEND_NONE


//// FilterWidget ////
struct FilterWidgetPrivate {
	FilterWidget* that;
	KComboBox* mComboBox;
	QStackedWidget* mStackedWidget;

	QMap<FilterType, AbstractFilterController*> mControllers;
	AbstractFilterController* mCurrentController;

	void setupWidgets() {
		// Combo box
		mComboBox = new KComboBox;
		#ifdef GWENVIEW_SEMANTICINFO_BACKEND_NONE
		mComboBox->hide();
		QLabel* label = new QLabel(i18n("Filter by Name:"));
		#endif

		// Stack
		mStackedWidget = new QStackedWidget;

		// Layout
		QHBoxLayout* layout = new QHBoxLayout(that);
		layout->setMargin(0);
		layout->setSpacing(0);
		#ifdef GWENVIEW_SEMANTICINFO_BACKEND_NONE
		layout->addWidget(label);
		#endif
		layout->addWidget(mComboBox);
		layout->addWidget(mStackedWidget);

	}

	void addController(const QString& caption, FilterType type, AbstractFilterController* controller) {
		mControllers[type] = controller;
		mComboBox->addItem(caption, type);
		mStackedWidget->addWidget(controller->widget());
	}

	void setupControllers() {
		mCurrentController = 0;
		#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
		#endif

		QObject::connect(mComboBox, SIGNAL(currentIndexChanged(int)),
			that, SLOT(activateSelectedController(int)) );
		that->activateSelectedController(0);
	}
};


FilterWidget::FilterWidget(QWidget* parent)
: QWidget(parent)
, d(new FilterWidgetPrivate) {
	d->that = this;
	d->setupWidgets();
	d->setupControllers();
	setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
}


FilterWidget::~FilterWidget() {
	delete d;
}


void FilterWidget::setDirModel(SortedDirModel* dirModel) {
	Q_FOREACH(AbstractFilterController* controller, d->mControllers) {
		controller->setDirModel(dirModel);
	}
}


void FilterWidget::activateSelectedController(int index) {
	/*
	if (d->mCurrentController) {
		d->mCurrentController->reset();
	}

	FilterType filterType = (FilterType)d->mComboBox->itemData(index).toInt();
	d->mCurrentController = d->mControllers.value(filterType);

	d->mStackedWidget->setCurrentWidget(d->mCurrentController->widget());
	d->mCurrentController->init(this);
	*/
}


} // namespace
