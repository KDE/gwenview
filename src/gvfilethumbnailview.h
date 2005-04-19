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
#ifndef GVFILETHUMBNAILVIEW_H
#define GVFILETHUMBNAILVIEW_H

// Qt includes
#include <qguardedptr.h>
#include <qptrlist.h>

// KDE includes
#include <kiconview.h>

// Our includes
#include "gvfileviewbase.h"
#include "gvbusylevelmanager.h"

class QDragEnterEvent;
class QIconViewItem;
class QPopupMenu;
class QShowEvent;

class KConfig;
class KFileItem;
typedef QPtrList<KFileItem> KFileItemList;

class GVFileThumbnailViewItem;
class ThumbnailLoadJob;

class GVFileThumbnailView : public KIconView, public GVFileViewBase {
Q_OBJECT
	friend class GVFileThumbnailViewItem;
	
public:
	GVFileThumbnailView(QWidget* parent);
	~GVFileThumbnailView();

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

	void setItemTextPos(ItemTextPos);
	
	void readConfig(KConfig*,const QString&);
	void writeConfig(KConfig*,const QString&) const;
	/**
	 * Used by the KParts, equivalent of readConfig(), this sets
	 * some values but just uses the defaults rather than using
	 * KConfig
	 */
	void kpartConfig();

	void setShownFileItem(KFileItem*);

	void updateThumbnail(const KFileItem*);

public slots:
	void setThumbnailPixmap(const KFileItem*,const QPixmap&, const QSize&);
	void startThumbnailUpdate();
	void stopThumbnailUpdate();

signals:
	void dropped(QDropEvent*, KFileItem* target);

protected:
	void showEvent(QShowEvent*);
	void contentsDragEnterEvent(QDragEnterEvent*);
	void startDrag();

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
	void slotBusyLevelChanged( GVBusyLevel );
	void slotUpdateEnded();
};


#endif
