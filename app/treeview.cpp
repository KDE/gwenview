// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aur�ien G�eau

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
#include "treeview.moc"

// Qt
#include <qheader.h>
#include <qtimer.h>

// KDE
#include <kdebug.h>
#include <kiconloader.h>
#include <kmimetype.h>
#include <kurldrag.h>

// Local
#include "../gvcore/fileoperation.h"

namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

const int AUTO_OPEN_DELAY=1000;
const int DND_ICON_COUNT=8;
const char* DND_PREFIX="dnd";


struct TreeView::Private {
	TreeView* mView;
	KFileTreeBranch* mBranch;
	KFileTreeViewItem* mDropTarget;
	QTimer* mAutoOpenTimer;

	KFileTreeViewItem* findViewItem(KFileTreeViewItem* parent,const QString& text) {
		QListViewItem* item;

		for (item=parent->firstChild();item;item=item->nextSibling()) {
			if (item->text(0)==text) {
				return static_cast<KFileTreeViewItem*>(item);
			}
		}
		return 0L;
	}
	
	void setURLInternal(const KURL& url) {
		LOG(url.prettyURL() );
		QString path=url.path();
		
		if (!mBranch || !mBranch->rootUrl().isParentOf(url)) {
			mView->createBranch(url);
			return;
		}

		// The request URL is a child of the branch, expand the branch to reach
		// the child
		LOG("Expanding to reach child");
		if(mBranch->rootUrl().path()!="/") {
			path.remove(0,mBranch->rootUrl().path().length());
		}
		LOG("Path=" << path);

		// Finds the deepest existing view item
		QStringList folderParts=QStringList::split('/',path);
		QStringList::Iterator folderIter=folderParts.begin();
		QStringList::Iterator endFolderIter=folderParts.end();
		KFileTreeViewItem* viewItem=mBranch->root();
		for(;folderIter!=endFolderIter;++folderIter) {

			KFileTreeViewItem* nextViewItem=findViewItem(viewItem,*folderIter);
			if (!nextViewItem) break;
			viewItem=nextViewItem;
		}
		LOG("Deepest existing view item=" << viewItem->url());

		// If this is the wanted item, select it,
		// otherwise set the url as the next to select
		if (viewItem->url().equals(url,true)) {
			LOG("We are done");
			mView->setCurrentItem(viewItem);
			mView->ensureItemVisible(viewItem);
			mView->slotSetNextUrlToSelect(KURL());
		} else {
			LOG("We continue");
			mView->slotSetNextUrlToSelect(url);
		}
		
		LOG("Opening deepest existing view item");
		viewItem->setOpen(true);
	}
};


TreeView::TreeView(QWidget* parent)
: KFileTreeView(parent) {
	d=new Private;
	d->mView=this;
	d->mBranch=0;
	d->mDropTarget=0;
	d->mAutoOpenTimer=new QTimer(this);
	
	// Look
	addColumn(QString::null);
	header()->hide();
	setAllColumnsShowFocus(true);
	setRootIsDecorated(false);
	setFullWidth(true);
	
	// Drag'n'drop
	setDragEnabled(true);
	setDropVisualizer(false);
	setDropHighlighter(true);
	setAcceptDrops(true);

	connect(d->mAutoOpenTimer, SIGNAL(timeout()),
		this, SLOT(autoOpenDropTarget()));
}


TreeView::~TreeView() {
	delete d;
}


void TreeView::setURL(const KURL& url) {
	LOG(url.prettyURL());
	if (currentURL().equals(url,true)) return;
	if (m_nextUrlToSelect.equals(url,true)) return;
	slotSetNextUrlToSelect(url);
	
	// Do not update the view if it's hidden, the url has been stored with
	// slotSetNextUrlToSelect. The view will expand to it next time it's shown.
	if (!isVisible()) {
		LOG("We are hidden, just store the url");
		return;
	}

	d->setURLInternal(url);
}


void TreeView::slotTreeViewPopulateFinished(KFileTreeViewItem* item) {
	QListViewItem* child;
	if (!item) return;
	KURL url=item->url();

	if (d->mDropTarget) {
		startAnimation(d->mDropTarget,DND_PREFIX,DND_ICON_COUNT);
	}

	LOG("itemURL=" << url);
	LOG("m_nextUrlToSelect=" << m_nextUrlToSelect);

	// We reached the URL to select, get out
	if (url.equals(m_nextUrlToSelect, true)) {
		slotSetNextUrlToSelect(KURL());
		return;
	}

	// This URL is not a parent of a wanted URL, get out
	if (!url.isParentOf(m_nextUrlToSelect)) return;

	// Find the next child item and open it
	LOG("Looking for next child");
	for (child=item->firstChild(); child; child=child->nextSibling()) {
		url=static_cast<KFileTreeViewItem*>(child)->url();
		if (url.isParentOf(m_nextUrlToSelect)) {
			LOG("Opening child with URL=" << url);
			ensureItemVisible(child);
			child->setOpen(true);
			return;
		}
	}
}


void TreeView::createBranch(const KURL& url) {
	if (d->mBranch) {
		removeBranch(d->mBranch);
	}
	QString title=url.prettyURL(0, KURL::StripFileProtocol);
	d->mBranch=addBranch(url, title, SmallIcon(KMimeType::iconForURL(url)) );
	setDirOnlyMode(d->mBranch, true);
	d->mBranch->setChildRecurse(false);
	d->mBranch->root()->setOpen(true);

	connect(d->mBranch, SIGNAL(populateFinished(KFileTreeViewItem*) ),
		this, SLOT(slotTreeViewPopulateFinished(KFileTreeViewItem*)) );
}
	


/**
 * Override this method to make sure that the item is selected, opened and
 * visible
 */
void TreeView::slotNewTreeViewItems(KFileTreeBranch* branch, const KFileTreeViewItemList& itemList) {
	if( ! branch ) return;
	LOG("");
	if(m_nextUrlToSelect.isEmpty()) return;

	KFileTreeViewItemListIterator it( itemList );

	for(;it.current(); ++it ) {
		KURL url = (*it)->url();

		// This is an URL to select
		// (We block signals to avoid simulating a click on the dir item)
		if (m_nextUrlToSelect.equals(url,true)) {
			blockSignals(true);
			setCurrentItem(*it);
			blockSignals(false);

			ensureItemVisible(*it);
			(*it)->setOpen(true);
			m_nextUrlToSelect = KURL();
			return;
		}
	}
}


/**
 * Override showEvent to make sure the view shows the correct
 * dir when it's shown. Since the view doesn't update if it's
 * hidden
 */
void TreeView::showEvent(QShowEvent* event) {
	LOG("m_nextUrlToSelect=" << m_nextUrlToSelect.pathOrURL());
	if (m_nextUrlToSelect.isValid() && !currentURL().equals(m_nextUrlToSelect,true)) {
		d->setURLInternal(m_nextUrlToSelect);
	}
	QWidget::showEvent(event);
}


void TreeView::contentsDragMoveEvent(QDragMoveEvent* event) {
	if (!KURLDrag::canDecode(event)) {
		event->ignore();
		return;
	}

	// Get a pointer to the new drop item
	QPoint point(0,event->pos().y());
	KFileTreeViewItem* newDropTarget=static_cast<KFileTreeViewItem*>( itemAt(contentsToViewport(point)) );
	if (!newDropTarget) {
		event->ignore();
		d->mAutoOpenTimer->stop();
		if (d->mDropTarget) {
			stopAnimation(d->mDropTarget);
			d->mDropTarget=0L;
		}
		return;
	}

	event->accept();
	if (newDropTarget==d->mDropTarget) return;
	if (d->mDropTarget) {
		stopAnimation(d->mDropTarget);
	}

	// Restart auto open timer if we are over a new item
	d->mAutoOpenTimer->stop();
	d->mDropTarget=newDropTarget;
	startAnimation(newDropTarget,DND_PREFIX,DND_ICON_COUNT);
	d->mAutoOpenTimer->start(AUTO_OPEN_DELAY,true);
}


void TreeView::contentsDragLeaveEvent(QDragLeaveEvent*) {
	d->mAutoOpenTimer->stop();
	if (d->mDropTarget) {
		stopAnimation(d->mDropTarget);
		d->mDropTarget=0L;
	}
}


void TreeView::contentsDropEvent(QDropEvent* event) {
	d->mAutoOpenTimer->stop();

	// Get data from drop (do it before showing menu to avoid mDropTarget changes)
	if (!d->mDropTarget) return;
	KURL dest=d->mDropTarget->url();

	KURL::List urls;
	if (!KURLDrag::decode(event,urls)) return;

	// Show popup
	bool wasMoved;
	FileOperation::openDropURLMenu(this, urls, dest, &wasMoved);

	if (wasMoved) {
		// If the current url was in the list, set the drop target as the new
		// current item
		KURL current=currentURL();
		KURL::List::ConstIterator it=urls.begin();
		for (; it!=urls.end(); ++it) {
			if (current.equals(*it,true)) {
				setCurrentItem(d->mDropTarget);
				break;
			}
		}
	}

	// Reset drop target
	if (d->mDropTarget) {
		stopAnimation(d->mDropTarget);
		d->mDropTarget=0L;
	}
}


void TreeView::autoOpenDropTarget() {
	if (d->mDropTarget) {
		d->mDropTarget->setOpen(true);
	}
}
} // namespace
