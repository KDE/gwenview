/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "mainwindow.moc"

// Qt
#include <QApplication>
#include <QDesktopWidget>
#include <QLabel>
#include <QTimer>
#include <QShortcut>
#include <QSplitter>
#include <QSlider>
#include <QStackedWidget>
#include <QVBoxLayout>

// KDE
#include <kactioncollection.h>
#include <kaction.h>
#include <kapplication.h>
#include <kde_file.h>
#include <kdirlister.h>
#include <kedittoolbar.h>
#include <kfiledialog.h>
#include <kfileitem.h>
#include <kfileplacesmodel.h>
#include <kimageio.h>
#include <kinputdialog.h>
#include <kio/netaccess.h>
#include <kmenubar.h>
#include <kmountpoint.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprotocolmanager.h>
#include <kstatusbar.h>
#include <ktogglefullscreenaction.h>
#include <ktoolbar.h>
#include <kurl.h>
#include <kurlnavigator.h>
#include <kxmlguifactory.h>
#include <kwindowsystem.h>

// Local
#include "configdialog.h"
#include "contextmanager.h"
#include "documentview.h"
#include "fileopscontextmanageritem.h"
#include "imageopscontextmanageritem.h"
#include "infocontextmanageritem.h"
#include "savebar.h"
#include "sidebar.h"
#include "thumbnailviewhelper.h"
#include <lib/archiveutils.h>
#include <lib/cropsidebar.h>
#include <lib/document/documentfactory.h>
#include <lib/fullscreenbar.h>
#include <lib/imagemetainfomodel.h>
#include <lib/gwenviewconfig.h>
#include <lib/imageviewpart.h>
#include <lib/mimetypeutils.h>
#include <lib/print/printhelper.h>
#include <lib/resizeimageoperation.h>
#include <lib/slideshow.h>
#include <lib/sorteddirmodel.h>
#include <lib/thumbnailview.h>
#include <lib/transformimageoperation.h>

namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << x
#else
#define LOG(x) ;
#endif


static const int PRELOAD_DELAY = 1000;

static const char* MAINWINDOW_SETTINGS = "MainWindow";

static bool urlIsDirectory(QWidget* parent, const KUrl& url) {
	if( url.fileName(KUrl::ObeyTrailingSlash).isEmpty()) {
		return true; // file:/somewhere/<nothing here>
	}

	// Do direct stat instead of using KIO if the file is local (faster)
	KMountPoint::List mpl = KMountPoint::currentMountPoints();
	KMountPoint::Ptr mp = mpl.findByPath( url.path() );

	if( url.isLocalFile() && !mp->probablySlow()) {
		KDE_struct_stat buff;
		if ( KDE_stat( QFile::encodeName(url.path()), &buff ) == 0 )  {
			return S_ISDIR( buff.st_mode );
		}
	}
	KIO::UDSEntry entry;
	if( KIO::NetAccess::stat( url, entry, parent)) {
		return entry.isDir();
	}
	return false;
}

struct MainWindowState {
	QAction* mActiveViewModeAction;
	bool mSideBarVisible;
	Qt::WindowStates mWindowState;
};

struct MainWindow::Private {
	MainWindow* mWindow;
	QSplitter* mCentralSplitter;
	DocumentView* mDocumentView;
	KUrlNavigator* mUrlNavigator;
	ThumbnailView* mThumbnailView;
	ThumbnailViewHelper* mThumbnailViewHelper;
	QSlider* mThumbnailSlider;
	QWidget* mThumbnailViewPanel;
	SideBar* mSideBar;
	QStackedWidget* mSideBarContainer;
	bool mSideBarWasVisibleBeforeTemporarySideBar;
	FullScreenBar* mFullScreenBar;
	SaveBar* mSaveBar;
	SlideShow* mSlideShow;

	QActionGroup* mViewModeActionGroup;
	QAction* mBrowseAction;
	QAction* mPreviewAction;
	QAction* mViewAction;
	QAction* mGoUpAction;
	QAction* mGoToPreviousAction;
	QAction* mGoToNextAction;
	QAction* mRotateLeftAction;
	QAction* mRotateRightAction;
	QAction* mMirrorAction;
	QAction* mFlipAction;
	QAction* mResizeAction;
	QAction* mCropAction;
	QAction* mToggleSideBarAction;
	KToggleFullScreenAction* mFullScreenAction;
	QAction* mToggleSlideShowAction;
	KToggleAction* mShowMenuBarAction;

	SortedDirModel* mDirModel;
	ContextManager* mContextManager;

	MainWindowState mStateBeforeFullScreen;

	KUrl mUrlToSelect;

	QString mCaption;

	QLabel *mInformationLabel;

	void setupWidgets() {
		QWidget* centralWidget = new QWidget(mWindow);
		mWindow->setCentralWidget(centralWidget);
		mSaveBar = new SaveBar(centralWidget);

		mCentralSplitter = new QSplitter(Qt::Horizontal, centralWidget);
		QVBoxLayout* layout = new QVBoxLayout(centralWidget);
		layout->addWidget(mSaveBar);
		layout->addWidget(mCentralSplitter);
		layout->setMargin(0);
		layout->setSpacing(0);

		setupThumbnailView(mCentralSplitter);
		mDocumentView = new DocumentView(mCentralSplitter);
		connect(mDocumentView, SIGNAL(completed()),
			mWindow, SLOT(slotPartCompleted()) );
		connect(mDocumentView, SIGNAL(partChanged(KParts::Part*)),
			mWindow, SLOT(createGUI(KParts::Part*)) );

		mSideBarContainer = new QStackedWidget(mCentralSplitter);
		mSideBar = new SideBar(mSideBarContainer);
		mSideBarContainer->addWidget(mSideBar);

		mSlideShow = new SlideShow(mWindow);

		mInformationLabel = new QLabel();

		connect(mSaveBar, SIGNAL(requestSave(const KUrl&)),
			mWindow, SLOT(save(const KUrl&)) );
		connect(mSaveBar, SIGNAL(goToUrl(const KUrl&)),
			mWindow, SLOT(goToUrl(const KUrl&)) );

		connect(mSlideShow, SIGNAL(goToUrl(const KUrl&)),
			mWindow, SLOT(goToUrl(const KUrl&)) );
	}

	void setupThumbnailView(QWidget* parent) {
		mThumbnailViewPanel = new QWidget(parent);

		// mThumbnailView
		mThumbnailView = new ThumbnailView(mThumbnailViewPanel);
		mThumbnailView->setModel(mDirModel);
		mThumbnailViewHelper = new ThumbnailViewHelper(mDirModel);
		mThumbnailView->setThumbnailViewHelper(mThumbnailViewHelper);
		mThumbnailView->setSelectionMode(QAbstractItemView::ExtendedSelection);
		connect(mThumbnailView, SIGNAL(indexActivated(const QModelIndex&)),
			mWindow, SLOT(openDirUrlFromIndex(const QModelIndex&)) );
		connect(mThumbnailView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
			mWindow, SLOT(slotSelectionChanged()) );

		connect(mThumbnailView, SIGNAL(saveDocumentRequested(const KUrl&)),
			mWindow, SLOT(save(const KUrl&)) );
		connect(mThumbnailView, SIGNAL(rotateDocumentLeftRequested(const KUrl&)),
			mWindow, SLOT(rotateLeft(const KUrl&)) );
		connect(mThumbnailView, SIGNAL(rotateDocumentRightRequested(const KUrl&)),
			mWindow, SLOT(rotateRight(const KUrl&)) );
		connect(mThumbnailView, SIGNAL(showDocumentInFullScreenRequested(const KUrl&)),
			mWindow, SLOT(showDocumentInFullScreen(const KUrl&)) );

		// mUrlNavigator
		KFilePlacesModel* places = new KFilePlacesModel(mThumbnailViewPanel);
		mUrlNavigator = new KUrlNavigator(places, KUrl(), mThumbnailViewPanel);
		connect(mUrlNavigator, SIGNAL(urlChanged(const KUrl&)),
			mWindow, SLOT(openDirUrl(const KUrl&)) );

		// Thumbnail slider
		KStatusBar* statusBar = new KStatusBar(mThumbnailViewPanel);
		mThumbnailSlider = new QSlider(statusBar);
		mThumbnailSlider->setMaximumWidth(200);
		statusBar->addPermanentWidget(mThumbnailSlider);
		mThumbnailSlider->setMinimum(40);
		mThumbnailSlider->setMaximum(256);
		mThumbnailSlider->setOrientation(Qt::Horizontal);
		connect(mThumbnailSlider, SIGNAL(valueChanged(int)), mThumbnailView, SLOT(setThumbnailSize(int)) );

		// Layout
		QVBoxLayout* layout = new QVBoxLayout(mThumbnailViewPanel);
		layout->setSpacing(0);
		layout->setMargin(0);
		layout->addWidget(mUrlNavigator);
		layout->addWidget(mThumbnailView);
		layout->addWidget(statusBar);
	}

	void setupActions() {
		KActionCollection* actionCollection = mWindow->actionCollection();

		KStandardAction::save(mWindow, SLOT(saveCurrent()), actionCollection);
		KStandardAction::saveAs(mWindow, SLOT(saveCurrentAs()), actionCollection);
		KStandardAction::print(mWindow, SLOT(print()), actionCollection);
		KStandardAction::quit(KApplication::kApplication(), SLOT(closeAllWindows()), actionCollection);

		QAction* action = actionCollection->addAction("reload");
		action->setText(i18nc("@action reload the currently viewed image", "Reload"));
		action->setIcon(KIcon("view-refresh"));
		connect(action, SIGNAL(triggered()),
			mWindow, SLOT(reload()) );

		mBrowseAction = actionCollection->addAction("browse");
		mBrowseAction->setText(i18n("Browse"));
		mBrowseAction->setCheckable(true);
		mBrowseAction->setIcon(KIcon("view-list-icons"));

		mPreviewAction = actionCollection->addAction("preview");
		mPreviewAction->setText(i18n("Preview"));
		mPreviewAction->setIcon(KIcon("view-list-details"));
		mPreviewAction->setCheckable(true);

		mViewAction = actionCollection->addAction("view");
		mViewAction->setText(i18n("View"));
		mViewAction->setIcon(KIcon("view-preview"));
		mViewAction->setCheckable(true);

		mViewModeActionGroup = new QActionGroup(mWindow);
		mViewModeActionGroup->addAction(mBrowseAction);
		mViewModeActionGroup->addAction(mPreviewAction);
		mViewModeActionGroup->addAction(mViewAction);

		connect(mViewModeActionGroup, SIGNAL(triggered(QAction*)),
			mWindow, SLOT(setActiveViewModeAction(QAction*)) );

		mFullScreenAction = KStandardAction::fullScreen(mWindow, SLOT(toggleFullScreen()), mWindow, actionCollection);

		QShortcut* exitFullScreenShortcut = new QShortcut(mWindow);
		exitFullScreenShortcut->setKey(Qt::Key_Escape);
		connect(exitFullScreenShortcut, SIGNAL(activated()),
			mWindow, SLOT(exitFullScreen()) );

		mGoToPreviousAction = KStandardAction::prior(mWindow, SLOT(goToPrevious()), actionCollection);
		mGoToPreviousAction->setText(i18nc("@action Go to previous image", "Previous"));
		mGoToPreviousAction->setToolTip(i18n("Go to Previous Image"));
		mGoToPreviousAction->setShortcut(Qt::Key_Backspace);

		mGoToNextAction = KStandardAction::next(mWindow, SLOT(goToNext()), actionCollection);
		mGoToNextAction->setText(i18nc("@action Go to next image", "Next"));
		mGoToNextAction->setToolTip(i18n("Go to Next Image"));
		mGoToNextAction->setShortcut(Qt::Key_Space);

		mGoUpAction = KStandardAction::up(mWindow, SLOT(goUp()), actionCollection);

		mRotateLeftAction = actionCollection->addAction("rotate_left");
		mRotateLeftAction->setText(i18n("Rotate Left"));
		mRotateLeftAction->setIcon(KIcon("object-rotate-left"));
		mRotateLeftAction->setShortcut(Qt::CTRL + Qt::Key_L);
		connect(mRotateLeftAction, SIGNAL(triggered()),
			mWindow, SLOT(rotateCurrentLeft()) );

		mRotateRightAction = actionCollection->addAction("rotate_right");
		mRotateRightAction->setText(i18n("Rotate Right"));
		mRotateRightAction->setIcon(KIcon("object-rotate-right"));
		mRotateRightAction->setShortcut(Qt::CTRL + Qt::Key_R);
		connect(mRotateRightAction, SIGNAL(triggered()),
			mWindow, SLOT(rotateCurrentRight()) );

		mMirrorAction = actionCollection->addAction("mirror");
		mMirrorAction->setText(i18n("Mirror"));
		connect(mMirrorAction, SIGNAL(triggered()),
			mWindow, SLOT(mirror()) );

		mFlipAction = actionCollection->addAction("flip");
		mFlipAction->setText(i18n("Flip"));
		connect(mFlipAction, SIGNAL(triggered()),
			mWindow, SLOT(flip()) );

		mResizeAction = actionCollection->addAction("resize");
		mResizeAction->setText(i18n("Resize"));
		connect(mResizeAction, SIGNAL(triggered()),
			mWindow, SLOT(resizeImage()) );

		mCropAction = actionCollection->addAction("crop");
		mCropAction->setText(i18n("Crop"));
		connect(mCropAction, SIGNAL(triggered()),
			mWindow, SLOT(crop()) );

		mToggleSideBarAction = actionCollection->addAction("toggle_sidebar");
		mToggleSideBarAction->setIcon(KIcon("view-sidetree"));
		mToggleSideBarAction->setShortcut(Qt::Key_F11);
		connect(mToggleSideBarAction, SIGNAL(triggered()),
			mWindow, SLOT(toggleSideBar()) );

		mToggleSlideShowAction = actionCollection->addAction("toggle_slideshow");
		mWindow->updateSlideShowAction();
		connect(mToggleSlideShowAction, SIGNAL(triggered()),
			mWindow, SLOT(toggleSlideShow()) );
		connect(mSlideShow, SIGNAL(stateChanged(bool)),
			mWindow, SLOT(updateSlideShowAction()) );

		mShowMenuBarAction = KStandardAction::showMenubar(mWindow, SLOT(toggleMenuBar()), actionCollection);

		KStandardAction::keyBindings(mWindow->guiFactory(),
			SLOT(configureShortcuts()), actionCollection);

		KStandardAction::preferences(mWindow,
			SLOT(showConfigDialog()), actionCollection);

		KStandardAction::configureToolbars(mWindow,
			SLOT(configureToolbars()), actionCollection);

		KAction* editLocationAction = actionCollection->addAction("edit_location");
		editLocationAction->setText(i18nc("@action:inmenu Navigation Bar", "Edit Location"));
		editLocationAction->setShortcut(Qt::Key_F6);
		connect(editLocationAction, SIGNAL(triggered()),
			mWindow, SLOT(editLocation()));
	}


	void setupContextManager() {
		mContextManager = new ContextManager(mWindow);
		mContextManager->setSideBar(mSideBar);

		mContextManager->addItem(new InfoContextManagerItem(mContextManager));

		QList<QAction*> actionList;
		actionList << mRotateLeftAction << mRotateRightAction << mMirrorAction << mFlipAction << mResizeAction << mCropAction;
		mContextManager->addItem(new ImageOpsContextManagerItem(mContextManager, actionList));

		FileOpsContextManagerItem* fileOpsItem = new FileOpsContextManagerItem(mContextManager);
		mContextManager->addItem(fileOpsItem);
		mThumbnailViewHelper->setFileOpsContextManagerItem(fileOpsItem);
	}


	void initDirModel() {
		KDirLister* dirLister = mDirModel->dirLister();
		QStringList mimeTypes;
		mimeTypes += MimeTypeUtils::dirMimeTypes();
		mimeTypes += MimeTypeUtils::imageMimeTypes();
		mimeTypes += MimeTypeUtils::videoMimeTypes();
		dirLister->setMimeFilter(mimeTypes);

		connect(mDirModel, SIGNAL(rowsInserted(const QModelIndex&, int, int)),
			mWindow, SLOT(slotDirModelNewItems()) );

		connect(dirLister, SIGNAL(deleteItem(const KFileItem&)),
			mWindow, SLOT(updatePreviousNextActions()) );

		connect(dirLister, SIGNAL(completed()),
			mWindow, SLOT(slotDirListerCompleted()) );
	}

	void updateToggleSideBarAction() {
		if (mSideBarContainer->isVisible()) {
			mToggleSideBarAction->setText(i18n("Hide Sidebar"));
		} else {
			mToggleSideBarAction->setText(i18n("Show Sidebar"));
		}
	}

	QModelIndex getRelativeIndex(int offset) {
		KUrl url = currentUrl();
		QModelIndex index = mDirModel->indexForUrl(url);
		int row = index.row() + offset;
		index = mDirModel->index(row, 0);
		if (!index.isValid()) {
			return QModelIndex();
		}

		KFileItem item = mDirModel->itemForIndex(index);
		if (!ArchiveUtils::fileItemIsDirOrArchive(item)) {
			return index;
		} else {
			return QModelIndex();
		}
	}

	void goTo(int offset) {
		QModelIndex index = getRelativeIndex(offset);
		if (index.isValid()) {
			mThumbnailView->setCurrentIndex(index);
			mThumbnailView->scrollTo(index);
		}
	}

	void goToFirstDocument() {
		QModelIndex index;
		for(int row=0;; ++row) {
			index = mDirModel->index(row, 0);
			if (!index.isValid()) {
				return;
			}

			KFileItem item = mDirModel->itemForIndex(index);
			if (!ArchiveUtils::fileItemIsDirOrArchive(item)) {
				break;
			}
		}
		Q_ASSERT(index.isValid());
		mThumbnailView->setCurrentIndex(index);
	}

	void spreadCurrentDirUrl(const KUrl& url) {
		mContextManager->setCurrentDirUrl(url);
		mThumbnailViewHelper->setCurrentDirUrl(url);
		mUrlNavigator->setUrl(url);
		mGoUpAction->setEnabled(url.path() != "/");
	}

	void createFullScreenBar() {
		mFullScreenBar = new FullScreenBar(mDocumentView);
		mFullScreenBar->addAction(mFullScreenAction);
		mFullScreenBar->addAction(mGoToPreviousAction);
		mFullScreenBar->addAction(mGoToNextAction);

		mFullScreenBar->addSeparator();
		mFullScreenBar->addAction(mToggleSlideShowAction);
		mFullScreenBar->addWidget(mSlideShow->intervalWidget());
		mFullScreenBar->addWidget(mSlideShow->optionsWidget());

		mFullScreenBar->addWidget(mInformationLabel);

		mFullScreenBar->resize(mFullScreenBar->sizeHint());
	}

	bool currentDocumentIsRasterImage() {
		if (mDocumentView->isVisible()) {
			return mDocumentView->currentDocumentIsRasterImage();
		} else {
			QModelIndex index = mThumbnailView->currentIndex();
			if (!index.isValid()) {
				return false;
			}
			KFileItem item = mDirModel->itemForIndex(index);
			Q_ASSERT(!item.isNull());
			return MimeTypeUtils::fileItemKind(item) == MimeTypeUtils::KIND_RASTER_IMAGE;
		}
	}

	void updateActions() {
		bool canModify = currentDocumentIsRasterImage();
		if (!mDocumentView->isVisible()) {
			// Since we only support image operations on one image for now,
			// disable actions if several images are selected and the document
			// view is not visible.
			QItemSelection selection = mThumbnailView->selectionModel()->selection();
			QModelIndexList indexList = selection.indexes();
			if (indexList.count() != 1) {
				canModify = false;
			}
		}

		KActionCollection* actionCollection = mWindow->actionCollection();
		actionCollection->action("file_save")->setEnabled(canModify);
		actionCollection->action("file_save_as")->setEnabled(canModify);
		mRotateLeftAction->setEnabled(canModify);
		mRotateRightAction->setEnabled(canModify);
		mMirrorAction->setEnabled(canModify);
		mFlipAction->setEnabled(canModify);
		mResizeAction->setEnabled(canModify);
		mCropAction->setEnabled(canModify && mDocumentView->isVisible());
	}

	KUrl currentUrl() const {
		if (mDocumentView->isVisible() && !mDocumentView->isEmpty()) {
			return mDocumentView->url();
		} else {
			QModelIndex index = mThumbnailView->currentIndex();
			if (!index.isValid()) {
				return KUrl();
			}
			KFileItem item = mDirModel->itemForIndex(index);
			Q_ASSERT(!item.isNull());
			return item.url();
		}
	}

	QString currentMimeType() const {
		if (mDocumentView->isVisible() && !mDocumentView->isEmpty()) {
			return MimeTypeUtils::urlMimeType(mDocumentView->url());
		} else {
			QModelIndex index = mThumbnailView->currentIndex();
			if (!index.isValid()) {
				return QString();
			}
			KFileItem item = mDirModel->itemForIndex(index);
			Q_ASSERT(!item.isNull());
			return item.mimetype();
		}
	}

	void applyImageOperation(AbstractImageOperation* op) {
		// For now, we only support operations on one image
		KUrl url = currentUrl();

		Document::Ptr doc = DocumentFactory::instance()->load(url);
		doc->waitUntilLoaded();
		op->apply(doc);
	}


	void spreadCurrentUrl() {
		KUrl url = currentUrl();
		mSaveBar->setCurrentUrl(url);
		mSlideShow->setCurrentUrl(url);
	}

	void selectUrlToSelect() {
		if (!mUrlToSelect.isValid()) {
			return;
		}
		QModelIndex index = mDirModel->indexForUrl(mUrlToSelect);
		if (index.isValid()) {
			mThumbnailView->setCurrentIndex(index);
			mUrlToSelect = KUrl();
			spreadCurrentUrl();
		}
	}

	/**
	 * Compute a width which ensures the "next" button is always visible
	 */
	int computeMinimumWidth() {
		QWidget* widget = mWindow->toolBar()->widgetForAction(mGoToNextAction);
		int width;
		if (widget) {
			QPoint pos = widget->rect().topRight();
			pos = widget->mapTo(mWindow, pos);
			width = pos.x();
			width += mWindow->style()->pixelMetric(QStyle::PM_ToolBarHandleExtent);

			// Add a few more pixels to prevent the "next" button from going
			// in the toolbar extent.
			width += 10;
		} else {
			width = mWindow->menuBar()->width();
		}
		return width;
	}


	void showTemporarySideBar(QWidget* sideBar) {
		mSideBarWasVisibleBeforeTemporarySideBar = mSideBarContainer->isVisible();
		// Move the sideBar inside a widget so that we can add a stretch below
		// it.
		QWidget* widget = new QWidget(mSideBarContainer);
		sideBar->setParent(widget);

		QVBoxLayout* layout = new QVBoxLayout(widget);
		layout->setMargin(0);
		layout->setSpacing(0);
		layout->addWidget(sideBar);
		layout->addStretch();

		mSideBarContainer->addWidget(widget);
		mSideBarContainer->setCurrentWidget(widget);
		mSideBarContainer->show();
	}


	void loadMainWindowConfig() {
		KConfigGroup cg(KGlobal::config(), MAINWINDOW_SETTINGS);
		mWindow->applyMainWindowSettings(cg);
	}


	void saveMainWindowConfig() {
		KConfigGroup cg(KGlobal::config(), MAINWINDOW_SETTINGS);
		mWindow->saveMainWindowSettings(cg);
		KGlobal::config()->sync();
	}
};


MainWindow::MainWindow()
: KParts::MainWindow(),
d(new MainWindow::Private)
{
	d->mWindow = this;
	d->mDirModel = new SortedDirModel(this);
	d->mFullScreenBar = 0;
	d->initDirModel();
	d->setupWidgets();
	d->setupActions();
	d->setupContextManager();
	d->updateActions();
	updatePreviousNextActions();

	createShellGUI();
	loadConfig();
	d->loadMainWindowConfig();
	connect(DocumentFactory::instance(), SIGNAL(documentChanged(const KUrl&)),
		SLOT(generateThumbnailForUrl(const KUrl&)) );

	connect(DocumentFactory::instance(), SIGNAL(modifiedDocumentListChanged()),
		SLOT(updateModifiedFlag()) );
}


void MainWindow::setCaption(const QString& caption) {
	// Keep a trace of caption to use it in updateModifiedFlag()
	d->mCaption = caption;
	KParts::MainWindow::setCaption(caption);
}

void MainWindow::setCaption(const QString& caption, bool modified) {
	d->mCaption = caption;
	KParts::MainWindow::setCaption(caption, modified);
}


void MainWindow::updateModifiedFlag() {
	QList<KUrl> list = DocumentFactory::instance()->modifiedDocumentList();
	bool modified = list.count() > 0;
	setCaption(d->mCaption, modified);
}


void MainWindow::setInitialUrl(const KUrl& url) {
	if (urlIsDirectory(this, url)) {
		d->mBrowseAction->trigger();
		openDirUrl(url);
	} else {
		d->mViewAction->trigger();
		d->mSideBarContainer->hide();
		// Resize the window once image is loaded
		connect(d->mDocumentView, SIGNAL(resizeRequested(const QSize&)),
			d->mWindow, SLOT(handleResizeRequest(const QSize&)) );
		openDocumentUrl(url);
	}
	d->updateToggleSideBarAction();
}


void MainWindow::setActiveViewModeAction(QAction* action) {
	bool showDocument, showThumbnail;
	if (action == d->mBrowseAction) {
		showDocument = false;
		showThumbnail = true;
	} else if (action == d->mPreviewAction) {
		showDocument = true;
		showThumbnail = true;
	} else { // image only
		showDocument = true;
		showThumbnail = false;
	}

	// Adjust splitter policy. Thumbnail should only stretch if there is no
	// document view.
	d->mCentralSplitter->setStretchFactor(0, showDocument ? 0 : 1); // thumbnail
	d->mCentralSplitter->setStretchFactor(1, 1); // image
	d->mCentralSplitter->setStretchFactor(2, 0); // sidebar

	d->mDocumentView->setVisible(showDocument);
	if (showDocument && d->mDocumentView->isEmpty()) {
		openSelectedDocument();
	} else if (!showDocument
		&& !d->mDocumentView->isEmpty()
		&& KProtocolManager::supportsListing(d->mDocumentView->url()))
	{
		// Reset the view to spare resources, but don't do it if we can't
		// browse the url, otherwise if the user starts Gwenview this way:
		// gwenview http://example.com/example.png
		// and switch to browse mode, switching back to view mode won't bring
		// his image back.
		d->mDocumentView->reset();
	}
	d->mThumbnailViewPanel->setVisible(showThumbnail);

	// Update actions and context because some context actions depends on the
	// view mode, for example: cropping should only be enabled when the
	// document view is visible.
	d->updateActions();
	updateContextManager();
}


void MainWindow::openDirUrlFromIndex(const QModelIndex& index) {
	if (!index.isValid()) {
		return;
	}

	KFileItem item = d->mDirModel->itemForIndex(index);
	if (item.isDir()) {
		openDirUrl(item.url());
	} else if (ArchiveUtils::fileItemIsArchive(item)) {
		KUrl url = item.url();
		QString protocol = ArchiveUtils::protocolForMimeType(item.mimetype());
		url.setProtocol(protocol);
		openDirUrl(url);
	}
}


void MainWindow::openSelectedDocument() {
	if (!d->mDocumentView->isVisible()) {
		return;
	}

	QItemSelection selection = d->mThumbnailView->selectionModel()->selection();
	if (selection.size() == 0) {
		return;
	}

	QModelIndex index = selection.indexes()[0];
	if (!index.isValid()) {
		return;
	}

	KFileItem item = d->mDirModel->itemForIndex(index);
	if (!ArchiveUtils::fileItemIsDirOrArchive(item)) {
		openDocumentUrl(item.url());
	}
}


void MainWindow::goUp() {
	KUrl url = d->mDirModel->dirLister()->url();
	url = url.upUrl();
	openDirUrl(url);
}


void MainWindow::openDirUrl(const KUrl& url) {
	if (url.equals(d->mDirModel->dirLister()->url(), KUrl::CompareWithoutTrailingSlash)) {
		return;
	}
	d->mDirModel->dirLister()->openUrl(url);
	d->spreadCurrentDirUrl(url);
	d->mDocumentView->reset();
}


void MainWindow::openDocumentUrl(const KUrl& url) {
	if (d->mDocumentView->url() == url) {
		return;
	}
	if (!d->mDocumentView->openUrl(url)) {
		return;
	}

	Document::Ptr doc = DocumentFactory::instance()->load(url);
	connect(doc.data(),SIGNAL(metaDataUpdated()),
		SLOT(updateFullScreenInformation()) );
	updateFullScreenInformation();

	d->mUrlToSelect = url;
	d->selectUrlToSelect();
}

void MainWindow::updateFullScreenInformation() {
	if (!d->mFullScreenAction->isChecked()) {
		return;
	}

	KUrl url = d->currentUrl();

	Document::Ptr doc = DocumentFactory::instance()->load(url);

	ImageMetaInfoModel *imageMetaInfo;
	imageMetaInfo = doc->metaInfo();

	QString aperture, exposureTime, iso, focalLength;
	QString filename;

	aperture = imageMetaInfo->getValueForKey("Exif.Photo.FNumber");
	exposureTime = imageMetaInfo->getValueForKey("Exif.Photo.ExposureTime");
	iso = imageMetaInfo->getValueForKey("Exif.Photo.ISOSpeedRatings");
	focalLength = imageMetaInfo->getValueForKey("Exif.Photo.FocalLength");

	filename = imageMetaInfo->getValueForKey("General.Name");

	QString info = GwenviewConfig::fullScreenInfo();
	info.replace("%a", aperture);
	info.replace("%t", exposureTime);
	info.replace("%i", iso);
	info.replace("%l", focalLength);
	info.replace("%f", filename);

	d->mInformationLabel->setText(info);

	if (d->mFullScreenBar) {
		d->mFullScreenBar->resize(d->mFullScreenBar->sizeHint());
	}
}

void MainWindow::slotSetStatusBarText(const QString& message) {
	d->mDocumentView->statusBar()->showMessage(message);
}

void MainWindow::toggleSideBar() {
	d->mSideBarContainer->setVisible(!d->mSideBarContainer->isVisible());
	d->updateToggleSideBarAction();
}


void MainWindow::updateContextManager() {
	QItemSelection selection = d->mThumbnailView->selectionModel()->selection();
	QModelIndexList indexList = selection.indexes();

	KFileItemList itemList;
	Q_FOREACH(const QModelIndex& index, indexList) {
		itemList << d->mDirModel->itemForIndex(index);
	}

	d->mContextManager->setSelection(itemList);
}

void MainWindow::slotPartCompleted() {
	Q_ASSERT(!d->mDocumentView->isEmpty());
	KUrl url = d->mDocumentView->url();
	if (!KProtocolManager::supportsListing(url)) {
		return;
	}

	KUrl dirUrl = url;
	dirUrl.setFileName("");
	if (dirUrl.equals(d->mDirModel->dirLister()->url(), KUrl::CompareWithoutTrailingSlash)) {
		QModelIndex index = d->mDirModel->indexForUrl(url);
		if (index.isValid()) {
			d->mThumbnailView->selectionModel()->select(index, QItemSelectionModel::SelectCurrent);
		}
	} else {
		d->mDirModel->dirLister()->openUrl(dirUrl);
		d->spreadCurrentDirUrl(dirUrl);
	}
}


void MainWindow::slotSelectionChanged() {
	openSelectedDocument();
	hideTemporarySideBar();
	d->updateActions();
	updatePreviousNextActions();
	updateContextManager();
	d->spreadCurrentUrl();
	QTimer::singleShot(PRELOAD_DELAY, this, SLOT(preloadNextUrl()) );
}


void MainWindow::slotDirModelNewItems() {
	if (d->mDocumentView->isEmpty()) {
		return;
	}

	QItemSelection selection = d->mThumbnailView->selectionModel()->selection();
	if (selection.size() > 0) {
		updatePreviousNextActions();
		return;
	}

	// Nothing selected in the view yet, check if there was an url waiting to
	// be selected
	d->selectUrlToSelect();
}


void MainWindow::slotDirListerCompleted() {
	if (!d->mDocumentView->isEmpty()) {
		return;
	}
	QItemSelection selection = d->mThumbnailView->selectionModel()->selection();
	if (selection.indexes().count() > 0) {
		return;
	}

	d->goToFirstDocument();
}


void MainWindow::goToPrevious() {
	d->goTo(-1);
}


void MainWindow::goToNext() {
	d->goTo(1);
}


void MainWindow::goToUrl(const KUrl& url) {
	if (d->mDocumentView->isVisible()) {
		openDocumentUrl(url);
		// No need to change thumbnail view, slotPartCompleted will do the work
		// for us
	} else {
		KUrl dirUrl = url;
		dirUrl.setFileName("");
		openDirUrl(dirUrl);
		d->mUrlToSelect = url;
		d->selectUrlToSelect();
	}
}


void MainWindow::updatePreviousNextActions() {
	QModelIndex index;

	index = d->getRelativeIndex(-1);
	d->mGoToPreviousAction->setEnabled(index.isValid());

	index = d->getRelativeIndex(1);
	d->mGoToNextAction->setEnabled(index.isValid());
}


void MainWindow::exitFullScreen() {
	if (d->mFullScreenAction->isChecked()) {
		d->mFullScreenAction->trigger();
	}
}


void MainWindow::toggleFullScreen() {
	setUpdatesEnabled(false);
	if (d->mFullScreenAction->isChecked()) {
		// Save MainWindow config now, this way if we quit while in
		// fullscreen, we are sure latest MainWindow changes are remembered.
		d->saveMainWindowConfig();

		// Go full screen
		d->mStateBeforeFullScreen.mActiveViewModeAction = d->mViewModeActionGroup->checkedAction();
		d->mStateBeforeFullScreen.mSideBarVisible = d->mSideBarContainer->isVisible();
		d->mStateBeforeFullScreen.mWindowState = windowState();

		d->mDocumentView->setViewBackgroundColor(Qt::black);

		d->mViewAction->trigger();
		d->mSideBarContainer->hide();

		setWindowState(windowState() | Qt::WindowFullScreen);
		menuBar()->hide();
		toolBar()->hide();
		d->mSaveBar->setForceHide(true);
		if (!d->mFullScreenBar) {
			d->createFullScreenBar();
		}
		d->mFullScreenBar->setActivated(true);
	} else {
		d->mStateBeforeFullScreen.mActiveViewModeAction->trigger();
		d->mSideBarContainer->setVisible(d->mStateBeforeFullScreen.mSideBarVisible);

		// Back to normal
		d->mDocumentView->setViewBackgroundColor(GwenviewConfig::viewBackgroundColor());
		d->mSlideShow->stop();
		d->mSaveBar->setForceHide(false);
		d->mFullScreenBar->setActivated(false);
		setWindowState(d->mStateBeforeFullScreen.mWindowState);
		menuBar()->setVisible(d->mShowMenuBarAction->isChecked());
		toolBar()->show();
	}
	setUpdatesEnabled(true);
}


void MainWindow::saveCurrent() {
	save(d->currentUrl());
}


void MainWindow::saveCurrentAs() {
	saveAs(d->currentUrl());
}


void MainWindow::reload() {
	KUrl url = d->currentUrl();
	Document::Ptr doc = DocumentFactory::instance()->load(url);
	doc->reload();
}


void MainWindow::save(const KUrl& url) {
	Document::Ptr doc = DocumentFactory::instance()->load(url);
	doc->waitUntilLoaded();
	QByteArray format = doc->format();
	QStringList availableTypes = KImageIO::types(KImageIO::Writing);
	if (availableTypes.contains(QString(format))) {
		doc->save(url, format);
		// FIXME: check return code of Document::save()
		return;
	}

	// We don't know how to save in 'format', ask the user for a format we can
	// write to.
	KGuiItem saveUsingAnotherFormat = KStandardGuiItem::saveAs();
	saveUsingAnotherFormat.setText(i18n("Save using another format"));
	int result = KMessageBox::warningContinueCancel(
		this,
		i18n("Gwenview cannot save images in '%1' format.", QString(format)),
		QString() /* caption */,
		saveUsingAnotherFormat
		);
	if (result == KMessageBox::Continue) {
		saveAs(url);
	}
}


void MainWindow::saveAs(const KUrl& url) {
	KFileDialog dialog(url, QString(), this);
	dialog.setOperationMode(KFileDialog::Saving);

	// Init mime filter
	QString mimeType = MimeTypeUtils::urlMimeType(url);
	QStringList availableMimeTypes = KImageIO::mimeTypes(KImageIO::Writing);
	dialog.setMimeFilter(availableMimeTypes, mimeType);

	// Show dialog
	if (!dialog.exec()) {
		return;
	}

	// Check for overwrite
	if (KIO::NetAccess::exists(dialog.selectedUrl(), KIO::NetAccess::DestinationSide, this)) {
		int answer = KMessageBox::warningContinueCancel(
			this,
			i18nc("@info",
				"A file named <filename>%1</filename> already exists.\n"
				"Are you sure you want to overwrite it?",
				url.fileName()),
			QString(),
			KStandardGuiItem::overwrite());
		if (answer == KMessageBox::Cancel) {
			return;
		}
	}

	// Start save
	mimeType = dialog.currentMimeFilter();
	QStringList typeList = KImageIO::typeForMime(mimeType);
	Q_ASSERT(typeList.count() > 0);
	Document::Ptr doc = DocumentFactory::instance()->load(url);
	doc->save(dialog.selectedUrl(), typeList[0].toAscii());
}


void MainWindow::showDocumentInFullScreen(const KUrl& url) {
	openDocumentUrl(url);
	d->mFullScreenAction->trigger();
}


void MainWindow::rotateCurrentLeft() {
	TransformImageOperation op(ROT_270);
	d->applyImageOperation(&op);
}


void MainWindow::rotateLeft(const KUrl& url) {
	TransformImageOperation op(ROT_270);
	Document::Ptr doc = DocumentFactory::instance()->load(url);
	doc->waitUntilLoaded();
	op.apply(doc);
}


void MainWindow::rotateCurrentRight() {
	TransformImageOperation op(ROT_90);
	d->applyImageOperation(&op);
}


void MainWindow::rotateRight(const KUrl& url) {
	TransformImageOperation op(ROT_90);
	Document::Ptr doc = DocumentFactory::instance()->load(url);
	doc->waitUntilLoaded();
	op.apply(doc);
}


void MainWindow::mirror() {
	TransformImageOperation op(HFLIP);
	d->applyImageOperation(&op);
}


void MainWindow::flip() {
	TransformImageOperation op(VFLIP);
	d->applyImageOperation(&op);
}


void MainWindow::resizeImage() {
	Document::Ptr doc = DocumentFactory::instance()->load(d->currentUrl());
	doc->waitUntilLoaded();
	int size = GwenviewConfig::imageResizeLastSize();
	if (size == -1) {
		size = qMax(doc->image().width(), doc->image().height());
	}
	bool ok = false;
	size = KInputDialog::getInteger(
		i18n("Image Resizing"),
		i18n("Enter the new size of the image:"),
		size, 0, 100000, 10 /* step */,
		&ok,
		this);
	if (!ok) {
		return;
	}
	GwenviewConfig::setImageResizeLastSize(size);
	ResizeImageOperation op(size);
	d->applyImageOperation(&op);
}


void MainWindow::crop() {
	ImageViewPart* imageViewPart = d->mDocumentView->imageViewPart();
	if (!imageViewPart) {
		kError() << "No ImageViewPart available!";
		return;
	}
	Document::Ptr doc = DocumentFactory::instance()->load(d->currentUrl());
	doc->waitUntilLoaded();
	CropSideBar* cropSideBar = new CropSideBar(this, imageViewPart->imageView(), doc);
	connect(cropSideBar, SIGNAL(done()), SLOT(hideTemporarySideBar()) );

	d->showTemporarySideBar(cropSideBar);
}


void MainWindow::toggleSlideShow() {
	if (d->mSlideShow->isRunning()) {
		d->mSlideShow->stop();
	} else {
		QList<KUrl> list;
		for (int pos=0; pos < d->mDirModel->rowCount(); ++pos) {
			QModelIndex index = d->mDirModel->index(pos, 0);
			KFileItem item = d->mDirModel->itemForIndex(index);
			MimeTypeUtils::Kind kind = MimeTypeUtils::fileItemKind(item);
			if (kind == MimeTypeUtils::KIND_FILE || kind == MimeTypeUtils::KIND_RASTER_IMAGE) {
				list << item.url();
			}
		}
		d->mSlideShow->start(list);
	}
	updateSlideShowAction();
}


void MainWindow::updateSlideShowAction() {
	if (d->mSlideShow->isRunning()) {
		d->mToggleSlideShowAction->setText(i18n("Stop Slideshow"));
		d->mToggleSlideShowAction->setIcon(KIcon("media-playback-pause"));
	} else {
		d->mToggleSlideShowAction->setText(i18n("Start Slideshow"));
		d->mToggleSlideShowAction->setIcon(KIcon("media-playback-start"));
	}
}



void MainWindow::generateThumbnailForUrl(const KUrl& url) {
	QModelIndex index = d->mDirModel->indexForUrl(url);
	if (!index.isValid()) {
		return;
	}

	KFileItem item = d->mDirModel->itemForIndex(index);
	if (item.isNull()) {
		return;
	}

	KFileItemList list;
	list << item;
	d->mThumbnailView->thumbnailViewHelper()->generateThumbnailsForItems(list);
}


bool MainWindow::queryClose() {
	saveConfig();
	if (!d->mFullScreenAction->isChecked()) {
		// Do not save config in fullscreen, we don't want to restart without
		// menu bar or toolbars...
		d->saveMainWindowConfig();
	}
	QList<KUrl> list = DocumentFactory::instance()->modifiedDocumentList();
	if (list.size() == 0) {
		return true;
	}

	KGuiItem yes(i18n("Save All Changes"), "document-save");
	KGuiItem no(i18n("Discard Changes"));
    QString msg =
        i18np("One image has been modified.", "%1 images have been modified.", list.size())
		+ '\n'
        + i18n("If you quit now, your changes will be lost.");
	int answer = KMessageBox::warningYesNoCancel(
		this,
        msg,
		QString() /* caption */,
		yes,
		no);

	switch (answer) {
	case KMessageBox::Yes:
		return d->mSaveBar->saveAll();

	case KMessageBox::No:
		return true;

	default: // cancel
		return false;
	}
}


void MainWindow::showConfigDialog() {
	ConfigDialog dialog(this);
	connect(&dialog, SIGNAL(settingsChanged(const QString&)), SLOT(loadConfig()));
	ImageViewPart* part = d->mDocumentView->imageViewPart();
	if (part) {
		connect(&dialog, SIGNAL(settingsChanged(const QString&)),
			part, SLOT(loadConfig()));
	}
	dialog.exec();
}


void MainWindow::configureToolbars() {
	KConfigGroup cg = KGlobal::config()->group("MainWindow");
	saveMainWindowSettings(cg);
	KEditToolBar dlg(factory(), this);
	connect(&dlg,SIGNAL(newToolBarConfig()),this,SLOT(slotNewToolbarConfig()));
	dlg.exec();
}


void MainWindow::slotNewToolbarConfig() {
	KConfigGroup cg = KGlobal::config()->group("MainWindow");
	applyMainWindowSettings(cg);
}


void MainWindow::toggleMenuBar() {
	if (!d->mFullScreenAction->isChecked()) {
		menuBar()->setVisible(d->mShowMenuBarAction->isChecked());
	}
}


void MainWindow::loadConfig() {
	QColor bgColor = GwenviewConfig::viewBackgroundColor();
	QColor fgColor = bgColor.value() > 128 ? Qt::black : Qt::white;

	QWidget* widget = d->mThumbnailView->viewport();
	QPalette palette = widget->palette();
	palette.setColor(QPalette::Base, bgColor);
	palette.setColor(QPalette::Text, fgColor);
	widget->setPalette(palette);

	d->mDocumentView->setPalette(palette);

	d->mThumbnailSlider->setValue(GwenviewConfig::thumbnailSize());
	// If GwenviewConfig::thumbnailSize() returns the current value of
	// mThumbnailSlider, it won't emit valueChanged() and the thumbnail view
	// won't be updated. That's why we do it ourself.
	d->mThumbnailView->setThumbnailSize(GwenviewConfig::thumbnailSize());
}


void MainWindow::saveConfig() {
	GwenviewConfig::setThumbnailSize(d->mThumbnailSlider->value());
}


void MainWindow::print() {
	if (!d->currentDocumentIsRasterImage()) {
		return;
	}

	Document::Ptr doc = DocumentFactory::instance()->load(d->currentUrl());
	PrintHelper printHelper(this);
	printHelper.print(doc);
}


void MainWindow::handleResizeRequest(const QSize& _size) {
	// Disconnect ourself to avoid resizing again when we load another image
	disconnect(d->mDocumentView, SIGNAL(resizeRequested(const QSize&)),
		d->mWindow, SLOT(handleResizeRequest(const QSize&)) );

	if (!d->mViewAction->isChecked()) {
		return;
	}
	if (isMaximized() || isMinimized() || d->mFullScreenAction->isChecked()) {
		return;
	}

	QSize size = _size;

	int sideBarWidth = d->mSideBar->isVisible() ? d->mSideBar->width() : 0;
	// innerMargin is the margin around the view, not including the window
	// frame
	QSize innerMargin = QSize(sideBarWidth, menuBar()->height() + toolBar()->height());
	// frameMargin is the size of the frame around the window
	QSize frameMargin = frameGeometry().size() - geometry().size();

	// Adjust size
	QRect availableRect;
	#ifdef Q_WS_X11
	availableRect = KWindowSystem::workArea();
	#else
	availableRect = QApplication::desktop()->availableGeometry();
	#endif
	QSize maxSize = availableRect.size() - innerMargin - frameMargin;

	if (size.width() > maxSize.width() || size.height() > maxSize.height()) {
		size.scale(maxSize, Qt::KeepAspectRatio);
	}

	int minWidth = d->computeMinimumWidth();
	if (size.width() < minWidth) {
		size.setWidth(minWidth);
	}

	QRect windowRect = geometry();
	windowRect.setSize(size + innerMargin);

	// Move the window if it does not fit in the screen
	int rightFrameMargin = frameGeometry().right() - geometry().right();
	if (windowRect.right() + rightFrameMargin > availableRect.right()) {
		windowRect.moveRight(availableRect.right() - rightFrameMargin);
	}

	int bottomFrameMargin = frameGeometry().bottom() - geometry().bottom();
	if (windowRect.bottom() + bottomFrameMargin > availableRect.bottom()) {
		windowRect.moveBottom(availableRect.bottom() - bottomFrameMargin);
	}

	// Define geometry
	setGeometry(windowRect);
}


void MainWindow::hideTemporarySideBar() {
	QWidget* temporarySideBar = d->mSideBarContainer->currentWidget();
	if (temporarySideBar == d->mSideBar) {
		return;
	}
	temporarySideBar->deleteLater();

	if (!d->mSideBarWasVisibleBeforeTemporarySideBar) {
		d->mSideBarContainer->hide();
	}
	d->mSideBarContainer->setCurrentWidget(d->mSideBar);
}


void MainWindow::preloadNextUrl() {
	QItemSelection selection = d->mThumbnailView->selectionModel()->selection();
	if (selection.size() != 1) {
		return;
	}

	QModelIndex index = selection.indexes()[0];
	if (!index.isValid()) {
		return;
	}

	QModelIndex nextIndex = d->mDirModel->sibling(index.row() + 1, index.column(), index);
	if (!nextIndex.isValid()) {
		return;
	}
	KFileItem item = d->mDirModel->itemForIndex(nextIndex);

	if (!ArchiveUtils::fileItemIsDirOrArchive(item)) {
		KUrl url = item.url();
		if (url.isLocalFile()) {
			DocumentFactory::instance()->load(url);
		}
	}
}


QSize MainWindow::sizeHint() const {
	return QSize(750, 500);
}


void MainWindow::showEvent(QShowEvent *event) {
	// We need to delay initializing the action state until the menu bar has
	// been initialized, that's why it's done only in the showEvent()
	d->mShowMenuBarAction->setChecked(menuBar()->isVisible());
	KParts::MainWindow::showEvent(event);
}


void MainWindow::editLocation() {
	d->mUrlNavigator->setUrlEditable(true);
	d->mUrlNavigator->setFocus();
}
} // namespace
