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

#include <config-gwenview.h>

// Qt
#include <QAction>
#include <QLineEdit>
#include <QTimer>
#include <QToolButton>

// KDE
#include <kcombobox.h>
#include <kdebug.h>
#include <kicon.h>
#include <klineedit.h>
#include <klocale.h>

// Local
#include <lib/flowlayout.h>
#include <lib/semanticinfo/sorteddirmodel.h>

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
// KDE
#include <nepomuk/kratingwidget.h>

// Local
#include <lib/semanticinfo/abstractsemanticinfobackend.h>
#include <lib/semanticinfo/tagmodel.h>
#endif

namespace Gwenview {

//// NameFilter ////
class NameFilter : public AbstractSortedDirModelFilter {
public:
	NameFilter(SortedDirModel* model)
	: AbstractSortedDirModelFilter(model)
	, mText(0) {}

	virtual bool needsSemanticInfo() const {
		return false;
	}

	virtual bool acceptsIndex(const QModelIndex& index) const {
		return index.data().toString().contains(mText);
	}

	void setText(const QString& text) {
		mText = text;
		model()->applyFilters();
	}

private:
	QString mText;
};

struct NameFilterWidgetPrivate {
	QPointer<NameFilter> mFilter;
	KLineEdit* mLineEdit;
};

NameFilterWidget::NameFilterWidget(SortedDirModel* model)
: d(new NameFilterWidgetPrivate)
{
	d->mFilter = new NameFilter(model);
	d->mLineEdit = new KLineEdit;
	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->addWidget(d->mLineEdit);

	QTimer* timer = new QTimer(this);
	timer->setInterval(350);
	timer->setSingleShot(true);
	connect(timer, SIGNAL(timeout()), SLOT(applyNameFilter()));

	QObject::connect(d->mLineEdit, SIGNAL(textChanged(const QString &)),
		timer, SLOT(start()));
}

NameFilterWidget::~NameFilterWidget()
{
	delete d->mFilter;
	delete d;
}

void NameFilterWidget::applyNameFilter() {
	d->mFilter->setText(d->mLineEdit->text());
}

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
//// RatingFilter ////
class RatingFilter : public AbstractSortedDirModelFilter {
public:
	RatingFilter(SortedDirModel* model)
	: AbstractSortedDirModelFilter(model)
	, mMinimumRating(0) {}

	virtual bool needsSemanticInfo() const {
		return true;
	}

	virtual bool acceptsIndex(const QModelIndex& index) const {
		SemanticInfo info = model()->semanticInfoForIndex(index);
		return info.mRating >= mMinimumRating;
	}

	void setMinimumRating(int value) {
		mMinimumRating = value;
		model()->applyFilters();
	}

private:
	int mMinimumRating;
};

struct RatingWidgetPrivate {
	KRatingWidget* mRatingWidget;
	QPointer<RatingFilter> mFilter;
};

RatingFilterWidget::RatingFilterWidget(SortedDirModel* model)
: d(new RatingWidgetPrivate) {
	d->mRatingWidget = new KRatingWidget;
	d->mRatingWidget->setHalfStepsEnabled(true);
	d->mRatingWidget->setMaxRating(10);
	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->addWidget(d->mRatingWidget);

	d->mFilter = new RatingFilter(model);

	QObject::connect(d->mRatingWidget, SIGNAL(ratingChanged(int)),
		SLOT(slotRatingChanged(int)));
}


RatingFilterWidget::~RatingFilterWidget() {
	delete d->mFilter;
	delete d;
}


void RatingFilterWidget::slotRatingChanged(int value) {
	d->mFilter->setMinimumRating(value);
}
#endif


//// TagFilter ////
class TagFilter : public AbstractSortedDirModelFilter {
public:
	TagFilter(SortedDirModel* model)
	: AbstractSortedDirModelFilter(model)
	{}

	virtual bool needsSemanticInfo() const {
		return true;
	}

	virtual bool acceptsIndex(const QModelIndex& index) const {
		SemanticInfo info = model()->semanticInfoForIndex(index);
		TagSet commonSet = info.mTags & mTagSet;
		return commonSet.size() == mTagSet.size();
	}

	void setTagSet(const TagSet& tagSet) {
		mTagSet = tagSet;
		model()->applyFilters();
	}

private:
	TagSet mTagSet;
};

struct TagFilterWidgetPrivate {
	KComboBox* mComboBox;
	QPointer<TagFilter> mFilter;
};

TagFilterWidget::TagFilterWidget(SortedDirModel* model)
: d(new TagFilterWidgetPrivate) {
	d->mFilter = new TagFilter(model);
	d->mComboBox = new KComboBox;

	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->addWidget(d->mComboBox);

	AbstractSemanticInfoBackEnd* backEnd = model->semanticInfoBackEnd();
	backEnd->refreshAllTags();
	TagModel* tagModel = TagModel::createAllTagsModel(this, backEnd);
	d->mComboBox->setModel(tagModel);
	d->mComboBox->setCurrentIndex(-1);
	connect(d->mComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateTagSetFilter()) );
}

TagFilterWidget::~TagFilterWidget() {
	delete d->mFilter;
	delete d;
}


void TagFilterWidget::updateTagSetFilter() {
	QModelIndex index = d->mComboBox->model()->index(d->mComboBox->currentIndex(), 0);
	if (!index.isValid()) {
		kWarning() << "Invalid index";
		return;
	}
	TagSet tagSet;
	tagSet << index.data(TagModel::TagRole).toString();
	d->mFilter->setTagSet(tagSet);
}


//// FilterWidgetContainer ////
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
	d->addFilter(new NameFilterWidget(d->mDirModel));
}


void FilterController::addFilterByRating() {
	d->addFilter(new RatingFilterWidget(d->mDirModel));
}


void FilterController::addFilterByTag() {
	d->addFilter(new TagFilterWidget(d->mDirModel));
}


} // namespace
