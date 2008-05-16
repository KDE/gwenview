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
#include "tagmodel.h"

// Qt

// KDE

// Local
#include "abstractmetadatabackend.h"

namespace Gwenview {


struct TagModelPrivate {
};


TagModel::TagModel(QObject* parent, AbstractMetaDataBackEnd* backEnd)
: QStandardItemModel(parent)
, d(new TagModelPrivate) {
	TagSet set = backEnd->allTags();

	Q_FOREACH(const MetaDataTag& tag, set) {
		QString label = backEnd->labelForTag(tag);
		QStandardItem* item = new QStandardItem(label);
		item->setData(tag, TagRole);
		appendRow(item);
	}
}


TagModel::~TagModel() {
	delete d;
}


} // namespace
