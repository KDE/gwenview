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
#include <QHeaderView>
#include <QItemDelegate>
#include <QListWidget>
#include <QPainter>
#include <QVBoxLayout>

// KDE
#include <kiconloader.h>
#include <klineedit.h>

// Local

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


struct TagWidgetPrivate {
	TagWidget* that;
	TagInfo mTagInfo;
	QListWidget* mTreeWidget;
	KLineEdit* mLineEdit;

	void setupWidgets() {
		mTreeWidget = new QListWidget;
		mTreeWidget->setItemDelegate(new TagItemDelegate(mTreeWidget));
		mLineEdit = new KLineEdit;

		QVBoxLayout* layout = new QVBoxLayout(that);
		layout->setMargin(0);
		layout->setSpacing(2);
		layout->addWidget(mTreeWidget);
		layout->addWidget(mLineEdit);
	}


	void fillTreeWidget() {
		mTreeWidget->clear();

		TagInfo::ConstIterator
			it = mTagInfo.begin(),
			end = mTagInfo.end();
		for(; it!=end; ++it) {
			new QListWidgetItem(it.key(), mTreeWidget);
		}
	}
};


TagWidget::TagWidget(QWidget* parent)
: QWidget(parent)
, d(new TagWidgetPrivate) {
	d->that = this;
	d->setupWidgets();

	connect(d->mTreeWidget, SIGNAL(itemClicked(QListWidgetItem*)),
		SLOT(slotItemClicked(QListWidgetItem*)) );

	connect(d->mLineEdit, SIGNAL(returnPressed()),
		SLOT(assignTag()) );
}


TagWidget::~TagWidget() {
	delete d;
}


void TagWidget::setTagInfo(const TagInfo& tagInfo) {
	d->mTagInfo = tagInfo;
	d->fillTreeWidget();
}


void TagWidget::assignTag() {
	QString tag = d->mLineEdit->text();
	d->mTagInfo[tag] = true;
	d->fillTreeWidget();
	d->mLineEdit->clear();

	emit tagAssigned(tag);
}


void TagWidget::slotItemClicked(QListWidgetItem* item) {
	if (!item) {
		return;
	}
	QString tag = item->text();
	d->mTagInfo.remove(tag);
	d->fillTreeWidget();

	emit tagRemoved(tag);
}


} // namespace
