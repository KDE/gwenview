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
#include <QComboBox>
#include <QHBoxLayout>
#include <QListView>
#include <QStackedWidget>
#include <QTimer>

// KDE
#include <kdebug.h>
#include <klineedit.h>
#include <klocale.h>

// KDE
#include <lib/metadata/sorteddirmodel.h>

#ifdef GWENVIEW_METADATA_BACKEND_NONE
// Qt
#include <QLabel>

#else

// KDE
#include <nepomuk/kratingwidget.h>

// Local
#include <lib/metadata/abstractmetadatabackend.h>
#include <lib/metadata/tagmodel.h>
#endif // GWENVIEW_METADATA_BACKEND_NONE

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

//// NameFilterController ////
struct NameFilterControllerPrivate {
	KLineEdit* mLineEdit;
};

NameFilterController::NameFilterController(QObject* parent)
: AbstractFilterController(parent)
, d(new NameFilterControllerPrivate) {
	d->mLineEdit = new KLineEdit;
	d->mLineEdit->setClearButtonShown(true);

	QTimer* timer = new QTimer(this);
	timer->setInterval(350);
	timer->setSingleShot(true);
	QObject::connect(timer, SIGNAL(timeout()),
		this, SLOT(applyNameFilter()));

	QObject::connect(d->mLineEdit, SIGNAL(textChanged(const QString &)),
		timer, SLOT(start()));
}


NameFilterController::~NameFilterController() {
	delete d;
}


void NameFilterController::reset() {
	mDirModel->setFilterRegExp(QString());
}


QWidget* NameFilterController::widget() const {
	return d->mLineEdit;
}


void NameFilterController::applyNameFilter() {
	mDirModel->setFilterRegExp(d->mLineEdit->text());
}


#ifndef GWENVIEW_METADATA_BACKEND_NONE
//// RatingController ////
struct RatingControllerPrivate {
	KRatingWidget* mRatingWidget;
};

RatingController::RatingController(QObject* parent)
: AbstractFilterController(parent)
, d(new RatingControllerPrivate) {
	d->mRatingWidget = new KRatingWidget;
	d->mRatingWidget->setHalfStepsEnabled(false);
	d->mRatingWidget->setMaxRating(5);
}


RatingController::~RatingController() {
	delete d;
}


void RatingController::setDirModel(SortedDirModel* model) {
	AbstractFilterController::setDirModel(model);
	QObject::connect(d->mRatingWidget, SIGNAL(ratingChanged(int)),
		model, SLOT(setMinimumRating(int)));
}


void RatingController::reset() {
	mDirModel->setMinimumRating(0);
}


QWidget* RatingController::widget() const {
	return d->mRatingWidget;
}


//// TagController ////
struct TagControllerPrivate {
	QPointer<QListView> mListView;
	KLineEdit* mLineEdit;
};

TagController::TagController(QObject* parent)
: AbstractFilterController(parent)
, d(new TagControllerPrivate) {
	d->mLineEdit = new KLineEdit;
	d->mLineEdit->setClearButtonShown(true);
	d->mLineEdit->setReadOnly(true);
}


TagController::~TagController() {
	d->mListView->deleteLater();
	delete d;
}


void TagController::init(QWidget* parentWidget) {
	Q_ASSERT(!d->mListView);
	d->mListView = new QListView(parentWidget->window());
	d->mListView->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
	d->mListView->setWrapping(true);
	d->mListView->setSelectionMode(QAbstractItemView::MultiSelection);

	AbstractMetaDataBackEnd* backEnd = mDirModel->metaDataBackEnd();
	TagModel* tagModel = new TagModel(d->mListView, backEnd);
	d->mListView->setModel(tagModel);

	connect(d->mListView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
		this, SLOT(updateTagSetFilter()) );

	// FIXME
	d->mListView->resize(500, 120);

	QPoint pos = d->mLineEdit->mapToGlobal(QPoint(0, 0));
	d->mListView->move(pos.x(), pos.y() - d->mListView->height() - 10);
	d->mListView->show();
}


void TagController::reset() {
	d->mListView->deleteLater();
	mDirModel->setTagSetFilter(TagSet());
}


QWidget* TagController::widget() const {
	return d->mLineEdit;
}


void TagController::updateTagSetFilter() {
	Q_ASSERT(d->mListView);

	QStringList labels;
	TagSet tagSet;

	Q_FOREACH(const QModelIndex& index, d->mListView->selectionModel()->selectedIndexes()) {
		MetaDataTag tag = index.data(TagModel::TagRole).toString();
		tagSet << tag;
		labels << index.data(Qt::DisplayRole).toString();
	}
	mDirModel->setTagSetFilter(tagSet);
	d->mLineEdit->setText(labels.join(", "));
}

#endif // GWENVIEW_METADATA_BACKEND_NONE


//// FilterWidget ////
struct FilterWidgetPrivate {
	FilterWidget* that;
	QComboBox* mComboBox;
	QStackedWidget* mStackedWidget;

	QMap<FilterType, AbstractFilterController*> mControllers;
	AbstractFilterController* mCurrentController;

	void setupWidgets() {
		// Combo box
		mComboBox = new QComboBox;
		#ifdef GWENVIEW_METADATA_BACKEND_NONE
		mComboBox->hide();
		QLabel* label = new QLabel(i18n("Filter by Name:"));
		#endif

		// Stack
		mStackedWidget = new QStackedWidget;

		// Layout
		QHBoxLayout* layout = new QHBoxLayout(that);
		layout->setMargin(0);
		layout->setSpacing(0);
		#ifdef GWENVIEW_METADATA_BACKEND_NONE
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
		addController(i18n("Filter by Name:"), FilterByName, new NameFilterController(that));
		#ifndef GWENVIEW_METADATA_BACKEND_NONE
		addController(i18n("Filter by Tag:"), FilterByTag, new TagController(that));
		addController(i18n("Filter by Rating:"), FilterByRating, new RatingController(that));
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
	if (d->mCurrentController) {
		d->mCurrentController->reset();
	}

	FilterType filterType = (FilterType)d->mComboBox->itemData(index).toInt();
	d->mCurrentController = d->mControllers.value(filterType);

	d->mStackedWidget->setCurrentWidget(d->mCurrentController->widget());
	d->mCurrentController->init(this);
}


} // namespace
