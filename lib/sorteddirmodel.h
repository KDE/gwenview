/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
#ifndef SORTEDDIRMODEL_H
#define SORTEDDIRMODEL_H

#include <QSortFilterProxyModel>

#include "gwenviewlib_export.h"
#include "abstractthumbnailviewhelper.h"

class QPixmap;

class KDirLister;
class KFileItem;
class KUrl;

namespace Gwenview {

class SortedDirModelPrivate;
class GWENVIEWLIB_EXPORT SortedDirModel : public QSortFilterProxyModel {
	Q_OBJECT
public:
	SortedDirModel(QObject* parent);
	~SortedDirModel();
	KDirLister* dirLister();
	KFileItem itemForIndex(const QModelIndex& index) const;
	QModelIndex indexForItem(const KFileItem& item) const;
	QModelIndex indexForUrl(const KUrl& url) const;

protected:
	virtual bool lessThan(const QModelIndex&, const QModelIndex&) const;

private:
	SortedDirModelPrivate * const d;
};

} // namespace

#endif /* SORTEDDIRMODEL_H */
