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
#include <QRegExp>
#include <QSet>

// KDE

// Local

namespace Gwenview {


struct TagWidgetPrivate {
	TagInfo mTagInfo;
};


TagWidget::TagWidget(QWidget* parent)
: QLineEdit(parent)
, d(new TagWidgetPrivate) {
	connect(this, SIGNAL(editingFinished()),
		SLOT(slotEditingFinished()) );
}


TagWidget::~TagWidget() {
	delete d;
}


void TagWidget::setTagInfo(const TagInfo& tagInfo) {
	d->mTagInfo = tagInfo;

	QStringList lst;
	TagInfo::ConstIterator
		it = tagInfo.begin(),
		end = tagInfo.end();
	for(; it!=end; ++it) {
		lst << it.key();
	}

	setText(lst.join(", "));
}


void TagWidget::slotEditingFinished() {
	QSet<QString> newTagSet = text().split(QRegExp(", *")).toSet();
	QSet<QString> oldTagSet = d->mTagInfo.keys().toSet();

	QSet<QString> assignedTagSet = newTagSet - oldTagSet;
	QSet<QString> removedTagSet = oldTagSet - newTagSet;

	Q_FOREACH(const QString& tag, assignedTagSet) {
		emit tagAssigned(tag);
	}

	Q_FOREACH(const QString& tag, removedTagSet) {
		emit tagRemoved(tag);
	}
}


} // namespace
