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
#ifndef ABSTRACTMETADATABACKEND_H
#define ABSTRACTMETADATABACKEND_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QObject>
#include <QSet>

// KDE

// Local

class KUrl;

namespace Gwenview {


typedef QString MetaDataTag;


/**
 * This class represents the set of tags associated to an url.
 *
 * It provides convenience methods to convert to and from QVariant, which are
 * useful to communicate with MetaDataDirModel.
 */
class GWENVIEWLIB_EXPORT TagSet : public QSet<MetaDataTag> {
public:
	TagSet();
	TagSet(const QSet<MetaDataTag>&);

	QVariant toVariant() const;
	static TagSet fromVariant(const QVariant&);
};


/**
 * A POD struct used by AbstractMetaDataBackEnd to store the metadata
 * associated to an url.
 */
struct MetaData {
	int mRating;
	QString mDescription;
	TagSet mTags;
};


/**
 * An abstract class, used by MetaDataDirModel to store and retrieve metadata.
 */
class AbstractMetaDataBackEnd : public QObject {
	Q_OBJECT
public:
	AbstractMetaDataBackEnd(QObject* parent);

	virtual void storeMetaData(const KUrl&, const MetaData&) = 0;

	virtual void retrieveMetaData(const KUrl&) = 0;

	virtual QString labelForTag(const MetaDataTag&) const = 0;

	virtual MetaDataTag tagForLabel(const QString&) const = 0;

Q_SIGNALS:
	void metaDataRetrieved(const KUrl&, const MetaData&);
};


} // namespace

#endif /* ABSTRACTMETADATABACKEND_H */
