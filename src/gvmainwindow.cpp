// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
#include <klocale.h>
#include <kmenubar.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <ksqueezedtextlabel.h>
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
#include "fileoperation.h"
#include "gvarchive.h"
#include "gvbatchmanipulator.h"
#include "gvbookmarkowner.h"
#include "gvconfigdialog.h"
#include "gvdirview.h"
#include "gvdocument.h"
#include "gvexternaltooldialog.h"
#include "gvfileviewbase.h"
#include "gvfileviewstack.h"
#include "gvhistory.h"
#include "gvscrollpixmapview.h"
#include "gvslideshow.h"
#include "gvslideshowdialog.h"
#include "gvmetaedit.h"
#include "gvprintdialog.h"
#include "gvcache.h"
#include "thumbnailloadjob.h"

#include "config.h"

#ifdef GV_HAVE_KIPI
#include "gvkipiinterface.h"
#endif

#include <kcursor.h>

#include "gvmainwindow.moc"

const char CONFIG_DOCK_GROUP[]="dock";
const char CONFIG_MAINWINDOW_GROUP[]="main window";
const char CONFIG_FILEWIDGET_GROUP[]="file widget";
const char CONFIG_DIRWIDGET_GROUP[]="dir widget";
const char CONFIG_PIXMAPWIDGET_GROUP[]="pixmap widget";
const char CONFIG_FILEOPERATION_GROUP[]="file operations";
const char CONFIG_SLIDESHOW_GROUP[]="slide show";
const char CONFIG_CACHE_GROUP[]="cache";
const char CONFIG_THUMBNAILLOADJOB_GROUP[]="thumbnail loading";

const char CONFIG_BUSYPTR_IN_FS[]="busy ptr in full screen";
const char CONFIG_SHOW_LOCATION_TOOLBAR[]="show address bar";
const char CONFIG_AUTO_DELETE_THUMBNAIL_CACHE[]="Delete Thumbnail Cache whe exit";
const char CONFIG_GWENVIEW_DOCK_VERSION[]="Gwenview version";

const char CONFIG_SESSION_URL[] = "url";

// This version is here to avoid configuration migration troubles when changes
// are made to the dock behavior
const int GWENVIEW_DOCK_VERSION=2;

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


GVMainWindow::GVMainWindow()
: KMainWindow(), mLoadingCursor(false)
{
	FileOperation::readConfig(KGlobal::config(),CONFIG_FILEOPERATION_GROUP);
	readConfig(KGlobal::config(),CONFIG_MAINWINDOW_GROUP);

	// Backend
	mDocument=new GVDocument(this);
	mGVHistory=new GVHistory(actionCollection());
	// GUI
	createWidgets();
	createActions();
	createLocationToolBar();

	setStandardToolBarMenuEnabled(true);
	createGUI("gwenviewui.rc", false);

	createConnections();
	mWindowListActions.setAutoDelete(true);
	updateWindowActions();
	loadPlugins();
	applyMainWindowSettings();

	mFileViewStack->setFocus();

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
}


bool GVMainWindow::queryClose() {
	if (!mDocument->saveBeforeClosing()) return false;

	KConfig* config=KGlobal::config();
	FileOperation::writeConfig(config, CONFIG_FILEOPERATION_GROUP);
	mPixmapView->writeConfig(config, CONFIG_PIXMAPWIDGET_GROUP);
	mFileViewStack->writeConfig(config, CONFIG_FILEWIDGET_GROUP);
	mDirView->writeConfig(config, CONFIG_DIRWIDGET_GROUP);
	mSlideShow->writeConfig(config, CONFIG_SLIDESHOW_GROUP);
	ThumbnailLoadJob::writeConfig(config, CONFIG_THUMBNAILLOADJOB_GROUP);

	// Don't store dock layout if only the image dock is visible. This avoid
	// saving layout when in "fullscreen" or "image only" mode.
	if (mFileViewStack->isVisible() || mDirView->isVisible()) {
		mDockArea->writeDockConfig(config,CONFIG_DOCK_GROUP);
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
			KIO::NetAccess::del(url, this);
		}
	}
	
	saveMainWindowSettings(KGlobal::config(), "MainWindow");

	return true;
}

void GVMainWindow::saveProperties( KConfig* cfg ) {
	cfg->writeEntry( CONFIG_SESSION_URL, mFileViewStack->url().url());
}

void GVMainWindow::readProperties( KConfig* cfg ) {
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
void GVMainWindow::openURL(const KURL& url) {
	mDocument->setURL(url);
	mFileViewStack->setDirURL(url.upURL());
	mFileViewStack->setFileNameToSelect(url.filename());
}


void GVMainWindow::slotDirURLChanged(const KURL& dirURL) {
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

void GVMainWindow::updateLocationURL() {
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
	mFileViewStack->setDirURL(url);
	mFileViewStack->setFileNameToSelect(childURL.fileName());
}


//-----------------------------------------------------------------------
//
// File operations
//
//-----------------------------------------------------------------------
void GVMainWindow::openHomeDir() {
	KURL url;
	url.setPath( QDir::homeDirPath() );
	mFileViewStack->setDirURL(url);
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
	modifyImage(GVImageUtils::VFLIP);
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
		mDocument->transform(orientation);
	}
}

void GVMainWindow::showFileDialog() {
	KURL url=KFileDialog::getOpenURL();
	if (!url.isValid()) return;

	openURL(url);
}


void GVMainWindow::printFile() {
	KPrinter printer;

	printer.setDocName(mDocument->filename());
	const KAboutData* pAbout = KApplication::kApplication()->aboutData();
	QString nm = pAbout->appName();
	nm += "-";
	nm += pAbout->version();
	printer.setCreator( nm );

	KPrinter::addDialogPage( new GVPrintDialogPage( mDocument, this, "GV page"));

	if (printer.setup(this, QString::null, true)) {
		mDocument->print(&printer);
	}
}


//-----------------------------------------------------------------------
//
// Private slots
//
//-----------------------------------------------------------------------
void GVMainWindow::slotImageLoading() {
	if (mShowBusyPtrInFullScreen || !mToggleFullScreen->isChecked()) {
		if( !mLoadingCursor ) {
			kapp->setOverrideCursor(KCursor::workingCursor());
		}
		mLoadingCursor = true;
	}
}


void GVMainWindow::slotImageLoaded() {
	if( mLoadingCursor )
		kapp->restoreOverrideCursor();
	mLoadingCursor = false;
	updateStatusInfo();
	updateImageActions();
	updateLocationURL();
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
	if (mToggleFullScreen->isChecked()) {
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
			mPixmapView->reparent(mViewModeWidget, QPoint(0,0));
			mCentralStack->raiseWidget(StackIDView);
		}
		KActionPtrList actions;
		actions.append(mFileViewStack->selectPrevious());
		actions.append(mFileViewStack->selectNext());
		actions.append(mToggleFullScreen);
		mPixmapView->setFullScreenActions(actions);
		mPixmapView->setFullScreen(true);
		mPixmapView->setFocus();
	} else {
		// Stop the slideshow if it's running, harmless if it does not
		mSlideShow->stop();

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
		mPixmapView->setFullScreen(false);
		
		if (mToggleBrowse->isChecked()) {
			mPixmapDock->setWidget(mPixmapView);
			mCentralStack->raiseWidget(StackIDBrowse);
		}
		mFileViewStack->setFocus();
	}
}


void GVMainWindow::startSlideShow() {
	KURL::List list;
	KFileItemListIterator it( *mFileViewStack->currentFileView()->items() );
	for ( ; it.current(); ++it ) {
		KFileItem* item=it.current();
		if (!item->isDir() && !GVArchive::fileItemIsArchive(item)) {
			list.append(item->url());
		}
	}
	if (list.count()==0) {
		return;
	}

	GVSlideShowDialog dialog(this,mSlideShow);
	if (!dialog.exec()) return;
	
	if (!mToggleFullScreen->isChecked()) {
		mToggleFullScreen->activate();
	}
	mSlideShow->start(list);
}


void GVMainWindow::showConfigDialog() {
	GVConfigDialog dialog(this);
	dialog.exec();
}


void GVMainWindow::showExternalToolDialog() {
	GVExternalToolDialog* dialog=new GVExternalToolDialog(this);
	dialog->show();
}


void GVMainWindow::showKeyDialog() {
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
	if (mToggleFullScreen->isChecked()) {
		mToggleFullScreen->activate();
	}
}


void GVMainWindow::slotDirRenamed(const KURL& oldURL, const KURL& newURL) {
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


void GVMainWindow::slotGo() {
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

void GVMainWindow::slotShownFileItemRefreshed(const KFileItem*) {
	LOG("");
	mDocument->reload();
}
	

void GVMainWindow::slotToggleCentralStack() {
	LOG("");
	if (mToggleBrowse->isChecked()) {
		mPixmapDock->setWidget(mPixmapView);
		mCentralStack->raiseWidget(StackIDBrowse);
		mFileViewStack->setSilentMode( false );
		// force re-reading the directory to show the error
		if( mFileViewStack->lastURLError()) mFileViewStack->retryURL();
	} else {
		mPixmapView->reparent(mViewModeWidget, QPoint(0,0));
		mCentralStack->raiseWidget(StackIDView);
		mFileViewStack->setSilentMode( true );
	}

	// Make sure the window list actions are disabled if we are in view mode,
	// otherwise weird things happens when we go back to browse mode
	QPtrListIterator<KAction> it(mWindowListActions);
	for (;it.current(); ++it) {
		it.current()->setEnabled(mToggleBrowse->isChecked());
	}
	updateLocationURL();
}


void GVMainWindow::resetDockWidgets() {
	mFolderDock->undock();
	mPixmapDock->undock();
	mMetaDock->undock();

	mFolderDock->manualDock(mFileDock, KDockWidget::DockLeft, 4000);
	mPixmapDock->manualDock(mFolderDock, KDockWidget::DockBottom, 3734);
	mMetaDock->manualDock(mPixmapDock, KDockWidget::DockBottom, 8560);
}


//-----------------------------------------------------------------------
//
// GUI
//
//-----------------------------------------------------------------------
void GVMainWindow::updateStatusInfo() {
	QString txt;
	uint count=mFileViewStack->fileCount();
#if KDE_IS_VERSION( 3, 4, 0 )
	QString url=mDocument->dirURL().pathOrURL();
#else
	QString url=mDocument->dirURL().prettyURL(0,KURL::StripFileProtocol);
#endif
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
		QString info=filename + QString(" %1x%2 @ %3%")
			.arg(mDocument->width()).arg(mDocument->height())
			.arg(int(mPixmapView->zoom()*100) );
		mSBDetailLabel->show();
		mSBDetailLabel->setText(info);
	} else {
		mSBDetailLabel->hide();
	}
	setCaption(filename);
}


void GVMainWindow::updateImageActions() {
	bool filenameIsValid=!mDocument->isNull();

	mStartSlideShow->setEnabled(filenameIsValid);
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

}


void GVMainWindow::createWidgets() {
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
	mSBDirLabel=new KSqueezedTextLabel("", statusBar());
	statusBar()->addWidget(mSBDirLabel,1);
	mSBDetailLabel=new QLabel("", statusBar());
	statusBar()->addWidget(mSBDetailLabel);

	// Pixmap widget
	mPixmapDock = mDockArea->createDockWidget("Image",SmallIcon("gwenview"),NULL,i18n("Image"));
	mPixmapView=new GVScrollPixmapView(mPixmapDock,mDocument,actionCollection());
	mPixmapDock->setWidget(mPixmapView);

	// Folder widget
	mFolderDock = mDockArea->createDockWidget("Folders",SmallIcon("folder_open"),NULL,i18n("Folders"));
	mDirView=new GVDirView(mFolderDock);
	mFolderDock->setWidget(mDirView);

	// File widget
	mFileDock = mDockArea->createDockWidget("Files",SmallIcon("image"),NULL,i18n("Files"));
	QVBox* vbox=new QVBox(this);
	(void)new KToolBar(vbox, "fileViewToolBar", true);
	mFileViewStack=new GVFileViewStack(vbox, actionCollection());
	mFileDock->setWidget(vbox);
	mFileDock->setEnableDocking(KDockWidget::DockNone);
	mDockArea->setMainDockWidget(mFileDock);

	// Meta info edit widget
	mMetaDock = mDockArea->createDockWidget("File Attributes", SmallIcon("info"),NULL,
		i18n("File Info"));
	mMetaEdit = new GVMetaEdit(mMetaDock, mDocument);
	mMetaDock->setWidget(mMetaEdit);

	// Slide show controller (not really a widget)
	mSlideShow=new GVSlideShow(mDocument);
	connect( mSlideShow, SIGNAL( nextURL( const KURL& )), SLOT( openURL( const KURL& )));

	// Default position on desktop
	setGeometry(20,20,720,520);

	// Default dock config
	// (The "magic numbers" were found by adjusting the layout from within the
	// app and looking at the result in the configuration file)
	mFolderDock->manualDock(mFileDock, KDockWidget::DockLeft, 4000);
	mPixmapDock->manualDock(mFolderDock, KDockWidget::DockBottom, 3734);
	mMetaDock->manualDock(mPixmapDock, KDockWidget::DockBottom, 8560);

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
	mPixmapView->readConfig(config,CONFIG_PIXMAPWIDGET_GROUP);
	mSlideShow->readConfig(config,CONFIG_SLIDESHOW_GROUP);
	ThumbnailLoadJob::readConfig(config,CONFIG_THUMBNAILLOADJOB_GROUP);
	GVCache::instance()->readConfig(config,CONFIG_CACHE_GROUP);
}


void GVMainWindow::createActions() {
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
	mStartSlideShow=new KAction(i18n("Slide Show..."),"slideshow",0,this,SLOT(startSlideShow()),actionCollection(),"slideshow");

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
	new KBookmarkMenu(manager, bookmarkOwner, bookmark->popupMenu(), 0, true);

	connect(bookmarkOwner,SIGNAL(openURL(const KURL&)),
		mFileViewStack,SLOT(setDirURL(const KURL&)) );

	connect(mFileViewStack,SIGNAL(directoryChanged(const KURL&)),
		bookmarkOwner,SLOT(setURL(const KURL&)) );

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
	unplugActionList("winlist");
	mWindowListActions.clear();
	createHideShowAction(mFolderDock);
	createHideShowAction(mPixmapDock);
	createHideShowAction(mMetaDock);
	plugActionList("winlist", mWindowListActions);
}


void GVMainWindow::createConnections() {
	connect(mGoUp->popupMenu(), SIGNAL(activated(int)),
		this,SLOT(goUpTo(int)));

	// Dir view connections
	connect(mDirView,SIGNAL(dirURLChanged(const KURL&)),
		mFileViewStack,SLOT(setDirURL(const KURL&)) );

	connect(mDirView, SIGNAL(dirRenamed(const KURL&, const KURL&)),
		this, SLOT(slotDirRenamed(const KURL&, const KURL&)) );

	// Pixmap view connections
	connect(mPixmapView,SIGNAL(selectPrevious()),
		mFileViewStack,SLOT(slotSelectPrevious()) );
	connect(mPixmapView,SIGNAL(selectNext()),
		mFileViewStack,SLOT(slotSelectNext()) );
	connect(mPixmapView,SIGNAL(zoomChanged(double)),
		this,SLOT(updateFileInfo()) );

	// File view connections
	connect(mFileViewStack,SIGNAL(urlChanged(const KURL&)),
		mDocument,SLOT(setURL(const KURL&)) );
	connect(mFileViewStack,SIGNAL(directoryChanged(const KURL&)),
		this,SLOT(slotDirURLChanged(const KURL&)) );
	connect(mFileViewStack,SIGNAL(directoryChanged(const KURL&)),
		mDirView,SLOT(setURL(const KURL&)) );
	connect(mFileViewStack,SIGNAL(directoryChanged(const KURL&)),
		mGVHistory,SLOT(addURLToHistory(const KURL&)) );

	connect(mFileViewStack,SIGNAL(completed()),
		this,SLOT(updateStatusInfo()) );
	connect(mFileViewStack,SIGNAL(canceled()),
		this,SLOT(updateStatusInfo()) );
	connect(mFileViewStack,SIGNAL(imageDoubleClicked()),
		mToggleBrowse,SLOT(activate()) );
	connect(mFileViewStack,SIGNAL(shownFileItemRefreshed(const KFileItem*)),
		this,SLOT(slotShownFileItemRefreshed(const KFileItem*)) );

	// GVHistory connections
	connect(mGVHistory, SIGNAL(urlChanged(const KURL&)),
		mFileViewStack, SLOT(setDirURL(const KURL&)) );
	
	// GVDocument connections
	connect(mDocument,SIGNAL(loading()),
		this,SLOT(slotImageLoading()) );
	connect(mDocument,SIGNAL(loaded(const KURL&)),
		this,SLOT(slotImageLoaded()) );
	connect(mDocument,SIGNAL(saved(const KURL&)),
		mFileViewStack,SLOT(updateThumbnail(const KURL&)) );
	connect(mDocument,SIGNAL(reloaded(const KURL&)),
		mFileViewStack,SLOT(updateThumbnail(const KURL&)) );

	// Slide show
	connect(mSlideShow,SIGNAL(finished()),
		mToggleFullScreen,SLOT(activate()) );

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


void GVMainWindow::createLocationToolBar() {
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


void GVMainWindow::clearLocationLabel() {
	mURLEdit->clearEdit();
	mURLEdit->setFocus();
}


void GVMainWindow::activateLocationLabel() {
	mURLEdit->setFocus();
	mURLEdit->lineEdit()->selectAll();
}


#ifdef GV_HAVE_KIPI
void GVMainWindow::loadPlugins() {
	// Sets up the plugin interface, and load the plugins
	GVKIPIInterface* interface = new GVKIPIInterface(this, mFileViewStack);
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

void GVMainWindow::slotReplug() {
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
void GVMainWindow::loadPlugins() {
	QPopupMenu *popup = static_cast<QPopupMenu*>(
		factory()->container( "plugins", this));
	delete popup;
}


void GVMainWindow::slotReplug() {
}
#endif


//-----------------------------------------------------------------------
//
// Properties
//
//-----------------------------------------------------------------------
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
	mShowBusyPtrInFullScreen=config->readBoolEntry(CONFIG_BUSYPTR_IN_FS, true);
	mAutoDeleteThumbnailCache=config->readBoolEntry(CONFIG_AUTO_DELETE_THUMBNAIL_CACHE, false);	
}


void GVMainWindow::writeConfig(KConfig* config,const QString& group) const {
	config->setGroup(group);
	config->writeEntry(CONFIG_BUSYPTR_IN_FS, mShowBusyPtrInFullScreen);
	config->writeEntry(CONFIG_AUTO_DELETE_THUMBNAIL_CACHE, mAutoDeleteThumbnailCache);
}
