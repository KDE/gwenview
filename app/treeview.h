// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2005 Aurelien Gateau

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
#ifndef TREEVIEW_H
#define TREEVIEW_H

// KDE
#include <kfiletreeview.h>

class KURL;
class QShowEvent;

namespace Gwenview {


class TreeView : public KFileTreeView {
Q_OBJECT
public:
	TreeView(QWidget* parent);
	~TreeView();

public slots:
	void setURL(const KURL&);
	void createBranch(const KURL&);

signals:
	void urlChanged(const KURL&);

protected:
	virtual void showEvent(QShowEvent*);
	virtual void contentsDragMoveEvent(QDragMoveEvent*);
	virtual void contentsDragLeaveEvent(QDragLeaveEvent*);
	virtual void contentsDropEvent(QDropEvent*);

protected slots:
	virtual void slotNewTreeViewItems(KFileTreeBranch*, const KFileTreeViewItemList&);

private:
	struct Private;
	Private* d;
	friend class Private;

private slots:
	// Do not name this slot "slotPopulateFinished", it will clash with
	// "KFileTreeView::slotPopulateFinished".
	void slotTreeViewPopulateFinished(KFileTreeViewItem*);

	void autoOpenDropTarget();
};


} // namespace
#endif
