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

// Qt includes
#include <qpopupmenu.h>

// KDE includes
#include <kaction.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kimageio.h>
#include <klocale.h>
#include <kpropertiesdialog.h>
#include <kstdaction.h>

// Our includes
#include "fileoperation.h"
#include "gvfilethumbnailview.h"
#include "gvarchive.h"
#include "gvfiledetailview.h"

#include "gvfileviewstack.moc"

static const char* CONFIG_START_WITH_THUMBNAILS="start with thumbnails";
static const char* CONFIG_AUTO_LOAD_IMAGE="automatically load first image";
static const char* CONFIG_SHOW_DIRS="show dirs";
static const char* CONFIG_SHOWN_COLOR="shown color";

inline bool isDirOrArchive(const KFileItem* item) {
	return item && (item->isDir() || GVArchive::fileItemIsArchive(item));
}


GVFileViewStack::GVFileViewStack(QWidget* parent,KActionCollection* actionCollection)
: QWidgetStack(parent), mMode(FileList)
{
// Actions
	mSelectFirst=new KAction(i18n("&First"),"gvfirst",Key_Home,
		this,SLOT(slotSelectFirst()), actionCollection, "first");

	mSelectLast=new KAction(i18n("&Last"),"gvlast",Key_End,
		this,SLOT(slotSelectLast()), actionCollection, "last");

	mSelectPrevious=KStdAction::back(this, SLOT(slotSelectPrevious()),actionCollection, "previous" );
	mSelectPrevious->setIcon("gvprevious");
	mSelectPrevious->setAccel(Key_Backspace);

	mSelectNext=KStdAction::forward(this, SLOT(slotSelectNext()),actionCollection, "next" );
	mSelectNext->setIcon("gvnext");
	mSelectNext->setAccel(Key_Space);

	mNoThumbnails=new KRadioAction(i18n("Details"),"view_detailed",0,this,SLOT(updateThumbnailSize()),actionCollection,"view_detailed");
	mNoThumbnails->setExclusiveGroup("thumbnails");
	mSmallThumbnails=new KRadioAction(i18n("Small Thumbnails"),"smallthumbnails",0,this,SLOT(updateThumbnailSize()),actionCollection,"smallthumbnails");
	mSmallThumbnails->setExclusiveGroup("thumbnails");
	mMedThumbnails=new KRadioAction(i18n("Medium Thumbnails"),"medthumbnails",0,this,SLOT(updateThumbnailSize()),actionCollection,"medthumbnails");
	mMedThumbnails->setExclusiveGroup("thumbnails");
	mLargeThumbnails=new KRadioAction(i18n("Large Thumbnails"),"largethumbnails",0,this,SLOT(updateThumbnailSize()),actionCollection,"largethumbnails");
	mLargeThumbnails->setExclusiveGroup("thumbnails");

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
		this,SLOT(viewExecuted()) );
	connect(mFileDetailView,SIGNAL(returnPressed(QListViewItem*)),
		this,SLOT(viewExecuted()) );
	connect(mFileDetailView,SIGNAL(clicked(QListViewItem*)),
		this,SLOT(viewClicked()) );
	connect(mFileDetailView,SIGNAL(contextMenu(KListView*, QListViewItem*, const QPoint&)),
		this,SLOT(openContextMenu(KListView*, QListViewItem*, const QPoint&)) );

// Thumbnail widget
	mFileThumbnailView=new GVFileThumbnailView(this);
	addWidget(mFileThumbnailView,1);

	connect(mFileThumbnailView,SIGNAL(executed(QIconViewItem*)),
		this,SLOT(viewExecuted()) );
	connect(mFileThumbnailView,SIGNAL(returnPressed(QIconViewItem*)),
		this,SLOT(viewExecuted()) );
	connect(mFileThumbnailView,SIGNAL(clicked(QIconViewItem*)),
		this,SLOT(viewClicked()) );
	connect(mFileThumbnailView,SIGNAL(contextMenuRequested(QIconViewItem*,const QPoint&)),
		this,SLOT(openContextMenu(QIconViewItem*,const QPoint&)) );

// Propagate signals
	connect(mFileThumbnailView,SIGNAL(updateStarted(int)),
		this,SIGNAL(updateStarted(int)) );
	connect(mFileThumbnailView,SIGNAL(updateEnded()),
		this,SIGNAL(updateEnded()) );
	connect(mFileThumbnailView,SIGNAL(updatedOneThumbnail()),
		this,SIGNAL(updatedOneThumbnail()) );
}


GVFileViewStack::~GVFileViewStack() {
	delete mDirLister;
}


void GVFileViewStack::plugActionsToAccel(KAccel* accel) {
	mSelectFirst->plugAccel(accel);
	mSelectLast->plugAccel(accel);
	mSelectPrevious->plugAccel(accel);
	mSelectNext->plugAccel(accel);
	mNoThumbnails->plugAccel(accel);
	mSmallThumbnails->plugAccel(accel);
	mMedThumbnails->plugAccel(accel);
	mLargeThumbnails->plugAccel(accel);
}


//-----------------------------------------------------------------------
//
// Public slots
//
//-----------------------------------------------------------------------
void GVFileViewStack::setURL(const KURL& dirURL,const QString& filename) {
	//kdDebug() << "GVFileViewStack::setURL " << dirURL.path() + " - " + filename << endl;
	if (mDirURL.cmp(dirURL,true)) return;
	
	mDirURL=dirURL;
	currentFileView()->setShownFileItem(0L);
	mFilenameToSelect=filename;
	
	mDirLister->openURL(mDirURL);
	updateActions();
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


void GVFileViewStack::selectFilename(QString filename) {
	if (filename.isEmpty()) {
		updateActions();
		return;
	}
	
	KFileItem *item;
	for (item=currentFileView()->firstFileItem();
		item;
		item=currentFileView()->nextItem(item) ) {

		if (item->name()==filename) {
			currentFileView()->setCurrentItem(item);
			currentFileView()->ensureItemVisible(item);
			emitURLChanged();
			break;
		}
	}
	updateActions();
}


void GVFileViewStack::slotSelectFirst() {
	KFileItem* item=findFirstImage();
	if (!item) return;
	
	currentFileView()->setCurrentItem(item);
	currentFileView()->clearSelection();
	currentFileView()->setSelected(item,true);
	currentFileView()->ensureItemVisible(item);
	emitURLChanged();
	updateActions();
}


void GVFileViewStack::slotSelectLast() {
	KFileItem* item=findLastImage();
	if (!item) return;
	
	currentFileView()->setCurrentItem(item);
	currentFileView()->clearSelection();
	currentFileView()->setSelected(item,true);
	currentFileView()->ensureItemVisible(item);
	emitURLChanged();
	updateActions();
}


void GVFileViewStack::slotSelectPrevious() {
	KFileItem* item=findPreviousImage();
	if (!item) return;
	
	currentFileView()->setCurrentItem(item);
	currentFileView()->clearSelection();
	currentFileView()->setSelected(item,true);
	currentFileView()->ensureItemVisible(item);
	emitURLChanged();
	updateActions();
}


void GVFileViewStack::slotSelectNext() {
	KFileItem* item=findNextImage();
	if (!item) return;
	
	currentFileView()->setCurrentItem(item);
	currentFileView()->clearSelection();
	currentFileView()->setSelected(item,true);
	currentFileView()->ensureItemVisible(item);
	emitURLChanged();
	updateActions();
}


//-----------------------------------------------------------------------
//
// Private slots
//
//-----------------------------------------------------------------------
void GVFileViewStack::viewExecuted() {
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


void GVFileViewStack::viewClicked() {
	updateActions();
	KFileItem* item=currentFileView()->currentFileItem();
	if (!item || isDirOrArchive(item)) return;

	emitURLChanged();
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


//-----------------------------------------------------------------------
//
// Context menu
//
//-----------------------------------------------------------------------
void GVFileViewStack::openContextMenu(const QPoint& pos) {
	int selectionSize=currentFileView()->selectedItems()->count();
	
	QPopupMenu menu(this);

	if (selectionSize==1) {
		if (!isDirOrArchive(currentFileView()->selectedItems()->getFirst())) {
			menu.connectItem(
				menu.insertItem( i18n("Open With &Editor") ),
				this,SLOT(openWithEditor()) );
			menu.insertSeparator();
		}
	}
	
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
			menu.insertItem( i18n("&Delete...") ),
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


void GVFileViewStack::openWithEditor() {
	KURL::List list=selectedURLs();
	if (list.isEmpty()) return;
	
	FileOperation::openWithEditor(list.first());
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
QString GVFileViewStack::filename() const {
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


/**
 * This method avoids the need to include kfileview.h for class users
 */
uint GVFileViewStack::fileCount() const {
	return currentFileView()->numFiles();
}


KURL GVFileViewStack::url() const {
	KURL url(mDirURL);
	url.addPath(filename());
	return url;
}


uint GVFileViewStack::selectionSize() const {
	const KFileItemList* selectedItems=currentFileView()->selectedItems();
	return selectedItems->count();
}


void GVFileViewStack::setMode(GVFileViewStack::Mode mode) {
	mMode=mode;
	GVFileViewBase* oldView;
	GVFileViewBase* newView;

	if (mMode==FileList) {
		mFileThumbnailView->stopThumbnailUpdate();
		oldView=mFileThumbnailView;
		newView=mFileDetailView;
	} else {
		oldView=mFileDetailView;
		newView=mFileThumbnailView;
	}

// Show the new active view
	raiseWidget(newView->widget());

// Fill the new view
	newView->clear();
	newView->addItemList(*oldView->items());
	newView->setShownFileItem(oldView->shownFileItem());
 
// Remove references to the old view from KFileItems
	const KFileItemList* items=oldView->items();
	for(KFileItemListIterator it(*items);it.current()!=0L;++it) {
		it.current()->removeExtraData(oldView);
	}

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
		}
	}
}


void GVFileViewStack::dirListerNewItems(const KFileItemList& items) {
	mThumbnailsNeedUpdate=true;
	currentFileView()->addItemList(items);
}


void GVFileViewStack::dirListerRefreshItems(const KFileItemList& list) {
	KFileItemListIterator it(list);
	for (; *it!=0L; ++it) {
		currentFileView()->updateView(*it);
	}
}


void GVFileViewStack::dirListerClear() {
	currentFileView()->clear();
}


void GVFileViewStack::dirListerStarted() {
	mThumbnailsNeedUpdate=false;
}


void GVFileViewStack::dirListerCompleted() {
	// FIXME: This is a work around to a bug which causes
	// GVFileThumbnailView::firstFileItem to return a wrong item.
	// This work around is not in the method because firstFileItem is 
	// const and sort is a non const method
	if (mMode==Thumbnail) {
		mFileThumbnailView->sort(mFileThumbnailView->sortDirection());
	}

	if (!mFilenameToSelect.isEmpty()) {
		selectFilename(mFilenameToSelect);
		mFilenameToSelect=QString::null;
	} else {
		if (!currentFileView()->shownFileItem() && mAutoLoadImage) {
			slotSelectFirst();
		}
	}

	if (mMode==Thumbnail && mThumbnailsNeedUpdate) {
		mFileThumbnailView->startThumbnailUpdate();
	}
}


void GVFileViewStack::dirListerCanceled() {
	if (mMode==Thumbnail) {
		mFileThumbnailView->stopThumbnailUpdate();
	}

	if (!mFilenameToSelect.isEmpty()) {
		selectFilename(mFilenameToSelect);
		mFilenameToSelect=QString::null;
	} else {
		if (!currentFileView()->shownFileItem() && mAutoLoadImage) {
			slotSelectFirst();
		}
	}
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


//-----------------------------------------------------------------------
//
// Configuration
//
//-----------------------------------------------------------------------
void GVFileViewStack::readConfig(KConfig* config,const QString& group) {
	mFileThumbnailView->readConfig(config,group);

	config->setGroup(group);
	mShowDirs=config->readBoolEntry(CONFIG_SHOW_DIRS,true);
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


void GVFileViewStack::writeConfig(KConfig* config,const QString& group) const {
	mFileThumbnailView->writeConfig(config,group);

	config->setGroup(group);

	config->writeEntry(CONFIG_START_WITH_THUMBNAILS,!mNoThumbnails->isChecked());
	config->writeEntry(CONFIG_AUTO_LOAD_IMAGE, mAutoLoadImage);
	config->writeEntry(CONFIG_SHOW_DIRS, mShowDirs);
	config->writeEntry(CONFIG_SHOWN_COLOR,mShownColor);
}

