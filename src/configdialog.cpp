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
#include "configdialogbase.h"
#include "fileoperation.h"
#include "gvfilethumbnailview.h"
#include "gvfileviewstack.h"
#include "gvjpegtran.h"
#include "gvpixmap.h"
#include "gvscrollpixmapview.h"
#include "gvmainwindow.h"
#include "thumbnailloadjob.h"

#include "configdialog.moc"


class ConfigDialogPrivate {
public:
	ConfigDialogBase* mContent;
	GVMainWindow* mMainWindow;
};


ConfigDialog::ConfigDialog(QWidget* parent,GVMainWindow* mainWindow)
: KDialogBase(parent)
{
	d=new ConfigDialogPrivate;
	d->mContent=new ConfigDialogBase(this);
	d->mMainWindow=mainWindow;

	setMainWidget(d->mContent);
	setCaption(d->mContent->caption());
	
	GVFileViewStack* fileViewStack=d->mMainWindow->fileViewStack();
	GVScrollPixmapView* pixmapView=d->mMainWindow->pixmapView();
	GVPixmap* gvPixmap=d->mMainWindow->gvPixmap();

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
	typedef QMap<GVScrollPixmapView::Tool,QString> MouseBehaviours;
	MouseBehaviours behaviours;
	behaviours[GVScrollPixmapView::None]=i18n("None");
	behaviours[GVScrollPixmapView::Scroll]=i18n("Scroll");
	behaviours[GVScrollPixmapView::Browse]=i18n("Browse");
	behaviours[GVScrollPixmapView::Zoom]=i18n("Zoom");

	MouseBehaviours::Iterator it;
	for( it = behaviours.begin(); it!=behaviours.end(); ++it ) {
		int index=int(it.key());
		d->mContent->mWheelOnly->insertItem(*it,index);
		d->mContent->mControlPlusWheel->insertItem(*it,index);
		d->mContent->mShiftPlusWheel->insertItem(*it,index);
		d->mContent->mAltPlusWheel->insertItem(*it,index);
	}
	d->mContent->mWheelOnly->setCurrentItem(int(pixmapView->buttonStateTool(NoButton) ));
	d->mContent->mControlPlusWheel->setCurrentItem(int(pixmapView->buttonStateTool(ControlButton) ));
	d->mContent->mShiftPlusWheel->setCurrentItem(int(pixmapView->buttonStateTool(ShiftButton) ));
	d->mContent->mAltPlusWheel->setCurrentItem(int(pixmapView->buttonStateTool(AltButton) ));

	d->mContent->mSmoothScale->setChecked(pixmapView->smoothScale());
	d->mContent->mAutoZoomEnlarge->setChecked(pixmapView->enlargeSmallImages());
	d->mContent->mShowScrollBars->setChecked(pixmapView->showScrollBars());
	
	// Full Screen tab
	d->mContent->mShowPathInFullScreen->setChecked(d->mMainWindow->pixmapView()->showPathInFullScreen());
	d->mContent->mShowMenuBarInFullScreen->setChecked(d->mMainWindow->showMenuBarInFullScreen());
	d->mContent->mShowToolBarInFullScreen->setChecked(d->mMainWindow->showToolBarInFullScreen());
	d->mContent->mShowStatusBarInFullScreen->setChecked(d->mMainWindow->showStatusBarInFullScreen());

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
	d->mContent->mModifiedBehaviorGroup->setButton( int(gvPixmap->modifiedBehavior()) );
}



ConfigDialog::~ConfigDialog() {
	delete d;
}


void ConfigDialog::slotOk() {
	slotApply();
	accept();
}


void ConfigDialog::slotApply() {
	GVFileViewStack* fileViewStack=d->mMainWindow->fileViewStack();
	GVScrollPixmapView* pixmapView=d->mMainWindow->pixmapView();
	GVPixmap* gvPixmap=d->mMainWindow->gvPixmap();

	// Image List tab
	fileViewStack->fileThumbnailView()->setMarginSize(d->mContent->mThumbnailMargin->value());
	fileViewStack->fileThumbnailView()->setWordWrapIconText(d->mContent->mWordWrapFilename->isChecked());
	fileViewStack->fileThumbnailView()->arrangeItemsInGrid();
	fileViewStack->setAutoLoadImage(d->mContent->mAutoLoadImage->isChecked());
	fileViewStack->setShowDirs(d->mContent->mShowDirs->isChecked());
	fileViewStack->setShownColor(d->mContent->mShownColor->color());
	
	// Image View tab		
	pixmapView->setButtonStateTool(NoButton,      GVScrollPixmapView::Tool(d->mContent->mWheelOnly->currentItem()) );
	pixmapView->setButtonStateTool(ControlButton, GVScrollPixmapView::Tool(d->mContent->mControlPlusWheel->currentItem()) );
	pixmapView->setButtonStateTool(ShiftButton,   GVScrollPixmapView::Tool(d->mContent->mShiftPlusWheel->currentItem()) );
	pixmapView->setButtonStateTool(AltButton,     GVScrollPixmapView::Tool(d->mContent->mAltPlusWheel->currentItem()) );
	pixmapView->updateDefaultCursor();

	pixmapView->setSmoothScale(d->mContent->mSmoothScale->isChecked());
	pixmapView->setEnlargeSmallImages(d->mContent->mAutoZoomEnlarge->isChecked());
	pixmapView->setShowScrollBars(d->mContent->mShowScrollBars->isChecked());
	
	// Full Screen tab
	d->mMainWindow->pixmapView()->setShowPathInFullScreen( d->mContent->mShowPathInFullScreen->isChecked() );
	d->mMainWindow->setShowMenuBarInFullScreen( d->mContent->mShowMenuBarInFullScreen->isChecked() );
	d->mMainWindow->setShowToolBarInFullScreen( d->mContent->mShowToolBarInFullScreen->isChecked() );
	d->mMainWindow->setShowStatusBarInFullScreen( d->mContent->mShowStatusBarInFullScreen->isChecked() );

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
	gvPixmap->setModifiedBehavior( GVPixmap::ModifiedBehavior(d->mContent->mModifiedBehaviorGroup->id(button)) );
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
