/*
Gwenview - A simple image viewer for KDE
Copyright (C) 2000-2002 Aurélien Gâteau

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

// KDE includes
#include <kiconview.h>

// Our includes
#include "gvfileview.h"
#include "thumbnailsize.h"

class QIconViewItem;
class QPopupMenu;

class KConfig;
class KFileItem;

class ThumbnailLoadJob;

class FileThumbnailView : public KIconView, public GVFileView {
Q_OBJECT
public:
	FileThumbnailView(QWidget* parent);
	~FileThumbnailView();

	QWidget* widget() { return this; }
	void startThumbnailUpdate();
	void stopThumbnailUpdate();

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

	KFileItem* firstFileItem() const;
	KFileItem* prevItem( const KFileItem*) const;
	KFileItem* currentFileItem() const;
	KFileItem* nextItem( const KFileItem*) const;

	/**
	 * Don't forget to call arrangeItemsInGrid to apply the changes
	 */
	void setThumbnailSize(ThumbnailSize value);
	ThumbnailSize thumbnailSize() const { return mThumbnailSize; }

	/**
	 * Don't forget to call arrangeItemsInGrid to apply the changes
	 */
	void setMarginSize(int value);
	int marginSize() const { return mMarginSize; }

	void readConfig(KConfig*,const QString&);
	void writeConfig(KConfig*,const QString&) const;

	const QIconViewItem* viewedItem() const;
	void setViewedFileItem(const KFileItem*);

public slots:
	void setThumbnailPixmap(const KFileItem*,const QPixmap&);

signals:
	void updateStarted(int);
	void updateEnded();
	void updatedOneThumbnail();

protected:
	void startDrag();

private:
	ThumbnailSize mThumbnailSize;
	int mMarginSize;
	QIconViewItem* mViewedItem;

	QGuardedPtr<ThumbnailLoadJob> mThumbnailLoadJob;

	void updateGrid();

private slots:
	void slotClicked(QIconViewItem*,const QPoint& pos);
};


#endif
