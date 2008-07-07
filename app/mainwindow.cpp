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
#include <config-gwenview.h>

// Qt
#include <QApplication>
#include <QDesktopWidget>
#include <QTimer>
#include <QShortcut>
#include <QSplitter>
#include <QSlider>
#include <QStackedWidget>
#include <QUndoGroup>
#include <QVBoxLayout>

// KDE
#include <kactioncollection.h>
#include <kaction.h>
#include <kapplication.h>
#include <kdirlister.h>
#include <kedittoolbar.h>
#include <kfiledialog.h>
#include <kfileitem.h>
#include <kmenubar.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprotocolmanager.h>
#include <kstatusbar.h>
#include <kstandardguiitem.h>
#include <kstandardshortcut.h>
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
#include "fullscreencontent.h"
#include "gvcore.h"
#include "imageopscontextmanageritem.h"
#include "infocontextmanageritem.h"
#ifdef KIPI_FOUND
#include "kipiinterface.h"
#endif
#ifndef GWENVIEW_METADATA_BACKEND_NONE
#include "nepomukcontextmanageritem.h"
#endif
#include "preloader.h"
#include "savebar.h"
#include "sidebar.h"
#include "startpage.h"
#include "thumbnailbarview.h"
#include "thumbnailviewhelper.h"
#include "thumbnailviewpanel.h"
#include <lib/archiveutils.h>
#include <lib/document/documentfactory.h>
#include <lib/fullscreenbar.h>
#include <lib/gwenviewconfig.h>
#include <lib/imageviewpart.h>
#include <lib/mimetypeutils.h>
#include <lib/print/printhelper.h>
#include <lib/slideshow.h>
#include <lib/signalblocker.h>
#include <lib/metadata/sorteddirmodel.h>
#include <lib/thumbnailview/thumbnailview.h>
#include <lib/urlutils.h>

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


struct MainWindowState {
	QAction* mActiveViewModeAction;
	bool mSideBarVisible;
	bool mToolBarVisible;
	Qt::WindowStates mWindowState;
};


struct MainWindow::Private {
	GvCore* mGvCore;
	MainWindow* mWindow;
	QSplitter* mCentralSplitter;
	DocumentView* mDocumentView;
	KUrlNavigator* mUrlNavigator;
	ThumbnailView* mThumbnailView;
	ThumbnailViewHelper* mThumbnailViewHelper;
	QSlider* mThumbnailSlider;
	ThumbnailViewPanel* mThumbnailViewPanel;
	StartPage* mStartPage;
	SideBar* mSideBar;
	QStackedWidget* mViewStackedWidget;
	QStackedWidget* mSideBarContainer;
	bool mSideBarWasVisibleBeforeTemporarySideBar;
	FullScreenBar* mFullScreenBar;
	FullScreenContent* mFullScreenContent;
	SaveBar* mSaveBar;
	bool mStartSlideShowWhenDirListerCompleted;
	SlideShow* mSlideShow;
	Preloader* mPreloader;
#ifdef KIPI_FOUND
	KIPIInterface* mKIPIInterface;
#endif

	QActionGroup* mViewModeActionGroup;
	QAction* mBrowseAction;
	QAction* mViewAction;
	QAction* mGoUpAction;
	QAction* mGoToPreviousAction;
	QAction* mGoToNextAction;
	QAction* mToggleSideBarAction;
	KToggleFullScreenAction* mFullScreenAction;
	QAction* mToggleSlideShowAction;
	KToggleAction* mShowMenuBarAction;

	SortedDirModel* mDirModel;
	ContextManager* mContextManager;

	MainWindowState mStateBeforeFullScreen;

	KUrl mUrlToSelect;

	QString mCaption;

	void setupWidgets() {
		QWidget* centralWidget = new QWidget(mWindow);
		mWindow->setCentralWidget(centralWidget);
		mSaveBar = new SaveBar(centralWidget, mWindow->actionCollection());

		mCentralSplitter = new QSplitter(Qt::Horizontal, centralWidget);
		QVBoxLayout* layout = new QVBoxLayout(centralWidget);
		layout->addWidget(mSaveBar);
		layout->addWidget(mCentralSplitter);
		layout->setMargin(0);
		layout->setSpacing(0);

		mViewStackedWidget = new QStackedWidget(mCentralSplitter);

		setupThumbnailView(mViewStackedWidget);
		setupDocumentView(mViewStackedWidget);
		setupStatusBars();
		setupStartPage(mViewStackedWidget);
		mViewStackedWidget->addWidget(mThumbnailViewPanel);
		mViewStackedWidget->addWidget(mDocumentView);
		mViewStackedWidget->addWidget(mStartPage);
		mViewStackedWidget->setCurrentWidget(mThumbnailViewPanel);

		mSideBarContainer = new QStackedWidget(mCentralSplitter);
		mSideBar = new SideBar(mSideBarContainer);
		mSideBarContainer->addWidget(mSideBar);

		mCentralSplitter->setStretchFactor(0, 1);
		mCentralSplitter->setStretchFactor(1, 0);

		mStartSlideShowWhenDirListerCompleted = false;
		mSlideShow = new SlideShow(mWindow);

		connect(mSaveBar, SIGNAL(requestSaveAll()),
			mGvCore, SLOT(saveAll()) );
		connect(mSaveBar, SIGNAL(goToUrl(const KUrl&)),
			mWindow, SLOT(goToUrl(const KUrl&)) );

		connect(mSlideShow, SIGNAL(goToUrl(const KUrl&)),
			mWindow, SLOT(goToUrl(const KUrl&)) );
	}

	void setupThumbnailView(QWidget* parent) {
		mThumbnailViewPanel = new ThumbnailViewPanel(parent, mDirModel, mWindow->actionCollection());

		mThumbnailView = mThumbnailViewPanel->thumbnailView();
		mThumbnailSlider = mThumbnailViewPanel->thumbnailSlider();
		mUrlNavigator = mThumbnailViewPanel->urlNavigator();

		mThumbnailViewHelper = new ThumbnailViewHelper(mDirModel);
		mThumbnailView->setThumbnailViewHelper(mThumbnailViewHelper);

		// Connect thumbnail view
		connect(mThumbnailView, SIGNAL(indexActivated(const QModelIndex&)),
			mWindow, SLOT(slotThumbnailViewIndexActivated(const QModelIndex&)) );
		connect(mThumbnailView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
			mWindow, SLOT(slotSelectionChanged()) );

		// Connect delegate
		QAbstractItemDelegate* delegate = mThumbnailView->itemDelegate();
		connect(delegate, SIGNAL(saveDocumentRequested(const KUrl&)),
			mGvCore, SLOT(save(const KUrl&)) );
		connect(delegate, SIGNAL(rotateDocumentLeftRequested(const KUrl&)),
			mGvCore, SLOT(rotateLeft(const KUrl&)) );
		connect(delegate, SIGNAL(rotateDocumentRightRequested(const KUrl&)),
			mGvCore, SLOT(rotateRight(const KUrl&)) );
		connect(delegate, SIGNAL(showDocumentInFullScreenRequested(const KUrl&)),
			mWindow, SLOT(showDocumentInFullScreen(const KUrl&)) );

		// Connect url navigator
		connect(mUrlNavigator, SIGNAL(urlChanged(const KUrl&)),
			mWindow, SLOT(openDirUrl(const KUrl&)) );
	}

	void setupDocumentView(QWidget* parent) {
		mDocumentView = new DocumentView(parent, mWindow->actionCollection());
		connect(mDocumentView, SIGNAL(completed()),
			mWindow, SLOT(slotPartCompleted()) );
		connect(mDocumentView, SIGNAL(partChanged(KParts::Part*)),
			mWindow, SLOT(createGUI(KParts::Part*)) );
		connect(mDocumentView, SIGNAL(previousImageRequested()),
			mWindow, SLOT(goToPrevious()) );
		connect(mDocumentView, SIGNAL(nextImageRequested()),
			mWindow, SLOT(goToNext()) );
		connect(mDocumentView, SIGNAL(enterFullScreenRequested()),
			mWindow, SLOT(enterFullScreen()) );

		ThumbnailBarView* bar = mDocumentView->thumbnailBar();
		bar->setModel(mDirModel);
		bar->setThumbnailViewHelper(mThumbnailViewHelper);
		bar->setSelectionModel(mThumbnailView->selectionModel());
	}

	void setupStatusBars() {
		const int frameWidth = mWindow->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, mWindow);
		const int height = mWindow->fontMetrics().height() + 4 * frameWidth;
		mThumbnailViewPanel->setStatusBarHeight(height);
		mDocumentView->setStatusBarHeight(height);
	}


	void setupStartPage(QWidget* parent) {
		mStartPage = new StartPage(parent);
		connect(mStartPage, SIGNAL(urlSelected(const KUrl&)),
			mWindow, SLOT(slotStartPageUrlSelected(const KUrl&)) );
	}


	void setupActions() {
		KActionCollection* actionCollection = mWindow->actionCollection();

		KStandardAction::save(mWindow, SLOT(saveCurrent()), actionCollection);
		KStandardAction::saveAs(mWindow, SLOT(saveCurrentAs()), actionCollection);
		KStandardAction::open(mWindow, SLOT(openFile()), actionCollection);
		KStandardAction::print(mWindow, SLOT(print()), actionCollection);
		KStandardAction::quit(KApplication::kApplication(), SLOT(closeAllWindows()), actionCollection);

		QAction* action = actionCollection->addAction("reload");
		action->setText(i18nc("@action reload the currently viewed image", "Reload"));
		action->setIcon(KIcon("view-refresh"));
		action->setShortcut(Qt::Key_F5);
		connect(action, SIGNAL(triggered()),
			mWindow, SLOT(reload()) );

		mBrowseAction = actionCollection->addAction("browse");
		mBrowseAction->setText(i18n("Browse"));
		mBrowseAction->setCheckable(true);
		mBrowseAction->setIcon(KIcon("view-list-icons"));

		mViewAction = actionCollection->addAction("view");
		mViewAction->setText(i18n("View"));
		mViewAction->setIcon(KIcon("view-preview"));
		mViewAction->setCheckable(true);

		mViewModeActionGroup = new QActionGroup(mWindow);
		mViewModeActionGroup->addAction(mBrowseAction);
		mViewModeActionGroup->addAction(mViewAction);

		connect(mViewModeActionGroup, SIGNAL(triggered(QAction*)),
			mWindow, SLOT(setActiveViewModeAction(QAction*)) );

		mFullScreenAction = KStandardAction::fullScreen(mWindow, SLOT(toggleFullScreen()), mWindow, actionCollection);

		QShortcut* reduceLevelOfDetailsShortcut = new QShortcut(mWindow);
		reduceLevelOfDetailsShortcut->setKey(Qt::Key_Escape);
		connect(reduceLevelOfDetailsShortcut, SIGNAL(activated()),
			mWindow, SLOT(reduceLevelOfDetails()) );

		mGoToPreviousAction = KStandardAction::prior(mWindow, SLOT(goToPrevious()), actionCollection);
		mGoToPreviousAction->setText(i18nc("@action Go to previous image", "Previous"));
		mGoToPreviousAction->setToolTip(i18n("Go to Previous Image"));
		mGoToPreviousAction->setShortcut(Qt::Key_Backspace);

		mGoToNextAction = KStandardAction::next(mWindow, SLOT(goToNext()), actionCollection);
		mGoToNextAction->setText(i18nc("@action Go to next image", "Next"));
		mGoToNextAction->setToolTip(i18n("Go to Next Image"));
		mGoToNextAction->setShortcut(Qt::Key_Space);

		mGoUpAction = KStandardAction::up(mWindow, SLOT(goUp()), actionCollection);

		action = actionCollection->addAction("go_start_page");
		action->setIcon(KIcon("go-home"));
		action->setText(i18nc("@action", "Start Page"));
		connect(action, SIGNAL(triggered()),
			mWindow, SLOT(showStartPage()) );

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
	}

	void setupUndoActions() {
		// There is no KUndoGroup similar to KUndoStack. This code basically
		// does the same as KUndoStack, but for the KUndoGroup actions.
		QUndoGroup* undoGroup = DocumentFactory::instance()->undoGroup();
		QAction* action;
		KActionCollection* actionCollection =  mWindow->actionCollection();

		action = undoGroup->createRedoAction(actionCollection);
		action->setObjectName(KStandardAction::name(KStandardAction::Redo));
		action->setIcon(KIcon("edit-redo"));
		action->setIconText(i18n("Redo"));
		action->setShortcuts(KStandardShortcut::redo());
		actionCollection->addAction(action->objectName(), action);

		action = undoGroup->createUndoAction(actionCollection);
		action->setObjectName(KStandardAction::name(KStandardAction::Undo));
		action->setIcon(KIcon("edit-undo"));
		action->setIconText(i18n("Undo"));
		action->setShortcuts(KStandardShortcut::undo());
		actionCollection->addAction(action->objectName(), action);
	}


	void setupContextManager() {
		KActionCollection* actionCollection = mWindow->actionCollection();

		mContextManager = new ContextManager(mWindow);
		mContextManager->setSideBar(mSideBar);
		mContextManager->setDirModel(mDirModel);

		mContextManager->addItem(new InfoContextManagerItem(mContextManager));

		#ifndef GWENVIEW_METADATA_BACKEND_NONE
		mContextManager->addItem(new NepomukContextManagerItem(mContextManager, actionCollection));
		#endif

		mContextManager->addItem(new ImageOpsContextManagerItem(mContextManager, mWindow));

		FileOpsContextManagerItem* fileOpsItem = new FileOpsContextManagerItem(mContextManager, actionCollection);
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

		connect(mDirModel, SIGNAL(rowsRemoved(const QModelIndex&, int, int)),
			mWindow, SLOT(updatePreviousNextActions()) );
		connect(mDirModel, SIGNAL(modelReset()),
			mWindow, SLOT(updatePreviousNextActions()) );

		connect(dirLister, SIGNAL(completed()),
			mWindow, SLOT(slotDirListerCompleted()) );
	}

	void updateToggleSideBarAction() {
		if (mSideBarContainer->isVisibleTo(mSideBarContainer->parentWidget())) {
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
		mGvCore->addUrlToRecentFolders(url);
	}

	void setupFullScreenBar() {
		mFullScreenBar = new FullScreenBar(mDocumentView);
		mFullScreenContent = new FullScreenContent(
			mFullScreenBar, mWindow->actionCollection(), mSlideShow);

		ThumbnailBarView* view = mFullScreenContent->thumbnailBar();
		view->setModel(mDirModel);
		view->setThumbnailViewHelper(mThumbnailViewHelper);
		view->setSelectionModel(mThumbnailView->selectionModel());

		mFullScreenBar->adjustSize();
	}


	void updateActions() {
		bool canSave = mWindow->currentDocumentIsRasterImage();
		if (!mDocumentView->isVisible()) {
			// Saving only makes sense if exactly one image is selected
			QItemSelection selection = mThumbnailView->selectionModel()->selection();
			QModelIndexList indexList = selection.indexes();
			if (indexList.count() != 1) {
				canSave = false;
			}
		}

		KActionCollection* actionCollection = mWindow->actionCollection();
		actionCollection->action("file_save")->setEnabled(canSave);
		actionCollection->action("file_save_as")->setEnabled(canSave);
	}

	KUrl currentUrl() const {
		if (mDocumentView->isVisible() && !mDocumentView->isEmpty()) {
			return mDocumentView->url();
		} else {
			QModelIndex index = mThumbnailView->currentIndex();
			if (!index.isValid()) {
				return KUrl();
			}
			// Ignore the current index if it's not part of the selection. This
			// situation can happen when you select an image, then click on the
			// background of the view.
			if (!mThumbnailView->selectionModel()->isSelected(index)) {
				return KUrl();
			}
			KFileItem item = mDirModel->itemForIndex(index);
			Q_ASSERT(!item.isNull());
			return item.url();
		}
	}

	void selectUrlToSelect() {
		if (!mUrlToSelect.isValid()) {
			return;
		}
		QModelIndex index = mDirModel->indexForUrl(mUrlToSelect);
		if (index.isValid()) {
			mThumbnailView->setCurrentIndex(index);
			mUrlToSelect = KUrl();
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


	void updateContextDependentComponents() {
		// Gather info
		KUrl url = currentUrl();
		KFileItemList selectedItemList;
		if (KProtocolManager::supportsListing(url)) {
			QItemSelection selection = mThumbnailView->selectionModel()->selection();
			QModelIndexList indexList = selection.indexes();

			Q_FOREACH(const QModelIndex& index, indexList) {
				selectedItemList << mDirModel->itemForIndex(index);
			}
		} else if (url.isValid()) {
			KFileItem item(KFileItem::Unknown, KFileItem::Unknown, url);
			selectedItemList << item;
		}

		// Update
		mContextManager->setContext(url, selectedItemList);
		mSaveBar->setCurrentUrl(url);
		mSlideShow->setCurrentUrl(url);
	}

	/**
	 * Sometimes we can get in handleResizeRequest() before geometry() has been
	 * correctly initialized. In this case the difference between geometry
	 */
	void ensureGeometryIsValid() {
		// Wait until height difference between frameGeometry and geometry is less than maxDelta
		// But don't wait for more than timeout (avoid infinite loop)
		const int timeout = 1000;
		const int maxDelta = 100;
		QTime chrono;
		chrono.start();
		while (mWindow->frameGeometry().height() - mWindow->geometry().height() > maxDelta
			&& chrono.elapsed() < timeout)
		{
			qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
		}
	}

};


MainWindow::MainWindow()
: KParts::MainWindow(),
d(new MainWindow::Private)
{
	d->mWindow = this;
	d->mGvCore = new GvCore(this);
	d->mDirModel = new SortedDirModel(this);
	d->mPreloader = new Preloader(this);
	d->initDirModel();
	d->setupWidgets();
	d->setupActions();
	d->setupUndoActions();
	d->setupContextManager();
	d->setupFullScreenBar();
	d->updateActions();
	updatePreviousNextActions();
	d->mSaveBar->initActionDependentWidgets();
	d->mThumbnailViewPanel->initActionDependentWidgets();

	createShellGUI();
	loadConfig();
	loadMainWindowConfig();
	connect(DocumentFactory::instance(), SIGNAL(documentChanged(const KUrl&)),
		SLOT(generateThumbnailForUrl(const KUrl&)) );

	connect(DocumentFactory::instance(), SIGNAL(modifiedDocumentListChanged()),
		SLOT(updateModifiedFlag()) );

#ifdef KIPI_FOUND
	d->mKIPIInterface = new KIPIInterface(this);
#endif
}


MainWindow::~MainWindow() {
	delete d;
}

ContextManager* MainWindow::contextManager() const {
	return d->mContextManager;
}

DocumentView* MainWindow::documentView() const {
	return d->mDocumentView;
}


bool MainWindow::currentDocumentIsRasterImage() const {
	if (d->mDocumentView->isVisible()) {
		return d->mDocumentView->currentDocumentIsRasterImage();
	} else {
		QModelIndex index = d->mThumbnailView->currentIndex();
		if (!index.isValid()) {
			return false;
		}
		KFileItem item = d->mDirModel->itemForIndex(index);
		Q_ASSERT(!item.isNull());
		return MimeTypeUtils::fileItemKind(item) == MimeTypeUtils::KIND_RASTER_IMAGE;
	}
}


void MainWindow::showTemporarySideBar(QWidget* sideBar) {
	d->mSideBarWasVisibleBeforeTemporarySideBar = d->mSideBarContainer->isVisible();
	// Move the sideBar inside a widget so that we can add a stretch below
	// it.
	QWidget* widget = new QWidget(d->mSideBarContainer);
	sideBar->setParent(widget);

	QVBoxLayout* layout = new QVBoxLayout(widget);
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addWidget(sideBar);
	layout->addStretch();

	d->mSideBarContainer->addWidget(widget);
	d->mSideBarContainer->setCurrentWidget(widget);
	d->mSideBarContainer->show();
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
	Q_ASSERT(url.isValid());
	if (UrlUtils::urlIsDirectory(url)) {
		d->mBrowseAction->trigger();
		openDirUrl(url);
	} else {
		d->mViewAction->trigger();
		// Resize the window once image is loaded
		connect(d->mDocumentView, SIGNAL(resizeRequested(const QSize&)),
			d->mWindow, SLOT(handleResizeRequest(const QSize&)) );
		openDocumentUrl(url);
	}
	d->updateContextDependentComponents();
	d->mSideBarContainer->setVisible(GwenviewConfig::sideBarIsVisible());
	d->updateToggleSideBarAction();
}


void MainWindow::startSlideShow() {
	// We need to wait until we have listed all images in the dirlister to
	// start the slideshow because the SlideShow objects needs an image list to
	// work.
	d->mStartSlideShowWhenDirListerCompleted = true;
}


void MainWindow::setActiveViewModeAction(QAction* action) {
	if (action == d->mViewAction) {
		// Switching to view mode
		QStringList mimeFilter = MimeTypeUtils::dirMimeTypes() + ArchiveUtils::mimeTypes();
		d->mDirModel->setMimeExcludeFilter(mimeFilter);
		d->mViewStackedWidget->setCurrentWidget(d->mDocumentView);
		if (d->mDocumentView->isEmpty()) {
			openSelectedDocument();
		}
	} else {
		// Switching to browse mode
		d->mViewStackedWidget->setCurrentWidget(d->mThumbnailViewPanel);
		if (!d->mDocumentView->isEmpty()
			&& KProtocolManager::supportsListing(d->mDocumentView->url()))
		{
			// Reset the view to spare resources, but don't do it if we can't
			// browse the url, otherwise if the user starts Gwenview this way:
			// gwenview http://example.com/example.png
			// and switch to browse mode, switching back to view mode won't bring
			// his image back.
			d->mDocumentView->reset();
		}
		d->mDirModel->setMimeExcludeFilter(QStringList());
	}

	emit viewModeChanged();
}


void MainWindow::slotThumbnailViewIndexActivated(const QModelIndex& index) {
	if (!index.isValid()) {
		return;
	}

	KFileItem item = d->mDirModel->itemForIndex(index);
	if (item.isDir()) {
		// Item is a dir, open it
		openDirUrl(item.url());
	} else if (ArchiveUtils::fileItemIsArchive(item)) {
		// Item is an archive, tweak url then open it
		KUrl url = item.url();
		QString protocol = ArchiveUtils::protocolForMimeType(item.mimetype());
		url.setProtocol(protocol);
		openDirUrl(url);
	} else {
		// Item is a document, switch to view mode
		d->mViewAction->trigger();
	}
}


void MainWindow::openSelectedDocument() {
	if (!d->mDocumentView->isVisible()) {
		return;
	}

	QModelIndex index = d->mThumbnailView->currentIndex();
	if (!index.isValid()) {
		return;
	}

	KFileItem item = d->mDirModel->itemForIndex(index);
	if (!item.isNull() && !ArchiveUtils::fileItemIsDirOrArchive(item)) {
		openDocumentUrl(item.url());
	}
}


void MainWindow::goUp() {
	KUrl url = d->mDirModel->dirLister()->url();
	url = url.upUrl();
	openDirUrl(url);
}


void MainWindow::showStartPage() {
	d->mBrowseAction->setEnabled(false);
	d->mViewAction->setEnabled(false);
	d->mFullScreenAction->setEnabled(false);
	d->mToggleSideBarAction->setEnabled(false);

	d->mSideBarContainer->hide();
	d->updateToggleSideBarAction();

	d->mViewStackedWidget->setCurrentWidget(d->mStartPage);
}


void MainWindow::slotStartPageUrlSelected(const KUrl& url) {
	d->mBrowseAction->setEnabled(true);
	d->mViewAction->setEnabled(true);
	d->mFullScreenAction->setEnabled(true);
	d->mToggleSideBarAction->setEnabled(true);
	openDirUrl(url);

	if (d->mBrowseAction->isChecked()) {
		// Silently uncheck the action so that trigger() does the right thing
		SignalBlocker blocker(d->mBrowseAction);
		d->mBrowseAction->setChecked(false);
	}
	d->mBrowseAction->trigger();
	d->mSideBarContainer->setVisible(GwenviewConfig::sideBarIsVisible());
	d->updateToggleSideBarAction();
}


void MainWindow::openDirUrl(const KUrl& url) {
	if (url.equals(d->mDirModel->dirLister()->url(), KUrl::CompareWithoutTrailingSlash)) {
		return;
	}

	KUrl currentUrl = d->mDirModel->dirLister()->url();
	if (url.isParentOf(currentUrl)) {
		// Keep first child between url and currentUrl selected
		// If currentPath is      "/home/user/photos/2008/event"
		// and wantedPath is      "/home/user/photos"
		// pathToSelect should be "/home/user/photos/2008"
		const QString currentPath = QDir::cleanPath(currentUrl.path(KUrl::RemoveTrailingSlash));
		const QString wantedPath  = QDir::cleanPath(url.path(KUrl::RemoveTrailingSlash));
		const QChar separator('/');
		const int slashCount = wantedPath.count(separator);
		const QString pathToSelect = currentPath.section(separator, 0, slashCount + 1);
		d->mUrlToSelect = url;
		d->mUrlToSelect.setPath(pathToSelect);
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

	d->mFullScreenContent->setCurrentUrl(url);

	Document::Ptr doc = DocumentFactory::instance()->load(url);
	QUndoGroup* undoGroup = DocumentFactory::instance()->undoGroup();
	undoGroup->addStack(doc->undoStack());
	undoGroup->setActiveStack(doc->undoStack());

	d->mUrlToSelect = url;
	d->selectUrlToSelect();
}


void MainWindow::slotSetStatusBarText(const QString& message) {
	d->mDocumentView->statusBar()->showMessage(message);
}

void MainWindow::toggleSideBar() {
	d->mSideBarContainer->setVisible(!d->mSideBarContainer->isVisible());
	d->updateToggleSideBarAction();
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
	if (d->mDocumentView->isVisible()) {
		// The user selected a new file in the thumbnail view, since the
		// document view is visible, let's show it
		openSelectedDocument();
	} else {
		// No document view, we need to load the document to set the undo group
		// of document factory to the correct QUndoStack
		QModelIndex index = d->mThumbnailView->currentIndex();
		KFileItem item;
		if (index.isValid()) {
			item = d->mDirModel->itemForIndex(index);
		}
		QUndoGroup* undoGroup = DocumentFactory::instance()->undoGroup();
		if (!item.isNull() && !ArchiveUtils::fileItemIsDirOrArchive(item)) {
			KUrl url = item.url();
			Document::Ptr doc = DocumentFactory::instance()->load(url);
			undoGroup->addStack(doc->undoStack());
			undoGroup->setActiveStack(doc->undoStack());
		} else {
			undoGroup->setActiveStack(0);
		}
	}

	// Update UI
	hideTemporarySideBar();
	d->updateActions();
	updatePreviousNextActions();
	d->updateContextDependentComponents();

	// Start preloading
	QTimer::singleShot(PRELOAD_DELAY, this, SLOT(preloadNextUrl()) );
}


void MainWindow::slotDirModelNewItems() {
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
	if (d->mStartSlideShowWhenDirListerCompleted) {
		d->mStartSlideShowWhenDirListerCompleted = false;
		QTimer::singleShot(0, d->mToggleSlideShowAction, SLOT(trigger()));
	}
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


void MainWindow::enterFullScreen() {
	if (!d->mFullScreenAction->isChecked()) {
		d->mFullScreenAction->trigger();
	}
}


void MainWindow::reduceLevelOfDetails() {
	if (d->mFullScreenAction->isChecked()) {
		d->mFullScreenAction->trigger();
	} else if (d->mViewAction->isChecked()) {
		d->mBrowseAction->trigger();
	}
}


void MainWindow::toggleFullScreen() {
	setUpdatesEnabled(false);
	if (d->mFullScreenAction->isChecked()) {
		// Save MainWindow config now, this way if we quit while in
		// fullscreen, we are sure latest MainWindow changes are remembered.
		saveMainWindowConfig();

		// Go full screen
		d->mStateBeforeFullScreen.mActiveViewModeAction = d->mViewModeActionGroup->checkedAction();
		d->mStateBeforeFullScreen.mSideBarVisible = d->mSideBarContainer->isVisible();
		d->mStateBeforeFullScreen.mToolBarVisible = toolBar()->isVisible();
		d->mStateBeforeFullScreen.mWindowState = windowState();

		d->mViewAction->trigger();
		d->mSideBarContainer->hide();

		setWindowState(windowState() | Qt::WindowFullScreen);
		menuBar()->hide();
		toolBar()->hide();
		d->mDocumentView->setFullScreenMode(true);
		d->mSaveBar->setFullScreenMode(true);
		d->mFullScreenBar->setActivated(true);
	} else {
                if ( d->mStateBeforeFullScreen.mActiveViewModeAction )
                    d->mStateBeforeFullScreen.mActiveViewModeAction->trigger();
		d->mSideBarContainer->setVisible(d->mStateBeforeFullScreen.mSideBarVisible);

		// Back to normal
		d->mDocumentView->setFullScreenMode(false);
		d->mSlideShow->stop();
		d->mSaveBar->setFullScreenMode(false);
		d->mFullScreenBar->setActivated(false);
		setWindowState(d->mStateBeforeFullScreen.mWindowState);
		menuBar()->setVisible(d->mShowMenuBarAction->isChecked());
		toolBar()->setVisible(d->mStateBeforeFullScreen.mToolBarVisible);
	}
	setUpdatesEnabled(true);
}


void MainWindow::saveCurrent() {
	d->mGvCore->save(d->currentUrl());
}


void MainWindow::saveCurrentAs() {
	d->mGvCore->saveAs(d->currentUrl());
}


void MainWindow::reload() {
	KUrl url = d->currentUrl();
	Document::Ptr doc = DocumentFactory::instance()->load(url);
	if (doc->isModified()) {
		KGuiItem cont = KStandardGuiItem::cont();
		cont.setText(i18nc("@action:button", "Discard Changes and Reload"));
		int answer = KMessageBox::warningContinueCancel(this,
			i18nc("@info", "This image has been modified. Reloading it will discard all your changes."),
			QString() /* caption */,
			cont);
		if (answer != KMessageBox::Continue) {
			return;
		}
	}
	doc->reload();
}




void MainWindow::openFile() {
	KUrl dirUrl = d->mDirModel->dirLister()->url();

	KFileDialog dialog(dirUrl, QString(), this);
	dialog.setCaption(i18nc("@title:window", "Open Image"));
	QStringList mimeFilter = MimeTypeUtils::imageMimeTypes();
	dialog.setMimeFilter(mimeFilter);
	dialog.setOperationMode(KFileDialog::Opening);
	if (!dialog.exec()) {
		return;
	}

	KUrl url = dialog.selectedUrl();
	goToUrl(url);
}


void MainWindow::showDocumentInFullScreen(const KUrl& url) {
	openDocumentUrl(url);
	d->mFullScreenAction->trigger();
}


void MainWindow::toggleSlideShow() {
	if (d->mSlideShow->isRunning()) {
		d->mSlideShow->stop();
	} else {
		if (!d->mFullScreenAction->isChecked()) {
			d->mFullScreenAction->trigger();
		}
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
	d->mThumbnailView->generateThumbnailForIndex(index);
}


bool MainWindow::queryClose() {
	saveConfig();
	if (!d->mFullScreenAction->isChecked()) {
		// Do not save config in fullscreen, we don't want to restart without
		// menu bar or toolbars...
		saveMainWindowConfig();
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
		d->mGvCore->saveAll();
		return DocumentFactory::instance()->modifiedDocumentList().size() == 0;

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
	saveMainWindowConfig();
	KEditToolBar dlg(factory(), this);
	connect(&dlg, SIGNAL(newToolBarConfig()), SLOT(loadMainWindowConfig()));
	dlg.exec();
}


void MainWindow::loadMainWindowConfig() {
	KConfigGroup cg = KGlobal::config()->group(MAINWINDOW_SETTINGS);
	applyMainWindowSettings(cg);
}


void MainWindow::saveMainWindowConfig() {
	KConfigGroup cg = KGlobal::config()->group(MAINWINDOW_SETTINGS);
	saveMainWindowSettings(cg);
	KGlobal::config()->sync();
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

	d->mStartPage->applyPalette(palette);

	d->mDocumentView->setNormalPalette(palette);

	d->mThumbnailSlider->setValue(GwenviewConfig::thumbnailSize());
	// If GwenviewConfig::thumbnailSize() returns the current value of
	// mThumbnailSlider, it won't emit valueChanged() and the thumbnail view
	// won't be updated. That's why we do it ourself.
	d->mThumbnailView->setThumbnailSize(GwenviewConfig::thumbnailSize());

	d->mDirModel->setBlackListedExtensions(GwenviewConfig::blackListedExtensions());
}


void MainWindow::saveConfig() {
	d->mDocumentView->saveConfig();
	GwenviewConfig::setThumbnailSize(d->mThumbnailSlider->value());
	GwenviewConfig::setSideBarIsVisible(d->mSideBarContainer->isVisible());
}


void MainWindow::print() {
	if (!currentDocumentIsRasterImage()) {
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

	d->ensureGeometryIsValid();

	QSize size = _size;

	int sideBarWidth = d->mSideBar->isVisible()
		? (d->mSideBar->width() + d->mCentralSplitter->handleWidth())
		: 0;
	// innerMargin is the margin around the view, not including the window
	// frame
	QSize innerMargin = QSize(
		sideBarWidth,
		menuBar()->height() + toolBar()->height() + d->mDocumentView->statusBar()->height());
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

	// This should not be necessary, but sometimes frameGeometry top left
	// corner is outside of availableRect
	int leftFrameMargin = geometry().left() - frameGeometry().left();
	if (windowRect.left() - leftFrameMargin < availableRect.left()) {
		kWarning() << "Window is too much on the left, adjusting";
		windowRect.moveLeft(availableRect.left() + leftFrameMargin);
	}

	int topFrameMargin = geometry().top() - frameGeometry().top();
	if (windowRect.top() - topFrameMargin < availableRect.top()) {
		kWarning() << "Window is too high, adjusting";
		windowRect.moveTop(availableRect.top() + topFrameMargin);
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

	if (d->mViewAction->isChecked()) {
		// If we are in view mode, preload the next url, otherwise preload the
		// selected one
		index = d->mDirModel->sibling(index.row() + 1, index.column(), index);
		if (!index.isValid()) {
			return;
		}
	}

	KFileItem item = d->mDirModel->itemForIndex(index);
	if (!ArchiveUtils::fileItemIsDirOrArchive(item)) {
		KUrl url = item.url();
		if (url.isLocalFile()) {
			QSize size = d->mViewStackedWidget->size();
			d->mPreloader->preload(url, size);
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


} // namespace
