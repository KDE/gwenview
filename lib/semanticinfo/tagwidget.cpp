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
#include <QItemDelegate>
#include <QListWidget>
#include <QPainter>
#include <QSortFilterProxyModel>
#include <QVBoxLayout>

// KDE
#include <kdebug.h>
#include <kiconloader.h>
#include <klineedit.h>

// Local
#include <lib/semanticinfo/tagmodel.h>

namespace Gwenview {


class TagItemDelegate : public QItemDelegate {
public:
	TagItemDelegate(QObject* parent = 0)
	: QItemDelegate(parent)
	, mRemovePixmap(SmallIcon("list-remove"))
	{}

protected:
	void drawDisplay(QPainter* painter, const QStyleOptionViewItem& option, const QRect& rect, const QString& text) const {
		QItemDelegate::drawDisplay(painter, option, rect, text);
		int left = rect.right() - mRemovePixmap.width();
		int top = rect.top() + (rect.height() - mRemovePixmap.height() ) / 2;
		painter->drawPixmap(left, top, mRemovePixmap);
	}

private:
	QPixmap mRemovePixmap;
};


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
	TagModel* mTagModel;


	void setupWidgets() {
		mListView = new QListView;
		mListView->setItemDelegate(new TagItemDelegate(mListView));
		mListView->setSelectionMode(QListView::NoSelection);
		mListView->setModel(mTagModel);

		mLineEdit = new KLineEdit;
		mLineEdit->setTrapReturnKey(true);

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

		TagSet tagSet;
		TagInfo::ConstIterator
			it = mTagInfo.begin(),
			end = mTagInfo.end();
		for(; it!=end; ++it) {
			tagSet << it.key();
		}
		mTagModel->setTagSet(tagSet);
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
	d->mTagModel = new TagModel(this);
	d->setupWidgets();

	connect(d->mListView, SIGNAL(clicked(const QModelIndex&)),
		SLOT(slotTagClicked(const QModelIndex&)));

	connect(d->mLineEdit, SIGNAL(returnPressed()),
		SLOT(assignTag()) );
}


TagWidget::~TagWidget() {
	delete d;
}


void TagWidget::setSemanticInfoBackEnd(AbstractSemanticInfoBackEnd* backEnd) {
	d->mBackEnd = backEnd;
	d->mTagModel->setSemanticInfoBackEnd(backEnd);
	d->mTagCompleterModel->setSemanticInfoBackEnd(backEnd);
}


void TagWidget::setTagInfo(const TagInfo& tagInfo) {
	d->mTagInfo = tagInfo;
	d->fillTagModel();
	d->updateCompleterModel();
}


void TagWidget::assignTag() {
	Q_ASSERT(d->mBackEnd);
	QString label = d->mLineEdit->text();
	SemanticInfoTag tag = d->mBackEnd->tagForLabel(label);
	d->mTagInfo[tag] = true;
	d->mLineEdit->clear();
	d->mTagModel->addTag(tag, label);
	d->updateCompleterModel();

	emit tagAssigned(tag);
}


void TagWidget::slotTagClicked(const QModelIndex& index) {
	SemanticInfoTag tag = index.data(TagModel::TagRole).toString();
	d->mTagInfo.remove(tag);
	d->mTagModel->removeTag(tag);
	d->updateCompleterModel();

	emit tagRemoved(tag);
}


} // namespace
