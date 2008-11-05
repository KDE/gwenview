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
#include "filtercontroller.moc"

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
#include <kiconloader.h>
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

/**
 * An AbstractSortedDirModelFilter which filters on the file names
 */
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
	layout->setMargin(0);
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
/**
 * An AbstractSortedDirModelFilter which filters on file ratings
 */
class RatingFilter : public AbstractSortedDirModelFilter {
public:
	enum Mode {
		GreaterOrEqual,
		Equal,
		LessOrEqual
	};

	RatingFilter(SortedDirModel* model)
	: AbstractSortedDirModelFilter(model)
	, mRating(0)
	, mMode(GreaterOrEqual) {}

	virtual bool needsSemanticInfo() const {
		return true;
	}

	virtual bool acceptsIndex(const QModelIndex& index) const {
		SemanticInfo info = model()->semanticInfoForIndex(index);
		switch (mMode) {
		case GreaterOrEqual:
			return info.mRating >= mRating;
		case Equal:
			return info.mRating == mRating;
		default: /* LessOrEqual */
			return info.mRating <= mRating;
		}
	}

	void setRating(int value) {
		mRating = value;
		model()->applyFilters();
	}

	void setMode(Mode mode) {
		mMode = mode;
		model()->applyFilters();
	}

private:
	int mRating;
	Mode mMode;
};

struct RatingWidgetPrivate {
	KComboBox* mModeComboBox;
	KRatingWidget* mRatingWidget;
	QPointer<RatingFilter> mFilter;
};

RatingFilterWidget::RatingFilterWidget(SortedDirModel* model)
: d(new RatingWidgetPrivate) {
	d->mModeComboBox = new KComboBox;
	d->mModeComboBox->addItem(i18n("Rating >="), RatingFilter::GreaterOrEqual);
	d->mModeComboBox->addItem(i18n("Rating =") , RatingFilter::Equal);
	d->mModeComboBox->addItem(i18n("Rating <="), RatingFilter::LessOrEqual);

	d->mRatingWidget = new KRatingWidget;
	d->mRatingWidget->setHalfStepsEnabled(true);
	d->mRatingWidget->setMaxRating(10);

	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setMargin(0);
	layout->addWidget(d->mModeComboBox);
	layout->addWidget(d->mRatingWidget);

	d->mFilter = new RatingFilter(model);

	QObject::connect(d->mModeComboBox, SIGNAL(currentIndexChanged(int)),
		SLOT(updateFilterMode()));

	QObject::connect(d->mRatingWidget, SIGNAL(ratingChanged(int)),
		SLOT(slotRatingChanged(int)));

	updateFilterMode();
}


RatingFilterWidget::~RatingFilterWidget() {
	delete d->mFilter;
	delete d;
}


void RatingFilterWidget::slotRatingChanged(int value) {
	d->mFilter->setRating(value);
}


void RatingFilterWidget::updateFilterMode() {
	QVariant data = d->mModeComboBox->itemData(d->mModeComboBox->currentIndex());
	d->mFilter->setMode(RatingFilter::Mode(data.toInt()));
}


/**
 * An AbstractSortedDirModelFilter which filters on associated tags
 */
class TagFilter : public AbstractSortedDirModelFilter {
public:
	TagFilter(SortedDirModel* model)
	: AbstractSortedDirModelFilter(model)
	, mWantMatchingTag(true)
	{}

	virtual bool needsSemanticInfo() const {
		return true;
	}

	virtual bool acceptsIndex(const QModelIndex& index) const {
		SemanticInfo info = model()->semanticInfoForIndex(index);
		if (mWantMatchingTag) {
			return info.mTags.contains(mTag);
		} else {
			return !info.mTags.contains(mTag);
		}
	}

	void setTag(const SemanticInfoTag& tag) {
		mTag = tag;
		model()->applyFilters();
	}

	void setWantMatchingTag(bool value) {
		mWantMatchingTag = value;
		model()->applyFilters();
	}

private:
	SemanticInfoTag mTag;
	bool mWantMatchingTag;
};

struct TagFilterWidgetPrivate {
	KComboBox* mModeComboBox;
	KComboBox* mTagComboBox;
	QPointer<TagFilter> mFilter;
};

TagFilterWidget::TagFilterWidget(SortedDirModel* model)
: d(new TagFilterWidgetPrivate) {
	d->mFilter = new TagFilter(model);

	d->mModeComboBox = new KComboBox;
	d->mModeComboBox->addItem(i18n("Tagged"), QVariant(true));
	d->mModeComboBox->addItem(i18n("Not Tagged"), QVariant(false));

	d->mTagComboBox = new KComboBox;

	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setMargin(0);
	layout->addWidget(d->mModeComboBox);
	layout->addWidget(d->mTagComboBox);

	AbstractSemanticInfoBackEnd* backEnd = model->semanticInfoBackEnd();
	backEnd->refreshAllTags();
	TagModel* tagModel = TagModel::createAllTagsModel(this, backEnd);
	d->mTagComboBox->setModel(tagModel);
	d->mTagComboBox->setCurrentIndex(-1);
	connect(d->mTagComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateTagSetFilter()) );

	connect(d->mModeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(updateTagSetFilter()) );
}

TagFilterWidget::~TagFilterWidget() {
	delete d->mFilter;
	delete d;
}


void TagFilterWidget::updateTagSetFilter() {
	QModelIndex index = d->mTagComboBox->model()->index(d->mTagComboBox->currentIndex(), 0);
	if (!index.isValid()) {
		kWarning() << "Invalid index";
		return;
	}
	SemanticInfoTag tag = index.data(TagModel::TagRole).toString();
	d->mFilter->setTag(tag);

	bool wantMatchingTag = d->mModeComboBox->itemData(d->mModeComboBox->currentIndex()).toBool();
	d->mFilter->setWantMatchingTag(wantMatchingTag);
}
#endif


/**
 * A container for all filter widgets. It features a close button on the right.
 */
class FilterWidgetContainer : public QWidget {
public:
	void setFilterWidget(QWidget* widget) {
		QToolButton* closeButton = new QToolButton;
		closeButton->setIcon(KIcon("window-close"));
		closeButton->setAutoRaise(true);
		int size = IconSize(KIconLoader::Small);
		closeButton->setIconSize(QSize(size, size));
		connect(closeButton, SIGNAL(clicked()), SLOT(deleteLater()));
		QHBoxLayout* layout = new QHBoxLayout(this);
		layout->setMargin(0);
		layout->setSpacing(2);
		layout->addWidget(widget);
		layout->addWidget(closeButton);
	}
};


struct FilterControllerPrivate {
	FilterController* that;
	QFrame* mFrame;
	SortedDirModel* mDirModel;
	QList<QAction*> mActionList;

	int mFilterWidgetCount; /** How many filter widgets are in mFrame */

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

		mFilterWidgetCount++;
		QObject::connect(container, SIGNAL(destroyed()), that, SLOT(slotFilterWidgetClosed()));
	}
};


FilterController::FilterController(QFrame* frame, SortedDirModel* dirModel)
: QObject(frame)
, d(new FilterControllerPrivate) {
	d->that = this;
	d->mFrame = frame;
	d->mDirModel = dirModel;
	d->mFilterWidgetCount = 0;

	d->mFrame->hide();
	new FlowLayout(d->mFrame);

	d->addAction(i18nc("@action:inmenu", "Filter by Name"), SLOT(addFilterByName()));
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
	d->addAction(i18nc("@action:inmenu", "Filter by Rating"), SLOT(addFilterByRating()));
	d->addAction(i18nc("@action:inmenu", "Filter by Tag"), SLOT(addFilterByTag()));
#endif
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


#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
void FilterController::addFilterByRating() {
	d->addFilter(new RatingFilterWidget(d->mDirModel));
}


void FilterController::addFilterByTag() {
	d->addFilter(new TagFilterWidget(d->mDirModel));
}
#endif


void FilterController::slotFilterWidgetClosed() {
	d->mFilterWidgetCount--;
	if (d->mFilterWidgetCount == 0) {
		d->mFrame->hide();
	}
}


} // namespace
