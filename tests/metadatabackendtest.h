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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#ifndef METADATABACKENDTEST_H
#define METADATABACKENDTEST_H

// STL
#include <memory>

// Qt
#include <QHash>
#include <QObject>

// KDE
#include <kurl.h>

// Local
#include <lib/metadata/abstractmetadatabackend.h>

namespace Gwenview {

/**
 * Helper class which gathers the metadata retrieved when
 * AbstractMetaDataBackEnd::retrieveMetaData() is called.
 */
class MetaDataBackEndClient : public QObject {
	Q_OBJECT
public:
	MetaDataBackEndClient(AbstractMetaDataBackEnd*);

	MetaData metaDataForUrl(const KUrl& url) const {
		return mMetaDataForUrl.value(url);
	}

private Q_SLOTS:
	void slotMetaDataRetrieved(const KUrl&, const MetaData&);

private:
	QHash<KUrl, MetaData> mMetaDataForUrl;
	AbstractMetaDataBackEnd* mBackEnd;
};


class MetaDataBackEndTest : public QObject {
	Q_OBJECT

private Q_SLOTS:
	void initTestCase();
	void init();
	void testRating();
	void testTagForLabel();

private:
	std::auto_ptr<AbstractMetaDataBackEnd> mBackEnd;
};

} // namespace

#endif // METADATABACKENDTEST_H
