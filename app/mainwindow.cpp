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
#include "mainwindow.moc"

// Qt
#include <qdir.h>
#include <qdockarea.h>
#include <qwidgetstack.h>

// KDE
#include <kaboutdata.h>
#include <kaccel.h>
#include <kaction.h>
#include <kapplication.h>
#include <kbookmarkmanager.h>
#include <kbookmarkmenu.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kcursor.h>
#include <kdebug.h>
#include <kdeversion.h>
#include <kdockwidget.h>
#include <kedittoolbar.h>
#include <kfiledialog.h>
#include <kglobal.h>
#include <khelpmenu.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <kkeydialog.h>
#include <klargefile.h>
#include <klistview.h>
#include <klocale.h>
#include <kmenubar.h>
#include <kmessagebox.h>
#include <kpopupmenu.h>
#include <kpropsdlg.h>
#include <kprotocolinfo.h>
#include <kstandarddirs.h>
#include <kstatusbar.h>
#include <kstdaccel.h>
#include <kstdaction.h>
#include <ktoolbarlabelaction.h>
#include <kurlcompletion.h>
#include <kurlpixmapprovider.h>
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
#include "dirviewcontroller.h"
#include "history.h"
#include "metaedit.h"
#include "truncatedtextlabel.h"
#include "vtabwidget.h"

#include "gvcore/externaltoolcontext.h"
#include "gvcore/externaltoolmanager.h"
#include "gvcore/fileoperation.h"
#include "gvcore/archive.h"
#include "gvcore/captionformatter.h"
#include "gvcore/document.h"
#include "gvcore/externaltooldialog.h"
#include "gvcore/fileviewbase.h"
#include "gvcore/fileviewcontroller.h"
#include "gvcore/imageview.h"
#include "gvcore/imageviewcontroller.h"
#include "gvcore/slideshow.h"
#include "gvcore/printdialog.h"
#include "gvcore/cache.h"
#include "gvcore/thumbnailloadjob.h"
#include <../gvcore/slideshowconfig.h>
#include <../gvcore/fullscreenconfig.h>
#include <../gvcore/fileviewconfig.h>
#include <../gvcore/miscconfig.h>

#include "config.h"

#ifdef GV_HAVE_KIPI
#include "kipiinterface.h"
#endif

namespace Gwenview {

const char CONFIG_DOCK_GROUP[]="dock";
const char CONFIG_DIRWIDGET_GROUP[]="dir widget";
const char CONFIG_PIXMAPWIDGET_GROUP[]="pixmap widget";
const char CONFIG_CACHE_GROUP[]="cache";

const char CONFIG_GWENVIEW_DOCK_VERSION[]="Gwenview version";

const char CONFIG_SESSION_URL[] = "url";

// This version is here to avoid configuration migration troubles when changes
// are made to the dock behavior
const int GWENVIEW_DOCK_VERSION=2;

// The timeout before an hint in the statusbar disappear (in msec)
const int HINT_TIMEOUT=10000;

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

enum { StackIDBrowse, StackIDView };


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
	KMainWindow::applyMainWindowSettings(KGlobal::config(), "MainWindow");
}


void MainWindow::setFullScreen(bool value) {
	if (value != mToggleFullScreen->isChecked()) {
		mToggleFullScreen->activate();
	}
}


bool MainWindow::queryClose() {
	mDocument->saveBeforeClosing();

	KConfig* config=KGlobal::config();

	// Don't store dock layout if only the image dock is visible. This avoid
	// saving layout when in "fullscreen" or "image only" mode.
	if (mFileViewController->isVisible() || mDirViewController->widget()->isVisible()) {
		mDockArea->writeDockConfig(config,CONFIG_DOCK_GROUP);
	}

	if (FileViewConfig::deleteCacheOnExit()) {
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
	MiscConfig::setHistory( mURLEdit->historyItems() );
	MiscConfig::writeConfig();
	return true;
}

void MainWindow::saveProperties( KConfig* cfg ) {
	cfg->writeEntry( CONFIG_SESSION_URL, mFileViewController->url().url());
}

void MainWindow::readProperties( KConfig* cfg ) {
	KURL url(cfg->readEntry(CONFIG_SESSION_URL));
	openURL(url);
}

//-----------------------------------------------------------------------
//
// Public slots
//
//-----------------------------------------------------------------------
void MainWindow::openURL(const KURL& url) {
	bool isDir = urlIsDirectory(this, url);
	LOG("url=" << url.prettyURL() << ", isDir=" << isDir);

	if (isDir) {
		mFileViewController->setDirURL(url);
		mFileViewController->setFocus();
	} else {
		mDocument->setURL(url);
		mFileViewController->setDirURL(url.upURL());
		mFileViewController->setFileNameToSelect(url.filename());
		mImageViewController->setFocus();
	}
	
	if (!mToggleFullScreen->isChecked() && !isDir && !mSwitchToViewMode->isChecked()) {
		mSwitchToViewMode->activate();
	}
}


void MainWindow::slotDirURLChanged(const KURL& dirURL) {
	LOG(dirURL.prettyURL(0,KURL::StripFileProtocol));
	
	mGoUp->setEnabled(dirURL.path()!="/");

	updateStatusInfo();
	updateImageActions();
	updateLocationURL();
}

void MainWindow::updateLocationURL() {
	LOG("");
	KURL url;
	if (mSwitchToBrowseMode->isChecked()) {
		url=mFileViewController->dirURL();
		if (!url.isValid()) {
			url=mDocument->url();
		}
	} else {
		url=mDocument->url();
	}
	LOG(url.prettyURL());
	mURLEdit->setEditText(url.pathOrURL());
	mURLEdit->addToHistory(url.pathOrURL());
}

void MainWindow::goUp() {
	KURL url = mFileViewController->dirURL();
	mFileViewController->setDirURL(url.upURL());
	mFileViewController->setFileNameToSelect(url.fileName());
}

void MainWindow::updateFullScreenLabel() {
	CaptionFormatter formatter;
	formatter.mPath=mDocument->url().path();
	formatter.mFileName=mDocument->url().fileName();
	formatter.mComment=mDocument->comment();
	formatter.mImageSize=mDocument->image().size();
	formatter.mPosition=mFileViewController->shownFilePosition()+1;
	formatter.mCount=mFileViewController->fileCount();
	
	QString txt=formatter.format( FullScreenConfig::osdFormat() );
	mFullScreenLabelAction->label()->setText(txt);
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
	mFileViewController->setDirURL(url);
	mFileViewController->setFileNameToSelect(childURL.fileName());
}

void MainWindow::fillGoUpMenu() {
	QPopupMenu* menu = mGoUp->popupMenu();
	menu->clear();
	int pos = 0;
	KURL url = mFileViewController->dirURL().upURL();
	for (; url.hasPath() && pos<10; url=url.upURL(), ++pos) {
		menu->insertItem(url.pathOrURL());
		if (url.path()=="/") break;
	}
}


//-----------------------------------------------------------------------
//
// File operations
//
//-----------------------------------------------------------------------
void MainWindow::goHome() {
	KURL url;
	url.setPath( QDir::homeDirPath() );
	mFileViewController->setDirURL(url);
}


void MainWindow::renameFile() {
	KURL url;
	if (mFileViewController->isVisible()) {
		KURL::List list = mFileViewController->selectedURLs();
		Q_ASSERT(list.count()==1);
		if (list.count()!=1) return;
		url = list.first();
	} else {
		url = mDocument->url();
	}
	FileOperation::rename(url, this);
}


void MainWindow::copyFiles() {
	KURL::List list;
	if (mFileViewController->isVisible()) {
		list = mFileViewController->selectedURLs();
	} else {
		list << mDocument->url();
	}
	FileOperation::copyTo(list, this);
}

void MainWindow::linkFiles() {
	KURL::List list;
	if (mFileViewController->isVisible()) {
		list = mFileViewController->selectedURLs();
	} else {
		list << mDocument->url();
	}
	FileOperation::linkTo(list, this);
}


void MainWindow::moveFiles() {
	KURL::List list;
	if (mFileViewController->isVisible()) {
		list = mFileViewController->selectedURLs();
	} else {
		list << mDocument->url();
	}
	FileOperation::moveTo(list, this);
}


void MainWindow::deleteFiles() {
	KURL::List list;
	if (mFileViewController->isVisible()) {
		list = mFileViewController->selectedURLs();
	} else {
		list << mDocument->url();
	}
	FileOperation::del(list, this);
}


void MainWindow::makeDir() {
	FileOperation::makeDir(mFileViewController->dirURL(), this);
}


void MainWindow::showFileProperties() {
	if (mFileViewController->isVisible()) {
		const KFileItemList* list = mFileViewController->currentFileView()->selectedItems();
		(void)new KPropertiesDialog(*list, this);
	} else {
		(void)new KPropertiesDialog(mDocument->url(), this);
	}
}


void MainWindow::rotateLeft() {
	mDocument->transform(ImageUtils::ROT_270);
}

void MainWindow::rotateRight() {
	mDocument->transform(ImageUtils::ROT_90);
}

void MainWindow::mirror() {
	mDocument->transform(ImageUtils::HFLIP);
}

void MainWindow::flip() {
	mDocument->transform(ImageUtils::VFLIP);
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
void MainWindow::openFileViewControllerContextMenu(const QPoint& pos, bool onItem) {
	int selectionSize;
	ExternalToolContext* externalToolContext;

	if (onItem) {
		const KFileItemList* items = mFileViewController->currentFileView()->selectedItems();
		selectionSize = items->count();
		externalToolContext =
			ExternalToolManager::instance()->createContext(this, items);
	} else {
		selectionSize = 0;
		externalToolContext =
			ExternalToolManager::instance()->createContext(this, mFileViewController->dirURL());
	}

	QPopupMenu menu(this);

	menu.insertItem(
		i18n("External Tools"), externalToolContext->popupMenu());

	actionCollection()->action("view_sort")->plug(&menu);
	mGoUp->plug(&menu);

	menu.insertItem(SmallIcon("folder_new"), i18n("New Folder..."), this, SLOT(makeDir()));

	menu.insertSeparator();

	if (selectionSize==1) {
		mRenameFile->plug(&menu);
	}

	if (selectionSize>=1) {
		mCopyFiles->plug(&menu);
		mMoveFiles->plug(&menu);
		mLinkFiles->plug(&menu);
		mDeleteFiles->plug(&menu);
		menu.insertSeparator();
	}

	mShowFileProperties->plug(&menu);
	menu.exec(pos);
}


void MainWindow::slotImageLoading() {
	if (FullScreenConfig::showBusyPtr() || !mToggleFullScreen->isChecked()) {
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
	if (mToggleFullScreen->isChecked()) {
		updateFullScreenLabel();
	}
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
		
		if (mSwitchToBrowseMode->isChecked()) {
			mImageViewController->widget()->reparent(mViewModeWidget, QPoint(0,0));
			mCentralStack->raiseWidget(StackIDView);
		}
		updateFullScreenLabel();
		mImageViewController->setFullScreen(true);
		mImageViewController->setFocus();
	} else {
		// Stop the slideshow if it's running
		if (mSlideShow->isRunning()) {
			mToggleSlideShow->activate();
		}

		// Make sure the file view points to the right URL, it might not be the
		// case if we are getting out of a slideshow
		mFileViewController->setDirURL(mDocument->url().upURL());
		mFileViewController->setFileNameToSelect(mDocument->url().fileName());

		showNormal();
		menuBar()->show();
		
		showToolBars();
		leftDock()->show();
		rightDock()->show();
		topDock()->show();
		bottomDock()->show();
		
		statusBar()->show();
		mImageViewController->setFullScreen(false);
		
		if (mSwitchToBrowseMode->isChecked()) {
			mImageDock->setWidget(mImageViewController->widget());
			mCentralStack->raiseWidget(StackIDBrowse);
			mFileViewController->setFocus();
		}
	}
}


void MainWindow::toggleSlideShow() {
	if (mSlideShow->isRunning()) {
		mSlideShow->stop();
		return;
	}
	
	KURL::List list;
	KFileItemListIterator it( *mFileViewController->currentFileView()->items() );
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
#ifdef GV_HAVE_KIPI
	if (!mPluginLoader) loadPlugins();
	ConfigDialog dialog(this, mPluginLoader);
#else
	ConfigDialog dialog(this, 0);
#endif
	connect(&dialog, SIGNAL(settingsChanged()),
		mSlideShow, SLOT(slotSettingsChanged()) );
	connect(&dialog, SIGNAL(settingsChanged()),
		mImageViewController, SLOT(updateFromSettings()) );
	connect(&dialog, SIGNAL(settingsChanged()),
		mFileViewController, SLOT(updateFromSettings()) );
	dialog.exec();
}


void MainWindow::showExternalToolDialog() {
	ExternalToolDialog* dialog=new ExternalToolDialog(this);
	dialog->show();
}


void MainWindow::showKeyDialog() {
	KKeyDialog dialog(true, this);
	dialog.insert(actionCollection());
	dialog.configure(true);
}


void MainWindow::showToolBarDialog() {
	saveMainWindowSettings(KGlobal::config(), "MainWindow");
	KEditToolbar dlg(actionCollection());
	connect(&dlg,SIGNAL(newToolbarConfig()),this,SLOT(applyMainWindowSettings()));
	dlg.exec();
}

void MainWindow::applyMainWindowSettings() {
	createGUI();
	KMainWindow::applyMainWindowSettings(KGlobal::config(), "MainWindow");
}



void MainWindow::escapePressed() {
	if (mToggleFullScreen->isChecked()) {
		mToggleFullScreen->activate();
	}
}


void MainWindow::slotDirRenamed(const KURL& oldURL, const KURL& newURL) {
	LOG(oldURL.prettyURL(0,KURL::StripFileProtocol) << " to " << newURL.prettyURL(0,KURL::StripFileProtocol));

	KURL url(mFileViewController->dirURL());
	if (!oldURL.isParentOf(url) ) {
		LOG(oldURL.prettyURL() << " is not a parent of " << url.prettyURL());
		return;
	}

	QString oldPath=oldURL.path();
	LOG("current path: " << url.path() );
	QString path=newURL.path() + url.path().mid(oldPath.length());
	LOG("new path: " << path);
	url.setPath(path);
	mFileViewController->setDirURL(url);
}


void MainWindow::slotGo() {
	KURL url(mURLEditCompletion->replacedPath(mURLEdit->currentText()));
	LOG(url.prettyURL());
	openURL(url);
	mFileViewController->setFocus();
}

void MainWindow::slotShownFileItemRefreshed(const KFileItem*) {
	LOG("");
	mDocument->reload();
}
	

void MainWindow::slotToggleCentralStack() {
	LOG("");
	if (mSwitchToBrowseMode->isChecked()) {
		mImageDock->setWidget(mImageViewController->widget());
		mCentralStack->raiseWidget(StackIDBrowse);
		mFileViewController->setSilentMode( false );
		// force re-reading the directory to show the error
		if( mFileViewController->lastURLError()) mFileViewController->retryURL();
	} else {
		mImageViewController->widget()->reparent(mViewModeWidget, QPoint(0,0));
		mCentralStack->raiseWidget(StackIDView);
		mFileViewController->setSilentMode( true );
	}

	// Make sure the window list actions are disabled if we are in view mode,
	// otherwise weird things happens when we go back to browse mode
	QPtrListIterator<KAction> it(mWindowListActions);
	for (;it.current(); ++it) {
		it.current()->setEnabled(mSwitchToBrowseMode->isChecked());
	}
	updateImageActions();
	updateLocationURL();
}


void MainWindow::resetDockWidgets() {
	int answer=KMessageBox::warningContinueCancel(this,
		i18n("You are about to revert the window setup to factory defaults, are you sure?"),
		QString::null /* caption */,
		i18n("Reset"));
	if (answer==KMessageBox::Cancel) return;
	
	mFolderDock->undock();
	mImageDock->undock();
	mMetaDock->undock();

	mFolderDock->manualDock(mFileDock, KDockWidget::DockLeft, 4000);
	mImageDock->manualDock(mFolderDock, KDockWidget::DockBottom, 3734);
	mMetaDock->manualDock(mImageDock, KDockWidget::DockBottom, 8560);
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
	QString txt;

	if ( KProtocolInfo::supportsListing(mFileViewController->url()) ) {
		int pos = mFileViewController->shownFilePosition();
		uint count = mFileViewController->fileCount();
		if (count > 0) {
			txt = i18n("%1/%2 - ").arg(pos+1).arg(count);
		} else {
			txt = i18n("No images");
		}
	}

	QString filename = mDocument->filename();
	txt += filename;

	QSize size = mDocument->image().size();
	if (!size.isEmpty()) {
		txt += QString(" %1x%2").arg(size.width()).arg(size.height());
	}
	
	mSBDetailLabel->setText(txt);
	setCaption(filename);
}


void MainWindow::updateImageActions() {
	mToggleSlideShow->setEnabled(mDocument->urlKind()!=MimeTypeUtils::KIND_UNKNOWN);
	
	bool imageActionsEnabled = !mDocument->isNull();
	
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
		|| (mFileViewController->isVisible() && mFileViewController->selectionSize()>0);

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
	mImageDock = mDockArea->createDockWidget("Image",SmallIcon("gwenview"),NULL,i18n("Image"));
	mImageViewController=new ImageViewController(mImageDock, mDocument, actionCollection());
	mImageDock->setWidget(mImageViewController->widget());
	connect(mImageViewController, SIGNAL(requestHintDisplay(const QString&)),
		this, SLOT(showHint(const QString&)) );

	// Folder widget
	mFolderDock = mDockArea->createDockWidget("Folders",SmallIcon("folder_open"),NULL,i18n("Folders"));
	VTabWidget* vtabWidget=new VTabWidget(mFolderDock);
	mFolderDock->setWidget(vtabWidget);
	
	mDirViewController=new DirViewController(vtabWidget);
	vtabWidget->addTab(mDirViewController->widget(), SmallIcon("folder"), i18n("Folders"));

	mBookmarkViewController=new BookmarkViewController(vtabWidget);
	vtabWidget->addTab(mBookmarkViewController->widget(), SmallIcon("bookmark"), i18n("Bookmarks"));

	// File widget
	mFileDock = mDockArea->createDockWidget("Files",SmallIcon("image"),NULL,i18n("Files"));
	mFileViewController=new FileViewController(this, actionCollection());
	mFileDock->setWidget(mFileViewController);
	mFileDock->setEnableDocking(KDockWidget::DockNone);
	mDockArea->setMainDockWidget(mFileDock);

	// Meta info edit widget
	mMetaDock = mDockArea->createDockWidget("File Attributes", SmallIcon("info"),NULL,
		i18n("Image Comment"));
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
	mImageDock->manualDock(mFolderDock, KDockWidget::DockBottom, 3734);
	mMetaDock->manualDock(mImageDock, KDockWidget::DockBottom, 8560);

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
	Cache::instance()->readConfig(config,CONFIG_CACHE_GROUP);
}


/**
 * This method creates all the actions Interactions between them and with
 * widgets are created in createObjectInteractions
 */
void MainWindow::createActions() {
	// Stack
	mSwitchToBrowseMode=new KRadioAction(i18n("Browse"), "folder_image", CTRL + Key_Return, this, SLOT(slotToggleCentralStack()), actionCollection(), "switch_to_browse_mode");
	mSwitchToBrowseMode->setExclusiveGroup("centralStackMode");
	mSwitchToBrowseMode->setChecked(true);
	mSwitchToViewMode=new KRadioAction(i18n("View Image"), "image", 0, this, SLOT(slotToggleCentralStack()), actionCollection(), "switch_to_view_mode");
	mSwitchToViewMode->setExclusiveGroup("centralStackMode");
	
	// File
	KStdAction::open(this,SLOT(showFileDialog()),actionCollection() );
	mSaveFile=KStdAction::save(mDocument,SLOT(save()),actionCollection() );
	mSaveFileAs=KStdAction::saveAs(mDocument,SLOT(saveAs()),actionCollection() );
	mFilePrint = KStdAction::print(this, SLOT(printFile()), actionCollection());
	mRenameFile=new KAction(i18n("&Rename..."),Key_F2,this,SLOT(renameFile()),actionCollection(),"file_rename");
	mCopyFiles=new KAction(i18n("&Copy To..."),Key_F7,this,SLOT(copyFiles()),actionCollection(),"file_copy");
	mMoveFiles=new KAction(i18n("&Move To..."),Key_F8,this,SLOT(moveFiles()),actionCollection(),"file_move");
	mLinkFiles=new KAction(i18n("&Link To..."),Key_F9,this,SLOT(linkFiles()),actionCollection(),"file_link");
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
	mFullScreenLabelAction=new KToolBarLabelAction("", 0, 0, 0, actionCollection(), "fullscreen_label");
  
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
	// Actions in image view
	{
		KActionPtrList actions;
		actions
			<< mToggleFullScreen
			<< mToggleSlideShow
			<< mFileViewController->selectPrevious()
			<< mFileViewController->selectNext()
			<< mRotateLeft
			<< mRotateRight
			<< mFullScreenLabelAction
			;
		mImageViewController->setFullScreenCommonActions(actions);
	}
	
	{
		KActionPtrList actions;
		actions
			<< mFileViewController->selectPrevious()
			<< mFileViewController->selectNext()
			<< mReload
			;
		mImageViewController->setNormalCommonActions(actions);
	}

	{
		KActionPtrList actions;
		actions
			<< actionCollection()->action("view_zoom_in")
			<< actionCollection()->action("view_zoom_to")
			<< actionCollection()->action("view_zoom_out")
			<< mRotateLeft
			<< mRotateRight
			;
		mImageViewController->setImageViewActions(actions);
	}

	// Make sure file actions are correctly updated
	connect(mFileViewController, SIGNAL(selectionChanged()),
		this, SLOT(updateImageActions()) );

	connect(mFileViewController, SIGNAL(requestContextMenu(const QPoint&, bool)),
		this, SLOT(openFileViewControllerContextMenu(const QPoint&, bool)) );

	// Bookmarks
	QString file = locate( "data", "kfile/bookmarks.xml" );
	if (file.isEmpty()) {
		file = locateLocal( "data", "kfile/bookmarks.xml" );
	}

	KBookmarkManager* manager=KBookmarkManager::managerForFile(file,false);
	manager->setUpdate(true);
	manager->setShowNSBookmarks(false);
	mBookmarkViewController->init(manager);

	BookmarkOwner* bookmarkOwner=new BookmarkOwner(this);

	KActionMenu* bookmark=new KActionMenu(i18n( "&Bookmarks" ), "bookmark", actionCollection(), "bookmarks" );
	new KBookmarkMenu(manager, bookmarkOwner, bookmark->popupMenu(), 0, true);

	connect(bookmarkOwner,SIGNAL(openURL(const KURL&)),
		mFileViewController,SLOT(setDirURL(const KURL&)) );

	connect(mFileViewController,SIGNAL(directoryChanged(const KURL&)),
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
	createHideShowAction(mImageDock);
	createHideShowAction(mMetaDock);
	plugActionList("winlist", mWindowListActions);
}


void MainWindow::createConnections() {
	connect(mGoUp->popupMenu(), SIGNAL(aboutToShow()),
		this,SLOT(fillGoUpMenu()));
	
	connect(mGoUp->popupMenu(), SIGNAL(activated(int)),
		this,SLOT(goUpTo(int)));
	
	// Slideshow connections
	connect( mSlideShow, SIGNAL(nextURL(const KURL&)),
		SLOT( openURL(const KURL&)) );
	connect( mSlideShow, SIGNAL( stateChanged(bool)),
		SLOT( slotSlideShowChanged(bool)) );

	// Dir view connections
	connect(mDirViewController, SIGNAL(urlChanged(const KURL&)),
		mFileViewController, SLOT(setDirURL(const KURL&)) );
	connect(mDirViewController, SIGNAL(urlRenamed(const KURL&, const KURL&)),
		this, SLOT(slotDirRenamed(const KURL&, const KURL&)) );

	// Bookmark view connections
	connect(mBookmarkViewController, SIGNAL(openURL(const KURL&)),
		mFileViewController,SLOT(setDirURL(const KURL&)) );
	connect(mFileViewController, SIGNAL(directoryChanged(const KURL&)),
		mBookmarkViewController, SLOT(setURL(const KURL&)) );

	// Pixmap view connections
	connect(mImageViewController, SIGNAL(selectPrevious()),
		mFileViewController, SLOT(slotSelectPrevious()) );
	connect(mImageViewController, SIGNAL(selectNext()),
		mFileViewController, SLOT(slotSelectNext()) );
	connect(mImageViewController, SIGNAL(doubleClicked()),
		mToggleFullScreen, SLOT(activate()) );

	// File view connections
	connect(mFileViewController,SIGNAL(urlChanged(const KURL&)),
		mDocument,SLOT(setURL(const KURL&)) );
	connect(mFileViewController,SIGNAL(directoryChanged(const KURL&)),
		this,SLOT(slotDirURLChanged(const KURL&)) );
	connect(mFileViewController,SIGNAL(directoryChanged(const KURL&)),
		mDirViewController,SLOT(setURL(const KURL&)) );
	connect(mFileViewController,SIGNAL(directoryChanged(const KURL&)),
		mHistory,SLOT(addURLToHistory(const KURL&)) );

	connect(mFileViewController,SIGNAL(completed()),
		this,SLOT(updateStatusInfo()) );
	connect(mFileViewController,SIGNAL(canceled()),
		this,SLOT(updateStatusInfo()) );
	connect(mFileViewController,SIGNAL(imageDoubleClicked()),
		mToggleFullScreen,SLOT(activate()) );
	connect(mFileViewController,SIGNAL(shownFileItemRefreshed(const KFileItem*)),
		this,SLOT(slotShownFileItemRefreshed(const KFileItem*)) );
	connect(mFileViewController,SIGNAL(sortingChanged()),
		this, SLOT(updateStatusInfo()) );

	// History connections
	connect(mHistory, SIGNAL(urlChanged(const KURL&)),
		mFileViewController, SLOT(setDirURL(const KURL&)) );
	
	// Document connections
	connect(mDocument,SIGNAL(loading()),
		this,SLOT(slotImageLoading()) );
	connect(mDocument,SIGNAL(loaded(const KURL&)),
		this,SLOT(slotImageLoaded()) );
	connect(mDocument,SIGNAL(saved(const KURL&)),
		mFileViewController,SLOT(updateThumbnail(const KURL&)) );
	connect(mDocument,SIGNAL(reloaded(const KURL&)),
		mFileViewController,SLOT(updateThumbnail(const KURL&)) );

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

	// Plugin menu
	QPopupMenu *popup = static_cast<QPopupMenu*>(
		factory()->container( "plugins", this));
    connect(popup, SIGNAL(aboutToShow()), this, SLOT(loadPlugins()) );
}


void MainWindow::createLocationToolBar() {
	// URL Combo
	mURLEdit=new KHistoryCombo(this);
	mURLEdit->setDuplicatesEnabled(false);
	mURLEdit->setPixmapProvider(new KURLPixmapProvider);
	mURLEdit->setHistoryItems(MiscConfig::history());
	
	// Avoid stealing focus
	mURLEdit->setFocusPolicy(ClickFocus);

	mURLEditCompletion=new KURLCompletion();

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
	if (mPluginLoader) return;

	LOG("Load plugins");
	// Sets up the plugin interface, and load the plugins
	KIPIInterface* interface = new KIPIInterface(this, mFileViewController);
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
	// Create a dummy "no KIPI" action list
	KAction* noPlugin=new KAction(i18n("No KIPI support"), 0, 0, 0, actionCollection(), "no_plugin");
	noPlugin->setShortcutConfigurable(false);
	noPlugin->setEnabled(false);
	QPtrList<KAction> noPluginList;
	noPluginList.append(noPlugin);

	QStringList lst;
	lst	<< "image_actions"
		<< "effect_actions"
		<< "tool_actions"
		<< "import_actions"
		<< "export_actions"
		<< "batch_actions"
		<< "collection_actions";
	
	// Fill the menu
	QStringList::ConstIterator catIt=lst.begin(), catItEnd=lst.end();
	for (; catIt!=catItEnd; ++catIt) {
		plugActionList(*catIt, noPluginList);
	}
}

void MainWindow::slotReplug() {
}
#endif


} // namespace
