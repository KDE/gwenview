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

// Qt includes
#include <qcursor.h>
#include <qdragobject.h>
#include <qheader.h>
#include <qpopupmenu.h>
#include <qstylesheet.h>
#include <qtimer.h>

// KDE includes
#include <kdebug.h>
#include <kiconloader.h>
#include <klineeditdlg.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpropsdlg.h>
#include <kurl.h>
#include <kurldrag.h>

// Our includes
#include <dirview.moc>


const int AUTO_OPEN_DELAY=1000;
const int DND_ICON_COUNT=8;
const char* DND_PREFIX="dnd";


DirView::DirView(QWidget* parent) : KFileTreeView(parent),mDropTarget(0) {

// Look tweaking
	addColumn(i18n("Folders"));
	header()->hide();
	setAllColumnsShowFocus(true);

// Create branch
	KFileTreeBranch* rootBranch=addBranch(KURL("/"),"/");
	setDirOnlyMode(rootBranch,true);

	connect(rootBranch,SIGNAL( populateFinished(KFileTreeViewItem*) ),
		this, SLOT( onPopulateFinished(KFileTreeViewItem*) ) );

// Popup
	mPopupMenu=new QPopupMenu(this);
	mPopupMenu->insertItem(SmallIcon("folder_new"),i18n("New Folder"),this,SLOT(makeDir()));
	mPopupMenu->insertSeparator();
	mPopupMenu->insertItem(i18n("Rename..."),this,SLOT(renameDir()));
	mPopupMenu->insertItem(SmallIcon("editdelete"),i18n("Delete..."),this,SLOT(removeDir()));
	mPopupMenu->insertSeparator();
	mPopupMenu->insertItem(i18n("Properties..."),this,SLOT(showPropertiesDialog()));

	connect(this,SIGNAL(contextMenu(KListView*,QListViewItem*,const QPoint&)),
		this,SLOT(onContextMenu(KListView*,QListViewItem*,const QPoint&)));

// Dir selection
	connect(this,SIGNAL(selectionChanged()),
		this,SLOT(onSelectionChanged()) );

// Drag'n'drop
	setDragEnabled(true);
	setDropVisualizer(false);
	setDropHighlighter(true);
	setAcceptDrops(true);

	mAutoOpenTimer=new QTimer(this);
	connect(mAutoOpenTimer, SIGNAL(timeout()),
		this,SLOT(autoOpenDropTarget()));
}


void DirView::setURL(const KURL& url,const QString&) {
	//kdDebug() << "DirView::setURL " << url.path() << endl; 

	if (currentURL().cmp(url,true)) {
		//kdDebug() << "DirView::setURL : same as current\n";
		return;
	}

	QStringList folderParts;
	QStringList::Iterator folderIter,endFolderIter;
	QString folder="/";
	KFileTreeViewItem* viewItem;
	KFileTreeViewItem* nextViewItem;

	folderParts=QStringList::split('/',url.path());
	folderIter=folderParts.begin();
	endFolderIter=folderParts.end();
	viewItem=static_cast<KFileTreeViewItem*>(branch("/")->root());

// Finds the deepest existing view item
	for(;folderIter!=endFolderIter;++folderIter) {

		nextViewItem=findViewItem(viewItem,*folderIter);
		if (nextViewItem) {
			folder+=*folderIter + "/";
			viewItem=nextViewItem;
		} else {
			break;
		}
	}

// If this is the wanted item, select it, 
// otherwise set the url as the next to select
	if (viewItem->url().cmp(url,true)) {
		setCurrentItem(viewItem);
		ensureItemVisible(viewItem);
		viewItem->setOpen(true);
	} else {
		slotSetNextUrlToSelect(url);
		viewItem->setOpen(true);
	}
}


//-Protected slots---------------------------------------------------------
/**
 * Override this method to make sure that the item is selected, opened and
 * visible
 */
void DirView::slotNewTreeViewItems( KFileTreeBranch* branch, const KFileTreeViewItemList& itemList ) {
	if( ! branch ) return;
	kdDebug(250) << "hitting slotNewTreeViewItems" << endl;
	if(m_nextUrlToSelect.isEmpty()) return;
	
	KFileTreeViewItemListIterator it( itemList );

	for(;it.current(); ++it ) {
		KURL url = (*it)->url();

	// This is an URL to select
	// (We block signals to avoid simulating a click on the dir item)
		if( m_nextUrlToSelect.cmp(url, true )) {
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


//-Private slots-----------------------------------------------------------
void DirView::onSelectionChanged() {
/*	if (currentURL().isParentOf(m_nextUrlToSelect)) {
		return;
	}*/
	emit dirURLChanged(currentURL());
}


void DirView::onPopulateFinished(KFileTreeViewItem* item) {
	QListViewItem* child;
	KURL url=item->url();

	if (mDropTarget) {
		startAnimation(mDropTarget,DND_PREFIX,DND_ICON_COUNT);
	}

// We reached the URL to select, get out
	if (url.cmp(m_nextUrlToSelect,true)) return;

// This URL is not a parent of a wanted URL, get out
	if (!url.isParentOf(m_nextUrlToSelect)) return;

// Find the next child item and open it
	for (child=item->firstChild(); child; child=child->nextSibling()) {
		url=static_cast<KFileTreeViewItem*>(child)->url();
		if (url.isParentOf(m_nextUrlToSelect)) {
			ensureItemVisible(child);
			child->setOpen(true);
			return;
		}
	}
}


//-Private-----------------------------------------------------------
KFileTreeViewItem* DirView::findViewItem(KFileTreeViewItem* parent,const QString& text) {
	QListViewItem* item;

	for (item=parent->firstChild();item;item=item->nextSibling()) {
		if (item->text(0)==text) {
			return static_cast<KFileTreeViewItem*>(item);
		}
	}
	return 0L;
}


//-Drag'n drop------------------------------------------------------
void DirView::contentsDragMoveEvent(QDragMoveEvent* event) {
	if (!QUriDrag::canDecode(event)) {
		event->ignore();
		return;
	}

// Get a pointer to the new drop item
	QPoint point(0,event->pos().y());
	KFileTreeViewItem* newDropTarget=static_cast<KFileTreeViewItem*>( itemAt(contentsToViewport(point)) );
	if (!newDropTarget) {
		event->ignore();
		mAutoOpenTimer->stop();
		return;
	}

	event->accept();
	if (newDropTarget==mDropTarget) return;
	if (mDropTarget) {
		stopAnimation(mDropTarget);
	}

// Restart auto open timer if we are over a new item 
	mAutoOpenTimer->stop();
	mDropTarget=newDropTarget;
	startAnimation(newDropTarget,DND_PREFIX,DND_ICON_COUNT);
	mAutoOpenTimer->start(AUTO_OPEN_DELAY,true);
}


void DirView::contentsDragLeaveEvent(QDragLeaveEvent*) {
	mAutoOpenTimer->stop();
	if (mDropTarget) {
		stopAnimation(mDropTarget);
		mDropTarget=0L;
	}
}


void DirView::contentsDropEvent(QDropEvent* event) {
	mAutoOpenTimer->stop();

// Quit if no valid target
	if (!mDropTarget) return;

// Get data from drop (do it before showing menu to avoid mDropTarget changes)
	KURL dest=mDropTarget->url();

	KURL::List urls;
	KURLDrag::decode(event,urls);

// Show popup
	QPopupMenu menu(this);
	int copyItemID = menu.insertItem( i18n("&Copy Here") );
	int moveItemID = menu.insertItem( i18n("&Move Here") );

	menu.setMouseTracking(true);
	int id = menu.exec(QCursor::pos());

// Handle menu choice
	if (id!=-1) {
		if (id==copyItemID) {
			KIO::copy(urls, dest, true);
		} else if (id==moveItemID) {
			KIO::move(urls, dest, true);
			setCurrentItem(mDropTarget);
		}
	}

// Reset drop target
	if (mDropTarget) {
		stopAnimation(mDropTarget);
		mDropTarget=0L;
	}
}


void DirView::autoOpenDropTarget() {
	if (mDropTarget) {
		mDropTarget->setOpen(true);
	}
}


//- Popup ----------------------------------------------------------
void DirView::onContextMenu(KListView*,QListViewItem*,const QPoint& pos) {
	mPopupMenu->popup(pos);
}


void DirView::makeDir() {
	KIO::Job* job;
	if (!currentItem()) return;

	bool ok;
	QString newDir=KLineEditDlg::getText(i18n("Enter the name of the new folder:"),"",&ok,this);
	if (!ok) return;
	
	KURL newURL(currentURL());
	newURL.addPath(newDir);
	job=KIO::mkdir(newURL);
	
	connect(job,SIGNAL(result(KIO::Job*)),
		this,SLOT(onDirMade(KIO::Job*)));
}


void DirView::onDirMade(KIO::Job* job) {
	if (job->error()) {
		job->showErrorDialog(this);
		return;
	}
	if (!currentItem()) return;
	currentItem()->setOpen(true);
}


void DirView::renameDir() {
	KIO::Job* job;
	if (!currentItem()) return;
	
	bool ok;
	QString newDir=KLineEditDlg::getText(i18n("Rename this folder to:"),currentURL().filename(),&ok,this);
	if (!ok) return;

	KURL newURL=currentURL().upURL();
	
	newURL.addPath(newDir);
	job=KIO::rename(currentURL(),newURL,false);

	connect(job,SIGNAL(result(KIO::Job*)),
		this,SLOT(onDirRenamed(KIO::Job*)));
}


void DirView::onDirRenamed(KIO::Job* job) {
	if (job->error()) {
		job->showErrorDialog(this);
		return;
	}
}


void DirView::removeDir() {
	KIO::Job* job;
	if (!currentItem()) return;

	QString dir=QStyleSheet::escape(currentURL().path());
	int response=KMessageBox::questionYesNo(this,
		"<qt>" + i18n("Are you sure you want to delete the folder <b>%1</b>?").arg(dir) + "</qt>");
	if (response==KMessageBox::No) return;

	job=KIO::del(currentURL());
	connect(job,SIGNAL(result(KIO::Job*)),
		this,SLOT(onDirRemoved(KIO::Job*)));
	QListViewItem* item=currentItem();
	if (!item) return;
	item=item->parent();
	if (!item) return;
	setCurrentItem(item);
}


void DirView::onDirRemoved(KIO::Job* job) {
	if (job->error()) {
		job->showErrorDialog(this);
	}
}


void DirView::showPropertiesDialog() {
	(void)new KPropertiesDialog(currentURL());
}

