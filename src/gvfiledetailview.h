// vim: set tabstop=4 shiftwidth=4 noexpandtab
/* This file is based on kfiledetailview.h v1.30 from the KDE libs. Original
   copyright follows.
*/
/* This file is part of the KDE libraries
	Copyright (C) 1997 Stephan Kulow <coolo@kde.org>

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.

	You should have received a copy of the GNU Library General Public License
	along with this library; see the file COPYING.LIB.	If not, write to
	the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA 02111-1307, USA.
*/

#ifndef GVFILEDETAILVIEW_H
#define GVFILEDETAILVIEW_H

class KFileItem;
class QWidget;
class QKeyEvent;

// Qt includes
#include <qdir.h>

// KDE includes
#include <klistview.h>
#include <kmimetyperesolver.h>

// Our includes
#include "gvfileviewbase.h"

class GVFileDetailViewItem;

class GVFileDetailView : public KListView, public GVFileViewBase
{
	Q_OBJECT

	friend class GVFileDetailViewItem;

public:
	GVFileDetailView(QWidget* parent, const char* name);
	virtual ~GVFileDetailView();

	virtual QWidget* widget() { return this; }
	virtual void clearView();

	virtual void updateView( bool );
	virtual void updateView(const KFileItem*);
	virtual void removeItem( const KFileItem* );
	virtual void listingCompleted();

	virtual void setSelected(const KFileItem* , bool);
	virtual bool isSelected(const KFileItem* i) const;
	virtual void clearSelection();
	virtual void selectAll();
	virtual void invertSelection();

	virtual void setCurrentItem( const KFileItem* );
	virtual KFileItem* currentFileItem() const;
	virtual KFileItem* firstFileItem() const;
	virtual KFileItem* nextItem( const KFileItem* ) const;
	virtual KFileItem* prevItem( const KFileItem* ) const;

	virtual void insertItem( KFileItem* i );

	// implemented to get noticed about sorting changes (for sortingIndicator)
	virtual void setSorting( QDir::SortSpec );

	void ensureItemVisible( const KFileItem*  );

	// for KMimeTypeResolver
	void mimeTypeDeterminationFinished();
	void determineIcon( GVFileDetailViewItem* item );
	QScrollView* scrollWidget() { return this; }
	
	void setShownFileItem(KFileItem* fileItem);

signals:
	void dropped(QDropEvent* event, KFileItem* item);
	void sortingChanged(QDir::SortSpec);
	
protected:
	virtual bool acceptDrag(QDropEvent*) const;
	virtual void contentsDropEvent(QDropEvent*);
	virtual void keyPressEvent(QKeyEvent*);

	int mSortingCol;

protected slots:
	void slotSelectionChanged();

private slots:
	void slotSortingChanged( int );
	void selected( QListViewItem* item );
	void slotActivate( QListViewItem* item );
	void highlighted( QListViewItem* item );
	void slotActivateMenu ( QListViewItem* item, const QPoint& pos );

private:
	bool mBlockSortingSignal;
	KMimeTypeResolver<GVFileDetailViewItem,GVFileDetailView>* mResolver;

	virtual void insertItem(QListViewItem* i) { KListView::insertItem(i); }
	virtual void setSorting(int i, bool b) { KListView::setSorting(i, b); }
	virtual void setSelected(QListViewItem* i, bool b) { KListView::setSelected(i, b); }

	GVFileDetailViewItem* viewItem( const KFileItem* item ) const {
		if (item) return (GVFileDetailViewItem*)item->extraData(this);
		return 0L;
	}

	void setSortingKey(GVFileDetailViewItem* item, const KFileItem* i);
	
	void startDrag();
};


#endif
