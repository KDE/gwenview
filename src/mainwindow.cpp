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
#include "gvslideshow.h"
#include "pixmapview.h"
#include "statusbarprogress.h"

#include "mainwindow.moc"


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

MainWindow::MainWindow()
: KDockMainWindow(), mProgress(0L)
{
	FileOperation::readConfig(KGlobal::config(),CONFIG_FILEOPERATION_GROUP);
	readConfig(KGlobal::config(),CONFIG_MAINWINDOW_GROUP);

// Backend
	mGVPixmap=new GVPixmap(this);

// GUI
	createWidgets();
	createActions();
	createAccels();
	createMenu();
	createMainToolBar();
	createAddressToolBar();

// Slide show
	mSlideShow=new GVSlideShow(mFileViewStack->selectFirst(),mFileViewStack->selectNext());
	mSlideShow->readConfig(KGlobal::config(),CONFIG_SLIDESHOW_GROUP);
	
// Dir view connections
	connect(mGVDirView,SIGNAL(dirURLChanged(const KURL&)),
		mGVPixmap,SLOT(setDirURL(const KURL&)) );

// Pixmap view connections
	connect(mPixmapView,SIGNAL(selectPrevious()),
		mFileViewStack,SLOT(slotSelectPrevious()) );
	connect(mPixmapView,SIGNAL(selectNext()),
		mFileViewStack,SLOT(slotSelectNext()) );

// Scroll pixmap view connections
	connect(mPixmapView,SIGNAL(zoomChanged(double)),
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
	
// Set focus
	if (mFileViewStack->isVisible()) {
		mFileViewStack->setFocus();
	} else if (mPixmapView->isVisible()) {
		mPixmapView->setFocus();
	}
}


MainWindow::~MainWindow() {
	KConfig* config=KGlobal::config();
	FileOperation::writeConfig(config,CONFIG_FILEOPERATION_GROUP);
	mPixmapView->writeConfig(config,CONFIG_PIXMAPWIDGET_GROUP);
	mFileViewStack->writeConfig(config,CONFIG_FILEWIDGET_GROUP);
	mSlideShow->writeConfig(config,CONFIG_SLIDESHOW_GROUP);
	writeDockConfig(config,CONFIG_DOCK_GROUP);
	writeConfig(config,CONFIG_MAINWINDOW_GROUP);
	mAccel->writeSettings();
}


//-Public slots----------------------------------------------------------
void MainWindow::setURL(const KURL& url,const QString&) {
	//kdDebug() << "MainWindow::setURL " << url.path() << " - " << filename << endl;

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
void MainWindow::openParentDir() {
	KURL url=mGVPixmap->dirURL().upURL();
	mGVPixmap->setURL(url);	
}


void MainWindow::renameFile() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->renameFile();
	} else {
		mPixmapView->renameFile();
	}
}


void MainWindow::copyFiles() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->copyFiles();
	} else {
		mPixmapView->copyFile();
	}
}


void MainWindow::moveFiles() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->moveFiles();
	} else {
		mPixmapView->moveFile();
	}
}


void MainWindow::deleteFiles() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->deleteFiles();
	} else {
		mPixmapView->deleteFile();
	}
}


void MainWindow::showFileProperties() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->showFileProperties();
	} else {
		mPixmapView->showFileProperties();
	}
}


void MainWindow::openWithEditor() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->openWithEditor();
	} else {
		mPixmapView->openWithEditor();
	}
}


void MainWindow::openFile() {
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
void MainWindow::pixmapLoading() {
	kapp->setOverrideCursor(QCursor(WaitCursor));
}


void MainWindow::toggleFullScreen() {
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
		mPixmapView->setFullScreen(true);
		showFullScreen();
		mPixmapView->setFocus();
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
}


void MainWindow::toggleSlideShow() {
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


void MainWindow::showConfigDialog() {
	ConfigDialog dialog(this,this);
	dialog.exec();
}


void MainWindow::showKeyDialog() {
	KKeyDialog::configureKeys(mAccel);
}


void MainWindow::escapePressed() {
	if (mToggleSlideShow->isChecked()) {
		mToggleSlideShow->activate();
		return;
	}
	if (mToggleFullScreen->isChecked()) {
		mToggleFullScreen->activate();
	}
}


void MainWindow::thumbnailUpdateStarted(int count) {
	mProgress=new StatusBarProgress(statusBar(),i18n("Generating thumbnails..."),count);
	mProgress->progress()->setFormat("%v/%m");
	mProgress->show();
	mStop->setEnabled(true);
}


void MainWindow::thumbnailUpdateEnded() {
	mStop->setEnabled(false);
	if (mProgress) {
		mProgress->hide();
		delete mProgress;
		mProgress=0L;
	}
}


void MainWindow::thumbnailUpdateProcessedOne() {
	mProgress->progress()->advance(1);
}


void MainWindow::slotURLEditChanged(const QString &str) {
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
void MainWindow::updateStatusBar() {
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


void MainWindow::updateFileStatusBar() {
	QString txt;
	QString filename=mGVPixmap->filename();
	if (!filename.isEmpty()) {
		txt=QString("%1 %2x%3 @ %4%")
			.arg(filename).arg(mGVPixmap->width()).arg(mGVPixmap->height())
			.arg(int(mPixmapView->zoom()*100) );
	} else {
		txt="";
	}
	statusBar()->changeItem( txt, SB_FILE );
}


void MainWindow::createWidgets() {
	KConfig* config=KGlobal::config();

// Status bar
	statusBar()->insertItem("",SB_FOLDER);
	statusBar()->setItemAlignment(SB_FOLDER, AlignLeft|AlignVCenter);
	statusBar()->insertItem("",SB_FILE,1);
	statusBar()->setItemAlignment(SB_FILE, AlignLeft|AlignVCenter);

// Pixmap widgets
	mPixmapDock = createDockWidget("Image",SmallIcon("gwenview"),NULL,i18n("Image"));

	mPixmapView=new PixmapView(mPixmapDock,mGVPixmap,actionCollection());
	mPixmapDock->setWidget(mPixmapView);
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

// Default dock config
	setGeometry(20,20,600,400);
	mFolderDock->manualDock( mPixmapDock,KDockWidget::DockLeft,30);
	mFileDock->manualDock( mPixmapDock,KDockWidget::DockTop,30);

// Load config
	readDockConfig(config,CONFIG_DOCK_GROUP);
	mFileViewStack->readConfig(config,CONFIG_FILEWIDGET_GROUP);
	mPixmapView->readConfig(config,CONFIG_PIXMAPWIDGET_GROUP);
}


void MainWindow::createActions() {
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
}


void MainWindow::createAccels() {
// Associate actions with accelerator
	mAccel=new KAccel(this);
	int count=actionCollection()->count();

	for (int pos=0;pos<count;++pos) {
		KAction *action = actionCollection()->action(pos);
		if ( action->accel() ) {
			action->plugAccel(mAccel);
		}
	}
	mFileViewStack->plugActionsToAccel(mAccel);
	mPixmapView->plugActionsToAccel(mAccel);

// Read user accelerator
	mAccel->readSettings();

	QAccel* accel=new QAccel(this);
	accel->connectItem(accel->insertItem(Key_Escape),this,SLOT(escapePressed()));
}


void MainWindow::createMenu() {
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
	mPixmapView->autoZoom()->plug(viewMenu);
	mPixmapView->zoomIn()->plug(viewMenu);
	mPixmapView->zoomOut()->plug(viewMenu);
	mPixmapView->resetZoom()->plug(viewMenu);
	mPixmapView->lockZoom()->plug(viewMenu);
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


void MainWindow::createMainToolBar() {
	KToolBar* mainBar=new KToolBar(this,topDock(),true);
	mainBar->setLabel(i18n("Main tool bar"));
	mOpenParentDir->plug(toolBar());
	mFileViewStack->selectFirst()->plug(toolBar());
	mFileViewStack->selectPrevious()->plug(toolBar());
	mFileViewStack->selectNext()->plug(toolBar());
	mFileViewStack->selectLast()->plug(toolBar());

	toolBar()->insertLineSeparator();
	mStop->plug(toolBar());

	toolBar()->insertLineSeparator();
	mFileViewStack->noThumbnails()->plug(toolBar());
	mFileViewStack->smallThumbnails()->plug(toolBar());
	mFileViewStack->medThumbnails()->plug(toolBar());
	mFileViewStack->largeThumbnails()->plug(toolBar());

	toolBar()->insertLineSeparator();
	mToggleFullScreen->plug(toolBar());
	mToggleSlideShow->plug(toolBar());
	
	toolBar()->insertLineSeparator();
	mPixmapView->autoZoom()->plug(toolBar());
	mPixmapView->zoomIn()->plug(toolBar());
	mPixmapView->zoomOut()->plug(toolBar());
	mPixmapView->resetZoom()->plug(toolBar());
	mPixmapView->lockZoom()->plug(toolBar());
}


void MainWindow::createAddressToolBar() {
	KToolBar* addressBar=new KToolBar(this,topDock(),true);
	addressBar->setLabel(i18n("Address tool bar"));

	QLabel* urlLabel=new QLabel(i18n("&URL:"),addressBar);
	mURLEdit=new KHistoryCombo(addressBar);
	urlLabel->setBuddy(mURLEdit);
	
	mURLEdit->setDuplicatesEnabled(false);
	
	mURLEditCompletion=new KURLCompletion(KURLCompletion::DirCompletion);
	mURLEditCompletion->setDir("/");
	
	mURLEdit->setCompletionObject(mURLEditCompletion);
	mURLEdit->setAutoDeleteCompletionObject(true);

	mURLEdit->setEditText(QDir::current().absPath());
	mURLEdit->addToHistory(QDir::current().absPath());
	mURLEdit->setDuplicatesEnabled(false);
	addressBar->setStretchableWidget(mURLEdit);
}

	
//-Properties-----------------------------------------------	
void MainWindow::setShowMenuBarInFullScreen(bool value) {
	mShowMenuBarInFullScreen=value;
	if (!mToggleFullScreen->isChecked()) return;
	if (value) {
		menuBar()->show();
	} else {
		menuBar()->hide();
	}
}

void MainWindow::setShowToolBarInFullScreen(bool value) {
	mShowToolBarInFullScreen=value;
	if (!mToggleFullScreen->isChecked()) return;
	if (value) {
		toolBar()->show();
	} else {
		toolBar()->hide();
	}
}

void MainWindow::setShowStatusBarInFullScreen(bool value) {
	mShowStatusBarInFullScreen=value;
	if (!mToggleFullScreen->isChecked()) return;
	if (value) {
		statusBar()->show();
	} else {
		statusBar()->hide();
	}
}


//-Configuration--------------------------------------------	
void MainWindow::readConfig(KConfig* config,const QString& group) {
	config->setGroup(group);
	mShowMenuBarInFullScreen=config->readBoolEntry(CONFIG_MENUBAR_IN_FS,false);
	mShowToolBarInFullScreen=config->readBoolEntry(CONFIG_TOOLBAR_IN_FS,true);
	mShowStatusBarInFullScreen=config->readBoolEntry(CONFIG_STATUSBAR_IN_FS,false);
}


void MainWindow::writeConfig(KConfig* config,const QString& group) const {
	config->setGroup(group);
	config->writeEntry(CONFIG_MENUBAR_IN_FS,mShowMenuBarInFullScreen);
	config->writeEntry(CONFIG_TOOLBAR_IN_FS,mShowToolBarInFullScreen);
	config->writeEntry(CONFIG_STATUSBAR_IN_FS,mShowStatusBarInFullScreen);
}
