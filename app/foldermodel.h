// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
#ifndef FOLDERMODEL_H
#define FOLDERMODEL_H

// Qt
#include <QAbstractItemModel>

// KDE

// Local

class KUrl;

namespace Gwenview {


class FolderModelPrivate;
class FolderModel : public QAbstractItemModel {
	Q_OBJECT
public:
	FolderModel(QObject*);
	~FolderModel();

	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex& index) const;
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual bool hasChildren(const QModelIndex& parent) const;
	virtual bool canFetchMore(const QModelIndex& parent) const;
	virtual void fetchMore(const QModelIndex& parent);

	KUrl urlForIndex(const QModelIndex&) const;

private Q_SLOTS:
	void slotPlacesRowsInserted(const QModelIndex&, int start, int end);
	void slotPlacesRowsAboutToBeRemoved(const QModelIndex&, int start, int end);
	void slotDirRowsAboutToBeInserted(const QModelIndex&, int start, int end);
	void slotDirRowsInserted(const QModelIndex&, int start, int end);
	void slotDirRowsAboutToBeRemoved(const QModelIndex&, int start, int end);
	void slotDirRowsRemoved(const QModelIndex&, int start, int end);

private:
	friend class FolderModelPrivate;
	FolderModelPrivate* const d;
};


} // namespace

#endif /* FOLDERMODEL_H */
