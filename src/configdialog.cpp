// vim: set tabstop=4 shiftwidth=4 noexpandtab
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
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qmap.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qstylesheet.h>

// KDE
#include <kcolorbutton.h>
#include <kdirsize.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kio/netaccess.h>
#include <kmessagebox.h>
#include <kurlrequester.h>

// Local 
#include "fileoperation.h"
#include "gvfilethumbnailview.h"
#include "gvfileviewstack.h"
#include "gvjpegtran.h"
#include "gvpixmap.h"
#include "gvscrollpixmapview.h"
#include "gvmainwindow.h"
#include "thumbnailloadjob.h"

#include "configdialog.moc"


ConfigDialog::ConfigDialog(QWidget* parent,GVMainWindow* mainWindow)
: ConfigDialogBase(parent,0L,true),
mMainWindow(mainWindow)
{
	GVFileViewStack* fileViewStack=mMainWindow->fileViewStack();
	GVScrollPixmapView* pixmapView=mMainWindow->pixmapView();
	GVPixmap* gvPixmap=mMainWindow->gvPixmap();

	// Image List tab
	mThumbnailMargin->setValue(fileViewStack->fileThumbnailView()->marginSize());
	mWordWrapFilename->setChecked(fileViewStack->fileThumbnailView()->wordWrapIconText());
	mAutoLoadImage->setChecked(fileViewStack->autoLoadImage());
	mShowDirs->setChecked(fileViewStack->showDirs());
	mShownColor->setColor(fileViewStack->shownColor());

	connect(mCalculateCacheSize,SIGNAL(clicked()),
		this,SLOT(calculateCacheSize()));
	connect(mEmptyCache,SIGNAL(clicked()),
		this,SLOT(emptyCache()));

	// Image View tab
	typedef QMap<GVScrollPixmapView::Tool,QString> MouseBehaviours;
	MouseBehaviours behaviours;
	behaviours[GVScrollPixmapView::None]=i18n("None");
	behaviours[GVScrollPixmapView::Scroll]=i18n("Scroll");
	behaviours[GVScrollPixmapView::Browse]=i18n("Browse");
	behaviours[GVScrollPixmapView::Zoom]=i18n("Zoom");

	MouseBehaviours::Iterator it;
	for( it = behaviours.begin(); it!=behaviours.end(); ++it ) {
		int index=int(it.key());
		mWheelOnly->insertItem(*it,index);
		mControlPlusWheel->insertItem(*it,index);
		mShiftPlusWheel->insertItem(*it,index);
		mAltPlusWheel->insertItem(*it,index);
	}
	mWheelOnly->setCurrentItem(int(pixmapView->buttonStateTool(NoButton) ));
	mControlPlusWheel->setCurrentItem(int(pixmapView->buttonStateTool(ControlButton) ));
	mShiftPlusWheel->setCurrentItem(int(pixmapView->buttonStateTool(ShiftButton) ));
	mAltPlusWheel->setCurrentItem(int(pixmapView->buttonStateTool(AltButton) ));

	mSmoothScale->setChecked(pixmapView->smoothScale());
	mAutoZoomEnlarge->setChecked(pixmapView->enlargeSmallImages());
	mShowScrollBars->setChecked(pixmapView->showScrollBars());
	
	// Full Screen tab
	mShowPathInFullScreen->setChecked(mMainWindow->pixmapView()->showPathInFullScreen());
	mShowMenuBarInFullScreen->setChecked(mMainWindow->showMenuBarInFullScreen());
	mShowToolBarInFullScreen->setChecked(mMainWindow->showToolBarInFullScreen());
	mShowStatusBarInFullScreen->setChecked(mMainWindow->showStatusBarInFullScreen());

	// File Operations tab
	mShowCopyDialog->setChecked(FileOperation::confirmCopy());
	mShowMoveDialog->setChecked(FileOperation::confirmMove());

	mDefaultDestDir->setURL(FileOperation::destDir());
	mDefaultDestDir->fileDialog()->setMode(
		static_cast<KFile::Mode>(KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly));
	
	mConfirmBeforeDelete->setChecked(FileOperation::confirmDelete());
	mDeleteGroup->setButton(FileOperation::deleteToTrash()?1:0);
	
	// Misc tab
	mJPEGTran->setURL(GVJPEGTran::programPath());
	mModifiedBehaviorGroup->setButton( int(gvPixmap->modifiedBehavior()) );
}


void ConfigDialog::slotOk() {
	slotApply();
	accept();
}


void ConfigDialog::slotApply() {
	GVFileViewStack* fileViewStack=mMainWindow->fileViewStack();
	GVScrollPixmapView* pixmapView=mMainWindow->pixmapView();
	GVPixmap* gvPixmap=mMainWindow->gvPixmap();

	// Image List tab
	fileViewStack->fileThumbnailView()->setMarginSize(mThumbnailMargin->value());
	fileViewStack->fileThumbnailView()->setWordWrapIconText(mWordWrapFilename->isChecked());
	fileViewStack->fileThumbnailView()->arrangeItemsInGrid();
	fileViewStack->setAutoLoadImage(mAutoLoadImage->isChecked());
	fileViewStack->setShowDirs(mShowDirs->isChecked());
	fileViewStack->setShownColor(mShownColor->color());
	
	// Image View tab		
	pixmapView->setButtonStateTool(NoButton,      GVScrollPixmapView::Tool(mWheelOnly->currentItem()) );
	pixmapView->setButtonStateTool(ControlButton, GVScrollPixmapView::Tool(mControlPlusWheel->currentItem()) );
	pixmapView->setButtonStateTool(ShiftButton,   GVScrollPixmapView::Tool(mShiftPlusWheel->currentItem()) );
	pixmapView->setButtonStateTool(AltButton,     GVScrollPixmapView::Tool(mAltPlusWheel->currentItem()) );
	pixmapView->updateDefaultCursor();

	pixmapView->setSmoothScale(mSmoothScale->isChecked());
	pixmapView->setEnlargeSmallImages(mAutoZoomEnlarge->isChecked());
	pixmapView->setShowScrollBars(mShowScrollBars->isChecked());
	
	// Full Screen tab
	mMainWindow->pixmapView()->setShowPathInFullScreen( mShowPathInFullScreen->isChecked() );
	mMainWindow->setShowMenuBarInFullScreen( mShowMenuBarInFullScreen->isChecked() );
	mMainWindow->setShowToolBarInFullScreen( mShowToolBarInFullScreen->isChecked() );
	mMainWindow->setShowStatusBarInFullScreen( mShowStatusBarInFullScreen->isChecked() );

	// File Operations tab
	FileOperation::setConfirmCopy(mShowCopyDialog->isChecked());
	FileOperation::setConfirmMove(mShowMoveDialog->isChecked());
	FileOperation::setDestDir(mDefaultDestDir->url());
	FileOperation::setConfirmDelete(mConfirmBeforeDelete->isChecked());
	FileOperation::setDeleteToTrash(mDeleteGroup->selected()==mDeleteToTrash);

	// Misc tab
	GVJPEGTran::setProgramPath(mJPEGTran->url());
	QButton* button=mModifiedBehaviorGroup->selected();
	Q_ASSERT(button);
	gvPixmap->setModifiedBehavior( GVPixmap::ModifiedBehavior(mModifiedBehaviorGroup->id(button)) );
}


void ConfigDialog::calculateCacheSize() {
	KURL url;
	url.setPath(ThumbnailLoadJob::thumbnailDir());
	unsigned long size=KDirSize::dirSize(url);
	KMessageBox::information( this,i18n("Cache size is %1").arg(KIO::convertSize(size)) );
}


void ConfigDialog::emptyCache() {
	QString dir=ThumbnailLoadJob::thumbnailDir();

	if (!QFile::exists(dir)) {
		KMessageBox::information( this,i18n("Cache is already empty.") );
		return;
	}

	int response=KMessageBox::questionYesNo(this,
		"<qt>" + i18n("Are you sure you want to empty the thumbnail cache?"
		" This will remove the folder <b>%1</b>.").arg(QStyleSheet::escape(dir)) + "</qt>");

	if (response==KMessageBox::No) return;

	KURL url;
	url.setPath(dir);
	if (KIO::NetAccess::del(url)) {
		KMessageBox::information( this,i18n("Cache emptied.") );
	}
}


void ConfigDialog::onCacheEmptied(KIO::Job* job) {
	if ( job->error() ) {
		job->showErrorDialog(this);
		return;
	}
	KMessageBox::information( this,i18n("Cache emptied.") );
}
