/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#ifndef THUMBNAILVIEW_H
#define THUMBNAILVIEW_H

#include "../gwenviewlib_export.h"

// Qt
#include <QListView>

// KDE
#include <kurl.h>

class KFileItem;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QPixmap;

namespace Gwenview {

class AbstractThumbnailViewHelper;

class ThumbnailViewPrivate;
class GWENVIEWLIB_EXPORT ThumbnailView : public QListView {
	Q_OBJECT
public:
	struct Thumbnail {
		Thumbnail(const QPixmap& pixmap);
		Thumbnail(const Thumbnail& other);
		Thumbnail();
		QPixmap mPixmap;
		bool mOpaque;
	};

	ThumbnailView(QWidget* parent);
	~ThumbnailView();

	void setThumbnailViewHelper(AbstractThumbnailViewHelper* helper);

	AbstractThumbnailViewHelper* thumbnailViewHelper() const;

	/**
	 * Returns the thumbnail size.
	 */
	int thumbnailSize() const;

	/**
	 * Returns the width of an item. This width is proportional to the
	 * thumbnail size.
	 */
	int itemWidth() const;

	/**
	 * Returns the height of an item. This width is proportional to the
	 * thumbnail size.
	 */
	int itemHeight() const;

	Thumbnail thumbnailForIndex(const QModelIndex&);

	/**
	 * Returns true if the document pointed by the index has been modified
	 * inside Gwenview.
	 */
	bool isModified(const QModelIndex&) const;

Q_SIGNALS:
	/**
	 * It seems we can't use the 'activated()' signals for now because it does
	 * not now about KDE single vs doubleclick settings. The indexActivated()
	 * signals replaces it for now.
	 */
	void indexActivated(const QModelIndex&);
	void saveDocumentRequested(const KUrl&);
	void rotateDocumentLeftRequested(const KUrl&);
	void rotateDocumentRightRequested(const KUrl&);
	void showDocumentInFullScreenRequested(const KUrl&);
	void urlListDropped(const KUrl::List& lst, const KUrl& destination);

public Q_SLOTS:
	/**
	 * Sets the thumbnail size, in pixels.
	 */
	void setThumbnailSize(int pixel);

protected:
	virtual void dragEnterEvent(QDragEnterEvent*);

	virtual void dragMoveEvent(QDragMoveEvent*);

	virtual void dropEvent(QDropEvent*);

	virtual void keyPressEvent(QKeyEvent*);

	virtual void resizeEvent(QResizeEvent*);

	virtual void scrollContentsBy(int dx, int dy);

protected Q_SLOTS:
	virtual void rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end);
	virtual void rowsInserted(const QModelIndex& parent, int start, int end);
	virtual void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

private Q_SLOTS:
	void showContextMenu();
	void setThumbnail(const KFileItem&, const QPixmap&);

	void slotSaveClicked();
	void slotRotateLeftClicked();
	void slotRotateRightClicked();
	void slotFullScreenClicked();
	void generateThumbnailsForVisibleItems();

private:
	ThumbnailViewPrivate * const d;
};

} // namespace

#endif /* THUMBNAILVIEW_H */
