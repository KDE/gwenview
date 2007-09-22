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

// KDE

// Local
#include <lib/abstractthumbnailviewhelper.h>

namespace Gwenview {

class FileOpsContextManagerItem;

class ThumbnailViewHelperPrivate;
class ThumbnailViewHelper : public AbstractThumbnailViewHelper {
	Q_OBJECT
public:
	ThumbnailViewHelper(QObject* parent);
	~ThumbnailViewHelper();

	virtual void generateThumbnailsForItems(const QList<KFileItem>& list);

	virtual void abortThumbnailGenerationForItems(const QList<KFileItem>& list);

	virtual void showContextMenu(QWidget* parent);

	void setFileOpsContextManagerItem(FileOpsContextManagerItem* item);

private:
	ThumbnailViewHelperPrivate* const d;
};


} // namespace

#endif /* THUMBNAILVIEWHELPER_H */
