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
#include <qcursor.h>
#include <qdockarea.h>

// KDE includes
#include <kaccel.h>
#include <kaction.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <khelpmenu.h>
#include <kiconloader.h>
#include <kkeydialog.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kprogress.h>
#include <kstatusbar.h>
#include <kstdaccel.h>
#include <kstdaction.h>
#include <kurlcompletion.h>
#include <kurlrequesterdlg.h>

// Our includes
#include "configdialog.h"
#include "fileoperation.h"
#include "gvdirview.h"
#include "gvfileviewstack.h"
#include "gvpixmap.h"
#include "gvpixmapviewstack.h"
#include "gvslideshow.h"
#include "statusbarprogress.h"

#include "gvmainwindow.moc"


enum { SB_FOLDER, SB_FILE};

const char* CONFIG_DOCK_GROUP="dock";
const char* CONFIG_MAINWINDOW_GROUP="main window";
const char* CONFIG_FILEWIDGET_GROUP="file widget";
const char* CONFIG_PIXMAPWIDGET_GROUP="pixmap widget";
const char* CONFIG_FILEOPERATION_GROUP="file operations";
const char* CONFIG_SLIDESHOW_GROUP="slide show";

const char* CONFIG_MENUBAR_IN_FS="menu bar in full screen";
const char* CONFIG_TOOLBAR_IN_FS="tool bar in full screen";
const char* CONFIG_STATUSBAR_IN_FS="status bar in full screen";
const char* CONFIG_SHOW_ADDRESSBAR="show address bar";

GVMainWindow::GVMainWindow()
: KDockMainWindow(), mProgress(0L), mAddressToolBar(0L)
{
	FileOperation::readConfig(KGlobal::config(),CONFIG_FILEOPERATION_GROUP);
	readConfig(KGlobal::config(),CONFIG_MAINWINDOW_GROUP);

	// Backend
	mGVPixmap=new GVPixmap(this);

	// GUI
	createWidgets();
	createActions();
	createMenu();
	createMainToolBar();
	createAddressToolBar();
	createConnections();
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
	mPixmapViewStack->writeConfig(config,CONFIG_PIXMAPWIDGET_GROUP);
	mFileViewStack->writeConfig(config,CONFIG_FILEWIDGET_GROUP);
	mSlideShow->writeConfig(config,CONFIG_SLIDESHOW_GROUP);
	writeDockConfig(config,CONFIG_DOCK_GROUP);
	writeConfig(config,CONFIG_MAINWINDOW_GROUP);
}


//-----------------------------------------------------------------------
//
// Public slots
//
//-----------------------------------------------------------------------
void GVMainWindow::setURL(const KURL& url,const QString&) {
	//kdDebug() << "GVMainWindow::setURL " << url.path() << " - " << filename << endl;

    bool filenameIsValid=!mGVPixmap->isNull();

	mToggleFullScreen->setEnabled(filenameIsValid);
	mRenameFile->setEnabled(filenameIsValid);
	mCopyFiles->setEnabled(filenameIsValid);
	mMoveFiles->setEnabled(filenameIsValid);
	mDeleteFiles->setEnabled(filenameIsValid);
	mShowFileProperties->setEnabled(filenameIsValid);
	mOpenWithEditor->setEnabled(filenameIsValid);
	mOpenParentDir->setEnabled(url.path()!="/");

	updateStatusBar();
	kapp->restoreOverrideCursor();

	mURLEditCompletion->addItem(url.prettyURL());
	mURLEdit->setEditText(url.prettyURL());
	mURLEdit->addToHistory(url.prettyURL());
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


void GVMainWindow::renameFile() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->renameFile();
	} else {
		mPixmapViewStack->renameFile();
	}
}


void GVMainWindow::copyFiles() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->copyFiles();
	} else {
		mPixmapViewStack->copyFile();
	}
}


void GVMainWindow::moveFiles() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->moveFiles();
	} else {
		mPixmapViewStack->moveFile();
	}
}


void GVMainWindow::deleteFiles() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->deleteFiles();
	} else {
		mPixmapViewStack->deleteFile();
	}
}


void GVMainWindow::showFileProperties() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->showFileProperties();
	} else {
		mPixmapViewStack->showFileProperties();
	}
}


void GVMainWindow::openWithEditor() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->openWithEditor();
	} else {
		mPixmapViewStack->openWithEditor();
	}
}


void GVMainWindow::openFile() {
	QString path=KFileDialog::getOpenFileName();
	if (path.isNull()) return;

	KURL url;
	url.setPath(path);
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


void GVMainWindow::toggleFullScreen() {
	KConfig* config=KGlobal::config();

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
		if (leftDock()->isEmpty())   leftDock()->hide();
		if (rightDock()->isEmpty())  rightDock()->hide();
		if (topDock()->isEmpty())    topDock()->hide();
		if (bottomDock()->isEmpty()) bottomDock()->hide();
		
		if (!mShowStatusBarInFullScreen) statusBar()->hide();
		writeDockConfig(config,CONFIG_DOCK_GROUP);
		makeDockInvisible(mFileDock);
		makeDockInvisible(mFolderDock);
		mPixmapViewStack->setFullScreen(true);
		showFullScreen();
		mPixmapViewStack->setFocus();
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
		mPixmapViewStack->setFullScreen(false);
		showNormal();
	}
}


void GVMainWindow::toggleSlideShow() {
	if (mToggleSlideShow->isChecked()) {
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
	} else if (mPixmapViewStack->isVisible()) {
		mPixmapViewStack->setFocus();
	}
}


//-----------------------------------------------------------------------
//
// GUI
//
//-----------------------------------------------------------------------
void GVMainWindow::updateStatusBar() {
	QString txt;
	uint count=mFileViewStack->fileCount();
	if (count>1) {
		txt=i18n("%1 - %2 images");
	} else {
		txt=i18n("%1 - %2 image");
	}
	txt=txt.arg(mGVPixmap->dirURL().prettyURL()).arg(count);

	statusBar()->changeItem( txt, SB_FOLDER );
	updateFileStatusBar();
}


void GVMainWindow::updateFileStatusBar() {
	QString txt;
	QString filename=mGVPixmap->filename();
	if (!filename.isEmpty()) {
		txt=QString("%1 %2x%3 @ %4%")
			.arg(filename).arg(mGVPixmap->width()).arg(mGVPixmap->height())
			.arg(int(mPixmapViewStack->zoom()*100) );
	} else {
		txt="";
	}
	statusBar()->changeItem( txt, SB_FILE );
}


void GVMainWindow::createWidgets() {
	KConfig* config=KGlobal::config();

	// Status bar
	statusBar()->insertItem("",SB_FOLDER);
	statusBar()->setItemAlignment(SB_FOLDER, AlignLeft|AlignVCenter);
	statusBar()->insertItem("",SB_FILE,1);
	statusBar()->setItemAlignment(SB_FILE, AlignLeft|AlignVCenter);

	// Pixmap widgets
	mPixmapDock = createDockWidget("Image",SmallIcon("gwenview"),NULL,i18n("Image"));

	mPixmapViewStack=new GVPixmapViewStack(mPixmapDock,mGVPixmap,actionCollection());
	mPixmapDock->setWidget(mPixmapViewStack);
	setView(mPixmapDock);
	setMainDockWidget(mPixmapDock);

	// Folder widget
	mFolderDock = createDockWidget("Folders",SmallIcon("folder_open"),NULL,i18n("Folders"));
	mGVDirView=new GVDirView(mFolderDock);
	mFolderDock->setWidget(mGVDirView);

	// File widget
	mFileDock = createDockWidget("Files",SmallIcon("image"),NULL,i18n("Files"));
	mFileViewStack=new GVFileViewStack(this,actionCollection());
	mFileDock->setWidget(mFileViewStack);

	// Slide show controller (not really a widget)
	mSlideShow=new GVSlideShow(mFileViewStack->selectFirst(),mFileViewStack->selectNext());
	
	// Default dock config
	setGeometry(20,20,600,400);
	mFolderDock->manualDock( mPixmapDock,KDockWidget::DockLeft,30);
	mFileDock->manualDock( mPixmapDock,KDockWidget::DockTop,30);

	// Load config
	readDockConfig(config,CONFIG_DOCK_GROUP);
	mFileViewStack->readConfig(config,CONFIG_FILEWIDGET_GROUP);
	mPixmapViewStack->readConfig(config,CONFIG_PIXMAPWIDGET_GROUP);
	mSlideShow->readConfig(config,CONFIG_SLIDESHOW_GROUP);
}


void GVMainWindow::createActions() {
	mOpenFile=KStdAction::open(this, SLOT(openFile()),actionCollection() );
	
	mRenameFile=new KAction(i18n("&Rename..."),Key_F2,this,SLOT(renameFile()),actionCollection(),"file_rename");

	mCopyFiles=new KAction(i18n("&Copy To..."),Key_F5,this,SLOT(copyFiles()),actionCollection(),"file_copy");

	mMoveFiles=new KAction(i18n("&Move To..."),Key_F6,this,SLOT(moveFiles()),actionCollection(),"file_move");

	mDeleteFiles=new KAction(i18n("&Delete..."),"editdelete",Key_Delete,this,SLOT(deleteFiles()),actionCollection(),"file_delete");

	mOpenWithEditor=new KAction(i18n("Open with &Editor"),"paintbrush",0,this,SLOT(openWithEditor()),actionCollection(),"file_edit");

	mToggleFullScreen=new KToggleAction(i18n("Full Screen"),"window_fullscreen",CTRL + Key_F,this,SLOT(toggleFullScreen()),actionCollection(),"view_fullscreen");

	mShowConfigDialog=new KAction(i18n("Configure Gwenview..."),"configure",0,this,SLOT(showConfigDialog()),actionCollection(),"show_config_dialog");

	mShowKeyDialog=KStdAction::keyBindings(this,SLOT(showKeyDialog()),actionCollection(),"show_key_dialog");
	
	mStop=new KAction(i18n("Stop"),"stop",Key_Escape,mFileViewStack,SLOT(cancel()),actionCollection(),"stop");
	mStop->setEnabled(false);

	mOpenParentDir=KStdAction::up(this, SLOT(openParentDir()),actionCollection() );

	mShowFileProperties=new KAction(i18n("Properties..."),0,this,SLOT(showFileProperties()),actionCollection(),"show_file_properties");

	mToggleSlideShow=new KToggleAction(i18n("Slide show"),"slideshow",0,this,SLOT(toggleSlideShow()),actionCollection(),"view_slideshow");
	
	actionCollection()->readShortcutSettings();
}


void GVMainWindow::createConnections() {
	// Dir view connections
	connect(mGVDirView,SIGNAL(dirURLChanged(const KURL&)),
		mGVPixmap,SLOT(setDirURL(const KURL&)) );

	// Pixmap view connections
	connect(mPixmapViewStack,SIGNAL(selectPrevious()),
		mFileViewStack,SLOT(slotSelectPrevious()) );
	connect(mPixmapViewStack,SIGNAL(selectNext()),
		mFileViewStack,SLOT(slotSelectNext()) );

	// Scroll pixmap view connections
	connect(mPixmapViewStack,SIGNAL(zoomChanged(double)),
		this,SLOT(updateFileStatusBar()) );

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
		this,SLOT(updateStatusBar()) );
	connect(mFileViewStack,SIGNAL(canceled()),
		this,SLOT(updateStatusBar()) );
		
	// GVPixmap connections
	connect(mGVPixmap,SIGNAL(loading()),
		this,SLOT(pixmapLoading()) );
	connect(mGVPixmap,SIGNAL(urlChanged(const KURL&,const QString&)),
		this,SLOT(setURL(const KURL&,const QString&)) );
	connect(mGVPixmap,SIGNAL(urlChanged(const KURL&,const QString&)),
		mGVDirView,SLOT(setURL(const KURL&,const QString&)) );
	connect(mGVPixmap,SIGNAL(urlChanged(const KURL&,const QString&)),
		mFileViewStack,SLOT(setURL(const KURL&,const QString&)) );

	// Slide show
	connect(mSlideShow,SIGNAL(finished()),
		mToggleSlideShow,SLOT(activate()) );
	
	// Address bar
	connect(mURLEdit,SIGNAL(returnPressed(const QString &)),
		this,SLOT(slotURLEditChanged(const QString &)));

	// Non configurable stop-fullscreen accel
	QAccel* accel=new QAccel(this);
	accel->connectItem(accel->insertItem(Key_Escape),this,SLOT(escapePressed()));
}


void GVMainWindow::createMenu() {
	QPopupMenu* fileMenu = new QPopupMenu;

	mOpenFile->plug(fileMenu);
	fileMenu->insertSeparator();
	mOpenWithEditor->plug(fileMenu);
	mRenameFile->plug(fileMenu);
	mCopyFiles->plug(fileMenu);
	mMoveFiles->plug(fileMenu);
	mDeleteFiles->plug(fileMenu);
	fileMenu->insertSeparator();
	mShowFileProperties->plug(fileMenu);
	fileMenu->insertSeparator();
	KStdAction::quit( kapp, SLOT (closeAllWindows()), actionCollection() )->plug(fileMenu);
	menuBar()->insertItem(i18n("&File"), fileMenu);

	QPopupMenu* viewMenu = new QPopupMenu;
	mStop->plug(viewMenu);
	
	viewMenu->insertSeparator();
	mFileViewStack->noThumbnails()->plug(viewMenu);
	mFileViewStack->smallThumbnails()->plug(viewMenu);
	mFileViewStack->medThumbnails()->plug(viewMenu);
	mFileViewStack->largeThumbnails()->plug(viewMenu);
	
	viewMenu->insertSeparator();
	mToggleFullScreen->plug(viewMenu);
	mToggleSlideShow->plug(viewMenu);

	viewMenu->insertSeparator();
	mPixmapViewStack->autoZoom()->plug(viewMenu);
	mPixmapViewStack->zoomIn()->plug(viewMenu);
	mPixmapViewStack->zoomOut()->plug(viewMenu);
	mPixmapViewStack->resetZoom()->plug(viewMenu);
	mPixmapViewStack->lockZoom()->plug(viewMenu);
	menuBar()->insertItem(i18n("&View"), viewMenu);

	QPopupMenu* goMenu = new QPopupMenu;
	mOpenParentDir->plug(goMenu);
	mFileViewStack->selectFirst()->plug(goMenu);
	mFileViewStack->selectPrevious()->plug(goMenu);
	mFileViewStack->selectNext()->plug(goMenu);
	mFileViewStack->selectLast()->plug(goMenu);
	menuBar()->insertItem(i18n("&Go"), goMenu);

	QPopupMenu* settingsMenu = new QPopupMenu;
	mShowConfigDialog->plug(settingsMenu);
	mShowKeyDialog->plug(settingsMenu);
	menuBar()->insertItem(i18n("&Settings"),settingsMenu);

	menuBar()->insertItem(i18n("&Windows"), dockHideShowMenu());

	menuBar()->insertItem(i18n("&Help"), helpMenu());

	menuBar()->show();
}


void GVMainWindow::createMainToolBar() {
	mMainToolBar=new KToolBar(this,topDock(),true);
	mMainToolBar->setLabel(i18n("Main tool bar"));
	mOpenParentDir->plug(mMainToolBar);
	mFileViewStack->selectFirst()->plug(mMainToolBar);
	mFileViewStack->selectPrevious()->plug(mMainToolBar);
	mFileViewStack->selectNext()->plug(mMainToolBar);
	mFileViewStack->selectLast()->plug(mMainToolBar);

	mMainToolBar->insertLineSeparator();
	mStop->plug(mMainToolBar);

	mMainToolBar->insertLineSeparator();
	mFileViewStack->noThumbnails()->plug(mMainToolBar);
	mFileViewStack->smallThumbnails()->plug(mMainToolBar);
	mFileViewStack->medThumbnails()->plug(mMainToolBar);
	mFileViewStack->largeThumbnails()->plug(mMainToolBar);

	mMainToolBar->insertLineSeparator();
	mToggleFullScreen->plug(mMainToolBar);
	mToggleSlideShow->plug(mMainToolBar);
	
	mMainToolBar->insertLineSeparator();
	mPixmapViewStack->autoZoom()->plug(mMainToolBar);
	mPixmapViewStack->zoomIn()->plug(mMainToolBar);
	mPixmapViewStack->zoomOut()->plug(mMainToolBar);
	mPixmapViewStack->resetZoom()->plug(mMainToolBar);
	mPixmapViewStack->lockZoom()->plug(mMainToolBar);
}


void GVMainWindow::createAddressToolBar() {
	mAddressToolBar=new KToolBar(this,topDock(),true);
	if (!mShowAddressBar) mAddressToolBar->hide();
	mAddressToolBar->setLabel(i18n("Address tool bar"));

	/* FIXME: Add a tooltip to the erase button and change "URL:" to
	 * "Location:"
	 */
	mAddressToolBar->insertButton("locationbar_erase",1,true);
	/* we use "kde toolbar widget" to avoid the flat background (looks bad with
	 * styles like Keramik). See konq_misc.cc.
	 */
	QLabel* urlLabel=new QLabel(i18n("&URL:"),mAddressToolBar,"kde toolbar widget");
	mURLEdit=new KHistoryCombo(mAddressToolBar);
	urlLabel->setBuddy(mURLEdit);
	mAddressToolBar->addConnection(1,SIGNAL(clicked(int)),mURLEdit,SLOT(clearEdit()));
	
	mURLEdit->setDuplicatesEnabled(false);
	
	mURLEditCompletion=new KURLCompletion(KURLCompletion::DirCompletion);
	mURLEditCompletion->setDir("/");
	
	mURLEdit->setCompletionObject(mURLEditCompletion);
	mURLEdit->setAutoDeleteCompletionObject(true);

	mURLEdit->setEditText(QDir::current().absPath());
	mURLEdit->addToHistory(QDir::current().absPath());
	mURLEdit->setDuplicatesEnabled(false);
	mAddressToolBar->setStretchableWidget(mURLEdit);
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

void GVMainWindow::setShowAddressBar(bool value) {
	mShowAddressBar=value;
	if (!mAddressToolBar) return;
	if (value) {
		mAddressToolBar->show();
	} else {
		mAddressToolBar->hide();
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
	setShowAddressBar(config->readBoolEntry(CONFIG_SHOW_ADDRESSBAR,true));
}


void GVMainWindow::writeConfig(KConfig* config,const QString& group) const {
	config->setGroup(group);
	config->writeEntry(CONFIG_MENUBAR_IN_FS,mShowMenuBarInFullScreen);
	config->writeEntry(CONFIG_TOOLBAR_IN_FS,mShowToolBarInFullScreen);
	config->writeEntry(CONFIG_STATUSBAR_IN_FS,mShowStatusBarInFullScreen);
	config->writeEntry(CONFIG_SHOW_ADDRESSBAR,mShowAddressBar);
}
