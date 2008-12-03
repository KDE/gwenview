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
#include "semanticinfocontextmanageritem.moc"

// Qt
#include <QShortcut>
#include <QSignalMapper>
#include <QStyle>
#include <QTimer>

// KDE
#include <kaction.h>
#include <kactioncategory.h>
#include <kactioncollection.h>
#include <kdebug.h>
#include <kdialog.h>
#include <klocale.h>

// Nepomuk
#include <nepomuk/kratingwidget.h>

// Local
#include "contextmanager.h"
#include "sidebar.h"
#include "ui_semanticinfosidebaritem.h"
#include "ui_semanticinfodialog.h"
#include <lib/hudwidget.h>
#include <lib/widgetfloater.h>
#include <lib/semanticinfo/abstractsemanticinfobackend.h>
#include <lib/semanticinfo/semanticinfodirmodel.h>
#include <lib/semanticinfo/sorteddirmodel.h>

namespace Gwenview {

static const int RATING_INDICATOR_HIDE_DELAY = 3000;


struct SemanticInfoDialog : public KDialog, public Ui_SemanticInfoDialog {
	SemanticInfoDialog(QWidget* parent)
	: KDialog(parent) {
		setButtons(None);
		QWidget* mainWidget = new QWidget;
		setMainWidget(mainWidget);
		setupUi(mainWidget);
		mainWidget->layout()->setMargin(0);
	}
};


class RatingIndicator : public HudWidget {
public:
	RatingIndicator(QWidget* parent)
	: HudWidget(parent)
	, mRatingWidget(new KRatingWidget)
	, mHideTimer(new QTimer)
	{
		init(mRatingWidget, OptionNone);
		WidgetFloater* floater = new WidgetFloater(parent);
		floater->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
		floater->setVerticalMargin(
			KDialog::marginHint()
			+ parent->style()->pixelMetric(QStyle::PM_ScrollBarExtent)
			);
		floater->setChildWidget(this);

		mHideTimer->setInterval(RATING_INDICATOR_HIDE_DELAY);
		mHideTimer->setSingleShot(true);
		connect(mHideTimer, SIGNAL(timeout()), SLOT(hide()));
		hide();
	}

	void show(int rating) {
		mRatingWidget->setRating(rating);
		HudWidget::show();
		raise();
		mHideTimer->start();
	}

private:
	KRatingWidget* mRatingWidget;
	QTimer* mHideTimer;
};


struct SemanticInfoContextManagerItemPrivate : public Ui_SemanticInfoSideBarItem {
	SemanticInfoContextManagerItem* that;
	SideBar* mSideBar;
	SideBarGroup* mGroup;
	KActionCollection* mActionCollection;
	QPointer<SemanticInfoDialog> mSemanticInfoDialog;
	TagInfo mTagInfo;
	KAction* mEditTagsAction;
	QSignalMapper* mRatingMapper;
	RatingIndicator* mRatingIndicator;
	/** A list of all actions, so that we can disable them when necessary */
	QList<KAction*> mActions;

	void setupActions() {
		KActionCategory* edit = new KActionCategory(i18nc("@title actions category","Edit"), mActionCollection);

		mEditTagsAction = edit->addAction("edit_tags");
		mEditTagsAction->setText(i18nc("@action", "Edit Tags"));
		mEditTagsAction->setShortcut(Qt::CTRL | Qt::Key_T);
		QObject::connect(mEditTagsAction, SIGNAL(triggered()),
			that, SLOT(showSemanticInfoDialog()) );
		mActions << mEditTagsAction;

		mRatingMapper = new QSignalMapper(that);
		for (int rating=0; rating <= 5; ++rating) {
			KAction* action = edit->addAction(QString("rate_%1").arg(rating));
			if (rating == 0) {
				action->setText(i18nc("@action Rating value of zero", "Zero"));
			} else {
				action->setText(QString(rating, QChar(0x22C6))); /* 0x22C6 is the 'star' character */
			}
			action->setShortcut(Qt::Key_0 + rating);
			QObject::connect(action, SIGNAL(triggered()), mRatingMapper, SLOT(map()) );
			mRatingMapper->setMapping(action, rating * 2);
			mActions << action;
		}
		QObject::connect(mRatingMapper, SIGNAL(mapped(int)), that, SLOT(slotRatingChanged(int)) );
	}


	void updateTagLabel() {
		if (that->contextManager()->selection().isEmpty()) {
			mTagLabel->clear();
			return;
		}

		AbstractSemanticInfoBackEnd* backEnd = that->contextManager()->dirModel()->semanticInfoBackEnd();

		TagInfo::ConstIterator
			it = mTagInfo.constBegin(),
			end = mTagInfo.constEnd();
		QMap<QString, QString> labelMap;
		for (; it!=end; ++it) {
			SemanticInfoTag tag = it.key();
			QString label = backEnd->labelForTag(tag);
			if (!it.value()) {
				// Tag is not present for all urls
				label += '*';
			}
			labelMap[label.toLower()] = label;
		}
		QStringList labels(labelMap.values());

		QString editLink = i18n("Edit");
		QString text = labels.join(", ") + QString(" <a href='edit'>%1</a>").arg(editLink);
		mTagLabel->setText(text);
	}


	void updateSemanticInfoDialog() {
		mSemanticInfoDialog->mTagWidget->setEnabled(!that->contextManager()->selection().isEmpty());
		mSemanticInfoDialog->mTagWidget->setTagInfo(mTagInfo);
	}
};


SemanticInfoContextManagerItem::SemanticInfoContextManagerItem(ContextManager* manager, KActionCollection* actionCollection, QWidget* documentPanel)
: AbstractContextManagerItem(manager)
, d(new SemanticInfoContextManagerItemPrivate) {
	d->that = this;
	d->mSideBar = 0;
	d->mGroup = 0;
	d->mActionCollection = actionCollection;

	d->mRatingIndicator = new RatingIndicator(documentPanel);

	connect(contextManager(), SIGNAL(selectionChanged()),
		SLOT(slotSelectionChanged()) );
	connect(contextManager(), SIGNAL(selectionDataChanged()),
		SLOT(update()) );
	connect(contextManager(), SIGNAL(currentDirUrlChanged()),
		SLOT(update()) );

	d->setupActions();
}


SemanticInfoContextManagerItem::~SemanticInfoContextManagerItem() {
	delete d;
}


void SemanticInfoContextManagerItem::setSideBar(SideBar* sideBar) {
	d->mSideBar = sideBar;
	connect(sideBar, SIGNAL(aboutToShow()),
		SLOT(update()) );

	d->mGroup = sideBar->createGroup(i18n("Semantic Information"));

	QWidget* container = new QWidget;
	d->setupUi(container);
	container->layout()->setMargin(0);
	d->mGroup->addWidget(container);

	d->mRatingWidget->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	d->mRatingWidget->setHalfStepsEnabled(true);
	d->mRatingWidget->setMaxRating(10);
	connect(d->mRatingWidget, SIGNAL(ratingChanged(int)),
		SLOT(slotRatingChanged(int)));
	connect(d->mRatingMapper, SIGNAL(mapped(int)),
		d->mRatingWidget, SLOT(setRating(int)) );

	connect(d->mDescriptionLineEdit, SIGNAL(editingFinished()),
		SLOT(storeDescription()));

	connect(d->mTagLabel, SIGNAL(linkActivated(const QString&)),
		d->mEditTagsAction, SLOT(trigger()) );
}


inline int ratingForVariant(const QVariant& variant) {
	if (variant.isValid()) {
		return variant.toInt();
	} else {
		return 0;
	}
}


void SemanticInfoContextManagerItem::slotSelectionChanged() {
	d->mRatingIndicator->hide();
	update();
}

void SemanticInfoContextManagerItem::update() {
	KFileItemList itemList = contextManager()->selection();

	bool first = true;
	int rating = 0;
	QString description;
	SortedDirModel* dirModel = contextManager()->dirModel();

	// This hash stores for how many items the tag is present
	// If you have 3 items, and only 2 have the "Holiday" tag,
	// then tagHash["Holiday"] will be 2 at the end of the loop.
	typedef QHash<QString, int> TagHash;
	TagHash tagHash;

	Q_FOREACH(const KFileItem& item, itemList) {
		QModelIndex index = dirModel->indexForItem(item);

		QVariant value = dirModel->data(index, SemanticInfoDirModel::RatingRole);
		if (first) {
			rating = ratingForVariant(value);
		} else if (rating != ratingForVariant(value)) {
			// Ratings aren't the same, reset
			rating = 0;
		}

		QString indexDescription = index.data(SemanticInfoDirModel::DescriptionRole).toString();
		if (first) {
			description = indexDescription;
		} else if (description != indexDescription) {
			description.clear();
		}

		// Fill tagHash, incrementing the tag count if it's already there
		TagSet tagSet = TagSet::fromVariant(index.data(SemanticInfoDirModel::TagsRole));
		Q_FOREACH(const QString& tag, tagSet) {
			TagHash::Iterator it = tagHash.find(tag);
			if (it == tagHash.end()) {
				tagHash[tag] = 1;
			} else {
				++it.value();
			}
		}

		first = false;
	}
	d->mRatingWidget->setRating(rating);
	d->mDescriptionLineEdit->setText(description);

	// Init tagInfo from tagHash
	d->mTagInfo.clear();
	int itemCount = itemList.count();
	TagHash::ConstIterator
		it = tagHash.begin(),
		end = tagHash.end();
	for (; it!=end; ++it) {
		QString tag = it.key();
		int count = it.value();
		d->mTagInfo[tag] = count == itemCount;
	}

	bool enabled = !contextManager()->selection().isEmpty();
	Q_FOREACH(KAction* action, d->mActions) {
		action->setEnabled(enabled);
	}
	d->updateTagLabel();
	if (d->mSemanticInfoDialog) {
		d->updateSemanticInfoDialog();
	}
}


void SemanticInfoContextManagerItem::slotRatingChanged(int rating) {
	KFileItemList itemList = contextManager()->selection();

	// Show rating indicator in view mode, and only if sidebar is not visible
	if (d->mRatingIndicator->parentWidget()->isVisible() && !d->mRatingWidget->isVisible()) {
		d->mRatingIndicator->show(rating);
	}

	SortedDirModel* dirModel = contextManager()->dirModel();
	Q_FOREACH(const KFileItem& item, itemList) {
		QModelIndex index = dirModel->indexForItem(item);
		dirModel->setData(index, rating, SemanticInfoDirModel::RatingRole);
	}
}


void SemanticInfoContextManagerItem::storeDescription() {
	QString description = d->mDescriptionLineEdit->text();
	KFileItemList itemList = contextManager()->selection();

	SortedDirModel* dirModel = contextManager()->dirModel();
	Q_FOREACH(const KFileItem& item, itemList) {
		QModelIndex index = dirModel->indexForItem(item);
		dirModel->setData(index, description, SemanticInfoDirModel::DescriptionRole);
	}
}


void SemanticInfoContextManagerItem::assignTag(const SemanticInfoTag& tag) {
	KFileItemList itemList = contextManager()->selection();

	SortedDirModel* dirModel = contextManager()->dirModel();
	Q_FOREACH(const KFileItem& item, itemList) {
		QModelIndex index = dirModel->indexForItem(item);
		TagSet tags = TagSet::fromVariant( dirModel->data(index, SemanticInfoDirModel::TagsRole) );
		if (!tags.contains(tag)) {
			tags << tag;
			dirModel->setData(index, tags.toVariant(), SemanticInfoDirModel::TagsRole);
		}
	}
}


void SemanticInfoContextManagerItem::removeTag(const SemanticInfoTag& tag) {
	KFileItemList itemList = contextManager()->selection();

	SortedDirModel* dirModel = contextManager()->dirModel();
	Q_FOREACH(const KFileItem& item, itemList) {
		QModelIndex index = dirModel->indexForItem(item);
		TagSet tags = TagSet::fromVariant( dirModel->data(index, SemanticInfoDirModel::TagsRole) );
		if (tags.contains(tag)) {
			tags.remove(tag);
			dirModel->setData(index, tags.toVariant(), SemanticInfoDirModel::TagsRole);
		}
	}
}


void SemanticInfoContextManagerItem::showSemanticInfoDialog() {
	if (!d->mSemanticInfoDialog) {
		d->mSemanticInfoDialog = new SemanticInfoDialog(d->mSideBar);
		d->mSemanticInfoDialog->setAttribute(Qt::WA_DeleteOnClose, true);

		connect(d->mSemanticInfoDialog->mPreviousButton, SIGNAL(clicked()),
			d->mActionCollection->action("go_previous"), SLOT(trigger()) );
		connect(d->mSemanticInfoDialog->mNextButton, SIGNAL(clicked()),
			d->mActionCollection->action("go_next"), SLOT(trigger()) );

		AbstractSemanticInfoBackEnd* backEnd = contextManager()->dirModel()->semanticInfoBackEnd();
		d->mSemanticInfoDialog->mTagWidget->setSemanticInfoBackEnd(backEnd);
		connect(d->mSemanticInfoDialog->mTagWidget, SIGNAL(tagAssigned(const SemanticInfoTag&)),
			SLOT(assignTag(const SemanticInfoTag&)) );
		connect(d->mSemanticInfoDialog->mTagWidget, SIGNAL(tagRemoved(const SemanticInfoTag&)),
			SLOT(removeTag(const SemanticInfoTag&)) );
	}
	d->updateSemanticInfoDialog();
	d->mSemanticInfoDialog->show();
}


} // namespace
