// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aurélien Gâteau

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#ifndef FILETHUMBNAILVIEW_H
#define FILETHUMBNAILVIEW_H

// Qt includes
#include <qguardedptr.h>
#include <qptrlist.h>

// KDE includes
#include <kiconview.h>

// Our includes
#include "fileviewbase.h"
#include "busylevelmanager.h"
#include "libgwenview_export.h"

class QDragEnterEvent;
class QIconViewItem;
class QPopupMenu;
class QShowEvent;

class KConfig;
class KFileItem;
typedef QPtrList<KFileItem> KFileItemList;

namespace Gwenview {
class FileThumbnailViewItem;

class LIBGWENVIEW_EXPORT FileThumbnailView : public KIconView, public FileViewBase {
Q_OBJECT
	friend class FileThumbnailViewItem;
	
public:
	enum ItemDetail { FILENAME=1, FILESIZE=2, FILEDATE=4, IMAGESIZE=8 };
	FileThumbnailView(QWidget* parent);
	~FileThumbnailView();

	QWidget* widget() { return this; }

	// KFileView methods
	void clearView();
	void clearSelection() { QIconView::clearSelection(); }
	void insertItem(KFileItem* item);
	void ensureItemVisible(const KFileItem* item);
	void setCurrentItem(const KFileItem* item);
	void setSelected(const KFileItem* item,bool enable);
	bool isSelected(const KFileItem* item) const;
	void removeItem(const KFileItem* item);
	void updateView(const KFileItem* item);
	void setSorting(QDir::SortSpec);

	KFileItem* firstFileItem() const;
	KFileItem* prevItem( const KFileItem*) const;
	KFileItem* currentFileItem() const;
	KFileItem* nextItem( const KFileItem*) const;

	void setThumbnailSize(int value);
	int thumbnailSize() const;

	void setMarginSize(int value);
	int marginSize() const;

	void setItemDetails(int);
	int itemDetails() const;
	
	void setItemTextPos(ItemTextPos);

	void setShownFileItem(KFileItem*);

	void updateThumbnail(const KFileItem*);

public slots:
	void setThumbnailPixmap(const KFileItem*,const QPixmap&, const QSize&);
	void startThumbnailUpdate();
	void stopThumbnailUpdate();

	void showThumbnailDetailsDialog();

signals:
	void dropped(QDropEvent*, KFileItem* target);

protected:
	void showEvent(QShowEvent*);
	void contentsDragEnterEvent(QDragEnterEvent*);
	void startDrag();
	virtual void keyPressEvent( QKeyEvent* );

private:
	class Private;
	Private* d;

	void updateGrid();
	QPixmap createItemPixmap(const KFileItem*) const;
	void doStartThumbnailUpdate(const KFileItemList*);
	void setSortingKey(QIconViewItem*, const KFileItem*);
	void updateVisibilityInfo( int x, int y );

private slots:
	void slotClicked(QIconViewItem*);
	void slotDoubleClicked(QIconViewItem*);
	void slotDropped(QDropEvent*);
	void slotContentsMoving( int, int );
	void slotCurrentChanged(QIconViewItem*);
	void slotBusyLevelChanged( BusyLevel );
	void slotUpdateEnded();
	void prefetchDone();
};


} // namespace
#endif

