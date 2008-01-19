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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "preferredimagemetainfomodel.h"


// Qt
#include <QStringList>


namespace Gwenview {


struct PreferredImageMetaInfoModelPrivate {
	PreferredImageMetaInfoModel* mModel;
	QStringList mPreferredMetaInfoKeyList;


	QVariant checkStateData(const QModelIndex& index) const {
		if (index.parent().isValid() & index.column() == 0) {
			QString key = mModel->keyForIndex(index);
			bool checked = mPreferredMetaInfoKeyList.contains(key);
			return QVariant(checked ? Qt::Checked: Qt::Unchecked);
		} else {
			return QVariant();
		}
	}


	void sortPreferredMetaInfoKeyList() {
		QStringList sortedList;
		int groupCount = mModel->rowCount();
		for (int groupRow = 0; groupRow < groupCount; ++groupRow) {
			QModelIndex groupIndex = mModel->index(groupRow, 0);
			int keyCount = mModel->rowCount(groupIndex);
			for (int keyRow = 0; keyRow < keyCount; ++keyRow) {
				QModelIndex keyIndex = mModel->index(keyRow, 0, groupIndex);
				QString key = mModel->keyForIndex(keyIndex);
				if (mPreferredMetaInfoKeyList.contains(key)) {
					sortedList << key;
				}
			}
		}
		mPreferredMetaInfoKeyList = sortedList;
	}
};


PreferredImageMetaInfoModel::PreferredImageMetaInfoModel()
: d(new PreferredImageMetaInfoModelPrivate) {
	d->mModel = this;
}


PreferredImageMetaInfoModel::~PreferredImageMetaInfoModel() {
	delete d;
}


QStringList PreferredImageMetaInfoModel::preferredMetaInfoKeyList() const {
	return d->mPreferredMetaInfoKeyList;
}


void PreferredImageMetaInfoModel::setPreferredMetaInfoKeyList(const QStringList& keyList) {
	d->mPreferredMetaInfoKeyList = keyList;
	emit preferredMetaInfoKeyListChanged(d->mPreferredMetaInfoKeyList);
}


Qt::ItemFlags PreferredImageMetaInfoModel::flags(const QModelIndex& index) const {
	Qt::ItemFlags fl = QAbstractItemModel::flags(index);
	if (index.parent().isValid() && index.column() == 0) {
		fl |= Qt::ItemIsUserCheckable;
	}
	return fl;
}


QVariant PreferredImageMetaInfoModel::data(const QModelIndex& index, int role) const {
	if (!index.isValid()) {
		return QVariant();
	}

	switch (role) {
	case Qt::CheckStateRole:
		return d->checkStateData(index);

	default:
		return ImageMetaInfoModel::data(index, role);
	}
}


bool PreferredImageMetaInfoModel::setData(const QModelIndex& index, const QVariant& value, int role) {
	if (role != Qt::CheckStateRole) {
		return false;
	}

	if (!index.parent().isValid()) {
		return false;
	}

	QString key = keyForIndex(index);
	if (value == Qt::Checked) {
		d->mPreferredMetaInfoKeyList << key;
		d->sortPreferredMetaInfoKeyList();
	} else {
		d->mPreferredMetaInfoKeyList.removeAll(key);
	}
	emit preferredMetaInfoKeyListChanged(d->mPreferredMetaInfoKeyList);
	emit dataChanged(index, index);
	return true;
}


} // namespace
