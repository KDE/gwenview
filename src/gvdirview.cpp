/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2003 Aurélien Gâteau

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

// Qt 
#include <qdragobject.h>
#include <qdir.h>
#include <qheader.h>
#include <qpopupmenu.h>
#include <qstylesheet.h>
#include <qtimer.h>

// KDE 
#include <kdebug.h>
#include <kdeversion.h>
#include <kiconloader.h>
#include <klineeditdlg.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpropsdlg.h>
#include <kurl.h>
#include <kurldrag.h>

// Local
#include "fileoperation.h"
#include "gvdirview.moc"

#if KDE_VERSION < 306
#define SAME_URL(u1,u2,slash) u1.cmp(u2,slash)
#else
#define SAME_URL(u1,u2,slash) u1.equals(u2,slash)
#endif

		
const int AUTO_OPEN_DELAY=1000;
const int DND_ICON_COUNT=8;
const char* DND_PREFIX="dnd";


GVDirView::GVDirView(QWidget* parent) : KFileTreeView(parent),mDropTarget(0) {
	// Look tweaking
	addColumn(QString::null);
	header()->hide();
	setAllColumnsShowFocus(true);
	setRootIsDecorated(true);

	// Create branches
	mHomeBranch=addBranch(KURL(QDir::homeDirPath()),i18n("Home Directory"),SmallIcon("folder_home"));
	mRootBranch=addBranch(KURL("/"),i18n("Root Directory"),SmallIcon("folder_red"));
	setDirOnlyMode(mHomeBranch,true);
	setDirOnlyMode(mRootBranch,true);
	mHomeBranch->root()->setExpandable(true);
	mRootBranch->root()->setExpandable(true);

	connect(mHomeBranch,SIGNAL( populateFinished(KFileTreeViewItem*) ),
		this, SLOT( slotDirViewPopulateFinished(KFileTreeViewItem*) ) );
	connect(mRootBranch,SIGNAL( populateFinished(KFileTreeViewItem*) ),
		this, SLOT( slotDirViewPopulateFinished(KFileTreeViewItem*) ) );

	// Popup
	mPopupMenu=new QPopupMenu(this);
	mPopupMenu->insertItem(SmallIcon("folder_new"),i18n("New Folder..."),this,SLOT(makeDir()));
	mPopupMenu->insertSeparator();
	mPopupMenu->insertItem(i18n("Rename..."),this,SLOT(renameDir()));
	mPopupMenu->insertItem(SmallIcon("editdelete"),i18n("Delete"),this,SLOT(removeDir()));
	mPopupMenu->insertSeparator();
	mPopupMenu->insertItem(i18n("Properties"),this,SLOT(showPropertiesDialog()));

	connect(this,SIGNAL(contextMenu(KListView*,QListViewItem*,const QPoint&)),
		this,SLOT(slotContextMenu(KListView*,QListViewItem*,const QPoint&)));

	// Dir selection
	connect(this,SIGNAL(executed(QListViewItem*)),
		this,SLOT(slotExecuted()) );

	// Drag'n'drop
	setDragEnabled(true);
	setDropVisualizer(false);
	setDropHighlighter(true);
	setAcceptDrops(true);

	mAutoOpenTimer=new QTimer(this);
	connect(mAutoOpenTimer, SIGNAL(timeout()),
		this,SLOT(autoOpenDropTarget()));
}


/**
 * We override showEvent to make sure the view shows the correct
 * dir when it's shown. Since the view doesn't update if it's
 * hidden
 */
void GVDirView::showEvent(QShowEvent* event) {
	if (!SAME_URL(currentURL(),m_nextUrlToSelect,true)) {
		setURLInternal(m_nextUrlToSelect);
	}	
	QWidget::showEvent(event);
}


void GVDirView::setURL(const KURL& url) {
	//kdDebug() << "GVDirView::setURL " << url.prettyURL() << ' ' << filename << endl;

	// Do nothing if we're browsing remote files
	if (!url.isLocalFile()) return;

	if (SAME_URL(currentURL(),url,true)) return;
	if (SAME_URL(m_nextUrlToSelect,url,true)) return;

	// Do not update the view if it's hidden, just store the url to
	// open next time the view is shown
	if (!isVisible()) {
		//kdDebug() << "GVDirView::setURL we are hidden, just store the url" << endl;
		slotSetNextUrlToSelect(url);
		return;
	}

	setURLInternal(url);
}

	
void GVDirView::setURLInternal(const KURL& url) {
	//kdDebug() << "GVDirView::setURLInternal " << url.prettyURL() << endl;
	QStringList folderParts;
	QStringList::Iterator folderIter,endFolderIter;
	QString folder="/";
	KFileTreeViewItem* viewItem;
	KFileTreeViewItem* nextViewItem;

	QString path=url.path();
	QString homePath=QDir::homeDirPath();
	if (path.startsWith(homePath)) {
		viewItem=static_cast<KFileTreeViewItem*>(mHomeBranch->root());
		path.remove(0,homePath.length());
	} else {
		viewItem=static_cast<KFileTreeViewItem*>(mRootBranch->root());
	}
	folderParts=QStringList::split('/',path);

	// Finds the deepest existing view item
	folderIter=folderParts.begin();
	endFolderIter=folderParts.end();
	for(;folderIter!=endFolderIter;++folderIter) {

		nextViewItem=findViewItem(viewItem,*folderIter);
		if (nextViewItem) {
			folder+=*folderIter + '/';
			viewItem=nextViewItem;
		} else {
			break;
		}
	}
	viewItem->setOpen(true);

	// If this is the wanted item, select it, 
	// otherwise set the url as the next to select
	if (SAME_URL(viewItem->url(),url,true)) {
		setCurrentItem(viewItem);
		ensureItemVisible(viewItem);
	} else {
		slotSetNextUrlToSelect(url);
	}
}


//-Protected slots---------------------------------------------------------
/**
 * Override this method to make sure that the item is selected, opened and
 * visible
 */
void GVDirView::slotNewTreeViewItems( KFileTreeBranch* branch, const KFileTreeViewItemList& itemList ) {
	if( ! branch ) return;
	kdDebug(250) << "hitting slotNewTreeViewItems" << endl;
	if(m_nextUrlToSelect.isEmpty()) return;
	
	KFileTreeViewItemListIterator it( itemList );

	for(;it.current(); ++it ) {
		KURL url = (*it)->url();

		// This is an URL to select
		// (We block signals to avoid simulating a click on the dir item)
		if (SAME_URL(m_nextUrlToSelect,url,true)) {
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
void GVDirView::slotExecuted() {
	KFileTreeViewItem* treeItem=currentKFileTreeViewItem();
	if (!treeItem) return;
	if (!treeItem->fileItem()) return;
	if (!treeItem->fileItem()->isReadable()) return;
	emit dirURLChanged(treeItem->url());
}


void GVDirView::slotDirViewPopulateFinished(KFileTreeViewItem* item) {
	QListViewItem* child;
	KURL url=item->url();

	if (mDropTarget) {
		startAnimation(mDropTarget,DND_PREFIX,DND_ICON_COUNT);
	}

	// We reached the URL to select, get out
	if (SAME_URL(url,m_nextUrlToSelect,true)) return;

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
KFileTreeViewItem* GVDirView::findViewItem(KFileTreeViewItem* parent,const QString& text) {
	QListViewItem* item;

	for (item=parent->firstChild();item;item=item->nextSibling()) {
		if (item->text(0)==text) {
			return static_cast<KFileTreeViewItem*>(item);
		}
	}
	return 0L;
}


//-Drag'n drop------------------------------------------------------
void GVDirView::contentsDragMoveEvent(QDragMoveEvent* event) {
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


void GVDirView::contentsDragLeaveEvent(QDragLeaveEvent*) {
	mAutoOpenTimer->stop();
	if (mDropTarget) {
		stopAnimation(mDropTarget);
		mDropTarget=0L;
	}
}


void GVDirView::contentsDropEvent(QDropEvent* event) {
	mAutoOpenTimer->stop();

	// Get data from drop (do it before showing menu to avoid mDropTarget changes)
	if (!mDropTarget) return;
	KURL dest=mDropTarget->url();
	
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
			if (current.cmp(*it,true)) {
				setCurrentItem(mDropTarget);
				break;
			}
		}
	}
	
	// Reset drop target
	if (mDropTarget) {
		stopAnimation(mDropTarget);
		mDropTarget=0L;
	}
}


void GVDirView::autoOpenDropTarget() {
	if (mDropTarget) {
		mDropTarget->setOpen(true);
	}
}


//- Popup ----------------------------------------------------------
void GVDirView::slotContextMenu(KListView*,QListViewItem*,const QPoint& pos) {
	mPopupMenu->popup(pos);
}


void GVDirView::makeDir() {
	KIO::Job* job;
	if (!currentItem()) return;

	bool ok;
	QString newDir=KLineEditDlg::getText(i18n("Enter the name of the new folder:"),"",&ok,this);
	if (!ok) return;
	
	KURL newURL(currentURL());
	newURL.addPath(newDir);
	job=KIO::mkdir(newURL);
	
	connect(job,SIGNAL(result(KIO::Job*)),
		this,SLOT(slotDirMade(KIO::Job*)));
}


void GVDirView::slotDirMade(KIO::Job* job) {
	if (job->error()) {
		job->showErrorDialog(this);
		return;
	}
	if (!currentItem()) return;
	currentItem()->setOpen(true);
}


void GVDirView::renameDir() {
	KIO::Job* job;
	if (!currentItem()) return;
	
	bool ok;
	QString newDir=KLineEditDlg::getText(i18n("Rename this folder to:"),currentURL().filename(),&ok,this);
	if (!ok) return;

	KURL newURL=currentURL().upURL();
	
	newURL.addPath(newDir);
	job=KIO::rename(currentURL(),newURL,false);

	connect(job,SIGNAL(result(KIO::Job*)),
		this,SLOT(slotDirRenamed(KIO::Job*)));
}


void GVDirView::slotDirRenamed(KIO::Job* job) {
	if (job->error()) {
		job->showErrorDialog(this);
		return;
	}
}


void GVDirView::removeDir() {
	KIO::Job* job;
	if (!currentItem()) return;

	QString dir=QStyleSheet::escape(currentURL().path());
	int response=KMessageBox::questionYesNo(this,
		"<qt>" + i18n("Are you sure you want to delete the folder <b>%1</b>?").arg(dir) + "</qt>");
	if (response==KMessageBox::No) return;

	job=KIO::del(currentURL());
	connect(job,SIGNAL(result(KIO::Job*)),
		this,SLOT(slotDirRemoved(KIO::Job*)));
	QListViewItem* item=currentItem();
	if (!item) return;
	item=item->parent();
	if (!item) return;
	setCurrentItem(item);
}


void GVDirView::slotDirRemoved(KIO::Job* job) {
	if (job->error()) {
		job->showErrorDialog(this);
	}
}


void GVDirView::showPropertiesDialog() {
	(void)new KPropertiesDialog(currentURL());
}

