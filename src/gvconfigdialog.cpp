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
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qmap.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qstylesheet.h>

// KDE
#include <kcolorbutton.h>
#include <kdeversion.h>
#include <kdirsize.h>
#include <kfiledialog.h>
#include <klocale.h>
#include <kio/netaccess.h>
#include <kmessagebox.h>
#include <kurlrequester.h>

// Local 
#include "gvconfigdialogbase.h"
#include "fileoperation.h"
#include "gvfilethumbnailview.h"
#include "gvfileviewstack.h"
#include "gvjpegtran.h"
#include "gvdocument.h"
#include "gvscrollpixmapview.h"
#include "gvmainwindow.h"
#include "thumbnailloadjob.h"

#include "gvconfigdialog.moc"


class GVConfigDialogPrivate {
public:
	GVConfigDialogBase* mContent;
	GVMainWindow* mMainWindow;
};


GVConfigDialog::GVConfigDialog(QWidget* parent,GVMainWindow* mainWindow)
: KDialogBase(parent)
{
	d=new GVConfigDialogPrivate;
	d->mContent=new GVConfigDialogBase(this);
	d->mMainWindow=mainWindow;

	setMainWidget(d->mContent);
	setCaption(d->mContent->caption());
	
	GVFileViewStack* fileViewStack=d->mMainWindow->fileViewStack();
	GVScrollPixmapView* pixmapView=d->mMainWindow->pixmapView();
	GVDocument* document=d->mMainWindow->document();

	// Image List tab
	d->mContent->mThumbnailMargin->setValue(fileViewStack->fileThumbnailView()->marginSize());
	d->mContent->mWordWrapFilename->setChecked(fileViewStack->fileThumbnailView()->wordWrapIconText());
	d->mContent->mAutoLoadImage->setChecked(fileViewStack->autoLoadImage());
	d->mContent->mShowDirs->setChecked(fileViewStack->showDirs());
	d->mContent->mShownColor->setColor(fileViewStack->shownColor());

	connect(d->mContent->mCalculateCacheSize,SIGNAL(clicked()),
		this,SLOT(calculateCacheSize()));
	connect(d->mContent->mEmptyCache,SIGNAL(clicked()),
		this,SLOT(emptyCache()));

	// Image View tab
	d->mContent->mSmoothScale->setChecked(pixmapView->smoothScale());
	d->mContent->mAutoZoomEnlarge->setChecked(pixmapView->enlargeSmallImages());
	d->mContent->mShowScrollBars->setChecked(pixmapView->showScrollBars());
	d->mContent->mMouseWheelGroup->setButton(pixmapView->mouseWheelScroll()?1:0);
	
	// Full Screen tab
	d->mContent->mShowPathInFullScreen->setChecked(pixmapView->showPathInFullScreen());
	d->mContent->mShowMenuBarInFullScreen->setChecked(d->mMainWindow->showMenuBarInFullScreen());
	d->mContent->mShowToolBarInFullScreen->setChecked(d->mMainWindow->showToolBarInFullScreen());
	d->mContent->mShowStatusBarInFullScreen->setChecked(d->mMainWindow->showStatusBarInFullScreen());
	d->mContent->mShowBusyPtrInFullScreen->setChecked(d->mMainWindow->showBusyPtrInFullScreen());

	// File Operations tab
	d->mContent->mShowCopyDialog->setChecked(FileOperation::confirmCopy());
	d->mContent->mShowMoveDialog->setChecked(FileOperation::confirmMove());

	d->mContent->mDefaultDestDir->setURL(FileOperation::destDir());
	d->mContent->mDefaultDestDir->fileDialog()->setMode(
		static_cast<KFile::Mode>(KFile::Directory | KFile::ExistingOnly | KFile::LocalOnly));
	
	d->mContent->mConfirmBeforeDelete->setChecked(FileOperation::confirmDelete());
	d->mContent->mDeleteGroup->setButton(FileOperation::deleteToTrash()?1:0);
	
	// Misc tab
	d->mContent->mJPEGTran->setURL(GVJPEGTran::programPath());
	d->mContent->mModifiedBehaviorGroup->setButton( int(document->modifiedBehavior()) );
}



GVConfigDialog::~GVConfigDialog() {
	delete d;
}


void GVConfigDialog::slotOk() {
	slotApply();
	accept();
}


void GVConfigDialog::slotApply() {
	GVFileViewStack* fileViewStack=d->mMainWindow->fileViewStack();
	GVScrollPixmapView* pixmapView=d->mMainWindow->pixmapView();
	GVDocument* document=d->mMainWindow->document();

	// Image List tab
	fileViewStack->fileThumbnailView()->setMarginSize(d->mContent->mThumbnailMargin->value());
	fileViewStack->fileThumbnailView()->setWordWrapIconText(d->mContent->mWordWrapFilename->isChecked());
	fileViewStack->fileThumbnailView()->arrangeItemsInGrid();
	fileViewStack->setAutoLoadImage(d->mContent->mAutoLoadImage->isChecked());
	fileViewStack->setShowDirs(d->mContent->mShowDirs->isChecked());
	fileViewStack->setShownColor(d->mContent->mShownColor->color());
	
	// Image View tab		
	pixmapView->setSmoothScale(d->mContent->mSmoothScale->isChecked());
	pixmapView->setEnlargeSmallImages(d->mContent->mAutoZoomEnlarge->isChecked());
	pixmapView->setShowScrollBars(d->mContent->mShowScrollBars->isChecked());
	pixmapView->setMouseWheelScroll(d->mContent->mMouseWheelGroup->selected()==d->mContent->mMouseWheelScroll);
	
	// Full Screen tab
	pixmapView->setShowPathInFullScreen( d->mContent->mShowPathInFullScreen->isChecked() );
	d->mMainWindow->setShowMenuBarInFullScreen( d->mContent->mShowMenuBarInFullScreen->isChecked() );
	d->mMainWindow->setShowToolBarInFullScreen( d->mContent->mShowToolBarInFullScreen->isChecked() );
	d->mMainWindow->setShowStatusBarInFullScreen( d->mContent->mShowStatusBarInFullScreen->isChecked() );
	d->mMainWindow->setShowBusyPtrInFullScreen(d->mContent->mShowBusyPtrInFullScreen->isChecked() );

	// File Operations tab
	FileOperation::setConfirmCopy(d->mContent->mShowCopyDialog->isChecked());
	FileOperation::setConfirmMove(d->mContent->mShowMoveDialog->isChecked());
	FileOperation::setDestDir(d->mContent->mDefaultDestDir->url());
	FileOperation::setConfirmDelete(d->mContent->mConfirmBeforeDelete->isChecked());
	FileOperation::setDeleteToTrash(d->mContent->mDeleteGroup->selected()==d->mContent->mDeleteToTrash);

	// Misc tab
	GVJPEGTran::setProgramPath(d->mContent->mJPEGTran->url());
	QButton* button=d->mContent->mModifiedBehaviorGroup->selected();
	Q_ASSERT(button);
	document->setModifiedBehavior( GVDocument::ModifiedBehavior(d->mContent->mModifiedBehaviorGroup->id(button)) );
}


void GVConfigDialog::calculateCacheSize() {
	KURL url;
	url.setPath(ThumbnailLoadJob::thumbnailBaseDir());
	unsigned long size=KDirSize::dirSize(url);
	KMessageBox::information( this,i18n("Cache size is %1").arg(KIO::convertSize(size)) );
}


void GVConfigDialog::emptyCache() {
	QString dir=ThumbnailLoadJob::thumbnailBaseDir();

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
#if KDE_VERSION >= 0x30200
	if (KIO::NetAccess::del(url, 0)) {
#else
	if (KIO::NetAccess::del(url)) {
#endif
		KMessageBox::information( this,i18n("Cache emptied.") );
	}
}


void GVConfigDialog::onCacheEmptied(KIO::Job* job) {
	if ( job->error() ) {
		job->showErrorDialog(this);
		return;
	}
	KMessageBox::information( this,i18n("Cache emptied.") );
}
