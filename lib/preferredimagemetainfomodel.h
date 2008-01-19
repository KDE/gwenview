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
#ifndef PREFERREDIMAGEMETAINFOMODEL_H
#define PREFERREDIMAGEMETAINFOMODEL_H

#include "gwenviewlib_export.h"

// Local
#include "imagemetainfomodel.h"

namespace Gwenview {


/**
 * This model extends ImageMetaInfoModel to make it possible to select your
 * preferred image metainfo key by checking them.
 */
class PreferredImageMetaInfoModelPrivate;
class GWENVIEWLIB_EXPORT PreferredImageMetaInfoModel : public ImageMetaInfoModel {
	Q_OBJECT
public:
	PreferredImageMetaInfoModel();
	~PreferredImageMetaInfoModel();

	QStringList preferredMetaInfoKeyList() const;
	void setPreferredMetaInfoKeyList(const QStringList& keyList);

	virtual QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const;
	virtual bool setData(const QModelIndex& index, const QVariant& value, int role);
	virtual Qt::ItemFlags flags(const QModelIndex& index) const;

Q_SIGNALS:
	void preferredMetaInfoKeyListChanged(const QStringList&);

private:
	PreferredImageMetaInfoModelPrivate* const d;
	friend class PreferredImageMetaInfoModelPrivate;
};


} // namespace

#endif /* PREFERREDIMAGEMETAINFOMODEL_H */
