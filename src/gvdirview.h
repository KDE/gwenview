/*
Gwenview - A simple image viewer for KDE
Copyright (c) 2000-2003 Aurélien Gâteau

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
#ifndef GVDIRVIEW_H
#define GVDIRVIEW_H

#include <kfiletreeview.h>

class QPopupMenu;
class QShowEvent;

class GVDirView : public KFileTreeView {
Q_OBJECT
public:
	GVDirView(QWidget* parent);

public slots:
	void setURL(const KURL&);
	
signals:
	void dirURLChanged(const KURL&);

protected:
	void showEvent(QShowEvent*);
	
// Drag'n'drop
	void contentsDragMoveEvent(QDragMoveEvent* event);
	void contentsDragLeaveEvent(QDragLeaveEvent* event);
	void contentsDropEvent(QDropEvent*);

protected slots:
	void slotNewTreeViewItems(KFileTreeBranch*,const KFileTreeViewItemList&); 
	
private slots:
	void slotExecuted(QListViewItem*);

	// Do not name this slot "slotPopulateFinished", it will clash with
	// "KFileTreeView::slotPopulateFinished".
	void slotDirViewPopulateFinished(KFileTreeViewItem*);

// Drag'n'Drop
	void autoOpenDropTarget();

// Popup menu
	void slotContextMenu(KListView*,QListViewItem*,const QPoint&);
	void makeDir();
	void renameDir();
	void removeDir();
	void showPropertiesDialog();

	void slotDirMade(KIO::Job*);
	void slotDirRenamed(KIO::Job*);
	void slotDirRemoved(KIO::Job*);

private:
	KFileTreeViewItem* findViewItem(KFileTreeViewItem*,const QString&);
	QPopupMenu* mPopupMenu;
	QTimer* mAutoOpenTimer;
	KFileTreeViewItem* mDropTarget;
	KFileTreeBranch* mHomeBranch;
	KFileTreeBranch* mRootBranch;

	// FIXME: Doc
	void setURLInternal(const KURL&);
};


#endif
