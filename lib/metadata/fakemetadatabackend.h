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
#ifndef FAKEMETADATABACKEND_H
#define FAKEMETADATABACKEND_H

// Qt

// KDE

// Local
#include "abstractmetadatabackend.h"

namespace Gwenview {


/**
 * A fake metadata backend, useful to test the ui layer.
 * It provides fake rating values based on the image url.
 */
class FakeMetaDataBackEnd : public AbstractMetaDataBackEnd {
	Q_OBJECT
public:
	FakeMetaDataBackEnd(QObject* parent) : AbstractMetaDataBackEnd(parent) {}

	virtual void storeMetaData(const KUrl&, const MetaData&);

	virtual void retrieveMetaData(const KUrl&);

	virtual QString labelForTag(const MetaDataTag&) const;

	virtual MetaDataTag tagForLabel(const QString&) const;
};


} // namespace

#endif /* FAKEMETADATABACKEND_H */
