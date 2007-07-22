// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
#ifndef THUMBNAILVIEWHELPER_H
#define THUMBNAILVIEWHELPER_H

// Qt
#include <QObject>

// KDE

// Local
#include <lib/abstractthumbnailviewhelper.h>

class SortedDirModel;
class QPixmap;

class KFileItem;

namespace Gwenview {

class SortedDirModel;
class FileOpsContextManagerItem;

class ThumbnailViewHelperPrivate;
class ThumbnailViewHelper : public QObject, public AbstractThumbnailViewHelper {
	Q_OBJECT
public:
	ThumbnailViewHelper(SortedDirModel* model);
	~ThumbnailViewHelper();

	virtual void generateThumbnailsForItems(const QList<KFileItem>& list, int size);

	virtual void showContextMenuForItems(QWidget* parent, const QList<KFileItem>& list);

	virtual void showContextMenuForViewport(QWidget* parent);

	void setFileOpsContextManagerItem(FileOpsContextManagerItem* item);

private Q_SLOTS:
	void setItemPreview(const KFileItem&, const QPixmap&);

private:
	ThumbnailViewHelperPrivate* const d;
};


} // namespace

#endif /* THUMBNAILVIEWHELPER_H */
