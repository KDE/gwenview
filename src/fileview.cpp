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
#include <qpopupmenu.h>

// KDE includes
#include <kaction.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kimageio.h>
#include <klocale.h>
#include <kstdaction.h>

// Our includes
#include "filednddetailview.h"
#include "fileoperation.h"
#include "filethumbnailview.h"

#include "fileview.moc"

static const char* CONFIG_START_WITH_THUMBNAILS="start with thumbnails";
static const char* CONFIG_AUTO_LOAD_IMAGE="automatically load first image";

FileView::FileView(QWidget* parent,KActionCollection* actionCollection)
: QWidgetStack(parent), mMode(FileList), mPopupMenu(0L)
{
// Actions
	mSelectFirst=new KAction(i18n("&First"),"start",Key_Home,
		this,SLOT(slotSelectFirst()), actionCollection, "first");

	mSelectLast=new KAction(i18n("&Last"),"finish",Key_End,
		this,SLOT(slotSelectLast()), actionCollection, "last");

	mSelectPrevious=KStdAction::back(this, SLOT(slotSelectPrevious()),actionCollection );
	mSelectPrevious->setAccel(Key_Backspace);

	mSelectNext=KStdAction::forward(this, SLOT(slotSelectNext()),actionCollection );
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
	QStringList mimeTypes=KImageIO::mimeTypes(KImageIO::Reading);
	mimeTypes.append("image/x-xcf-gimp");
	mimeTypes.append("image/pjpeg"); // KImageIO does not return this one :'(
	mDirLister->setMimeFilter(mimeTypes);

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
	mFileDetailView=new FileDnDDetailView(this,"filedetailview");
	addWidget(mFileDetailView,0);

	connect(mFileDetailView,SIGNAL(clicked(QListViewItem*)),
		this,SLOT(detailChanged(QListViewItem*)) );
	connect(mFileDetailView,SIGNAL(returnPressed(QListViewItem*)),
		this,SLOT(detailChanged(QListViewItem*)) );
	connect(mFileDetailView,SIGNAL(rightButtonClicked(QListViewItem*,const QPoint&,int)),
		this,SLOT(detailRightButtonClicked(QListViewItem*,const QPoint&,int)) );

// Thumbnail widget
	mFileThumbnailView=new FileThumbnailView(this);
	addWidget(mFileThumbnailView,1);

	connect(mFileThumbnailView,SIGNAL(clicked(QIconViewItem*)),
		this,SLOT(thumbnailChanged(QIconViewItem*)) );
	connect(mFileThumbnailView,SIGNAL(returnPressed(QIconViewItem*)),
		this,SLOT(thumbnailChanged(QIconViewItem*)) );
	connect(mFileThumbnailView,SIGNAL(rightButtonClicked(QIconViewItem*,const QPoint&)),
		this,SLOT(thumbnailRightButtonClicked(QIconViewItem*,const QPoint&)) );

// Propagate signals
	connect(mFileThumbnailView,SIGNAL(updateStarted(int)),
		this,SIGNAL(updateStarted(int)) );
	connect(mFileThumbnailView,SIGNAL(updateEnded()),
		this,SIGNAL(updateEnded()) );
	connect(mFileThumbnailView,SIGNAL(updatedOneThumbnail()),
		this,SIGNAL(updatedOneThumbnail()) );

}


FileView::~FileView() {
	delete mDirLister;
}


void FileView::plugActionsToAccel(KAccel* accel) {
	mSelectFirst->plugAccel(accel);
	mSelectLast->plugAccel(accel);
	mSelectPrevious->plugAccel(accel);
	mSelectNext->plugAccel(accel);
	mNoThumbnails->plugAccel(accel);
	mSmallThumbnails->plugAccel(accel);
	mMedThumbnails->plugAccel(accel);
	mLargeThumbnails->plugAccel(accel);
}


void FileView::installRBPopup(QPopupMenu* menu) {
	mPopupMenu=menu;
}


//-Public slots----------------------------------------------------------
void FileView::setURL(const KURL& dirURL,const QString& filename) {
	//kdDebug() << "FileView::setURL " << dirURL.path() + " - " + filename << endl;
	if (mDirURL.cmp(dirURL,true)) return;
	
	mDirURL=dirURL;
	mFilenameToSelect=filename;
	mDirLister->openURL(mDirURL);
	updateActions();
}


void FileView::cancel() {
	if (!mDirLister->isFinished()) {
		mDirLister->stop();
		return;
	}
	if (mMode==Thumbnail) {
		mFileThumbnailView->stopThumbnailUpdate();
	}
}


void FileView::selectFilename(QString filename) {
	KFileItem* item;
	currentFileView()->setCurrentItem(filename);
	item=currentFileView()->currentFileItem();
	if (item) {
		currentFileView()->ensureItemVisible(item);
	}
	updateActions();
	emitURLChanged();
}


void FileView::slotSelectFirst() {
	// FIXME : This is a work around to a bug which causes
	// FileThumbnailView::firstFileItem to return a wrong item.
	// This work around is not in the method because firstFileItem is 
	// const and sort is a non const method
	if (mMode==Thumbnail) {
		mFileThumbnailView->sort(mFileThumbnailView->sortDirection());
	}

	KFileItem* item=currentFileView()->firstFileItem();
	if (!item) return;
	
	currentFileView()->setCurrentItem(item);
	currentFileView()->setSelected(item,true);
	currentFileView()->ensureItemVisible(item);
	emitURLChanged();
}


void FileView::slotSelectLast() {
	KFileItem* item=currentFileView()->items()->getLast();
	if (!item) return;
	
	currentFileView()->setCurrentItem(item);
	currentFileView()->setSelected(item,true);
	currentFileView()->ensureItemVisible(item);
	emitURLChanged();
}


void FileView::slotSelectPrevious() {
	KFileItem* item=currentFileView()->currentFileItem();
	if (item) {
		item=currentFileView()->prevItem(item);
	} else {
		item=currentFileView()->firstFileItem();
	}
	if (!item) return;
	
	currentFileView()->setCurrentItem(item);
	currentFileView()->setSelected(item,true);
	currentFileView()->ensureItemVisible(item);
	emitURLChanged();
}


void FileView::slotSelectNext() {
	KFileItem* item=currentFileView()->currentFileItem();
	if (item) {
		item=currentFileView()->nextItem(item);
	} else {
		item=currentFileView()->firstFileItem();
	}
	if (!item) return;
	
	currentFileView()->setCurrentItem(item);
	currentFileView()->setSelected(item,true);
	currentFileView()->ensureItemVisible(item);
	emitURLChanged();
}


//-Private slots---------------------------------------------------------
void FileView::detailChanged(QListViewItem* item) {
	if (!item) return;
	emitURLChanged();
}


void FileView::thumbnailChanged(QIconViewItem* item) {
	if (!item) return;
	emitURLChanged();
}


void FileView::detailRightButtonClicked(QListViewItem* item,const QPoint& pos,int) {
	if (!item) return;
	emitURLChanged();
	if (mPopupMenu)
		mPopupMenu->popup(pos);
}


void FileView::thumbnailRightButtonClicked(QIconViewItem* item,const QPoint& pos) {
	if (!item) return;
	emitURLChanged();
	mPopupMenu->popup(pos);
}


void FileView::updateThumbnailSize() {
	if (mNoThumbnails->isChecked()) {
		setMode(FileView::FileList);
		return;
	} else {
		if (mSmallThumbnails->isChecked()) {
			mFileThumbnailView->setThumbnailSize(ThumbnailSize::Small);
		} else if (mMedThumbnails->isChecked()) {
			mFileThumbnailView->setThumbnailSize(ThumbnailSize::Med);
		} else {
			mFileThumbnailView->setThumbnailSize(ThumbnailSize::Large);
		}
		setMode(FileView::Thumbnail);
	}
}


//-Properties------------------------------------------------------------
QString FileView::filename() const {
	KFileItem* item=currentFileView()->currentFileItem();
	if (!item) return "";
	return item->text();
}


KFileView* FileView::currentFileView() const {
	if (mMode==FileList) {
		return mFileDetailView;
	} else {
		return mFileThumbnailView;
	}
}


/**
 * This methods avoid the need to include kfileview.h for class users
 */
uint FileView::fileCount() const {
	return currentFileView()->count();
}


KURL FileView::url() const {
	KURL url(mDirURL);
	url.addPath(filename());
	return url;
}


bool FileView::currentIsFirst() const {
	KFileItem* item=currentFileView()->currentFileItem();
	if (!item) return false;

	return currentFileView()->prevItem(item)==0L;
}


bool FileView::currentIsLast() const {
	KFileItem* item=currentFileView()->currentFileItem();
	if (!item) return false;

	return currentFileView()->nextItem(item)==0L;
}


void FileView::setMode(FileView::Mode mode) {
	mFilenameToSelect=filename();
	mMode=mode;

	if (mMode==FileList) {
		mFileThumbnailView->stopThumbnailUpdate();
		raiseWidget(mFileDetailView);
	} else {
		raiseWidget(mFileThumbnailView);
	}

	KURL url=mDirLister->url();
	if (url.isValid()) mDirLister->openURL(url);
}


//-Dir lister slots------------------------------------------------------
void FileView::dirListerDeleteItem(KFileItem* item) {
	mFileThumbnailView->removeItem(item);
	mFileDetailView->removeItem(item);
}


void FileView::dirListerNewItems(const KFileItemList& items) {
	mThumbnailsNeedUpdate=true;
	currentFileView()->addItemList(items);
}


void FileView::dirListerRefreshItems(const KFileItemList& list) {
	KFileItemListIterator it(list);
	for (; *it!=0L; ++it) {
		currentFileView()->updateView(*it);
	}
}


void FileView::dirListerClear() {
	currentFileView()->clear();
}


void FileView::dirListerStarted() {
	mThumbnailsNeedUpdate=false;
	if (mFilenameToSelect.isEmpty()) {
		mFilenameToSelect=filename();
	}
}


void FileView::dirListerCompleted() {
	if (mFilenameToSelect.isEmpty()) {
		if (mAutoLoadImage) {
			slotSelectFirst();
		} else {
			updateActions();
		}
	} else {
		selectFilename(mFilenameToSelect);
	}
	
	if (mMode==Thumbnail && mThumbnailsNeedUpdate) {
		mFileThumbnailView->startThumbnailUpdate();
	}
}


void FileView::dirListerCanceled() {
	if (mMode==Thumbnail) {
		mFileThumbnailView->stopThumbnailUpdate();
	}
	if (mFilenameToSelect.isEmpty()) {
		slotSelectFirst();
	} else {
		selectFilename(mFilenameToSelect);
	}
}


//-Private---------------------------------------------------------------
void FileView::updateActions() {
	if (currentFileView()->count()==0) {
		mSelectFirst->setEnabled(false);
		mSelectPrevious->setEnabled(false);
		mSelectNext->setEnabled(false);
		mSelectLast->setEnabled(false);	
		return;
	}

	KFileItem* item=currentFileView()->currentFileItem();
	if (!item) {
		mSelectFirst->setEnabled(true);
		mSelectPrevious->setEnabled(true);
		mSelectNext->setEnabled(true);
		mSelectLast->setEnabled(true);	
		return;
	}
	bool isFirst=currentIsFirst();
	bool isLast=currentIsLast();
	
	mSelectFirst->setEnabled(!isFirst);
	mSelectPrevious->setEnabled(!isFirst);	
	mSelectNext->setEnabled(!isLast);
	mSelectLast->setEnabled(!isLast);	
}


void FileView::emitURLChanged() {
// We use a tmp value because the signal parameter is a reference
	KURL tmp=url();
	//kdDebug() << "urlChanged : " << tmp.path() << endl;
	emit urlChanged(tmp);
	updateActions();
}


//-File operations-------------------------------------------------------
void FileView::copyFile() {
	FileOperation::copyTo(url(),this);
}


void FileView::moveFile() {
// Get the next item text or the previous one if there's no next item
	KFileItem* currentItem=currentFileView()->currentFileItem();
	KFileItem* newItem=currentFileView()->nextItem(currentItem);
	if (!newItem) {
		newItem=currentFileView()->prevItem(currentItem);
	}
	if (newItem) {
		mNewFilenameToSelect=newItem->name();
	} else {
		mNewFilenameToSelect="";
	}
	//kdDebug() << "file to select after move : " << mNewFilenameToSelect << endl;
	
// move the file, slotSelectNewFilename will be called on success
	FileOperation::moveTo(url(),this,this,SLOT(slotSelectNewFilename()) );
}


void FileView::deleteFile() {
// Get the next item text or the previous one if there's no next item
	KFileItem* currentItem=currentFileView()->currentFileItem();
	KFileItem* newItem=currentFileView()->nextItem(currentItem);
	if (!newItem) {
		newItem=currentFileView()->prevItem(currentItem);
	}
	if (newItem) {
		mNewFilenameToSelect=newItem->name();
	} else {
		mNewFilenameToSelect="";
	}
	
// Delete the file, slotSelectNewFilename will be called on success
	FileOperation::del(url(),this,this,SLOT(slotSelectNewFilename()) );
}


void FileView::slotSelectNewFilename() {
	mFilenameToSelect=mNewFilenameToSelect;
}


void FileView::renameFile() {
	FileOperation::rename(url(),this,this,SLOT(slotRenamed(const QString&)) );
}


void FileView::slotRenamed(const QString& newFilename) {
	mNewFilenameToSelect=newFilename;
	slotSelectNewFilename();
}


//-Configuration------------------------------------------------------------
void FileView::readConfig(KConfig* config,const QString& group) {
	mFileThumbnailView->readConfig(config,group);

	config->setGroup(group);
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
	} else {
		mNoThumbnails->setChecked(true);
	}

	mAutoLoadImage=config->readBoolEntry(CONFIG_AUTO_LOAD_IMAGE, true);
}


void FileView::writeConfig(KConfig* config,const QString& group) const {
	mFileThumbnailView->writeConfig(config,group);

	config->setGroup(group);

	config->writeEntry(CONFIG_START_WITH_THUMBNAILS,!mNoThumbnails->isChecked());
	config->writeEntry(CONFIG_AUTO_LOAD_IMAGE, mAutoLoadImage);
}

void FileView::setAutoLoadImage(bool autoLoadImage) {
	mAutoLoadImage=autoLoadImage;
}

