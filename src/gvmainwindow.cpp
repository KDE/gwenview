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
#include <qcursor.h>
#include <qdir.h>
#include <qdockarea.h>
#include <qhbox.h>
#include <qtooltip.h>

// KDE
#include <kaboutdata.h>
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
#include <kedittoolbar.h>
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
#include <kio/netaccess.h>
#include <kurlcompletion.h>
#include <kurlrequesterdlg.h>
#include <kprinter.h>

#include <config.h>
// KIPI
#ifdef HAVE_KIPI
#include <libkipi/plugin.h>
#include <libkipi/pluginloader.h>
#endif

// Local
#include "fileoperation.h"
#include "gvbatchmanipulator.h"
#include "gvbookmarkowner.h"
#include "gvconfigdialog.h"
#include "gvdirview.h"
#include "gvdocument.h"
#include "gvexternaltooldialog.h"
#include "gvfileviewstack.h"
#include "gvhistory.h"
#include "gvjpegtran.h"
#include "gvscrollpixmapview.h"
#include "gvslideshow.h"
#include "gvslideshowdialog.h"
#include "gvmetaedit.h"
#include "gvprintdialog.h"
#include "statusbarprogress.h"
#include "gvcache.h"
#include "thumbnailloadjob.h"

#include "config.h"

#ifdef HAVE_KIPI
#include "gvkipiinterface.h"
#endif

#if KDE_VERSION < 0x30100
#include "libgvcompat/kwidgetaction.h"
#else
#include <kcursor.h>
#endif

#include "gvmainwindow.moc"

const char* CONFIG_DOCK_GROUP="dock";
const char* CONFIG_MAINWINDOW_GROUP="main window";
const char* CONFIG_FILEWIDGET_GROUP="file widget";
const char* CONFIG_DIRWIDGET_GROUP="dir widget";
const char* CONFIG_JPEGTRAN_GROUP="jpegtran";
const char* CONFIG_PIXMAPWIDGET_GROUP="pixmap widget";
const char* CONFIG_FILEOPERATION_GROUP="file operations";
const char* CONFIG_SLIDESHOW_GROUP="slide show";
const char* CONFIG_CACHE_GROUP="cache";

const char* CONFIG_MENUBAR_IN_FS="menu bar in full screen";
const char* CONFIG_TOOLBAR_IN_FS="tool bar in full screen";
const char* CONFIG_STATUSBAR_IN_FS="status bar in full screen";
const char* CONFIG_BUSYPTR_IN_FS="busy ptr in full screen";
const char* CONFIG_SHOW_LOCATION_TOOLBAR="show address bar";
const char* CONFIG_AUTO_DELETE_THUMBNAIL_CACHE="Delete Thumbnail Cache whe exit";


//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif


GVMainWindow::GVMainWindow()
: KDockMainWindow(), mProgress(0L), mLocationToolBar(0L), mLoadingCursor(false)
{
	FileOperation::readConfig(KGlobal::config(),CONFIG_FILEOPERATION_GROUP);
	readConfig(KGlobal::config(),CONFIG_MAINWINDOW_GROUP);

	// Backend
	mDocument=new GVDocument(this);
	mGVHistory=new GVHistory(mDocument, actionCollection());
	// GUI
	createWidgets();
	createActions();
	createLocationToolBar();

	#if KDE_VERSION >= 0x30100
	setStandardToolBarMenuEnabled(true);
	#endif
	createGUI("gwenviewui.rc", false);
	createConnections();
	mWindowListActions.setAutoDelete(true);
	updateWindowActions();
	loadPlugins();
	applyMainWindowSettings();

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
		mToggleFullScreen->activate();
	}

	// Go to requested file
	mDocument->setURL(url);
}


bool GVMainWindow::queryClose() {
	if (!mDocument->saveBeforeClosing()) return false;

	KConfig* config=KGlobal::config();
	FileOperation::writeConfig(config, CONFIG_FILEOPERATION_GROUP);
	mPixmapView->writeConfig(config, CONFIG_PIXMAPWIDGET_GROUP);
	mFileViewStack->writeConfig(config, CONFIG_FILEWIDGET_GROUP);
	mDirView->writeConfig(config, CONFIG_DIRWIDGET_GROUP);
	mSlideShow->writeConfig(config, CONFIG_SLIDESHOW_GROUP);
	GVJPEGTran::writeConfig(config, CONFIG_JPEGTRAN_GROUP);

	// Don't store dock layout if only the image dock is visible. This avoid
	// saving layout when in "fullscreen" or "image only" mode.
	if (mFileViewStack->isVisible() || mDirView->isVisible()) {
		writeDockConfig(config,CONFIG_DOCK_GROUP);
	}
	writeConfig(config,CONFIG_MAINWINDOW_GROUP);

	// If we are in fullscreen mode, we need to make the needed GUI elements
	// visible before saving the window settings.
	if (mToggleFullScreen->isChecked()) {
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
	}
	
	if (mAutoDeleteThumbnailCache) {
		QString dir=ThumbnailLoadJob::thumbnailBaseDir();

		if (QFile::exists(dir)) {
			KURL url;
			url.setPath(dir);
#if KDE_VERSION >= 0x30200
			KIO::NetAccess::del(url, 0);
#else
			KIO::NetAccess::del(url);
#endif
		}
	}
	
	saveMainWindowSettings(KGlobal::config(), "MainWindow");

	return true;
}


//-----------------------------------------------------------------------
//
// Public slots
//
//-----------------------------------------------------------------------
void GVMainWindow::setURL(const KURL& url,const QString& /*filename*/) {
	LOG(url.path() << " - " << filename);

	bool filenameIsValid=!mDocument->isNull();

	mRenameFile->setEnabled(filenameIsValid);
	mCopyFiles->setEnabled(filenameIsValid);
	mMoveFiles->setEnabled(filenameIsValid);
	mDeleteFiles->setEnabled(filenameIsValid);
	mShowFileProperties->setEnabled(filenameIsValid);
	mRotateLeft->setEnabled(filenameIsValid);
	mRotateRight->setEnabled(filenameIsValid);
	mMirror->setEnabled(filenameIsValid);
	mFlip->setEnabled(filenameIsValid);
	mSaveFile->setEnabled(filenameIsValid);
	mSaveFileAs->setEnabled(filenameIsValid);
	mFilePrint->setEnabled(filenameIsValid);
	mReload->setEnabled(filenameIsValid);

	QPopupMenu *upPopup = mGoUp->popupMenu();
	upPopup->clear();
	int i = 0;
	KURL urlUp = url.upURL();
	while(urlUp.hasPath()) {
		upPopup->insertItem(urlUp.url()), urlUp.prettyURL();
		if(urlUp.path() == "/" || ++i > 10)
			break;
		urlUp = urlUp.upURL();
	}

	mGoUp->setEnabled(url.path() != "/");
	updateStatusInfo();

	if( mLoadingCursor )
		kapp->restoreOverrideCursor();
	mLoadingCursor = false;

	mURLEditCompletion->addItem(url.prettyURL());
	mURLEdit->setEditText(url.prettyURL());
	mURLEdit->addToHistory(url.prettyURL());
}

void GVMainWindow::goUp() {
	goUpTo(mGoUp->popupMenu()->idAt(0));
}

void GVMainWindow::goUpTo(int id) {
	KPopupMenu* menu=mGoUp->popupMenu();
	KURL url(menu->text(id));

	KURL childURL;
	int index=menu->indexOf(id);
	if (index>0) {
		childURL=KURL(menu->text(menu->idAt(index-1)));
	} else {
		childURL=mDocument->dirURL();
	}

	mDocument->setDirURL(url);
	mFileViewStack->setFileNameToSelect(childURL.filename());
}


//-----------------------------------------------------------------------
//
// File operations
//
//-----------------------------------------------------------------------
void GVMainWindow::openHomeDir() {
	KURL url;
	url.setPath( QDir::homeDirPath() );
	mDocument->setURL(url);
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


void GVMainWindow::rotateLeft() {
	modifyImage(GVImageUtils::ROT_270);
}

void GVMainWindow::rotateRight() {
	modifyImage(GVImageUtils::ROT_90);
}

void GVMainWindow::mirror() {
	modifyImage(GVImageUtils::HFLIP);
}

void GVMainWindow::flip() {
	modifyImage(GVImageUtils::VFlip);
}

void GVMainWindow::modifyImage(GVImageUtils::Orientation orientation) {
	const KURL::List& urls=mFileViewStack->selectedURLs();
	if (mFileViewStack->isVisible() && urls.size()>1) {
		GVBatchManipulator manipulator(this, urls, orientation);
		connect(&manipulator, SIGNAL(imageModified(const KURL&)),
			mFileViewStack, SLOT(updateThumbnail(const KURL&)) );
		manipulator.apply();
		if (urls.find(mDocument->url())!=urls.end()) {
			mDocument->reload();
		}
	} else {
		mDocument->modify(orientation);
	}
}

void GVMainWindow::openFile() {
	KURL url=KFileDialog::getOpenURL();
	if (!url.isValid()) return;

	mDocument->setURL(url);
}


void GVMainWindow::printFile() {
	KPrinter printer;

	printer.setDocName(mDocument->filename());
	const KAboutData* pAbout = KApplication::kApplication()->aboutData();
	QString nm = pAbout->appName();
	nm += "-";
	nm += pAbout->version();
	printer.setCreator( nm );

	KPrinter::addDialogPage( new GVPrintDialogPage( this, "GV page"));

	if (printer.setup(this, QString::null, true)) {
		mDocument->print(&printer);
	}
}


//-----------------------------------------------------------------------
//
// Private slots
//
//-----------------------------------------------------------------------
void GVMainWindow::pixmapLoading() {
	if (mShowBusyPtrInFullScreen || !mToggleFullScreen->isChecked()) {
		if( !mLoadingCursor ) {
#if KDE_VERSION >= 0x30100
			kapp->setOverrideCursor(KCursor::workingCursor());
#else
			kapp->setOverrideCursor(QCursor(WaitCursor));
#endif
		}
		mLoadingCursor = true;
	}
}

void GVMainWindow::toggleDirAndFileViews() {
	KConfig* config=KGlobal::config();

	if (mFileDock->isVisible() || mFolderDock->isVisible()) {
		writeDockConfig(config,CONFIG_DOCK_GROUP);
		makeDockInvisible(mFileDock);
		makeDockInvisible(mFolderDock);
	} else {
		readDockConfig(config,CONFIG_DOCK_GROUP);
	}

	mPixmapView->setFocus();
}



void GVMainWindow::hideToolBars() {
	QPtrListIterator<KToolBar> it=toolBarIterator();
	KToolBar* bar;

	for(;it.current()!=0L; ++it) {
		bar=it.current();
		if (bar->area()) {
			bar->area()->hide();
		} else {
			bar->hide();
		}
	}
}


void GVMainWindow::showToolBars() {
	QPtrListIterator<KToolBar> it=toolBarIterator();

	KToolBar* bar;

	for(;it.current()!=0L; ++it) {
		bar=it.current();
		if (bar->area()) {
			bar->area()->show();
		} else {
			bar->show();
		}
	}
}


void GVMainWindow::toggleFullScreen() {
	KConfig* config=KGlobal::config();

	mToggleDirAndFileViews->setEnabled(!mToggleFullScreen->isChecked());

	if (mToggleFullScreen->isChecked()) {
		showFullScreen();
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
			hideToolBars();
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
	} else {
		readDockConfig(config,CONFIG_DOCK_GROUP);
		// workaround Qt bug - it unsets the fullscreen state on setGeometry(),
		// which will be triggered by readDockConfig(), but KWin will ignore
		// the geometry change and won't reset the state
#if QT_VERSION >= 0x030300
		setWState(WState_FullScreen);
#else
	// FIXME
#endif
		statusBar()->show();

		showToolBars();
		leftDock()->show();
		rightDock()->show();
		topDock()->show();
		bottomDock()->show();

		menuBar()->show();
		mPixmapView->setFullScreen(false);
#if QT_VERSION >= 0x030300
		setWindowState( windowState() & ~WindowFullScreen );
#else
		showNormal();
#endif
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
	GVConfigDialog dialog(this,this);
	dialog.exec();
}


void GVMainWindow::showExternalToolDialog() {
	GVExternalToolDialog* dialog=new GVExternalToolDialog(this);
	dialog->show();
}


void GVMainWindow::showKeyDialog() {
	KKeyDialog::configure(actionCollection());
}


void GVMainWindow::showToolBarDialog() {
	saveMainWindowSettings(KGlobal::config(), "MainWindow");
	KEditToolbar dlg(actionCollection());
	connect(&dlg,SIGNAL(newToolbarConfig()),this,SLOT(applyMainWindowSettings()));
	if (dlg.exec()) {
		createGUI();
	}
}

void GVMainWindow::applyMainWindowSettings() {
	KMainWindow::applyMainWindowSettings(KGlobal::config(), "MainWindow");
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
	statusBar()->addWidget(mProgress);
	mProgress->show();
	mStop->setEnabled(true);
}


void GVMainWindow::thumbnailUpdateEnded() {
	mStop->setEnabled(false);
	if (mProgress) {
		delete mProgress;
		mProgress=0L;
	}
}


void GVMainWindow::thumbnailUpdateProcessedOne() {
	mProgress->progress()->advance(1);
}


void GVMainWindow::slotURLEditChanged(const QString &str) {
	KURL url(mURLEditCompletion->replacedPath(str));
	mDocument->setURL(url);
	if (mFileViewStack->isVisible()) {
		mFileViewStack->setFocus();
	} else if (mPixmapView->isVisible()) {
		mPixmapView->setFocus();
	}
}


void GVMainWindow::slotDirRenamed(const KURL& oldURL, const KURL& newURL) {
	LOG(oldURL.prettyURL() << " to " << newURL.prettyURL());

	KURL url(mDocument->url());
	if (!oldURL.isParentOf(url) ) return;

	QString oldPath=oldURL.path();
	LOG("current path: " << url.path() );
	QString path=newURL.path() + url.path().mid(oldPath.length());
	LOG("new path: " << path);
	url.setPath(path);
	mDocument->setURL(url);
}


void GVMainWindow::slotGo() {
	KURL url(mURLEditCompletion->replacedPath(mURLEdit->currentText()));
	mDocument->setURL(url);
}

void GVMainWindow::slotShownFileItemRefreshed(const KFileItem* item) {
	LOG("");
	if (int(item->size())!=mDocument->fileSize()) {
		LOG("need reload " << int(item->size()) << "!=" << mDocument->fileSize());
		mDocument->reload();
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
	QString url=mDocument->dirURL().prettyURL();
	if (count==0) {
		txt=i18n("%1 - No Images").arg(url);
	} else {
		txt=i18n("%1 - One Image","%1 - %n images",count).arg(url);
	}
	mSBDirLabel->setText(txt);

	updateFileInfo();
}


void GVMainWindow::updateFileInfo() {
	QString filename=mDocument->filename();
	if (!filename.isEmpty()) {
		QString info=QString("%1 %2x%3 @ %4%")
			.arg(filename).arg(mDocument->width()).arg(mDocument->height())
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

	// Pixmap widget
#ifdef GV_HACK_SUFFIX
	mPixmapDock = createDockWidget("Image",SmallIcon("gwenview_hack"),NULL,i18n("Image"));
#else
	mPixmapDock = createDockWidget("Image",SmallIcon("gwenview"),NULL,i18n("Image"));
#endif	

	mPixmapView=new GVScrollPixmapView(mPixmapDock,mDocument,actionCollection());
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
	mMetaEdit = new GVMetaEdit(mMetaDock, mDocument);
	mMetaDock->setWidget(mMetaEdit);

	// Slide show controller (not really a widget)
	mSlideShow=new GVSlideShow(mFileViewStack->selectFirst(),mFileViewStack->selectNext());

	// Default position on desktop
	setGeometry(20,20,600,400);

	// Default dock config
	mFolderDock->manualDock(mPixmapDock,KDockWidget::DockLeft,30);
	mFileDock->manualDock(mFolderDock,KDockWidget::DockBottom,50);
	mMetaDock->manualDock(mPixmapDock,KDockWidget::DockTop,10);

	// Load config
	readDockConfig(config,CONFIG_DOCK_GROUP);
	mFileViewStack->readConfig(config,CONFIG_FILEWIDGET_GROUP);
	mDirView->readConfig(config,CONFIG_DIRWIDGET_GROUP);
	mPixmapView->readConfig(config,CONFIG_PIXMAPWIDGET_GROUP);
	mSlideShow->readConfig(config,CONFIG_SLIDESHOW_GROUP);
	GVJPEGTran::readConfig(config,CONFIG_JPEGTRAN_GROUP);
	GVCache::instance()->readConfig(config,CONFIG_CACHE_GROUP);
}


void GVMainWindow::createActions() {
	// File
	mOpenFile=KStdAction::open(this,SLOT(openFile()),actionCollection() );
	mSaveFile=KStdAction::save(mDocument,SLOT(save()),actionCollection() );
	mSaveFileAs=KStdAction::saveAs(mDocument,SLOT(saveAs()),actionCollection() );
	mFilePrint = KStdAction::print(this, SLOT(printFile()), actionCollection());
	mRenameFile=new KAction(i18n("&Rename..."),Key_F2,this,SLOT(renameFile()),actionCollection(),"file_rename");
	mCopyFiles=new KAction(i18n("&Copy To..."),Key_F7,this,SLOT(copyFiles()),actionCollection(),"file_copy");
	mMoveFiles=new KAction(i18n("&Move To..."),Key_F8,this,SLOT(moveFiles()),actionCollection(),"file_move");
	mDeleteFiles=new KAction(i18n("&Delete"),"editdelete",Key_Delete,this,SLOT(deleteFiles()),actionCollection(),"file_delete");
	mShowFileProperties=new KAction(i18n("Properties"),0,this,SLOT(showFileProperties()),actionCollection(),"file_properties");
	KStdAction::quit( kapp, SLOT (closeAllWindows()), actionCollection() );

	// Edit
	mRotateLeft=new KAction(i18n("Rotate &Left"),"rotate_left",CTRL + Key_L, this, SLOT(rotateLeft()),actionCollection(),"rotate_left");
	mRotateRight=new KAction(i18n("Rotate &Right"),"rotate_right",CTRL + Key_R, this, SLOT(rotateRight()),actionCollection(),"rotate_right");
	mMirror=new KAction(i18n("&Mirror"),"mirror",0, this, SLOT(mirror()),actionCollection(),"mirror");
	mFlip=new KAction(i18n("&Flip"),"flip",0, this, SLOT(flip()),actionCollection(),"flip");

	// View
	mReload=new KAction(i18n("Reload"), "reload", Key_F5, mDocument, SLOT(reload()), actionCollection(), "reload");
	mReload->setEnabled(false);
	mStop=new KAction(i18n("Stop"),"stop",Key_Escape,mFileViewStack,SLOT(cancel()),actionCollection(), "stop");
	mStop->setEnabled(false);
//(0x3015A == 3.1.90)
#if KDE_VERSION>=0x3015A
	mToggleFullScreen= KStdAction::fullScreen(this,SLOT(toggleFullScreen()),actionCollection(),0);
#else
	mToggleFullScreen=new KToggleAction(i18n("Full Screen"),"window_fullscreen",CTRL + Key_F,this,SLOT(toggleFullScreen()),actionCollection(),"fullscreen");
#endif
	mToggleSlideShow=new KToggleAction(i18n("Slide Show..."),"slideshow",0,this,SLOT(toggleSlideShow()),actionCollection(),"slideshow");
	mToggleDirAndFileViews=new KAction(i18n("Hide Folder && File Views"),CTRL + Key_Return,this,SLOT(toggleDirAndFileViews()),actionCollection(),"toggle_dir_and_file_views");

	// Go
 	mGoUp=new KToolBarPopupAction(i18n("Up"), "up", ALT + Key_Up, this, SLOT(goUp()), actionCollection(), "go_up");
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
		mDocument,SLOT(setDirURL(const KURL&)) );

	connect(mDocument,SIGNAL(loaded(const KURL&,const QString&)),
		bookmarkOwner,SLOT(setURL(const KURL&)) );

	// Settings
	mShowConfigDialog=
		KStdAction::preferences(this, SLOT(showConfigDialog()), actionCollection() );
	mShowKeyDialog=
		KStdAction::keyBindings(this, SLOT(showKeyDialog()), actionCollection() );
	(void)new KAction(i18n("Configure External Tools..."), "configure",
		this, SLOT(showExternalToolDialog()), actionCollection(), "configure_tools");
	(void)KStdAction::configureToolbars(
		this, SLOT(showToolBarDialog()), actionCollection() );

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
	connect(mGoUp->popupMenu(), SIGNAL(activated(int)),
		this,SLOT(goUpTo(int)));

	// Dir view connections
	connect(mDirView,SIGNAL(dirURLChanged(const KURL&)),
		mDocument,SLOT(setDirURL(const KURL&)) );

	connect(mDirView, SIGNAL(dirRenamed(const KURL&, const KURL&)),
		this, SLOT(slotDirRenamed(const KURL&, const KURL&)) );

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
		mDocument,SLOT(setURL(const KURL&)) );
	connect(mFileViewStack,SIGNAL(completed()),
		this,SLOT(updateStatusInfo()) );
	connect(mFileViewStack,SIGNAL(canceled()),
		this,SLOT(updateStatusInfo()) );
	connect(mFileViewStack,SIGNAL(imageDoubleClicked()),
		mToggleFullScreen,SLOT(activate()) );
	connect(mFileViewStack,SIGNAL(shownFileItemRefreshed(const KFileItem*)),
		this,SLOT(slotShownFileItemRefreshed(const KFileItem*)) );
	// Don't connect mDocument::loaded to mDirView. mDirView will be
	// updated _after_ the file view is done, since it's less important to the
	// user
	connect(mFileViewStack,SIGNAL(completedURLListing(const KURL&)),
		mDirView,SLOT(setURL(const KURL&)) );

	// GVDocument connections
	connect(mDocument,SIGNAL(loading()),
		this,SLOT(pixmapLoading()) );
	connect(mDocument,SIGNAL(loaded(const KURL&,const QString&)),
		this,SLOT(setURL(const KURL&,const QString&)) );
	connect(mDocument,SIGNAL(loaded(const KURL&,const QString&)),
		mFileViewStack,SLOT(setURL(const KURL&,const QString&)) );
	connect(mDocument,SIGNAL(saved(const KURL&)),
		mFileViewStack,SLOT(updateThumbnail(const KURL&)) );
	connect(mDocument,SIGNAL(reloaded(const KURL&)),
		mFileViewStack,SLOT(updateThumbnail(const KURL&)) );

	// Slide show
	connect(mSlideShow,SIGNAL(finished()),
		mToggleSlideShow,SLOT(activate()) );

	// Location bar
	connect(mURLEdit,SIGNAL(activated(const QString &)),
		this,SLOT(slotURLEditChanged(const QString &)));
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
	// URL Combo
	mURLEdit=new KHistoryCombo(this);

	mURLEdit->setDuplicatesEnabled(false);

	mURLEditCompletion=new KURLCompletion(KURLCompletion::DirCompletion);
	mURLEditCompletion->setDir("/");

	mURLEdit->setCompletionObject(mURLEditCompletion);
	mURLEdit->setAutoDeleteCompletionObject(true);

	KURL url;
	url.setPath(QDir::current().absPath());
	mURLEdit->setEditText(url.prettyURL());
	mURLEdit->addToHistory(url.prettyURL());
	mURLEdit->setDuplicatesEnabled(false);

	KWidgetAction* comboAction=new KWidgetAction( mURLEdit, i18n("Location Bar"), 0,
		0, 0, actionCollection(), "location_url");
	comboAction->setShortcutConfigurable(false);
	comboAction->setAutoSized(true);

	// Clear button
	(void)new KAction( i18n("Clear location bar"),
		QApplication::reverseLayout()?"clear_left" : "locationbar_erase",
		0, mURLEdit, SLOT(clearEdit()), actionCollection(), "clear_location");

	// URL Label
	/* we use "kde toolbar widget" to avoid the flat background (looks bad with
	 * styles like Keramik). See konq_misc.cc.
	 */
	QLabel* urlLabel=new QLabel(i18n("L&ocation:"), this, "kde toolbar widget");
	(void)new KWidgetAction( urlLabel, i18n("L&ocation: "), 0, 0, 0, actionCollection(), "location_label");
	urlLabel->setBuddy(mURLEdit);

	// Go button
	(void)new KAction(i18n("Go"), "key_enter", 0, this, SLOT(slotGo()), actionCollection(), "location_go");

}


#ifdef HAVE_KIPI
void GVMainWindow::loadPlugins() {
	QMap<KIPI::Category, QCString> categoryMap;
	categoryMap[KIPI::IMAGESPLUGIN]="kipi_images";
	categoryMap[KIPI::EFFECTSPLUGIN]="kipi_effects";
	categoryMap[KIPI::TOOLSPLUGIN]="kipi_tools";
	categoryMap[KIPI::BATCHPLUGIN]="kipi_batch";
	categoryMap[KIPI::IMPORTPLUGIN]="kipi_import";
	categoryMap[KIPI::EXPORTPLUGIN]="kipi_export";
	
	// Sets up the plugin interface, and load the plugins
	GVKIPIInterface* interface = new GVKIPIInterface(this, mFileViewStack);
	KIPI::PluginLoader* loader = new KIPI::PluginLoader(QStringList(), interface );
	loader->loadPlugins();

	// Fill the plugin menu
	KIPI::PluginLoader::PluginList::ConstIterator it(loader->pluginList().begin());
	KIPI::PluginLoader::PluginList::ConstIterator itEnd(loader->pluginList().end());
	for( ; it!=itEnd; ++it ) {
		KIPI::Plugin* plugin = (*it)->plugin;
		Q_ASSERT(plugin);
		if (!plugin) continue;

		if (!categoryMap.contains(plugin->category())) {
			kdWarning() << "Unknown category '" << plugin->category() << "'\n";
			continue;
		}
		QPopupMenu *popup = static_cast<QPopupMenu*>(
			factory()->container( categoryMap[plugin->category()], this));
		Q_ASSERT( popup );
		plugin->setup(this);
		KActionPtrList actions = plugin->actions();
		KActionPtrList::ConstIterator actionIt=actions.begin(), end=actions.end();
		for (; actionIt!=end; ++actionIt) {
			(*actionIt)->plug( popup );
		}
	}
}
#else
void GVMainWindow::loadPlugins() {
	QPopupMenu *popup = static_cast<QPopupMenu*>(
		factory()->container( "plugins", this));
	delete popup;
}
#endif


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
		showToolBars();
	} else {
		hideToolBars();
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


void GVMainWindow::setShowBusyPtrInFullScreen(bool value) {
	mShowBusyPtrInFullScreen=value;
}

void GVMainWindow::setAutoDeleteThumbnailCache(bool value){
	mAutoDeleteThumbnailCache=value;
}


//-----------------------------------------------------------------------
//
// Configuration
//
//-----------------------------------------------------------------------
void GVMainWindow::readConfig(KConfig* config,const QString& group) {
	config->setGroup(group);
	mShowMenuBarInFullScreen=config->readBoolEntry(CONFIG_MENUBAR_IN_FS, false);
	mShowToolBarInFullScreen=config->readBoolEntry(CONFIG_TOOLBAR_IN_FS, true);
	mShowStatusBarInFullScreen=config->readBoolEntry(CONFIG_STATUSBAR_IN_FS, false);
	mShowBusyPtrInFullScreen=config->readBoolEntry(CONFIG_BUSYPTR_IN_FS, true);
	mAutoDeleteThumbnailCache=config->readBoolEntry(CONFIG_AUTO_DELETE_THUMBNAIL_CACHE, false);	
}


void GVMainWindow::writeConfig(KConfig* config,const QString& group) const {
	config->setGroup(group);
	config->writeEntry(CONFIG_MENUBAR_IN_FS, mShowMenuBarInFullScreen);
	config->writeEntry(CONFIG_TOOLBAR_IN_FS, mShowToolBarInFullScreen);
	config->writeEntry(CONFIG_STATUSBAR_IN_FS, mShowStatusBarInFullScreen);
	config->writeEntry(CONFIG_BUSYPTR_IN_FS, mShowBusyPtrInFullScreen);
	config->writeEntry(CONFIG_AUTO_DELETE_THUMBNAIL_CACHE, mAutoDeleteThumbnailCache);
}

