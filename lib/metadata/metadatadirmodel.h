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
#ifndef METADATADIRMODEL_H
#define METADATADIRMODEL_H

// Qt

// KDE
#include <kdirmodel.h>

// Local

class KUrl;

namespace Gwenview {


class MetaData;
class MetaDataDirModelPrivate;
/**
 * Extends KDirModel by providing read/write access to image metadata such as
 * rating, tags and descriptions.
 */
class MetaDataDirModel : public KDirModel {
	Q_OBJECT
public:
	enum {
		RatingRole = 0x21a43a51,
		DescriptionRole = 0x26FB33FA
	};
	MetaDataDirModel(QObject* parent);
	~MetaDataDirModel();

	bool metaDataAvailableForIndex(const QModelIndex&) const;

	void retrieveMetaDataForIndex(const QModelIndex&);

	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

	bool setData(const QModelIndex& index, const QVariant& data, int role = Qt::EditRole);

Q_SIGNALS:
	void metaDataRetrieved(const KUrl&, const MetaData&);

private:
	MetaDataDirModelPrivate* const d;

private Q_SLOTS:
	void storeRetrievedMetaData(const KUrl& url, const MetaData&);

	void slotRowsAboutToBeRemoved(const QModelIndex&, int, int);
	void slotModelAboutToBeReset();
};


} // namespace

#endif /* METADATADIRMODEL_H */
