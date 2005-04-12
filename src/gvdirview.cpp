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

// Qt
#include <qdir.h>
#include <qheader.h>
#include <qpopupmenu.h>
#include <qstylesheet.h>
#include <qtimer.h>

// KDE
#include <kdebug.h>
#include <kdeversion.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpropsdlg.h>
#include <kurl.h>
#include <kurldrag.h>

// Local
#include "fileoperation.h"
#include "gvbranchpropertiesdialog.h"
#include "gvdirview.moc"

//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

const int AUTO_OPEN_DELAY=1000;
const int DND_ICON_COUNT=8;
const char* DND_PREFIX="dnd";
const char* CONFIG_BRANCH_URL="url";
const char* CONFIG_BRANCH_ICON="icon";
const char* CONFIG_BRANCH_TITLE="title";
const char* CONFIG_NUM_BRANCHES="num branches";

static QString dirSyntax(const QString &d) {
	if(!d.isEmpty()) {
		QString ds(d);

		ds.replace("//", "/");

		int slashPos=ds.findRev('/');

		if(slashPos!=(((int)ds.length())-1))
			ds.append('/');

		return ds;
	}

	return d;
}

GVFileTreeBranch::GVFileTreeBranch(KFileTreeView *tv, const KURL& url, const QString& title, const QString& icon)
: KFileTreeBranch(tv, url, title, SmallIcon(icon)),
mIcon(icon)
{
}

GVDirView::GVDirView(QWidget* parent) : KFileTreeView(parent),mDropTarget(0) {
	// Look tweaking
	addColumn(QString::null);
	header()->hide();
	setAllColumnsShowFocus(true);
	setRootIsDecorated(true);

	// Popup
	mPopupMenu=new QPopupMenu(this);
	mPopupMenu->insertItem(SmallIcon("folder_new"),i18n("New Folder..."),this,SLOT(makeDir()));
	mPopupMenu->insertSeparator();
	mPopupMenu->insertItem(i18n("Rename..."),this,SLOT(renameDir()));
	mPopupMenu->insertItem(SmallIcon("editdelete"),i18n("Delete"),this,SLOT(removeDir()));
	mPopupMenu->insertSeparator();
	mPopupMenu->insertItem(i18n("Properties"),this,SLOT(showPropertiesDialog()));

	mBranchPopupMenu=new QPopupMenu(this);
	mBranchNewFolderItem=mBranchPopupMenu->insertItem(SmallIcon("folder_new"),i18n("New Folder..."),this,
				SLOT(makeDir()));
	mBranchPopupMenu->insertSeparator();
	mBranchPopupMenu->insertItem(i18n("New Branch..."),this,SLOT(makeBranch()));
	mBranchPopupMenu->insertItem(SmallIcon("editdelete"),i18n("Delete Branch"),this,SLOT(removeBranch()));
	mBranchPopupMenu->insertItem(i18n("Properties"),this,SLOT(showBranchPropertiesDialog()));

	connect(this,SIGNAL(contextMenu(KListView*,QListViewItem*,const QPoint&)),
		this,SLOT(slotContextMenu(KListView*,QListViewItem*,const QPoint&)));

	// Dir selection
	connect(this, SIGNAL(selectionChanged(QListViewItem*)),
		this, SLOT(slotExecuted()) );

	// Drag'n'drop
	setDragEnabled(true);
	setDropVisualizer(false);
	setDropHighlighter(true);
	setAcceptDrops(true);

	mAutoOpenTimer=new QTimer(this);
	connect(mAutoOpenTimer, SIGNAL(timeout()),
		this,SLOT(autoOpenDropTarget()));
}


//------------------------------------------------------------------------
//
// Config
//
//------------------------------------------------------------------------

static QString branchGroupKey(const QString &group, unsigned int num) {
	QString grp;
	grp.sprintf("%s - branch:%d", group.latin1(), num+1);
	return grp;
}

void GVDirView::readConfig(KConfig* config, const QString& group) {
	// Note: Previously the number of branches was not saved -> use a default of 20
	// so that any old branches are still used.
	unsigned int numBranches=config->readNumEntry(CONFIG_NUM_BRANCHES, 20);
	for(unsigned int num=0; num<numBranches; num++) {
		QString grp(branchGroupKey(group, num));

		if(config->hasGroup(grp)) {
			config->setGroup(grp);
			QString url;
			QString icon;
			QString title;

			url=config->readPathEntry(CONFIG_BRANCH_URL);
			icon=config->readEntry(CONFIG_BRANCH_ICON);
			title=config->readEntry(CONFIG_BRANCH_TITLE);
			if (url.isEmpty() || icon.isEmpty() || title.isEmpty()) {
				break;
			} else {
				addBranch(url, title, icon);
			}
		} else {
			break;
		}
	}
	if (0==mBranches.count()) {
		defaultBranches();
	}
}


void GVDirView::writeConfig(KConfig* config, const QString& group) {
	GVFileTreeBranch *branch;
	unsigned int num=0;
	unsigned int oldBranches=config->readNumEntry(CONFIG_NUM_BRANCHES);

	config->writeEntry(CONFIG_NUM_BRANCHES, mBranches.count());
	for (branch=mBranches.first(); branch; branch=mBranches.next(), num++) {
		config->setGroup(branchGroupKey(group, num));
		if (branch->rootUrl().isLocalFile()) {
			config->writePathEntry(CONFIG_BRANCH_URL, branch->rootUrl().path());
		} else {
			config->writeEntry(CONFIG_BRANCH_URL, branch->rootUrl().prettyURL());
		}
		config->writeEntry(CONFIG_BRANCH_ICON, branch->icon());
		config->writeEntry(CONFIG_BRANCH_TITLE, branch->name());
	}

	if(oldBranches>mBranches.count()) {
		for(num=mBranches.count(); num<oldBranches; ++num) {
			config->deleteGroup(branchGroupKey(group, num));
		}
	}
}


/**
 * We override showEvent to make sure the view shows the correct
 * dir when it's shown. Since the view doesn't update if it's
 * hidden
 */
void GVDirView::showEvent(QShowEvent* event) {
	if (!currentURL().equals(m_nextUrlToSelect,true)) {
		setURLInternal(m_nextUrlToSelect);
	}
	QWidget::showEvent(event);
}


void GVDirView::setURL(const KURL& url) {
	LOG(url.prettyURL() << ' ' << filename);

	// Do nothing if we're browsing remote files
	if (!url.isLocalFile()) return;

	if (currentURL().equals(url,true)) return;
	if (m_nextUrlToSelect.equals(url,true)) return;

	// Do not update the view if it's hidden, just store the url to
	// open next time the view is shown
	if (!isVisible()) {
		LOG("we are hidden, just store the url");
		slotSetNextUrlToSelect(url);
		return;
	}

	setURLInternal(url);
}


void GVDirView::setURLInternal(const KURL& url) {
	LOG(url.prettyURL() );
	QStringList folderParts;
	QStringList::Iterator folderIter,endFolderIter;
	QString folder="/";
	KFileTreeViewItem* viewItem;
	KFileTreeViewItem* nextViewItem;

	QString path=dirSyntax(url.path());
	KFileTreeBranch *branch,
			*bestMatch=NULL;
	for (branch=mBranches.first(); branch; branch=mBranches.next()) {
		if(branch->rootUrl().protocol()==url.protocol() &&
		   path.startsWith(branch->rootUrl().path()) &&
		   (!bestMatch || branch->rootUrl().path().length() > bestMatch->rootUrl().path().length())) {
			bestMatch=branch;
		}
	}

	if(bestMatch) {
		viewItem=static_cast<KFileTreeViewItem*>(bestMatch->root());
		if(bestMatch->rootUrl().path()!="/") {
			path.remove(0,bestMatch->rootUrl().path().length());
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
		if (viewItem->url().equals(url,true)) {
			setCurrentItem(viewItem);
			ensureItemVisible(viewItem);
			slotSetNextUrlToSelect(KURL());
		} else {
			slotSetNextUrlToSelect(url);
		}
	}
}


//-Protected slots---------------------------------------------------------
/**
 * Override this method to make sure that the item is selected, opened and
 * visible
 */
void GVDirView::slotNewTreeViewItems( KFileTreeBranch* branch, const KFileTreeViewItemList& itemList ) {
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
	if (!item) return;
	KURL url=item->url();

	if (mDropTarget) {
		startAnimation(mDropTarget,DND_PREFIX,DND_ICON_COUNT);
	}

	// We reached the URL to select, get out
	if (url.equals(m_nextUrlToSelect, true)) return;

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


void GVDirView::addBranch(const QString& url, const QString& title, const QString& icon) {
	GVFileTreeBranch *branch= new GVFileTreeBranch(this, KURL(dirSyntax(url)), title, icon);
	KFileTreeView::addBranch(branch);
	setDirOnlyMode(branch,true);
	branch->root()->setExpandable(true);
	branch->setChildRecurse(false);

	connect(branch,SIGNAL( populateFinished(KFileTreeViewItem*) ),
		this, SLOT( slotDirViewPopulateFinished(KFileTreeViewItem*) ) );

	connect(branch,SIGNAL( refreshItems(const KFileItemList&)),
		this, SLOT( slotItemsRefreshed(const KFileItemList&) ) );
	mBranches.append(branch);
}

void GVDirView::defaultBranches() {
	addBranch(QDir::homeDirPath(),i18n("Home Folder"),"folder_home");
	addBranch("/",i18n("Root Folder"),"folder_red");
}

//-Drag'n drop------------------------------------------------------
void GVDirView::contentsDragMoveEvent(QDragMoveEvent* event) {
	if (!KURLDrag::canDecode(event)) {
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
			if (current.equals(*it,true)) {
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
void GVDirView::slotContextMenu(KListView*,QListViewItem* item,const QPoint& pos) {
	if(item && item->parent()) {
		mPopupMenu->popup(pos);
	} else {
		mBranchPopupMenu->setItemEnabled(mBranchNewFolderItem, item);
		mBranchPopupMenu->popup(pos);
	}
}


void GVDirView::makeDir() {
	KIO::Job* job;
	if (!currentItem()) return;

	bool ok;
	QString newDir=KInputDialog::getText(
			i18n("Creating Folder"),
			i18n("Enter the name of the new folder:"),
			QString::null, &ok, this);
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
	QString newDir=KInputDialog::getText(
		i18n("Renaming Folder"),
		i18n("Rename this folder to:"),
		currentURL().filename(), &ok, this);
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
	int response=KMessageBox::warningContinueCancel(this,
		"<qt>" + i18n("Are you sure you want to delete the folder <b>%1</b>?").arg(dir) + "</qt>",
		i18n("Delete Folder"),
#if KDE_IS_VERSION(3, 3, 0)
		KStdGuiItem::del()
#else
		KGuiItem( i18n( "&Delete" ), "editdelete", i18n( "Delete item(s)" ) )
#endif
		);
	if (response==KMessageBox::Cancel) return;

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
	(void)new KPropertiesDialog(currentURL(),this);
}

void GVDirView::makeBranch() {
	showBranchPropertiesDialog(0L);
}

void GVDirView::removeBranch() {
	QListViewItem* li=selectedItem();
	KFileTreeBranch *br=li ? branch(li->text(0)) : 0L;

	if (br && KMessageBox::Continue==KMessageBox::warningContinueCancel(this,
			"<qt>" + i18n("Do you really want to remove\n <b>'%1'</b>?").arg(li->text(0))
			+ "</qt>")) {
		mBranches.remove(static_cast<GVFileTreeBranch*>(br));
		KFileTreeView::removeBranch(br);
		if (0==childCount()) {
			KMessageBox::information(this,
				"<qt>" + i18n("You have removed all folders. The list will now rollback to the default.")
				+ "</qt>");
			defaultBranches();
		}
	}
}

void GVDirView::showBranchPropertiesDialog() {
	QListViewItem* li=selectedItem();
	KFileTreeBranch *br=li ? branch(li->text(0)) : 0L;

	if (br) {
		showBranchPropertiesDialog(static_cast<GVFileTreeBranch*>(br));
	}
}

void GVDirView::showBranchPropertiesDialog(GVFileTreeBranch* editItem)
{
	GVBranchPropertiesDialog dialog(this);

	if(editItem) {
		dialog.setContents(editItem->icon(), editItem->name(), editItem->rootUrl().prettyURL());
	}

	if(QDialog::Accepted==dialog.exec()) {
		KURL newUrl(dirSyntax(dialog.url()));

		if (editItem) {
			if (dialog.icon()!=editItem->icon() || newUrl!=editItem->rootUrl()) {
				mBranches.remove(editItem);
				KFileTreeView::removeBranch(editItem);
				addBranch(dialog.url(), dialog.title(), dialog.icon());
			} else {
				if (dialog.title()!=editItem->name()) {
					editItem->setName(dialog.title());
				}
			}
		} else {
			if (NULL!=branch(dialog.title())) {
				KMessageBox::error(this,
					"<qt>"+i18n("An entry already exists with the title \"%1\".")
					.arg(dialog.title())+"</qt>");
			} else {
				KFileTreeBranch *existingBr;
				for (existingBr=mBranches.first(); existingBr; existingBr=mBranches.next()) {
					if(existingBr->rootUrl()==newUrl) {
						break;
					}
				}
				if (existingBr) {
					KMessageBox::error(this,
						"<qt>"+i18n("An entry already exists with the URL \"%1\".")
						.arg(dialog.url())+"</qt>");
				} else {
					addBranch(dialog.url(), dialog.title(), dialog.icon());
				}
			}
		}
	}
}

//- This code should go in KFileTreeBranch -------------------------------
void GVDirView::refreshBranch(KFileItem* item, KFileTreeBranch* branch) {
	KFileTreeViewItem* tvItem=
		static_cast<KFileTreeViewItem*>( item->extraData(branch) );
	if (!tvItem) return;

	QString oldText=tvItem->text(0);
	QString newText=item->text();
	if (oldText!=newText) {
		tvItem->setText(0, newText);
		KURL oldURL(item->url());
		oldURL.setFileName(oldText);
		emit dirRenamed(oldURL, item->url());
	}
}

void GVDirView::slotItemsRefreshed(const KFileItemList& items) {
	KFileItemListIterator it(items);
	for (;it.current(); ++it) {
		KFileItem* item=it.current();
		KFileTreeBranch *branch;
		for (branch=mBranches.first(); branch; branch=mBranches.next()) {
			refreshBranch(item, branch);
		}
	}
}

