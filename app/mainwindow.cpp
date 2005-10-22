// vim: set tabstop=4 shiftwidth=4 noexpandtab:
// kate: indent-mode csands; indent-width 4; replace-tabs-save off; replace-tabs off; replace-trailing-space-save off; space-indent off; tabs-indents on; tab-width 4;
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aur�ien G�eau

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
#include <qtabwidget.h>

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
#include <kdockwidget.h>
#include <kedittoolbar.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <khelpmenu.h>
#include <kiconloader.h>
#include <kimageio.h>
#include <kio/netaccess.h>
#include <kkeydialog.h>
#include <klargefile.h>
#include <klistview.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kstandarddirs.h>
#include <kstatusbar.h>
#include <kstdaccel.h>
#include <kstdaction.h>
#include <ktoolbarbutton.h>
#include <kurlcompletion.h>
#include <kurlpixmapprovider.h>
#include <kurlrequesterdlg.h>
#include <kprinter.h>

#include <config.h>
// KIPI
#ifdef GV_HAVE_KIPI
#include <libkipi/plugin.h>
#include <libkipi/pluginloader.h>
#endif

// Local
#include "bookmarkowner.h"
#include "bookmarkviewcontroller.h"
#include "configdialog.h"
#include "dirview.h"
#include "history.h"
#include "metaedit.h"
#include "truncatedtextlabel.h"

#include "gvcore/fileoperation.h"
#include "gvcore/archive.h"
#include "gvcore/batchmanipulator.h"
#include "gvcore/captionformatter.h"
#include "gvcore/document.h"
#include "gvcore/externaltooldialog.h"
#include "gvcore/fileviewbase.h"
#include "gvcore/fileviewstack.h"
#include "gvcore/imageview.h"
#include "gvcore/slideshow.h"
#include "gvcore/printdialog.h"
#include "gvcore/cache.h"
#include "gvcore/thumbnailloadjob.h"
#include <../gvcore/slideshowconfig.h>
#include <../gvcore/fullscreenconfig.h>

#include "config.h"

#ifdef GV_HAVE_KIPI
#include "gvcore/kipiinterface.h"
#endif

#include <kcursor.h>

#include "mainwindow.moc"
namespace Gwenview {

const char CONFIG_DOCK_GROUP[]="dock";
const char CONFIG_MAINWINDOW_GROUP[]="main window";
const char CONFIG_FILEWIDGET_GROUP[]="file widget";
const char CONFIG_DIRWIDGET_GROUP[]="dir widget";
const char CONFIG_PIXMAPWIDGET_GROUP[]="pixmap widget";
const char CONFIG_FILEOPERATION_GROUP[]="file operations";
const char CONFIG_CACHE_GROUP[]="cache";
const char CONFIG_THUMBNAILLOADJOB_GROUP[]="thumbnail loading";

const char CONFIG_SHOW_LOCATION_TOOLBAR[]="show address bar";
const char CONFIG_AUTO_DELETE_THUMBNAIL_CACHE[]="Delete Thumbnail Cache whe exit";
const char CONFIG_GWENVIEW_DOCK_VERSION[]="Gwenview version";

const char CONFIG_SESSION_URL[] = "url";

// This version is here to avoid configuration migration troubles when changes
// are made to the dock behavior
const int GWENVIEW_DOCK_VERSION=2;

// The timeout before an hint in the statusbar disappear (in msec)
const int HINT_TIMEOUT=10000;

// Time before we try to load the KIPI plugins (in msec)
const int LOAD_PLUGIN_DELAY=1000;

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

enum { StackIDBrowse, StackIDView };


// This function is used in the ctor to find out whether to start in viewing
// or browsing mode if URL is passed on the command line.
static bool urlIsDirectory(QWidget* parent, const KURL& url) {
	if( url.filename( false ).isEmpty()) return true; // file:/somewhere/<nothing here>
	// Do direct stat instead of using KIO if the file is local (faster)
	if( url.isLocalFile()
		&& !KIO::probably_slow_mounted( url.path())) {
		KDE_struct_stat buff;
		if ( KDE_stat( QFile::encodeName(url.path()), &buff ) == 0 )  {
			return S_ISDIR( buff.st_mode );
		}
	}
	KIO::UDSEntry entry;
	if( KIO::NetAccess::stat( url, entry, parent)) {
		KIO::UDSEntry::ConstIterator it;
		for(it=entry.begin();it!=entry.end();++it) {
			if ((*it).m_uds==KIO::UDS_FILE_TYPE) {
				return S_ISDIR( (*it).m_long );
			}
		}
	}
	return false;
}


MainWindow::MainWindow()
: KMainWindow()
, mLoadingCursor(false)
#ifdef GV_HAVE_KIPI
, mPluginLoader(0)
#endif
{
	readConfig(KGlobal::config(),CONFIG_MAINWINDOW_GROUP);

	// Backend
	mDocument=new Document(this);
	mHistory=new History(actionCollection());
	// GUI
	createActions();
	createWidgets();
	createLocationToolBar();
	createObjectInteractions();

	setStandardToolBarMenuEnabled(true);
	createGUI("gwenviewui.rc", false);

	createConnections();
	mWindowListActions.setAutoDelete(true);
	updateWindowActions();
	applyMainWindowSettings();

	if( !kapp->isSessionRestored()) {
		// Command line
		KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

		if (args->count()==0) {
			KURL url;
			url.setPath( QDir::currentDirPath() );
			mFileViewStack->setDirURL(url);
		} else {
			bool fullscreen=args->isSet("f");
			if (fullscreen) mToggleFullScreen->activate();
			KURL url=args->url(0);

			if( urlIsDirectory(this, url)) {
				mFileViewStack->setDirURL(url);
			} else {
				if (!fullscreen) mToggleBrowse->activate();
				openURL(url);
			}
			updateLocationURL();
		}
	}
	
	if (mToggleBrowse->isChecked()) {
		mFileViewStack->setFocus();
	} else {
		mImageView->setFocus();
	}

	QTimer::singleShot(LOAD_PLUGIN_DELAY, this, SLOT(loadPlugins()) );
}


bool MainWindow::queryClose() {
	mDocument->saveBeforeClosing();

	KConfig* config=KGlobal::config();
	mImageView->writeConfig(config, CONFIG_PIXMAPWIDGET_GROUP);
	mFileViewStack->writeConfig(config, CONFIG_FILEWIDGET_GROUP);
	mDirView->writeConfig(config, CONFIG_DIRWIDGET_GROUP);
	SlideShowConfig::writeConfig();
	ThumbnailLoadJob::writeConfig(config, CONFIG_THUMBNAILLOADJOB_GROUP);

	// Don't store dock layout if only the image dock is visible. This avoid
	// saving layout when in "fullscreen" or "image only" mode.
	if (mFileViewStack->isVisible() || mDirView->isVisible()) {
		mDockArea->writeDockConfig(config,CONFIG_DOCK_GROUP);
	}
	writeConfig(config,CONFIG_MAINWINDOW_GROUP);

	if (mAutoDeleteThumbnailCache) {
		QString dir=ThumbnailLoadJob::thumbnailBaseDir();

		if (QFile::exists(dir)) {
			KURL url;
			url.setPath(dir);
			KIO::NetAccess::del(url, this);
		}
	}
	
	if (!mToggleFullScreen->isChecked()) {
		saveMainWindowSettings(KGlobal::config(), "MainWindow");
	}
	return true;
}

void MainWindow::saveProperties( KConfig* cfg ) {
	cfg->writeEntry( CONFIG_SESSION_URL, mFileViewStack->url().url());
}

void MainWindow::readProperties( KConfig* cfg ) {
	KURL url(cfg->readEntry(CONFIG_SESSION_URL));
	if( urlIsDirectory(this, url)) {
		mFileViewStack->setDirURL(url);
	} else {
		openURL(url);
	}
}

//-----------------------------------------------------------------------
//
// Public slots
//
//-----------------------------------------------------------------------
void MainWindow::openURL(const KURL& url) {
	mDocument->setURL(url);
	mFileViewStack->setDirURL(url.upURL());
	mFileViewStack->setFileNameToSelect(url.filename());
}


void MainWindow::slotDirURLChanged(const KURL& dirURL) {
	LOG(dirURL.prettyURL(0,KURL::StripFileProtocol));
	
	if (dirURL.path()!="/") {
		mGoUp->setEnabled(true);
		QPopupMenu *upPopup = mGoUp->popupMenu();
		upPopup->clear();
		int pos = 0;
		KURL url = dirURL.upURL();
		for (; url.hasPath() && pos<10; url=url.upURL(), ++pos) {
			upPopup->insertItem(url.url());
			if (url.path()=="/") break;
		}
	} else {
		mGoUp->setEnabled(false);
	}

	updateStatusInfo();
	updateImageActions();
	updateLocationURL();
}

void MainWindow::updateLocationURL() {
	LOG("");
	KURL url;
	if (mToggleBrowse->isChecked()) {
		url=mFileViewStack->dirURL();
		if (!url.isValid()) {
			url=mDocument->url();
		}
	} else {
		url=mDocument->url();
	}
	LOG(url.prettyURL());
#if KDE_IS_VERSION( 3, 4, 0 )
	mURLEdit->setEditText(url.pathOrURL());
	mURLEdit->addToHistory(url.pathOrURL());
#else
	mURLEdit->setEditText(url.prettyURL(0,KURL::StripFileProtocol));
	mURLEdit->addToHistory(url.prettyURL(0,KURL::StripFileProtocol));
#endif
}

void MainWindow::goUp() {
	goUpTo(mGoUp->popupMenu()->idAt(0));
}

void MainWindow::goUpTo(int id) {
	KPopupMenu* menu=mGoUp->popupMenu();
	KURL url(menu->text(id));
	KURL childURL;
	int index=menu->indexOf(id);
	if (index>0) {
		childURL=KURL(menu->text(menu->idAt(index-1)));
	} else {
		childURL=mDocument->dirURL();
	}
	mFileViewStack->setDirURL(url);
	mFileViewStack->setFileNameToSelect(childURL.fileName());
}


//-----------------------------------------------------------------------
//
// File operations
//
//-----------------------------------------------------------------------
void MainWindow::goHome() {
	KURL url;
	url.setPath( QDir::homeDirPath() );
	mFileViewStack->setDirURL(url);
}


void MainWindow::renameFile() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->renameFile();
	} else {
		mImageView->renameFile();
	}
}


void MainWindow::copyFiles() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->copyFiles();
	} else {
		mImageView->copyFile();
	}
}

void MainWindow::linkFiles() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->linkFiles();
	} else {
		mImageView->linkFile();
	}
}


void MainWindow::moveFiles() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->moveFiles();
	} else {
		mImageView->moveFile();
	}
}


void MainWindow::deleteFiles() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->deleteFiles();
	} else {
		mImageView->deleteFile();
	}
}


void MainWindow::showFileProperties() {
	if (mFileViewStack->isVisible()) {
		mFileViewStack->showFileProperties();
	} else {
		mImageView->showFileProperties();
	}
}


void MainWindow::rotateLeft() {
	modifyImage(ImageUtils::ROT_270);
}

void MainWindow::rotateRight() {
	modifyImage(ImageUtils::ROT_90);
}

void MainWindow::mirror() {
	modifyImage(ImageUtils::HFLIP);
}

void MainWindow::flip() {
	modifyImage(ImageUtils::VFLIP);
}

void MainWindow::modifyImage(ImageUtils::Orientation orientation) {
	const KURL::List& urls=mFileViewStack->selectedImageURLs();
	if (mFileViewStack->isVisible() && urls.size()>1) {
		BatchManipulator manipulator(this, urls, orientation);
		connect(&manipulator, SIGNAL(imageModified(const KURL&)),
			mFileViewStack, SLOT(updateThumbnail(const KURL&)) );
		manipulator.apply();
		if (urls.find(mDocument->url())!=urls.end()) {
			mDocument->reload();
		}
	} else {
		mDocument->transform(orientation);
	}
}

void MainWindow::showFileDialog() {
	KURL url=KFileDialog::getOpenURL();
	if (!url.isValid()) return;

	openURL(url);
}


void MainWindow::printFile() {
	KPrinter printer;

	printer.setDocName(mDocument->filename());
	const KAboutData* pAbout = KApplication::kApplication()->aboutData();
	QString nm = pAbout->appName();
	nm += "-";
	nm += pAbout->version();
	printer.setCreator( nm );

	KPrinter::addDialogPage( new PrintDialogPage( mDocument, this, " page"));

	if (printer.setup(this, QString::null, true)) {
		mDocument->print(&printer);
	}
}


//-----------------------------------------------------------------------
//
// Private slots
//
//-----------------------------------------------------------------------
void MainWindow::slotImageLoading() {
	if (FullScreenConfig::self()->showBusyPtr() || !mToggleFullScreen->isChecked()) {
		if( !mLoadingCursor ) {
			kapp->setOverrideCursor(KCursor::workingCursor());
		}
		mLoadingCursor = true;
	}
}


void MainWindow::slotImageLoaded() {
	if( mLoadingCursor )
		kapp->restoreOverrideCursor();
	mLoadingCursor = false;
	updateStatusInfo();
	updateImageActions();
	updateLocationURL();
}


void MainWindow::hideToolBars() {
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


void MainWindow::showToolBars() {
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


void MainWindow::toggleFullScreen() {
	if (mToggleFullScreen->isChecked()) {
		saveMainWindowSettings(KGlobal::config(), "MainWindow");

		showFullScreen();
		menuBar()->hide();
		statusBar()->hide();
		
		/* Hide toolbar
		 * If the toolbar is docked we hide the DockArea to avoid
		 * having a one pixel band remaining
		 * For the same reason, we hide all the empty DockAreas
		 *
		 * NOTE: This does not work really well if the toolbar is in
		 * the left or right dock area.
		 */
		hideToolBars();
		if (leftDock()->isEmpty())	 leftDock()->hide();
		if (rightDock()->isEmpty())  rightDock()->hide();
		if (topDock()->isEmpty())	 topDock()->hide();
		if (bottomDock()->isEmpty()) bottomDock()->hide();
		
		if (mToggleBrowse->isChecked()) {
			mImageView->reparent(mViewModeWidget, QPoint(0,0));
			mCentralStack->raiseWidget(StackIDView);
		}
		mImageView->setFullScreen(true);
		mImageView->setFocus();
	} else {
		// Stop the slideshow if it's running
		if (mSlideShow->isRunning()) {
			mToggleSlideShow->activate();
		}

		// Make sure the file view points to the right URL, it might not be the
		// case if we are getting out of a slideshow
		mFileViewStack->setDirURL(mDocument->url().upURL());
		mFileViewStack->setFileNameToSelect(mDocument->url().fileName());

		showNormal();
		menuBar()->show();
		
		showToolBars();
		leftDock()->show();
		rightDock()->show();
		topDock()->show();
		bottomDock()->show();
		
		statusBar()->show();
		mImageView->setFullScreen(false);
		
		if (mToggleBrowse->isChecked()) {
			mPixmapDock->setWidget(mImageView);
			mCentralStack->raiseWidget(StackIDBrowse);
		}
		mFileViewStack->setFocus();
	}
}


void MainWindow::toggleSlideShow() {
	if (mSlideShow->isRunning()) {
		mSlideShow->stop();
		return;
	}
	
	KURL::List list;
	KFileItemListIterator it( *mFileViewStack->currentFileView()->items() );
	for ( ; it.current(); ++it ) {
		KFileItem* item=it.current();
		if (!item->isDir() && !Archive::fileItemIsArchive(item)) {
			list.append(item->url());
		}
	}
	if (list.count()==0) {
		return;
	}

	if (SlideShowConfig::fullscreen() && !mToggleFullScreen->isChecked()) {
		mToggleFullScreen->activate();
	}
	mSlideShow->start(list);
	}


void MainWindow::slotSlideShowChanged(bool running) {
 	mToggleSlideShow->setIcon(running ? "slideshow_pause" : "slideshow_play");
}


void MainWindow::showConfigDialog() {
	ConfigDialog dialog(this);
	connect(&dialog, SIGNAL(settingsChanged()),
		mSlideShow, SLOT(slotSettingsChanged()) );
	dialog.exec();
}


void MainWindow::showExternalToolDialog() {
	ExternalToolDialog* dialog=new ExternalToolDialog(this);
	dialog->show();
}


void MainWindow::showKeyDialog() {
	KKeyDialog dialog(true, this);
	dialog.insert(actionCollection());
#ifdef GV_HAVE_KIPI
	KIPI::PluginLoader::PluginList pluginList=mPluginLoader->pluginList();
	KIPI::PluginLoader::PluginList::ConstIterator it(pluginList.begin());
	KIPI::PluginLoader::PluginList::ConstIterator itEnd(pluginList.end());
	for( ; it!=itEnd; ++it ) {
		KIPI::Plugin* plugin=(*it)->plugin();
		if (plugin) {
			dialog.insert(plugin->actionCollection(), (*it)->name());
		}
	}
#endif
	dialog.configure(true);
}


void MainWindow::showToolBarDialog() {
	saveMainWindowSettings(KGlobal::config(), "MainWindow");
	KEditToolbar dlg(actionCollection());
	connect(&dlg,SIGNAL(newToolbarConfig()),this,SLOT(applyMainWindowSettings()));
	if (dlg.exec()) {
		createGUI();
	}
}

void MainWindow::applyMainWindowSettings() {
	KMainWindow::applyMainWindowSettings(KGlobal::config(), "MainWindow");
}



void MainWindow::escapePressed() {
	if (mToggleFullScreen->isChecked()) {
		mToggleFullScreen->activate();
	}
}


void MainWindow::slotDirRenamed(const KURL& oldURL, const KURL& newURL) {
	LOG(oldURL.prettyURL(0,KURL::StripFileProtocol) << " to " << newURL.prettyURL(0,KURL::StripFileProtocol));

	KURL url(mFileViewStack->dirURL());
	if (!oldURL.isParentOf(url) ) {
		LOG(oldURL.prettyURL() << " is not a parent of " << url.prettyURL());
		return;
	}

	QString oldPath=oldURL.path();
	LOG("current path: " << url.path() );
	QString path=newURL.path() + url.path().mid(oldPath.length());
	LOG("new path: " << path);
	url.setPath(path);
	mFileViewStack->setDirURL(url);
}


void MainWindow::slotGo() {
	KURL url(mURLEditCompletion->replacedPath(mURLEdit->currentText()));
	LOG(url.prettyURL());
	if( urlIsDirectory(this, url)) {
		LOG(" '-> is a directory");
		mFileViewStack->setDirURL(url);
	} else {
		LOG(" '-> is not a directory");
		openURL(url);
	}
	mFileViewStack->setFocus();
}

void MainWindow::slotShownFileItemRefreshed(const KFileItem*) {
	LOG("");
	mDocument->reload();
}
	

void MainWindow::slotToggleCentralStack() {
	LOG("");
	if (mToggleBrowse->isChecked()) {
		mPixmapDock->setWidget(mImageView);
		mCentralStack->raiseWidget(StackIDBrowse);
		mFileViewStack->setSilentMode( false );
		// force re-reading the directory to show the error
		if( mFileViewStack->lastURLError()) mFileViewStack->retryURL();
	} else {
		mImageView->reparent(mViewModeWidget, QPoint(0,0));
		mCentralStack->raiseWidget(StackIDView);
		mFileViewStack->setSilentMode( true );
	}

	// Make sure the window list actions are disabled if we are in view mode,
	// otherwise weird things happens when we go back to browse mode
	QPtrListIterator<KAction> it(mWindowListActions);
	for (;it.current(); ++it) {
		it.current()->setEnabled(mToggleBrowse->isChecked());
	}
	updateImageActions();
	updateLocationURL();
}


void MainWindow::resetDockWidgets() {
	mFolderDock->undock();
	mBookmarkDock->undock();
	mPixmapDock->undock();
	mMetaDock->undock();

	mFolderDock->manualDock(mFileDock, KDockWidget::DockLeft, 4000);
	mPixmapDock->manualDock(mFolderDock, KDockWidget::DockBottom, 3734);
	mMetaDock->manualDock(mPixmapDock, KDockWidget::DockBottom, 8560);
	mBookmarkDock->manualDock(mFolderDock, KDockWidget::DockCenter, 0);
}


/**
 * Display a hint as a temporary message in the status bar
 */
void MainWindow::showHint(const QString& hint) {
	mSBHintLabel->setText(hint);

	mSBHintLabel->show();
	mHintTimer->start(HINT_TIMEOUT, true);
}


//-----------------------------------------------------------------------
//
// GUI
//
//-----------------------------------------------------------------------
void MainWindow::updateStatusInfo() {
	int pos=mFileViewStack->shownFilePosition();
	uint count=mFileViewStack->fileCount();
	QString filename=mDocument->filename();
	QString txt;
	if (count==0) {
		txt=i18n("No Images");
	} else {
		txt=i18n("%1/%2 - %3 %4x%5 @ %6%")
			.arg(pos+1)
			.arg(count)
			.arg(filename)
			.arg(mDocument->width())
			.arg(mDocument->height())
			.arg(int(mImageView->zoom()*100) );
	}
	mSBDetailLabel->setText(txt);
	setCaption(filename);
}


void MainWindow::updateImageActions() {
	bool imageActionsEnabled = !mDocument->isNull();
	
	mToggleSlideShow->setEnabled(imageActionsEnabled);
	mRotateLeft->setEnabled(imageActionsEnabled);
	mRotateRight->setEnabled(imageActionsEnabled);
	mMirror->setEnabled(imageActionsEnabled);
	mFlip->setEnabled(imageActionsEnabled);
	mSaveFile->setEnabled(imageActionsEnabled);
	mSaveFileAs->setEnabled(imageActionsEnabled);
	mFilePrint->setEnabled(imageActionsEnabled);
	mReload->setEnabled(imageActionsEnabled);

	bool fileActionsEnabled = 
		imageActionsEnabled
		|| (mFileViewStack->isVisible() && mFileViewStack->selectionSize()>0);

	mRenameFile->setEnabled(fileActionsEnabled);
	mCopyFiles->setEnabled(fileActionsEnabled);
	mMoveFiles->setEnabled(fileActionsEnabled);
	mLinkFiles->setEnabled(fileActionsEnabled);
	mDeleteFiles->setEnabled(fileActionsEnabled);
	mShowFileProperties->setEnabled(fileActionsEnabled);
}


/**
 * This method creates all the widgets. Interactions between them and with
 * actions are created in createObjectInteractions
 */
void MainWindow::createWidgets() {
	KConfig* config=KGlobal::config();

	mCentralStack=new QWidgetStack(this);
	setCentralWidget(mCentralStack);

	mDockArea=new KDockArea(mCentralStack);
	mCentralStack->addWidget(mDockArea, StackIDBrowse);
	mDockArea->manager()->setSplitterHighResolution(true);
	mDockArea->manager()->setSplitterOpaqueResize(true);
	
	mViewModeWidget=new QWidget(mCentralStack);
	QVBoxLayout* layout=new QVBoxLayout(mViewModeWidget);
	layout->setAutoAdd(true);
	mCentralStack->addWidget(mViewModeWidget);

	// Status bar
	mSBDetailLabel=new QLabel("", statusBar());
	
	mSBHintLabel=new TruncatedTextLabel(statusBar());
	QFont font=mSBHintLabel->font();
	font.setItalic(true);
	mSBHintLabel->setFont(font);
	
	statusBar()->addWidget(mSBDetailLabel, 0);
	statusBar()->addWidget(mSBHintLabel, 1);
	mHintTimer=new QTimer(this);
	connect(mHintTimer, SIGNAL(timeout()),
		mSBHintLabel, SLOT(clear()) );

	// Pixmap widget
	mPixmapDock = mDockArea->createDockWidget("Image",SmallIcon("gwenview"),NULL,i18n("Image"));
	mImageView=new ImageView(mPixmapDock,mDocument,actionCollection());
	mPixmapDock->setWidget(mImageView);
	connect(mImageView, SIGNAL(requestHintDisplay(const QString&)),
		this, SLOT(showHint(const QString&)) );

	// Folder widget
	mFolderDock = mDockArea->createDockWidget("Folders",SmallIcon("folder_open"),NULL,i18n("Folders"));
	mDirView=new DirView(mFolderDock);
	mFolderDock->setWidget(mDirView);
	
	// Bookmark widget
	mBookmarkDock = mDockArea->createDockWidget("Bookmarks", SmallIcon("bookmark"),NULL,i18n("Bookmarks"));
	QVBox* bmBox=new QVBox(mBookmarkDock);
	mBookmarkView=new KListView(bmBox);
	mBookmarkToolBar=new KToolBar(bmBox, "", true);
	mBookmarkDock->setWidget(bmBox);

	// File widget
	mFileDock = mDockArea->createDockWidget("Files",SmallIcon("image"),NULL,i18n("Files"));
	QVBox* vbox=new QVBox(this);
	mFileViewToolBar=new KToolBar(vbox, "", true);
	mFileViewStack=new FileViewStack(vbox, actionCollection());
	mFileDock->setWidget(vbox);
	mFileDock->setEnableDocking(KDockWidget::DockNone);
	mDockArea->setMainDockWidget(mFileDock);

	// Meta info edit widget
	mMetaDock = mDockArea->createDockWidget("File Attributes", SmallIcon("info"),NULL,
		i18n("File Info"));
	mMetaEdit = new MetaEdit(mMetaDock, mDocument);
	mMetaDock->setWidget(mMetaEdit);

	// Slide show controller (not really a widget)
	mSlideShow=new SlideShow(mDocument);

	// Default position on desktop
	setGeometry(20,20,720,520);

	// Default dock config
	// (The "magic numbers" were found by adjusting the layout from within the
	// app and looking at the result in the configuration file)
	mFolderDock->manualDock(mFileDock, KDockWidget::DockLeft, 4000);
	mPixmapDock->manualDock(mFolderDock, KDockWidget::DockBottom, 3734);
	mMetaDock->manualDock(mPixmapDock, KDockWidget::DockBottom, 8560);
	mBookmarkDock->manualDock(mFolderDock, KDockWidget::DockCenter, 0);

	// Load dock config if up to date
	if (config->hasGroup(CONFIG_DOCK_GROUP)) {
		config->setGroup(CONFIG_DOCK_GROUP);
		if (config->readNumEntry(CONFIG_GWENVIEW_DOCK_VERSION, 1)==GWENVIEW_DOCK_VERSION) {
			mDockArea->readDockConfig(config,CONFIG_DOCK_GROUP);
		} else {
			KMessageBox::sorry(this, i18n(
				"<qt><b>Configuration update</b><br>"
				"Due to some changes in the dock behavior, your old dock configuration has been discarded. "
				"Please adjust your docks again.</qt>")
				);
			// Store the default dock config and create the
			// GWENVIEW_DOCK_VERSION entry
			mDockArea->writeDockConfig(config,CONFIG_DOCK_GROUP);
			config->writeEntry(CONFIG_GWENVIEW_DOCK_VERSION, GWENVIEW_DOCK_VERSION);
			config->sync();
		}
	} else {
		// There was no dock config, lets create the GWENVIEW_DOCK_VERSION entry
		config->setGroup(CONFIG_DOCK_GROUP);
		config->writeEntry(CONFIG_GWENVIEW_DOCK_VERSION, GWENVIEW_DOCK_VERSION);
		config->sync();
	}
	
	// Load config
	mFileViewStack->readConfig(config,CONFIG_FILEWIDGET_GROUP);
	mDirView->readConfig(config,CONFIG_DIRWIDGET_GROUP);
	mImageView->readConfig(config,CONFIG_PIXMAPWIDGET_GROUP);
	//mSlideShow->readConfig(config,CONFIG_SLIDESHOW_GROUP);
	ThumbnailLoadJob::readConfig(config,CONFIG_THUMBNAILLOADJOB_GROUP);
	Cache::instance()->readConfig(config,CONFIG_CACHE_GROUP);
}


/**
 * This method creates all the actions Interactions between them and with
 * widgets are created in createObjectInteractions
 */
void MainWindow::createActions() {
	// Stack
	mToggleBrowse=new KToggleAction(i18n("Browse"), "folder", CTRL + Key_Return, this, SLOT(slotToggleCentralStack()), actionCollection(), "toggle_browse");
	mToggleBrowse->setChecked(true);
	mToggleBrowse->setShortcut(CTRL + Key_Return);
	
	// File
	KStdAction::open(this,SLOT(showFileDialog()),actionCollection() );
	mSaveFile=KStdAction::save(mDocument,SLOT(save()),actionCollection() );
	mSaveFileAs=KStdAction::saveAs(mDocument,SLOT(saveAs()),actionCollection() );
	mFilePrint = KStdAction::print(this, SLOT(printFile()), actionCollection());
	mRenameFile=new KAction(i18n("&Rename..."),Key_F2,this,SLOT(renameFile()),actionCollection(),"file_rename");
	mCopyFiles=new KAction(i18n("&Copy To..."),Key_F7,this,SLOT(copyFiles()),actionCollection(),"file_copy");
	mMoveFiles=new KAction(i18n("&Move To..."),Key_F8,this,SLOT(moveFiles()),actionCollection(),"file_move");
	mLinkFiles=new KAction(i18n("&Link to..."),Key_F9,this,SLOT(linkFiles()),actionCollection(),"file_link");
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

	mToggleFullScreen= KStdAction::fullScreen(this,SLOT(toggleFullScreen()),actionCollection(),0);
	mToggleSlideShow=new KAction(i18n("Slide Show"),"slideshow_play",0,this,SLOT(toggleSlideShow()),actionCollection(),"slideshow");
  
	// Go
	mGoUp=new KToolBarPopupAction(i18n("Up"), "up", ALT + Key_Up, this, SLOT(goUp()), actionCollection(), "go_up");
	new KAction( i18n( "Home" ), "gohome", KStdAccel::shortcut(KStdAccel::Home), this, SLOT(goHome()), actionCollection(), "go_home");

	// Window
	mResetDockWidgets = new KAction(i18n("Reset"), 0, this, SLOT(resetDockWidgets()), actionCollection(), "reset_dock_widgets");

	// Settings
	mShowConfigDialog=
		KStdAction::preferences(this, SLOT(showConfigDialog()), actionCollection() );
	mShowKeyDialog=
		KStdAction::keyBindings(this, SLOT(showKeyDialog()), actionCollection() );
	(void)new KAction(i18n("Configure External Tools..."), "configure", 0,
		this, SLOT(showExternalToolDialog()), actionCollection(), "configure_tools");
	(void)KStdAction::configureToolbars(
		this, SLOT(showToolBarDialog()), actionCollection() );

	actionCollection()->readShortcutSettings();
}


/**
 * This method creates the interactions between objects, when it's called, all
 * widgets and actions have already been created
 */
void MainWindow::createObjectInteractions() {
	// File view toolbar
	mFileViewStack->listMode()->plug(mFileViewToolBar);
	mFileViewStack->sideThumbnailMode()->plug(mFileViewToolBar);
	mFileViewStack->bottomThumbnailMode()->plug(mFileViewToolBar);
	actionCollection()->action("thumbnails_slider")->plug(mFileViewToolBar);
	actionCollection()->action("thumbnail_details_dialog")->plug(mFileViewToolBar);
	
	// Pixmap view caption formatter
	mCaptionFormatter.reset( new CaptionFormatter(mFileViewStack, mDocument) );
	mImageView->setOSDFormatter(mCaptionFormatter.get());
	
	// Fullscreen actions in pixmap view
	KActionPtrList actions;
	actions.append(mFileViewStack->selectPrevious());
	actions.append(mFileViewStack->selectNext());
	actions.append(mToggleFullScreen);
	actions.append(mToggleSlideShow);
	mImageView->setFullScreenActions(actions);

	// Make sure file actions are correctly updated
	connect(mFileViewStack, SIGNAL(selectionChanged()),
		this, SLOT(updateImageActions()) );

	// Bookmarks
	QString file = locate( "data", "kfile/bookmarks.xml" );
	if (file.isEmpty()) {
		file = locateLocal( "data", "kfile/bookmarks.xml" );
	}

	KBookmarkManager* manager=KBookmarkManager::managerForFile(file,false);
	manager->setUpdate(true);
	manager->setShowNSBookmarks(false);

	BookmarkViewController* ctrl=new BookmarkViewController(mBookmarkView, mBookmarkToolBar, manager);
	connect(ctrl, SIGNAL(openURL(const KURL&)),
		mFileViewStack,SLOT(setDirURL(const KURL&)) );
	connect(mFileViewStack, SIGNAL(directoryChanged(const KURL&)),
		ctrl, SLOT(setURL(const KURL&)) );

	BookmarkOwner* bookmarkOwner=new BookmarkOwner(this);

	KActionMenu* bookmark=new KActionMenu(i18n( "&Bookmarks" ), "bookmark", actionCollection(), "bookmarks" );
	new KBookmarkMenu(manager, bookmarkOwner, bookmark->popupMenu(), 0, true);

	connect(bookmarkOwner,SIGNAL(openURL(const KURL&)),
		mFileViewStack,SLOT(setDirURL(const KURL&)) );

	connect(mFileViewStack,SIGNAL(directoryChanged(const KURL&)),
		bookmarkOwner,SLOT(setURL(const KURL&)) );
}


void MainWindow::createHideShowAction(KDockWidget* dock) {
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


void MainWindow::updateWindowActions() {
	unplugActionList("winlist");
	mWindowListActions.clear();
	createHideShowAction(mFolderDock);
	createHideShowAction(mBookmarkDock);
	createHideShowAction(mPixmapDock);
	createHideShowAction(mMetaDock);
	plugActionList("winlist", mWindowListActions);
}


void MainWindow::createConnections() {
	connect(mGoUp->popupMenu(), SIGNAL(activated(int)),
		this,SLOT(goUpTo(int)));
	
	// Slideshow connections
	connect( mSlideShow, SIGNAL(nextURL(const KURL&)),
		SLOT( openURL(const KURL&)) );
	connect( mSlideShow, SIGNAL( stateChanged(bool)),
		SLOT( slotSlideShowChanged(bool)) );

	// Dir view connections
	connect(mDirView,SIGNAL(dirURLChanged(const KURL&)),
		mFileViewStack,SLOT(setDirURL(const KURL&)) );

	connect(mDirView, SIGNAL(dirRenamed(const KURL&, const KURL&)),
		this, SLOT(slotDirRenamed(const KURL&, const KURL&)) );

	// Pixmap view connections
	connect(mImageView,SIGNAL(selectPrevious()),
		mFileViewStack,SLOT(slotSelectPrevious()) );
	connect(mImageView,SIGNAL(selectNext()),
		mFileViewStack,SLOT(slotSelectNext()) );
	connect(mImageView,SIGNAL(zoomChanged(double)),
		this,SLOT(updateStatusInfo()) );
	connect(mImageView,SIGNAL(doubleClicked()),
		mToggleFullScreen,SLOT(activate()) );

	// File view connections
	connect(mFileViewStack,SIGNAL(urlChanged(const KURL&)),
		mDocument,SLOT(setURL(const KURL&)) );
	connect(mFileViewStack,SIGNAL(directoryChanged(const KURL&)),
		this,SLOT(slotDirURLChanged(const KURL&)) );
	connect(mFileViewStack,SIGNAL(directoryChanged(const KURL&)),
		mDirView,SLOT(setURL(const KURL&)) );
	connect(mFileViewStack,SIGNAL(directoryChanged(const KURL&)),
		mHistory,SLOT(addURLToHistory(const KURL&)) );

	connect(mFileViewStack,SIGNAL(completed()),
		this,SLOT(updateStatusInfo()) );
	connect(mFileViewStack,SIGNAL(canceled()),
		this,SLOT(updateStatusInfo()) );
	connect(mFileViewStack,SIGNAL(imageDoubleClicked()),
		mToggleFullScreen,SLOT(activate()) );
	connect(mFileViewStack,SIGNAL(shownFileItemRefreshed(const KFileItem*)),
		this,SLOT(slotShownFileItemRefreshed(const KFileItem*)) );
	connect(mFileViewStack,SIGNAL(sortingChanged()),
		this, SLOT(updateStatusInfo()) );

	// History connections
	connect(mHistory, SIGNAL(urlChanged(const KURL&)),
		mFileViewStack, SLOT(setDirURL(const KURL&)) );
	
	// Document connections
	connect(mDocument,SIGNAL(loading()),
		this,SLOT(slotImageLoading()) );
	connect(mDocument,SIGNAL(loaded(const KURL&)),
		this,SLOT(slotImageLoaded()) );
	connect(mDocument,SIGNAL(saved(const KURL&)),
		mFileViewStack,SLOT(updateThumbnail(const KURL&)) );
	connect(mDocument,SIGNAL(reloaded(const KURL&)),
		mFileViewStack,SLOT(updateThumbnail(const KURL&)) );

	// Location bar
	connect(mURLEdit, SIGNAL(activated(const QString &)),
		this,SLOT(slotGo()) );
	connect(mURLEdit, SIGNAL(returnPressed()),
		this,SLOT(slotGo()) );

	// Non configurable stop-fullscreen accel
	QAccel* accel=new QAccel(this);
	accel->connectItem(accel->insertItem(Key_Escape),this,SLOT(escapePressed()));

	// Dock related
	connect(mDockArea->manager(), SIGNAL(change()),
		this, SLOT(updateWindowActions()) );
}


void MainWindow::createLocationToolBar() {
	// URL Combo
	mURLEdit=new KHistoryCombo(this);
	mURLEdit->setDuplicatesEnabled(false);
	mURLEdit->setPixmapProvider(new KURLPixmapProvider);

	mURLEditCompletion=new KURLCompletion();
	//mURLEditCompletion->setDir("/");

	mURLEdit->setCompletionObject(mURLEditCompletion);
	mURLEdit->setAutoDeleteCompletionObject(true);

	KWidgetAction* comboAction=new KWidgetAction( mURLEdit, i18n("Location Bar"), 0,
		0, 0, actionCollection(), "location_url");
	comboAction->setShortcutConfigurable(false);
	comboAction->setAutoSized(true);

	// Clear button
	(void)new KAction( i18n("Clear Location Bar"),
		QApplication::reverseLayout()?"clear_left" : "locationbar_erase",
		0, this, SLOT(clearLocationLabel()), actionCollection(), "clear_location");

	// URL Label
	/* we use "kde toolbar widget" to avoid the flat background (looks bad with
	 * styles like Keramik). See konq_misc.cc.
	 */
	QLabel* urlLabel=new QLabel(i18n("L&ocation:"), this, "kde toolbar widget");
	(void)new KWidgetAction( urlLabel, i18n("L&ocation: "), Key_F6, this, SLOT( activateLocationLabel()),
		actionCollection(), "location_label");
	urlLabel->setBuddy(mURLEdit);

	// Go button
	(void)new KAction(i18n("Go"), "key_enter", 0, this, SLOT(slotGo()), actionCollection(), "location_go");

}


void MainWindow::clearLocationLabel() {
	mURLEdit->clearEdit();
	mURLEdit->setFocus();
}


void MainWindow::activateLocationLabel() {
	mURLEdit->setFocus();
	mURLEdit->lineEdit()->selectAll();
}


#ifdef GV_HAVE_KIPI
void MainWindow::loadPlugins() {
	// Already done
	Q_ASSERT(!mPluginLoader);
	if (mPluginLoader) return;

	// Not yet, wait until we're less busy
	if (BusyLevelManager::instance()->busyLevel() > BUSY_THUMBNAILS) {
		LOG("Too busy, wait");
		QTimer::singleShot(LOAD_PLUGIN_DELAY, this, SLOT(loadPlugins()) );
		return;
	}
	
	LOG("Load plugins");
	// Sets up the plugin interface, and load the plugins
	KIPIInterface* interface = new KIPIInterface(this, mFileViewStack);
	mPluginLoader = new KIPI::PluginLoader(QStringList(), interface );
	connect( mPluginLoader, SIGNAL( replug() ), this, SLOT( slotReplug() ) );
	mPluginLoader->loadPlugins();
}


// Helper class for slotReplug(), gcc does not want to instantiate templates
// with local classes, so this is declared outside of slotReplug()
struct MenuInfo {
	QString mName;
	QPtrList<KAction> mActions;
	MenuInfo() {}
	MenuInfo(const QString& name) : mName(name) {}
};

void MainWindow::slotReplug() {
	typedef QMap<KIPI::Category, MenuInfo> CategoryMap;
	CategoryMap categoryMap;
	categoryMap[KIPI::IMAGESPLUGIN]=MenuInfo("image_actions");
	categoryMap[KIPI::EFFECTSPLUGIN]=MenuInfo("effect_actions");
	categoryMap[KIPI::TOOLSPLUGIN]=MenuInfo("tool_actions");
	categoryMap[KIPI::IMPORTPLUGIN]=MenuInfo("import_actions");
	categoryMap[KIPI::EXPORTPLUGIN]=MenuInfo("export_actions");
	categoryMap[KIPI::BATCHPLUGIN]=MenuInfo("batch_actions");
	categoryMap[KIPI::COLLECTIONSPLUGIN]=MenuInfo("collection_actions");
	
	// Fill the mActions
	KIPI::PluginLoader::PluginList pluginList=mPluginLoader->pluginList();
	KIPI::PluginLoader::PluginList::ConstIterator it(pluginList.begin());
	KIPI::PluginLoader::PluginList::ConstIterator itEnd(pluginList.end());
	for( ; it!=itEnd; ++it ) {
		if (!(*it)->shouldLoad()) continue;
		KIPI::Plugin* plugin = (*it)->plugin();
		Q_ASSERT(plugin);
		if (!plugin) continue;

		plugin->setup(this);
		KActionPtrList actions = plugin->actions();
		KActionPtrList::ConstIterator actionIt=actions.begin(), end=actions.end();
		for (; actionIt!=end; ++actionIt) {
			KIPI::Category category = plugin->category(*actionIt);

			if (!categoryMap.contains(category)) {
				kdWarning() << "Unknown category '" << category << "'\n";
				continue;
			}

			categoryMap[category].mActions.append(*actionIt);
		}
		plugin->actionCollection()->readShortcutSettings();
	}

	// Create a dummy "no plugin" action list
	KAction* noPlugin=new KAction(i18n("No Plugin"), 0, 0, 0, actionCollection(), "no_plugin");
	noPlugin->setShortcutConfigurable(false);
	noPlugin->setEnabled(false);
	QPtrList<KAction> noPluginList;
	noPluginList.append(noPlugin);
	
	// Fill the menu
	CategoryMap::ConstIterator catIt=categoryMap.begin(), catItEnd=categoryMap.end();
	for (; catIt!=catItEnd; ++catIt) {
		const MenuInfo& info=catIt.data();
		unplugActionList(info.mName);
		if (info.mActions.count()>0) {
			plugActionList(info.mName, info.mActions);
		} else {
			plugActionList(info.mName, noPluginList);
		}
	}
}
#else
void MainWindow::loadPlugins() {
	QPopupMenu *popup = static_cast<QPopupMenu*>(
		factory()->container( "plugins", this));
	delete popup;
}


void MainWindow::slotReplug() {
}
#endif


//-----------------------------------------------------------------------
//
// Properties
//
//-----------------------------------------------------------------------
void MainWindow::setShowBusyPtrInFullScreen(bool value) {
	mShowBusyPtrInFullScreen=value;
}

void MainWindow::setAutoDeleteThumbnailCache(bool value){
	mAutoDeleteThumbnailCache=value;
}


//-----------------------------------------------------------------------
//
// Configuration
//
//-----------------------------------------------------------------------
void MainWindow::readConfig(KConfig* config,const QString& group) {
	config->setGroup(group);
	mAutoDeleteThumbnailCache=config->readBoolEntry(CONFIG_AUTO_DELETE_THUMBNAIL_CACHE, false);	
}


void MainWindow::writeConfig(KConfig* config,const QString& group) const {
	config->setGroup(group);
	config->writeEntry(CONFIG_AUTO_DELETE_THUMBNAIL_CACHE, mAutoDeleteThumbnailCache);
}

} // namespace
