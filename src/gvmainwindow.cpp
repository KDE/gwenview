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
#include <qcursor.h>
#include <qdir.h>
#include <qdockarea.h>
#include <qhbox.h>
#include <qtooltip.h>

// KDE
#include <kaccel.h>
#include <kaction.h>
#include <kapplication.h>
#include <kbookmarkmanager.h>
#include <kbookmarkmenu.h>
#include <kcmdlineargs.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdeversion.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <khelpmenu.h>
#include <kiconloader.h>
#include <kimageio.h>
#include <kkeydialog.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kpopupmenu.h>
#include <kprogress.h>
#include <ksqueezedtextlabel.h>
#include <kstandarddirs.h>
#include <kstatusbar.h>
#include <kstdaccel.h>
#include <kstdaction.h>
#include <ktoolbarbutton.h>
#include <kurlcompletion.h>
#include <kurlrequesterdlg.h>

// Local
#include "configdialog.h"
#include "fileoperation.h"
#include "gvbookmarkowner.h"
#include "gvdirview.h"
#include "gvfileviewstack.h"
#include "gvjpegtran.h"
#include "gvpixmap.h"
#include "gvscrollpixmapview.h"
#include "gvslideshow.h"
#include "gvslideshowdialog.h"
#include "statusbarprogress.h"
#include "gvmetaedit.h"

#include "gvmainwindow.moc"


const char* CONFIG_DOCK_GROUP="dock";
const char* CONFIG_MAINWINDOW_GROUP="main window";
const char* CONFIG_FILEWIDGET_GROUP="file widget";
const char* CONFIG_JPEGTRAN_GROUP="jpegtran";
const char* CONFIG_PIXMAPWIDGET_GROUP="pixmap widget";
const char* CONFIG_FILEOPERATION_GROUP="file operations";
const char* CONFIG_SLIDESHOW_GROUP="slide show";

const char* CONFIG_MENUBAR_IN_FS="menu bar in full screen";
const char* CONFIG_TOOLBAR_IN_FS="tool bar in full screen";
const char* CONFIG_STATUSBAR_IN_FS="status bar in full screen";
const char* CONFIG_SHOW_LOCATION_TOOLBAR="show address bar";

GVMainWindow::GVMainWindow()
: KDockMainWindow(), mProgress(0L), mLocationToolBar(0L)
{
	FileOperation::readConfig(KGlobal::config(),CONFIG_FILEOPERATION_GROUP);
	readConfig(KGlobal::config(),CONFIG_MAINWINDOW_GROUP);

	// Backend
	mGVPixmap=new GVPixmap(this);

	// GUI
	createWidgets();
	createActions();
	createLocationToolBar();

	createGUI("gwenviewui.rc", false);
	createConnections();
	mWindowListActions.setAutoDelete(true);
	updateWindowActions();

	mFileViewStack->setFocus();

	// Command line
	KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

	KURL url;
	if (args->count()==0) {
		url.setPath( QDir::currentDirPath() );
	} else {
		url=args->url(0);
	}

	// Check if we should start in fullscreen mode
	if (args->isSet("f")) {
		mFileViewStack->noThumbnails()->activate(); // No thumbnails needed
		mToggleFullScreen->activate();
	}

	// Go to requested file
	mGVPixmap->setURL(url);
}


GVMainWindow::~GVMainWindow() {
	KConfig* config=KGlobal::config();
	FileOperation::writeConfig(config,CONFIG_FILEOPERATION_GROUP);
	mPixmapView->writeConfig(config,CONFIG_PIXMAPWIDGET_GROUP);
	mFileViewStack->writeConfig(config,CONFIG_FILEWIDGET_GROUP);
	mSlideShow->writeConfig(config,CONFIG_SLIDESHOW_GROUP);

	writeDockConfig(config,CONFIG_DOCK_GROUP);
	writeConfig(config,CONFIG_MAINWINDOW_GROUP);
	GVJPEGTran::writeConfig(config,CONFIG_JPEGTRAN_GROUP);
}


bool GVMainWindow::queryClose() {
	return mGVPixmap->saveIfModified();
}


//-----------------------------------------------------------------------
//
// Public slots
//
//-----------------------------------------------------------------------
void GVMainWindow::setURL(const KURL& url,const QString&) {
	//kdDebug() << "GVMainWindow::setURL " << url.path() << " - " << filename << endl;

	bool filenameIsValid=!mGVPixmap->isNull();

	mRenameFile->setEnabled(filenameIsValid);
	mCopyFiles->setEnabled(filenameIsValid);
	mMoveFiles->setEnabled(filenameIsValid);
	mDeleteFiles->setEnabled(filenameIsValid);
	mShowFileProperties->setEnabled(filenameIsValid);
	mOpenWithEditor->setEnabled(filenameIsValid);
	mRotateLeft->setEnabled(filenameIsValid);
	mRotateRight->setEnabled(filenameIsValid);
	mMirror->setEnabled(filenameIsValid);
	mFlip->setEnabled(filenameIsValid);
	mSaveFile->setEnabled(filenameIsValid);
	mSaveFileAs->setEnabled(filenameIsValid);
	
	mOpenParentDir->setEnabled(url.path()!="/");

	updateStatusInfo();
	kapp->restoreOverrideCursor();

	/*
	mURLEditCompletion->addItem(url.prettyURL());
	mURLEdit->setEditText(url.prettyURL());
	mURLEdit->addToHistory(url.prettyURL());
	*/
}

//-----------------------------------------------------------------------
//
// File operations
//
//-----------------------------------------------------------------------
void GVMainWindow::openParentDir() {
	KURL url=mGVPixmap->dirURL().upURL();
	mGVPixmap->setURL(url);	
}


void GVMainWindow::openHomeDir() {
	KURL url;
	url.setPath( QDir::homeDirPath() );
	mGVPixmap->setURL(url);	
}


void GVMainWindow::renameFile() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->renameFile();
	} else {
		mPixmapView->renameFile();
	}
}


void GVMainWindow::copyFiles() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->copyFiles();
	} else {
		mPixmapView->copyFile();
	}
}


void GVMainWindow::moveFiles() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->moveFiles();
	} else {
		mPixmapView->moveFile();
	}
}


void GVMainWindow::deleteFiles() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->deleteFiles();
	} else {
		mPixmapView->deleteFile();
	}
}


void GVMainWindow::showFileProperties() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->showFileProperties();
	} else {
		mPixmapView->showFileProperties();
	}
}


void GVMainWindow::openWithEditor() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->openWithEditor();
	} else {
		mPixmapView->openWithEditor();
	}
}


void GVMainWindow::openFile() {
	KURL url=KFileDialog::getOpenURL();
	if (!url.isValid()) return;

	mGVPixmap->setURL(url);
}


//-----------------------------------------------------------------------
//
// Private slots
//
//-----------------------------------------------------------------------
void GVMainWindow::pixmapLoading() {
	kapp->setOverrideCursor(QCursor(WaitCursor));
}


void GVMainWindow::toggleDirAndFileViews() {
	KConfig* config=KGlobal::config();
	
	/*if (mToggleDirAndFileViews->isChecked()) {
		writeDockConfig(config,CONFIG_DOCK_GROUP);
		makeDockInvisible(mFileDock);
		makeDockInvisible(mFolderDock);
	} else {
		readDockConfig(config,CONFIG_DOCK_GROUP);
	}*/
	if (mFileDock->isVisible() || mFolderDock->isVisible()) {
		writeDockConfig(config,CONFIG_DOCK_GROUP);
		makeDockInvisible(mFileDock);
		makeDockInvisible(mFolderDock);
	} else {
		readDockConfig(config,CONFIG_DOCK_GROUP);
	}

	mPixmapView->setFocus();
}


void GVMainWindow::toggleFullScreen() {
	KConfig* config=KGlobal::config();
	
	mToggleDirAndFileViews->setEnabled(!mToggleFullScreen->isChecked());
	
	if (mToggleFullScreen->isChecked()) {
		if (!mShowMenuBarInFullScreen) menuBar()->hide();

	/* Hide toolbar
	 * If the toolbar is docked we hide the DockArea to avoid
	 * having a one pixel band remaining
	 * For the same reason, we hide all the empty DockAreas
	 *
	 * NOTE: This does not work really well if the toolbar is in
	 * the left or right dock area.
	 */
		if (!mShowToolBarInFullScreen) {
			if (toolBar()->area()) {
				toolBar()->area()->hide();
			} else {
				toolBar()->hide();
			}
		}
		if (leftDock()->isEmpty())	 leftDock()->hide();
		if (rightDock()->isEmpty())  rightDock()->hide();
		if (topDock()->isEmpty())	 topDock()->hide();
		if (bottomDock()->isEmpty()) bottomDock()->hide();
		
		if (!mShowStatusBarInFullScreen) statusBar()->hide();
		writeDockConfig(config,CONFIG_DOCK_GROUP);
		makeDockInvisible(mFileDock);
		makeDockInvisible(mFolderDock);
		makeDockInvisible(mMetaDock);
		mPixmapView->setFullScreen(true);
		showFullScreen();
	} else {
		readDockConfig(config,CONFIG_DOCK_GROUP);
		statusBar()->show();
		
		if (toolBar()->area()) {
			toolBar()->area()->show();
		} else {
			toolBar()->show();
		}
		leftDock()->show();
		rightDock()->show();
		topDock()->show();
		bottomDock()->show();
		
		menuBar()->show();
		mPixmapView->setFullScreen(false);
		showNormal();
	}
	mPixmapView->setFocus();
}


void GVMainWindow::toggleSlideShow() {
	if (mToggleSlideShow->isChecked()) {
		GVSlideShowDialog dialog(this,mSlideShow);
		if (!dialog.exec()) {
			mToggleSlideShow->setChecked(false);
			return;
		}
		if (!mToggleFullScreen->isChecked()) {
			mToggleFullScreen->activate();
		}
		mSlideShow->start();
	} else {
		mSlideShow->stop();
		if (mToggleFullScreen->isChecked()) {
			mToggleFullScreen->activate();
		}
	}
}


void GVMainWindow::showConfigDialog() {
	ConfigDialog dialog(this,this);
	dialog.exec();
}


void GVMainWindow::showKeyDialog() {
	KKeyDialog::configure(actionCollection());
}


void GVMainWindow::escapePressed() {
	if (mToggleSlideShow->isChecked()) {
		mToggleSlideShow->activate();
		return;
	}
	if (mToggleFullScreen->isChecked()) {
		mToggleFullScreen->activate();
	}
}


void GVMainWindow::thumbnailUpdateStarted(int count) {
	mProgress=new StatusBarProgress(statusBar(),i18n("Generating thumbnails..."),count);
	mProgress->progress()->setFormat("%v/%m");
	mProgress->show();
	mStop->setEnabled(true);
}


void GVMainWindow::thumbnailUpdateEnded() {
	mStop->setEnabled(false);
	if (mProgress) {
		mProgress->hide();
		delete mProgress;
		mProgress=0L;
	}
}


void GVMainWindow::thumbnailUpdateProcessedOne() {
	mProgress->progress()->advance(1);
}


void GVMainWindow::slotURLEditChanged(const QString &str) {
	mGVPixmap->setURL(str);
	if (mFileViewStack->isVisible()) {
		mFileViewStack->setFocus();
	} else if (mPixmapView->isVisible()) {
		mPixmapView->setFocus();
	}
}


//-----------------------------------------------------------------------
//
// GUI
//
//-----------------------------------------------------------------------
void GVMainWindow::updateStatusInfo() {
	QString txt;
	uint count=mFileViewStack->fileCount();
	QString url=mGVPixmap->dirURL().prettyURL();
	if (count==0) {
		txt=i18n("%1 - No Images").arg(url);
	} else {
		txt=i18n("%1 - One Image","%1 - %n images",count).arg(url);
	}
	mSBDirLabel->setText(txt);

	updateFileInfo();
}


void GVMainWindow::updateFileInfo() {
	QString filename=mGVPixmap->filename();
	if (!filename.isEmpty()) {
		QString info=QString("%1 %2x%3 @ %4%")
			.arg(filename).arg(mGVPixmap->width()).arg(mGVPixmap->height())
			.arg(int(mPixmapView->zoom()*100) );
		mSBDetailLabel->show();
		mSBDetailLabel->setText(info);
	} else {
		mSBDetailLabel->hide();
	}
	setCaption(filename);
}


void GVMainWindow::createWidgets() {
	KConfig* config=KGlobal::config();

	manager()->setSplitterHighResolution(true);
	manager()->setSplitterOpaqueResize(true);
	
	// Status bar
	mSBDirLabel=new KSqueezedTextLabel("", statusBar());
	statusBar()->addWidget(mSBDirLabel,1);
	mSBDetailLabel=new QLabel("", statusBar());
	statusBar()->addWidget(mSBDetailLabel);
	
	// Pixmap widgets
	mPixmapDock = createDockWidget("Image",SmallIcon("gwenview"),NULL,i18n("Image"));

	mPixmapView=new GVScrollPixmapView(mPixmapDock,mGVPixmap,actionCollection());
	mPixmapDock->setWidget(mPixmapView);
	setView(mPixmapDock);
	setMainDockWidget(mPixmapDock);

	// Folder widget
	mFolderDock = createDockWidget("Folders",SmallIcon("folder_open"),NULL,i18n("Folders"));
	mDirView=new GVDirView(mFolderDock);
	mFolderDock->setWidget(mDirView);

	// File widget
	mFileDock = createDockWidget("Files",SmallIcon("image"),NULL,i18n("Files"));
	mFileViewStack=new GVFileViewStack(this,actionCollection());
	mFileDock->setWidget(mFileViewStack);

	// Meta info edit widget
	mMetaDock = createDockWidget("File Attributes", SmallIcon("doc"),NULL,
		i18n("File Info"));
	mMetaEdit = new GVMetaEdit(mMetaDock, mGVPixmap);
	mMetaDock->setWidget(mMetaEdit);

	// Slide show controller (not really a widget)
	mSlideShow=new GVSlideShow(mFileViewStack->selectFirst(),mFileViewStack->selectNext());
	
	// Default dock config
	setGeometry(20,20,600,400);
	mFolderDock->manualDock( mPixmapDock,KDockWidget::DockLeft,30);
	mFileDock->manualDock( mPixmapDock,KDockWidget::DockTop,30);
	mMetaDock->manualDock( mPixmapDock,KDockWidget::DockTop,30);

	// Load config
	readDockConfig(config,CONFIG_DOCK_GROUP);
	mFileViewStack->readConfig(config,CONFIG_FILEWIDGET_GROUP);
	mPixmapView->readConfig(config,CONFIG_PIXMAPWIDGET_GROUP);
	mSlideShow->readConfig(config,CONFIG_SLIDESHOW_GROUP);
	GVJPEGTran::readConfig(config,CONFIG_JPEGTRAN_GROUP);
}


void GVMainWindow::createActions() {
	// File
	mOpenFile=KStdAction::open(this,SLOT(openFile()),actionCollection() );
	mSaveFile=KStdAction::save(mGVPixmap,SLOT(save()),actionCollection() );
	mSaveFileAs=KStdAction::saveAs(mGVPixmap,SLOT(saveAs()),actionCollection() );
	mRenameFile=new KAction(i18n("&Rename..."),Key_F2,this,SLOT(renameFile()),actionCollection(),"file_rename");
	mCopyFiles=new KAction(i18n("&Copy To..."),Key_F5,this,SLOT(copyFiles()),actionCollection(),"file_copy");
	mMoveFiles=new KAction(i18n("&Move To..."),Key_F6,this,SLOT(moveFiles()),actionCollection(),"file_move");
	mDeleteFiles=new KAction(i18n("&Delete"),"editdelete",Key_Delete,this,SLOT(deleteFiles()),actionCollection(),"file_delete");
	mShowFileProperties=new KAction(i18n("Properties"),0,this,SLOT(showFileProperties()),actionCollection(),"file_properties");
	KStdAction::quit( kapp, SLOT (closeAllWindows()), actionCollection() );

	// Edit
	mRotateLeft=new KAction(i18n("Rotate &Left"),"rotate_left",CTRL + Key_L,mGVPixmap,SLOT(rotateLeft()),actionCollection(),"rotate_left");
	mRotateRight=new KAction(i18n("Rotate &Right"),"rotate_right",CTRL + Key_R,mGVPixmap,SLOT(rotateRight()),actionCollection(),"rotate_right");
	mMirror=new KAction(i18n("&Mirror"),"mirror",0,mGVPixmap,SLOT(mirror()),actionCollection(),"mirror");
	mFlip=new KAction(i18n("&Flip"),"flip",0,mGVPixmap,SLOT(flip()),actionCollection(),"flip");
	mOpenWithEditor=new KAction(i18n("Open with &Editor"),"paintbrush",0,this,SLOT(openWithEditor()),actionCollection(),"open_with_editor");

	// View
	mStop=new KAction(i18n("Stop"),"stop",Key_Escape,mFileViewStack,SLOT(cancel()),actionCollection(),"stop");
	mStop->setEnabled(false);
#if KDE_IS_VERSION(3,1,90)
	mToggleFullScreen= KStdAction::fullScreen(this,SLOT(toggleFullScreen()),actionCollection());
#else
	mToggleFullScreen=new KToggleAction(i18n("Full Screen"),"window_fullscreen",CTRL + Key_F,this,SLOT(toggleFullScreen()),actionCollection(),"fullscreen");
#endif
	mToggleSlideShow=new KToggleAction(i18n("Slide Show..."),"slideshow",0,this,SLOT(toggleSlideShow()),actionCollection(),"slideshow");
	mToggleDirAndFileViews=new KAction(i18n("Hide Folder && File Views"),CTRL + Key_Return,this,SLOT(toggleDirAndFileViews()),actionCollection(),"toggle_dir_and_file_views");

	// Go
	mOpenParentDir=KStdAction::up(this, SLOT(openParentDir()), actionCollection() );
	mOpenHomeDir=KStdAction::home(this, SLOT(openHomeDir()), actionCollection() );

	// Bookmarks
	QString file = locate( "data", "kfile/bookmarks.xml" );
	if (file.isEmpty()) {
		file = locateLocal( "data", "kfile/bookmarks.xml" );
	}

	KBookmarkManager* manager=KBookmarkManager::managerForFile(file,false);
	manager->setUpdate(true);
	manager->setShowNSBookmarks(false);

	GVBookmarkOwner* bookmarkOwner=new GVBookmarkOwner(this);

	KActionMenu* bookmark=new KActionMenu(i18n( "&Bookmarks" ), "bookmark", actionCollection(), "bookmarks" );
	new KBookmarkMenu(manager, bookmarkOwner, bookmark->popupMenu(), this->actionCollection(), true);

	connect(bookmarkOwner,SIGNAL(openURL(const KURL&)),
		mGVPixmap,SLOT(setDirURL(const KURL&)) );

	connect(mGVPixmap,SIGNAL(urlChanged(const KURL&,const QString&)),
		bookmarkOwner,SLOT(setURL(const KURL&)) );

	// Settings
	mShowConfigDialog=KStdAction::preferences(this, SLOT(showConfigDialog()), actionCollection() );
	mShowKeyDialog=KStdAction::keyBindings(this, SLOT(showKeyDialog()), actionCollection() );

	actionCollection()->readShortcutSettings();
}


void GVMainWindow::createHideShowAction(KDockWidget* dock) {
	QString caption;
	if (dock->mayBeHide()) {
		caption=i18n("Hide %1").arg(dock->caption());
	} else {
		caption=i18n("Show %1").arg(dock->caption());
	}

	KAction* action=new KAction(caption, 0, dock, SLOT(changeHideShowState()), (QObject*)0 );
	if (dock->icon()) {
		action->setIconSet( QIconSet(*dock->icon()) );
	}
	mWindowListActions.append(action);
}


void GVMainWindow::updateWindowActions() {
	QString caption;
	if (mFolderDock->mayBeHide() || mFileDock->mayBeHide()) {
		caption=i18n("Hide Folder && File Views");
	} else {
		caption=i18n("Show Folder && File Views");
	}
	mToggleDirAndFileViews->setText(caption);

	unplugActionList("winlist");
	mWindowListActions.clear();
	createHideShowAction(mFolderDock);
	createHideShowAction(mFileDock);
	createHideShowAction(mPixmapDock);
	createHideShowAction(mMetaDock);
	plugActionList("winlist", mWindowListActions);
}


void GVMainWindow::createConnections() {
	// Dir view connections
	connect(mDirView,SIGNAL(dirURLChanged(const KURL&)),
		mGVPixmap,SLOT(setDirURL(const KURL&)) );

	// Pixmap view connections
	connect(mPixmapView,SIGNAL(selectPrevious()),
		mFileViewStack,SLOT(slotSelectPrevious()) );
	connect(mPixmapView,SIGNAL(selectNext()),
		mFileViewStack,SLOT(slotSelectNext()) );
	connect(mPixmapView,SIGNAL(zoomChanged(double)),
		this,SLOT(updateFileInfo()) );

	// Thumbnail view connections
	connect(mFileViewStack,SIGNAL(updateStarted(int)),
		this,SLOT(thumbnailUpdateStarted(int)) );
	connect(mFileViewStack,SIGNAL(updateEnded()),
		this,SLOT(thumbnailUpdateEnded()) );
	connect(mFileViewStack,SIGNAL(updatedOneThumbnail()),
		this,SLOT(thumbnailUpdateProcessedOne()) );

	// File view connections
	connect(mFileViewStack,SIGNAL(urlChanged(const KURL&)),
		mGVPixmap,SLOT(setURL(const KURL&)) );
	connect(mFileViewStack,SIGNAL(completed()),
		this,SLOT(updateStatusInfo()) );
	connect(mFileViewStack,SIGNAL(canceled()),
		this,SLOT(updateStatusInfo()) );
	// Don't connect mGVPixmap::urlChanged to mDirView. mDirView will be
	// updated _after_ the file view is done, since it's less important to the
	// user
	connect(mFileViewStack,SIGNAL(completedURLListing(const KURL&)),
		mDirView,SLOT(setURL(const KURL&)) );
		
	// GVPixmap connections
	connect(mGVPixmap,SIGNAL(loading()),
		this,SLOT(pixmapLoading()) );
	connect(mGVPixmap,SIGNAL(urlChanged(const KURL&,const QString&)),
		this,SLOT(setURL(const KURL&,const QString&)) );
	connect(mGVPixmap,SIGNAL(urlChanged(const KURL&,const QString&)),
		mFileViewStack,SLOT(setURL(const KURL&,const QString&)) );
	connect(mGVPixmap,SIGNAL(saved(const KURL&)),
		mFileViewStack,SLOT(updateThumbnail(const KURL&)) );

	// Slide show
	connect(mSlideShow,SIGNAL(finished()),
		mToggleSlideShow,SLOT(activate()) );
	
	// Address bar
	connect(mURLEdit,SIGNAL(returnPressed(const QString &)),
		this,SLOT(slotURLEditChanged(const QString &)));

	// Non configurable stop-fullscreen accel
	QAccel* accel=new QAccel(this);
	accel->connectItem(accel->insertItem(Key_Escape),this,SLOT(escapePressed()));

	// Dock related
	connect(manager(), SIGNAL(change()),
		this, SLOT(updateWindowActions()) );
}


void GVMainWindow::createLocationToolBar() {
	// Clear button
	(void)new KAction( i18n("Clear location bar"),
		QApplication::reverseLayout()?"clear_left" : "locationbar_erase",
		0, mURLEdit, SLOT(clearEdit()), actionCollection(), "clear_location");

	// URL Combo
	mURLEdit=new KHistoryCombo(this);

	mURLEdit->setDuplicatesEnabled(false);

	mURLEditCompletion=new KURLCompletion(KURLCompletion::DirCompletion);
	mURLEditCompletion->setDir("/");

	mURLEdit->setCompletionObject(mURLEditCompletion);
	mURLEdit->setAutoDeleteCompletionObject(true);

	mURLEdit->setEditText(QDir::current().absPath());
	mURLEdit->addToHistory(QDir::current().absPath());
	mURLEdit->setDuplicatesEnabled(false);

	KWidgetAction* comboAction=new KWidgetAction( mURLEdit, i18n("Location Bar"), 0,
		0, 0, actionCollection(), "location_url");
	comboAction->setShortcutConfigurable(false);
	comboAction->setAutoSized(true);

	// URL Label
	/* we use "kde toolbar widget" to avoid the flat background (looks bad with
	 * styles like Keramik). See konq_misc.cc.
	 */
	QLabel* urlLabel=new QLabel(i18n("L&ocation:"), this, "kde toolbar widget");
	(void)new KWidgetAction( urlLabel, i18n("L&ocation: "), 0, 0, 0, actionCollection(), "location_label");
	urlLabel->setBuddy(mURLEdit);


}

	
//-----------------------------------------------------------------------
//
// Properties
//
//-----------------------------------------------------------------------
void GVMainWindow::setShowMenuBarInFullScreen(bool value) {
	mShowMenuBarInFullScreen=value;
	if (!mToggleFullScreen->isChecked()) return;
	if (value) {
		menuBar()->show();
	} else {
		menuBar()->hide();
	}
}

void GVMainWindow::setShowToolBarInFullScreen(bool value) {
	mShowToolBarInFullScreen=value;
	if (!mToggleFullScreen->isChecked()) return;
	if (value) {
		toolBar()->show();
	} else {
		toolBar()->hide();
	}
}

void GVMainWindow::setShowStatusBarInFullScreen(bool value) {
	mShowStatusBarInFullScreen=value;
	if (!mToggleFullScreen->isChecked()) return;
	if (value) {
		statusBar()->show();
	} else {
		statusBar()->hide();
	}
}

void GVMainWindow::setShowLocationToolBar(bool value) {
	mShowLocationToolBar=value;
	if (!mLocationToolBar) return;
	if (value) {
		mLocationToolBar->show();
	} else {
		mLocationToolBar->hide();
	}
}


//-----------------------------------------------------------------------
//
// Configuration
//
//-----------------------------------------------------------------------
void GVMainWindow::readConfig(KConfig* config,const QString& group) {
	config->setGroup(group);
	mShowMenuBarInFullScreen=config->readBoolEntry(CONFIG_MENUBAR_IN_FS,false);
	mShowToolBarInFullScreen=config->readBoolEntry(CONFIG_TOOLBAR_IN_FS,true);
	mShowStatusBarInFullScreen=config->readBoolEntry(CONFIG_STATUSBAR_IN_FS,false);
	setShowLocationToolBar(config->readBoolEntry(CONFIG_SHOW_LOCATION_TOOLBAR,true));
}


void GVMainWindow::writeConfig(KConfig* config,const QString& group) const {
	config->setGroup(group);
	config->writeEntry(CONFIG_MENUBAR_IN_FS,mShowMenuBarInFullScreen);
	config->writeEntry(CONFIG_TOOLBAR_IN_FS,mShowToolBarInFullScreen);
	config->writeEntry(CONFIG_STATUSBAR_IN_FS,mShowStatusBarInFullScreen);
	config->writeEntry(CONFIG_SHOW_LOCATION_TOOLBAR,mShowLocationToolBar);
}
