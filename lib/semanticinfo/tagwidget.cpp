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
#include "tagwidget.moc"

// Qt
#include <QCompleter>
#include <QListView>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

// KDE
#include <kdebug.h>
#include <klineedit.h>

// Local
#include <lib/semanticinfo/tagitemdelegate.h>
#include <lib/semanticinfo/tagmodel.h>

namespace Gwenview {


class TagCompleterModel : public QSortFilterProxyModel {
public:
	TagCompleterModel(QObject* parent)
	: QSortFilterProxyModel(parent)
	{
	}


	void setTagInfo(const TagInfo& tagInfo) {
		mExcludedTagSet.clear();
		TagInfo::ConstIterator
			it = tagInfo.begin(),
			end = tagInfo.end();
		for(; it!=end; ++it) {
			if (it.value()) {
				mExcludedTagSet << it.key();
			}
		}
		invalidate();
	}


	void setSemanticInfoBackEnd(AbstractSemanticInfoBackEnd* backEnd) {
		setSourceModel(TagModel::createAllTagsModel(this, backEnd));
	}


protected:
	virtual bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
		QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
		SemanticInfoTag tag = sourceIndex.data(TagModel::TagRole).toString();
		return !mExcludedTagSet.contains(tag);
	}


private:
	TagSet mExcludedTagSet;
};


struct TagWidgetPrivate {
	TagWidget* that;
	TagInfo mTagInfo;
	QListView* mListView;
	KLineEdit* mLineEdit;
	AbstractSemanticInfoBackEnd* mBackEnd;
	TagCompleterModel* mTagCompleterModel;
	TagModel* mAssignedTagModel;


	void setupWidgets() {
		mListView = new QListView;
		TagItemDelegate* delegate = new TagItemDelegate(mListView);
		QObject::connect(delegate, SIGNAL(removeTagRequested(const SemanticInfoTag&)),
			that, SLOT(removeTag(const SemanticInfoTag&)));
		QObject::connect(delegate, SIGNAL(assignTagToAllRequested(const SemanticInfoTag&)),
			that, SLOT(assignTag(const SemanticInfoTag&)));
		mListView->setItemDelegate(delegate);
		mListView->setModel(mAssignedTagModel);

		mLineEdit = new KLineEdit;

		mTagCompleterModel = new TagCompleterModel(that);
		QCompleter* completer = new QCompleter(that);
		completer->setCaseSensitivity(Qt::CaseInsensitive);
		completer->setModel(mTagCompleterModel);
		mLineEdit->setCompleter(completer);

		QVBoxLayout* layout = new QVBoxLayout(that);
		layout->setMargin(0);
		layout->addWidget(mListView);
		layout->addWidget(mLineEdit);

		that->setTabOrder(mLineEdit, mListView);
	}


	void fillTagModel() {
		Q_ASSERT(mBackEnd);

		mAssignedTagModel->clear();
		TagInfo::ConstIterator
			it = mTagInfo.begin(),
			end = mTagInfo.end();
		for(; it!=end; ++it) {
			mAssignedTagModel->addTag(
				it.key(),
				QString(),
				it.value() ? TagModel::FullyAssigned : TagModel::PartiallyAssigned);
		}
	}


	void updateCompleterModel() {
		mTagCompleterModel->setTagInfo(mTagInfo);
	}
};


TagWidget::TagWidget(QWidget* parent)
: QWidget(parent)
, d(new TagWidgetPrivate) {
	d->that = this;
	d->mBackEnd = 0;
	d->mAssignedTagModel = new TagModel(this);
	d->setupWidgets();

	connect(d->mLineEdit, SIGNAL(returnPressed()),
		SLOT(slotReturnPressed()) );
}


TagWidget::~TagWidget() {
	delete d;
}


void TagWidget::setSemanticInfoBackEnd(AbstractSemanticInfoBackEnd* backEnd) {
	d->mBackEnd = backEnd;
	d->mAssignedTagModel->setSemanticInfoBackEnd(backEnd);
	d->mTagCompleterModel->setSemanticInfoBackEnd(backEnd);
}


void TagWidget::setTagInfo(const TagInfo& tagInfo) {
	d->mTagInfo = tagInfo;
	d->fillTagModel();
	d->updateCompleterModel();
}


void TagWidget::slotReturnPressed() {
	Q_ASSERT(d->mBackEnd);
	QString label = d->mLineEdit->text();
	if (label.isEmpty()) {
		return;
	}
	d->mLineEdit->clear();
	assignTag(d->mBackEnd->tagForLabel(label));
}


void TagWidget::assignTag(const SemanticInfoTag& tag) {
	d->mTagInfo[tag] = true;
	d->mAssignedTagModel->addTag(tag);
	d->updateCompleterModel();

	emit tagAssigned(tag);
}


void TagWidget::removeTag(const SemanticInfoTag& tag) {
	d->mTagInfo.remove(tag);
	d->mAssignedTagModel->removeTag(tag);
	d->updateCompleterModel();

	emit tagRemoved(tag);
}


} // namespace
