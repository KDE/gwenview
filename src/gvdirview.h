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
#ifndef GVDIRVIEW_H
#define GVDIRVIEW_H

// Qt
#include <qptrlist.h>

// KDE
#include <kfiletreebranch.h>
#include <kfiletreeview.h>

class QPopupMenu;
class QShowEvent;
class KURL;

class GVFileTreeBranch : public KFileTreeBranch {
public:
	GVFileTreeBranch(KFileTreeView* tv, const KURL& url, const QString& title, const QString& icon);
	~GVFileTreeBranch() {}

	const QString& icon() const { return mIcon; }
private:
	QString mIcon;
};

class GVDirView : public KFileTreeView {
Q_OBJECT
public:
	GVDirView(QWidget* parent);

	void readConfig(KConfig* config, const QString& group);
	void writeConfig(KConfig* config, const QString& group);

public slots:
	void setURL(const KURL&);
	
signals:
	void dirURLChanged(const KURL&);
	void dirRenamed(const KURL& oldURL, const KURL& newURL);

protected:
	void showEvent(QShowEvent*);
	
// Drag'n'drop
	void contentsDragMoveEvent(QDragMoveEvent* event);
	void contentsDragLeaveEvent(QDragLeaveEvent* event);
	void contentsDropEvent(QDropEvent*);

protected slots:
	void slotNewTreeViewItems(KFileTreeBranch*,const KFileTreeViewItemList&); 
	
private slots:
	void slotExecuted();
	void slotItemsRefreshed(const KFileItemList& items);

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
	void makeBranch();
	void removeBranch();
	void showBranchPropertiesDialog();

	void slotDirMade(KIO::Job*);
	void slotDirRenamed(KIO::Job*);
	void slotDirRemoved(KIO::Job*);

private:
	KFileTreeViewItem* findViewItem(KFileTreeViewItem*,const QString&);
	void addBranch(const QString& url, const QString& title, const QString& icon);
	void defaultBranches();
	void showBranchPropertiesDialog(GVFileTreeBranch* editItem);
	QPopupMenu* mPopupMenu;
	QPopupMenu* mBranchPopupMenu;
	int mBranchNewFolderItem;
	QTimer* mAutoOpenTimer;
	KFileTreeViewItem* mDropTarget;
	QPtrList<GVFileTreeBranch> mBranches;

	/**
	 * Really defines the url, does not check if the wanted url is already the
	 * current one
	 */
	void setURLInternal(const KURL&);
	void refreshBranch(KFileItem* item, KFileTreeBranch* branch);
};


#endif
