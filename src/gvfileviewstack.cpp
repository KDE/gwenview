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

// Qt
#include <qpopupmenu.h>

// KDE
#include <kaction.h>
#include <kapp.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kimageio.h>
#include <klocale.h>
#include <kpropertiesdialog.h>
#include <kstdaction.h>
#include <kurldrag.h>
#include <klistview.h>

// Local
#include "fileoperation.h"
#include "gvarchive.h"
#include "gvexternaltoolcontext.h"
#include "gvexternaltoolmanager.h"
#include "gvfilethumbnailview.h"
#include "gvfiledetailview.h"

#include "gvfileviewstack.moc"

static const char* CONFIG_START_WITH_THUMBNAILS="start with thumbnails";
static const char* CONFIG_AUTO_LOAD_IMAGE="automatically load first image";
static const char* CONFIG_SHOW_DIRS="show dirs";
static const char* CONFIG_SHOW_DOT_FILES="show dot files";
static const char* CONFIG_SHOWN_COLOR="shown color";

inline bool isDirOrArchive(const KFileItem* item) {
	return item && (item->isDir() || GVArchive::fileItemIsArchive(item));
}


//-----------------------------------------------------------------------
//
// GVFileViewStackPrivate
//
//-----------------------------------------------------------------------
class GVFileViewStackPrivate {
public:
	KSelectAction* mSortAction;
};


//-----------------------------------------------------------------------
//
// GVFileViewStack
//
//-----------------------------------------------------------------------
GVFileViewStack::GVFileViewStack(QWidget* parent,KActionCollection* actionCollection)
: QWidgetStack(parent), mMode(FileList)
{
	d=new GVFileViewStackPrivate;

	// Actions
	mSelectFirst=new KAction(i18n("&First"),
		QApplication::reverseLayout() ? "gvlast":"gvfirst", Key_Home,
		this,SLOT(slotSelectFirst()), actionCollection, "first");

	mSelectLast=new KAction(i18n("&Last"),
		QApplication::reverseLayout() ? "gvfirst":"gvlast", Key_End,
		this,SLOT(slotSelectLast()), actionCollection, "last");

	mSelectPrevious=new KAction(i18n("&Previous"),
		QApplication::reverseLayout() ? "gvnext":"gvprevious", Key_BackSpace,
		this,SLOT(slotSelectPrevious()), actionCollection, "previous");

	mSelectNext=new KAction(i18n("&Next"),
		QApplication::reverseLayout() ? "gvprevious":"gvnext", Key_Space,
		this,SLOT(slotSelectNext()), actionCollection, "next");

	mNoThumbnails=new KRadioAction(i18n("Details"),"view_detailed",0,this,SLOT(updateThumbnailSize()),actionCollection,"detailed");
	mNoThumbnails->setExclusiveGroup("thumbnails");
	mSmallThumbnails=new KRadioAction(i18n("Small Thumbnails"),"smallthumbnails",0,this,SLOT(updateThumbnailSize()),actionCollection,"small_thumbnails");
	mSmallThumbnails->setExclusiveGroup("thumbnails");
	mMedThumbnails=new KRadioAction(i18n("Medium Thumbnails"),"medthumbnails",0,this,SLOT(updateThumbnailSize()),actionCollection,"med_thumbnails");
	mMedThumbnails->setExclusiveGroup("thumbnails");
	mLargeThumbnails=new KRadioAction(i18n("Large Thumbnails"),"largethumbnails",0,this,SLOT(updateThumbnailSize()),actionCollection,"large_thumbnails");
	mLargeThumbnails->setExclusiveGroup("thumbnails");

	mShowDotFiles=new KToggleAction(i18n("Show &Hidden Files"),CTRL + Key_H,this,SLOT(toggleShowDotFiles()),actionCollection,"show_dot_files");

	d->mSortAction=new KSelectAction(i18n("Sort"), 0, this, SLOT(setSorting()), actionCollection, "view_sort");
	QStringList sortItems;
	sortItems << i18n("By Name") << i18n("By Date") << i18n("By Size");
	d->mSortAction->setItems(sortItems);
	d->mSortAction->setCurrentItem(0);

	// Dir lister
	mDirLister=new KDirLister;
	connect(mDirLister,SIGNAL(clear()),
		this,SLOT(dirListerClear()) );

	connect(mDirLister,SIGNAL(newItems(const KFileItemList&)),
		this,SLOT(dirListerNewItems(const KFileItemList&)) );

	connect(mDirLister,SIGNAL(deleteItem(KFileItem*)),
		this,SLOT(dirListerDeleteItem(KFileItem*)) );

	connect(mDirLister,SIGNAL(refreshItems(const KFileItemList&)),
		this,SLOT(dirListerRefreshItems(const KFileItemList&)) );

	connect(mDirLister,SIGNAL(started(const KURL&)),
		this,SLOT(dirListerStarted()) );

	connect(mDirLister,SIGNAL(completed()),
		this,SLOT(dirListerCompleted()) );

	connect(mDirLister,SIGNAL(canceled()),
		this,SLOT(dirListerCanceled()) );

	// Propagate completed and canceled signals
	connect(mDirLister,SIGNAL(completed()),
		this,SIGNAL(completed()) );
	connect(mDirLister,SIGNAL(canceled()),
		this,SIGNAL(canceled()) );

	// File detail widget
	mFileDetailView=new GVFileDetailView(this,"filedetailview");
	addWidget(mFileDetailView,0);

	connect(mFileDetailView,SIGNAL(executed(QListViewItem*)),
		this,SLOT(slotViewExecuted()) );
	connect(mFileDetailView,SIGNAL(returnPressed(QListViewItem*)),
		this,SLOT(slotViewExecuted()) );
	connect(mFileDetailView,SIGNAL(clicked(QListViewItem*)),
		this,SLOT(slotViewClicked()) );
	connect(mFileDetailView,SIGNAL(contextMenu(KListView*, QListViewItem*, const QPoint&)),
		this,SLOT(openContextMenu(KListView*, QListViewItem*, const QPoint&)) );
	connect(mFileDetailView,SIGNAL(dropped(QDropEvent*,KFileItem*)),
		this,SLOT(openDropURLMenu(QDropEvent*, KFileItem*)) );
	connect(mFileDetailView, SIGNAL(sortingChanged(QDir::SortSpec)),
		this, SLOT(updateSortMenu(QDir::SortSpec)) );
	connect(mFileDetailView, SIGNAL(doubleClicked(QListViewItem*)),
		this, SLOT(slotViewDoubleClicked()) );

	// Thumbnail widget
	mFileThumbnailView=new GVFileThumbnailView(this);
	addWidget(mFileThumbnailView,1);

	connect(mFileThumbnailView,SIGNAL(executed(QIconViewItem*)),
		this,SLOT(slotViewExecuted()) );
	connect(mFileThumbnailView,SIGNAL(returnPressed(QIconViewItem*)),
		this,SLOT(slotViewExecuted()) );
	connect(mFileThumbnailView,SIGNAL(clicked(QIconViewItem*)),
		this,SLOT(slotViewClicked()) );
	connect(mFileThumbnailView,SIGNAL(contextMenuRequested(QIconViewItem*,const QPoint&)),
		this,SLOT(openContextMenu(QIconViewItem*,const QPoint&)) );
	connect(mFileThumbnailView,SIGNAL(dropped(QDropEvent*,KFileItem*)),
		this,SLOT(openDropURLMenu(QDropEvent*, KFileItem*)) );
	connect(mFileThumbnailView, SIGNAL(doubleClicked(QIconViewItem*)),
		this, SLOT(slotViewDoubleClicked()) );

	// Propagate signals
	connect(mFileThumbnailView,SIGNAL(updateStarted(int)),
		this,SIGNAL(updateStarted(int)) );
	connect(mFileThumbnailView,SIGNAL(updateEnded()),
		this,SIGNAL(updateEnded()) );
	connect(mFileThumbnailView,SIGNAL(updatedOneThumbnail()),
		this,SIGNAL(updatedOneThumbnail()) );
}


GVFileViewStack::~GVFileViewStack() {
	delete d;
	delete mDirLister;
}


void GVFileViewStack::setFocus() {
	currentFileView()->widget()->setFocus();
}


void GVFileViewStack::setFileNameToSelect(const QString& fileName) {
	mFileNameToSelect=fileName;
}



//-----------------------------------------------------------------------
//
// Public slots
//
//-----------------------------------------------------------------------
void GVFileViewStack::setURL(const KURL& dirURL,const QString& fileName) {
	//kdDebug() << "GVFileViewStack::setURL " << dirURL.path() + " - " + fileName << endl;
	if ( !mDirURL.cmp(dirURL,true) ) {
		mDirURL=dirURL;
		currentFileView()->setShownFileItem(0L);
		mFileNameToSelect=fileName;
		mDirLister->openURL(mDirURL);
		updateActions();
	} else {
		browseTo(findItemByFileName(fileName));
	}
}


void GVFileViewStack::cancel() {
	if (!mDirLister->isFinished()) {
		mDirLister->stop();
		return;
	}
	if (mMode==Thumbnail) {
		mFileThumbnailView->stopThumbnailUpdate();
	}
}


void GVFileViewStack::slotSelectFirst() {
	browseTo(findFirstImage());
}

void GVFileViewStack::slotSelectLast() {
	browseTo(findLastImage());
}

void GVFileViewStack::slotSelectPrevious() {
	browseTo(findPreviousImage());
}

void GVFileViewStack::slotSelectNext() {
	browseTo(findNextImage());
}


void GVFileViewStack::browseTo(KFileItem* item) {
	if (!mBrowsing.tryLock()) return;
	if (item) {
		currentFileView()->setCurrentItem(item);
		currentFileView()->clearSelection();
		currentFileView()->setSelected(item,true);
		currentFileView()->ensureItemVisible(item);
		if (!item->isDir() && !GVArchive::fileItemIsArchive(item)) {
			emitURLChanged();
		}
	}
	updateActions();
	mBrowsing.unlock();
}


void GVFileViewStack::browseToFileNameToSelect() {
	// There's something to select
	if (!mFileNameToSelect.isEmpty()) {
		browseTo(findItemByFileName(mFileNameToSelect));
		mFileNameToSelect=QString::null;
		return;
	}

	// Nothing to select, but an item is already shown
	if (currentFileView()->shownFileItem()) return;

	// Now we have to make some default choice
	if (mAutoLoadImage) slotSelectFirst();

	// If no item is selected, make sure the first one is
	if (currentFileView()->selectedItems()->count()==0) {
		KFileItem* item=currentFileView()->firstFileItem();
		if (item) {
			currentFileView()->setCurrentItem(item);
			currentFileView()->setSelected(item, true);
			currentFileView()->ensureItemVisible(item);
		}
	}
}


void GVFileViewStack::updateThumbnail(const KURL& url) {
	if (mMode==FileList) return;

	KFileItem* item=mDirLister->findByURL(url);
	if (!item) return;
	mFileThumbnailView->updateThumbnail(item);
}


//-----------------------------------------------------------------------
//
// Private slots
//
//-----------------------------------------------------------------------
void GVFileViewStack::slotViewExecuted() {
	KFileItem* item=currentFileView()->currentFileItem();
	if (!item) return;

	bool isDir=item->isDir();
	bool isArchive=GVArchive::fileItemIsArchive(item);
	if (isDir || isArchive) {
		KURL tmp=url();

		if (isArchive) {
			tmp.setProtocol(GVArchive::protocolForMimeType(item->mimetype()));
			tmp.adjustPath(1);
		}

		emit urlChanged(tmp);
		updateActions();
	} else {
		emitURLChanged();
	}
}


void GVFileViewStack::slotViewClicked() {
	updateActions();
	KFileItem* item=currentFileView()->currentFileItem();
	if (!item || isDirOrArchive(item)) return;

	// Don't change the current image if the user is creating a multi-selection
	if (currentFileView()->selectedItems()->count()>1) return;
	emitURLChanged();
}


void GVFileViewStack::slotViewDoubleClicked() {
	updateActions();
	KFileItem* item=currentFileView()->currentFileItem();
	if (item && !isDirOrArchive(item)) emit imageDoubleClicked();
}


void GVFileViewStack::updateThumbnailSize() {
	if (mNoThumbnails->isChecked()) {
		setMode(GVFileViewStack::FileList);
		return;
	} else {
		if (mSmallThumbnails->isChecked()) {
			mFileThumbnailView->setThumbnailSize(ThumbnailSize::Small);
		} else if (mMedThumbnails->isChecked()) {
			mFileThumbnailView->setThumbnailSize(ThumbnailSize::Med);
		} else {
			mFileThumbnailView->setThumbnailSize(ThumbnailSize::Large);
		}
		if (mMode==GVFileViewStack::FileList) {
			setMode(GVFileViewStack::Thumbnail);
		} else {
			KFileItemList items=*mFileThumbnailView->items();
			KFileItem* shownFileItem=mFileThumbnailView->shownFileItem();

			mFileThumbnailView->GVFileViewBase::clear();
			mFileThumbnailView->addItemList(items);
			mFileThumbnailView->setShownFileItem(shownFileItem);
		}
		mFileThumbnailView->startThumbnailUpdate();
	}
}


void GVFileViewStack::toggleShowDotFiles() {
	mDirLister->setShowingDotFiles(mShowDotFiles->isChecked());
	mDirLister->openURL(mDirURL);
}


void GVFileViewStack::updateSortMenu(QDir::SortSpec _spec) {
	int	spec=_spec & (QDir::Name | QDir::Time | QDir::Size);
	int item;
	switch (spec) {
	case QDir::Name:
		item=0;
		break;
	case QDir::Time:
		item=1;
		break;
	case QDir::Size:
		item=2;
		break;
	default:
		item=-1;
		break;
	}
	d->mSortAction->setCurrentItem(item);
}


void GVFileViewStack::setSorting() {
	QDir::SortSpec spec;

	switch (d->mSortAction->currentItem()) {
	case 0: // Name
		spec=QDir::Name;
		break;
	case 1: // Date
		spec=QDir::Time;
		break;
	case 2: // Size
		spec=QDir::Size;
		break;
	default:
		return;
	}
	currentFileView()->setSorting(QDir::SortSpec(spec | QDir::DirsFirst));
}


//-----------------------------------------------------------------------
//
// Context menu
//
//-----------------------------------------------------------------------
void GVFileViewStack::openContextMenu(const QPoint& pos) {
	int selectionSize=currentFileView()->selectedItems()->count();

	QPopupMenu menu(this);

	GVExternalToolContext* externalToolContext=
		GVExternalToolManager::instance()->createContext(
		this, currentFileView()->selectedItems());

	menu.insertItem(
		i18n("External Tools"), externalToolContext->popupMenu());

	d->mSortAction->plug(&menu);

	menu.connectItem(
		menu.insertItem( i18n("Parent Dir") ),
		this,SLOT(openParentDir()) );

	menu.insertSeparator();

	if (selectionSize==1) {
		menu.connectItem(
			menu.insertItem( i18n("&Rename...") ),
			this,SLOT(renameFile()) );
	}

	if (selectionSize>=1) {
		menu.connectItem(
			menu.insertItem( i18n("&Copy To...") ),
			this,SLOT(copyFiles()) );
		menu.connectItem(
			menu.insertItem( i18n("&Move To...") ),
			this,SLOT(moveFiles()) );
		menu.connectItem(
			menu.insertItem( i18n("&Delete") ),
			this,SLOT(deleteFiles()) );
		menu.insertSeparator();
	}

	menu.connectItem(
		menu.insertItem( i18n("Properties") ),
		this,SLOT(showFileProperties()) );
	menu.exec(pos);
}


void GVFileViewStack::openContextMenu(KListView*,QListViewItem*,const QPoint& pos) {
	openContextMenu(pos);
}


void GVFileViewStack::openContextMenu(QIconViewItem*,const QPoint& pos) {
	openContextMenu(pos);
}


//-----------------------------------------------------------------------
//
// Drop URL menu
//
//-----------------------------------------------------------------------
void GVFileViewStack::openDropURLMenu(QDropEvent* event, KFileItem* item) {
	KURL dest;

	if (item) {
		dest=item->url();
	} else {
		dest=mDirURL;
	}

	KURL::List urls;
	if (!KURLDrag::decode(event,urls)) return;

	FileOperation::openDropURLMenu(this, urls, dest);
}


//-----------------------------------------------------------------------
//
// File operations
//
//-----------------------------------------------------------------------
KURL::List GVFileViewStack::selectedURLs() const {
	KURL::List list;

	KFileItemListIterator it( *currentFileView()->selectedItems() );
	for ( ; it.current(); ++it ) {
		list.append(it.current()->url());
	}
	if (list.isEmpty()) {
		const KFileItem* item=currentFileView()->shownFileItem();
		if (item) list.append(item->url());
	}
	return list;
}


void GVFileViewStack::openParentDir() {
	KURL url(mDirURL.upURL());
	emit urlChanged(url);
}


void GVFileViewStack::copyFiles() {
	KURL::List list=selectedURLs();
	FileOperation::copyTo(list,this);
}


void GVFileViewStack::moveFiles() {
	KURL::List list=selectedURLs();
	FileOperation::moveTo(list,this);
}


void GVFileViewStack::deleteFiles() {
	KURL::List list=selectedURLs();
	FileOperation::del(list,this);
}


void GVFileViewStack::showFileProperties() {
	const KFileItemList* selectedItems=currentFileView()->selectedItems();
	if (selectedItems->count()>0) {
		(void)new KPropertiesDialog(*selectedItems);
	} else {
		(void)new KPropertiesDialog(mDirURL);
	}
}


void GVFileViewStack::renameFile() {
	const KFileItemList* selectedItems=currentFileView()->selectedItems();
	const KFileItem* item;
	if (selectedItems->count()>0) {
		item=selectedItems->getFirst();
	} else {
		item=currentFileView()->shownFileItem();
	}
	if (item) FileOperation::rename(item->url(),this );
}


//-----------------------------------------------------------------------
//
// Properties
//
//-----------------------------------------------------------------------
QString GVFileViewStack::fileName() const {
	KFileItem* item=currentFileView()->currentFileItem();
	if (!item) return "";
	return item->text();
}


GVFileViewBase* GVFileViewStack::currentFileView() const {
	if (mMode==FileList) {
		return mFileDetailView;
	} else {
		return mFileThumbnailView;
	}
}


uint GVFileViewStack::fileCount() const {
	uint count=currentFileView()->count();

	KFileItem* item=currentFileView()->firstFileItem();
	while (item && isDirOrArchive(item)) {
		item=currentFileView()->nextItem(item);
		count--;
	}
	return count;
}


KURL GVFileViewStack::url() const {
	KURL url(mDirURL);
	url.addPath(fileName());
	return url;
}


uint GVFileViewStack::selectionSize() const {
	const KFileItemList* selectedItems=currentFileView()->selectedItems();
	return selectedItems->count();
}


void GVFileViewStack::setMode(GVFileViewStack::Mode mode) {
	const KFileItemList* items;
	GVFileViewBase* oldView;
	GVFileViewBase* newView;

	mMode=mode;

	if (mMode==FileList) {
		mFileThumbnailView->stopThumbnailUpdate();
		oldView=mFileThumbnailView;
		newView=mFileDetailView;
	} else {
		oldView=mFileDetailView;
		newView=mFileThumbnailView;
	}

	bool wasFocused=oldView->widget()->hasFocus();
	// Show the new active view
	raiseWidget(newView->widget());
	if (wasFocused) newView->widget()->setFocus();

	// Fill the new view
	newView->clear();
	newView->addItemList(*oldView->items());

	// Set the new view to the same state as the old
	items=oldView->selectedItems();
	for(KFileItemListIterator it(*items);it.current()!=0L;++it) {
		newView->setSelected(it.current(), true);
	}
	newView->setShownFileItem(oldView->shownFileItem());
	newView->setCurrentItem(oldView->currentFileItem());

	// Remove references to the old view from KFileItems
	items=oldView->items();
	for(KFileItemListIterator it(*items);it.current()!=0L;++it) {
		it.current()->removeExtraData(oldView);
	}

	// Update sorting
	newView->setSorting(oldView->sorting());

	// Clear the old view
	oldView->GVFileViewBase::clear();
}


bool GVFileViewStack::showDirs() const {
	return mShowDirs;
}


void GVFileViewStack::setShowDirs(bool value) {
	mShowDirs=value;
	initDirListerFilter();
}


void GVFileViewStack::setAutoLoadImage(bool autoLoadImage) {
	mAutoLoadImage=autoLoadImage;
}


void GVFileViewStack::setShownColor(const QColor& value) {
	mShownColor=value;
	mFileDetailView->setShownFileItemColor(mShownColor);
	mFileThumbnailView->setShownFileItemColor(mShownColor);
}

//-----------------------------------------------------------------------
//
// Dir lister slots
//
//-----------------------------------------------------------------------
void GVFileViewStack::dirListerDeleteItem(KFileItem* item) {
	KFileItem* newShownItem=0L;
	const KFileItem* shownItem=currentFileView()->shownFileItem();
	if (shownItem==item) {
		newShownItem=findNextImage();
		if (!newShownItem) newShownItem=findPreviousImage();
	}

	currentFileView()->removeItem(item);

	if (shownItem==item) {
		currentFileView()->setShownFileItem(newShownItem);
		if (newShownItem) {
			emit urlChanged(newShownItem->url());
		} else {
			emit urlChanged(KURL());
		}
	}
}


void GVFileViewStack::dirListerNewItems(const KFileItemList& items) {
	//kdDebug() << "GVFileViewStack::dirListerNewItems\n";
	mThumbnailsNeedUpdate=true;
	currentFileView()->addItemList(items);
}


void GVFileViewStack::dirListerRefreshItems(const KFileItemList& list) {
	//kdDebug() << "GVFileViewStack::dirListerRefreshItems\n";
	KFileItemListIterator it(list);
	for (; *it!=0L; ++it) {
		currentFileView()->updateView(*it);
	}
}


void GVFileViewStack::dirListerClear() {
	currentFileView()->clear();
}


void GVFileViewStack::dirListerStarted() {
	//kdDebug() << "GVFileViewStack::dirListerStarted\n";
	mThumbnailsNeedUpdate=false;
}


void GVFileViewStack::dirListerCompleted() {
	//kdDebug() << "GVFileViewStack::dirListerCompleted\n";
	// Delay the code to be executed when the dir lister has completed its job
	// to avoid crash in KDirLister (see bug #57991)
	QTimer::singleShot(0,this,SLOT(delayedDirListerCompleted()));
}


void GVFileViewStack::delayedDirListerCompleted() {
	// The call to sort() is a work around to a bug which causes
	// GVFileThumbnailView::firstFileItem() to return a wrong item.  This work
	// around is not in firstFileItem() because it's const and sort() is a non
	// const method
	if (mMode==Thumbnail) {
		mFileThumbnailView->sort(mFileThumbnailView->sortDirection());
	}

	browseToFileNameToSelect();
	emit completedURLListing(mDirURL);

	if (mMode==Thumbnail && mThumbnailsNeedUpdate) {
		mFileThumbnailView->startThumbnailUpdate();
	}
}


void GVFileViewStack::dirListerCanceled() {
	if (mMode==Thumbnail) {
		mFileThumbnailView->stopThumbnailUpdate();
	}

	browseToFileNameToSelect();
}


//-----------------------------------------------------------------------
//
// Private
//
//-----------------------------------------------------------------------
void GVFileViewStack::initDirListerFilter() {
	QStringList mimeTypes=KImageIO::mimeTypes(KImageIO::Reading);
	mimeTypes.append("image/x-xcf-gimp");
	mimeTypes.append("image/pjpeg"); // KImageIO does not return this one :'(
	if (mShowDirs) {
		mimeTypes.append("inode/directory");
		mimeTypes+=GVArchive::mimeTypes();
	}
	mDirLister->setShowingDotFiles(mShowDotFiles->isChecked());
	mDirLister->setMimeFilter(mimeTypes);
	mDirLister->emitChanges();
}


void GVFileViewStack::updateActions() {
	KFileItem* firstImage=findFirstImage();

	// There isn't any image, no need to continue
	if (!firstImage) {
		mSelectFirst->setEnabled(false);
		mSelectPrevious->setEnabled(false);
		mSelectNext->setEnabled(false);
		mSelectLast->setEnabled(false);
		return;
	}

	// We did not select any image, let's activate everything
	KFileItem* currentItem=currentFileView()->currentFileItem();
	if (!currentItem || isDirOrArchive(currentItem)) {
		mSelectFirst->setEnabled(true);
		mSelectPrevious->setEnabled(true);
		mSelectNext->setEnabled(true);
		mSelectLast->setEnabled(true);
		return;
	}

	// There is at least one image, and an image is selected, let's be precise
	bool isFirst=currentItem==firstImage;
	bool isLast=currentItem==findLastImage();

	mSelectFirst->setEnabled(!isFirst);
	mSelectPrevious->setEnabled(!isFirst);
	mSelectNext->setEnabled(!isLast);
	mSelectLast->setEnabled(!isLast);
}


void GVFileViewStack::emitURLChanged() {
	KFileItem* item=currentFileView()->currentFileItem();
	currentFileView()->setShownFileItem(item);

	// We use a tmp value because the signal parameter is a reference
	KURL tmp=url();
	//kdDebug() << "urlChanged : " << tmp.prettyURL() << endl;
	emit urlChanged(tmp);
}

KFileItem* GVFileViewStack::findFirstImage() const {
	KFileItem* item=currentFileView()->firstFileItem();
	while (item && isDirOrArchive(item)) {
		item=currentFileView()->nextItem(item);
	}
	return item;
}

KFileItem* GVFileViewStack::findLastImage() const {
	KFileItem* item=currentFileView()->items()->getLast();
	while (item && isDirOrArchive(item)) {
		item=currentFileView()->prevItem(item);
	}
	return item;
}

KFileItem* GVFileViewStack::findPreviousImage() const {
	KFileItem* item=currentFileView()->shownFileItem();
	if (!item) return 0L;
	do {
		item=currentFileView()->prevItem(item);
	} while (item && isDirOrArchive(item));
	return item;
}

KFileItem* GVFileViewStack::findNextImage() const {
	KFileItem* item=currentFileView()->shownFileItem();
	if (!item) return 0L;
	do {
		item=currentFileView()->nextItem(item);
	} while (item && isDirOrArchive(item));
	return item;
}

KFileItem* GVFileViewStack::findItemByFileName(const QString& fileName) const {
	KFileItem *item;
	if (fileName.isEmpty()) return 0L;
	for (item=currentFileView()->firstFileItem();
		item;
		item=currentFileView()->nextItem(item) ) {
		if (item->name()==fileName) return item;
	}

	return 0L;
}

//-----------------------------------------------------------------------
//
// Configuration
//
//-----------------------------------------------------------------------
void GVFileViewStack::readConfig(KConfig* config,const QString& group) {
	mFileThumbnailView->readConfig(config,group);

	config->setGroup(group);
	mShowDirs=config->readBoolEntry(CONFIG_SHOW_DIRS,true);
	mShowDotFiles->setChecked(config->readBoolEntry(CONFIG_SHOW_DOT_FILES,false));
	initDirListerFilter();

	bool startWithThumbnails=config->readBoolEntry(CONFIG_START_WITH_THUMBNAILS,false);
	setMode(startWithThumbnails?Thumbnail:FileList);

	if (startWithThumbnails) {
		switch (mFileThumbnailView->thumbnailSize()) {
		case ThumbnailSize::Small:
			mSmallThumbnails->setChecked(true);
			break;
		case ThumbnailSize::Med:
			mMedThumbnails->setChecked(true);
			break;
		case ThumbnailSize::Large:
			mLargeThumbnails->setChecked(true);
			break;
		}
		mFileThumbnailView->startThumbnailUpdate();
	} else {
		mNoThumbnails->setChecked(true);
	}

	setShownColor(config->readColorEntry(CONFIG_SHOWN_COLOR,&Qt::red));

	mAutoLoadImage=config->readBoolEntry(CONFIG_AUTO_LOAD_IMAGE, true);
}

void GVFileViewStack::kpartConfig() {
	mFileThumbnailView->kpartConfig();
	mShowDirs=true;
	mShowDotFiles->setChecked(false);
	initDirListerFilter();

	bool startWithThumbnails=true;
	setMode(Thumbnail);

	mFileThumbnailView->startThumbnailUpdate();

	setShownColor(Qt::red);

	mAutoLoadImage=true;
}

void GVFileViewStack::writeConfig(KConfig* config,const QString& group) const {
	mFileThumbnailView->writeConfig(config,group);

	config->setGroup(group);

	config->writeEntry(CONFIG_START_WITH_THUMBNAILS,!mNoThumbnails->isChecked());
	config->writeEntry(CONFIG_AUTO_LOAD_IMAGE, mAutoLoadImage);
	config->writeEntry(CONFIG_SHOW_DIRS, mShowDirs);
	config->writeEntry(CONFIG_SHOW_DOT_FILES, mShowDotFiles->isChecked());
	config->writeEntry(CONFIG_SHOWN_COLOR,mShownColor);
}

