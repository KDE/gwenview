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
#include <QDateTime>
#include <QDesktopWidget>
#include <QTimer>
#include <QShortcut>
#include <QSplitter>
#include <QSlider>
#include <QStackedWidget>
#include <QUndoGroup>
#include <QVBoxLayout>

// KDE
#include <kio/netaccess.h>
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
#include <kactioncategory.h>
#include <ktogglefullscreenaction.h>
#include <ktoolbar.h>
#include <kurl.h>
#include <kurlnavigator.h>
#include <kxmlguifactory.h>
#include <kwindowsystem.h>

// Local
#include "configdialog.h"
#include "contextmanager.h"
#include "documentpanel.h"
#include "fileopscontextmanageritem.h"
#include "fullscreencontent.h"
#include "gvcore.h"
#include "imageopscontextmanageritem.h"
#include "infocontextmanageritem.h"
#ifdef KIPI_FOUND
#include "kipiinterface.h"
#endif
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
#include "semanticinfocontextmanageritem.h"
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
#include <lib/documentview/documentview.h>
#include <lib/fullscreenbar.h>
#include <lib/gwenviewconfig.h>
#include <lib/mimetypeutils.h>
#include <lib/print/printhelper.h>
#include <lib/slideshow.h>
#include <lib/signalblocker.h>
#include <lib/semanticinfo/sorteddirmodel.h>
#include <lib/thumbnailview/thumbnailview.h>
#include <lib/thumbnailloadjob.h>
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
	DocumentPanel* mDocumentPanel;
	KUrlNavigator* mUrlNavigator;
	ThumbnailView* mThumbnailView;
	ThumbnailViewHelper* mThumbnailViewHelper;
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
	KAction* mBrowseAction;
	KAction* mViewAction;
	KAction* mGoUpAction;
	KAction* mGoToPreviousAction;
	KAction* mGoToNextAction;
	KAction* mToggleSideBarAction;
	KToggleFullScreenAction* mFullScreenAction;
	KAction* mToggleSlideShowAction;
	KToggleAction* mShowMenuBarAction;

	SortedDirModel* mDirModel;
	ContextManager* mContextManager;

	MainWindowState mStateBeforeFullScreen;

	KUrl mUrlToSelect;

	QString mCaption;

	QDateTime mFullScreenLeftAt;

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
		setupDocumentPanel(mViewStackedWidget);
		setupStartPage(mViewStackedWidget);
		mViewStackedWidget->addWidget(mThumbnailViewPanel);
		mViewStackedWidget->addWidget(mDocumentPanel);
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
		mUrlNavigator = mThumbnailViewPanel->urlNavigator();

		mThumbnailViewHelper = new ThumbnailViewHelper(mDirModel, mWindow->actionCollection());
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
		connect(delegate, SIGNAL(setDocumentRatingRequested(const KUrl&, int)),
			mGvCore, SLOT(setRating(const KUrl&, int)) );

		// Connect url navigator
		connect(mUrlNavigator, SIGNAL(urlChanged(const KUrl&)),
			mWindow, SLOT(openDirUrl(const KUrl&)) );
	}

	void setupDocumentPanel(QWidget* parent) {
		mDocumentPanel = new DocumentPanel(parent, mWindow->actionCollection());
		connect(mDocumentPanel, SIGNAL(captionUpdateRequested(const QString&)),
			mWindow, SLOT(setCaption(const QString&)) );
		connect(mDocumentPanel, SIGNAL(completed()),
			mWindow, SLOT(slotPartCompleted()) );
		connect(mDocumentPanel, SIGNAL(previousImageRequested()),
			mWindow, SLOT(goToPrevious()) );
		connect(mDocumentPanel, SIGNAL(nextImageRequested()),
			mWindow, SLOT(goToNext()) );

		ThumbnailBarView* bar = mDocumentPanel->thumbnailBar();
		bar->setModel(mDirModel);
		bar->setThumbnailViewHelper(mThumbnailViewHelper);
		bar->setSelectionModel(mThumbnailView->selectionModel());
	}

	void setupStartPage(QWidget* parent) {
		mStartPage = new StartPage(parent, mDirModel->semanticInfoBackEnd());
		connect(mStartPage, SIGNAL(urlSelected(const KUrl&)),
			mWindow, SLOT(slotStartPageUrlSelected(const KUrl&)) );
	}


	void setupActions() {
		KActionCollection* actionCollection = mWindow->actionCollection();
		KActionCategory* file = new KActionCategory(i18nc("@title actions category","File"), actionCollection);
		KActionCategory* view = new KActionCategory(i18nc("@title actions category - means actions changing smth in interface","View"), actionCollection);

		file->addAction(KStandardAction::Save,mWindow, SLOT(saveCurrent()));
		file->addAction(KStandardAction::SaveAs,mWindow, SLOT(saveCurrentAs()));
		file->addAction(KStandardAction::Open,mWindow, SLOT(openFile()));
		file->addAction(KStandardAction::Print,mWindow, SLOT(print()));
		file->addAction(KStandardAction::Quit,KApplication::kApplication(), SLOT(closeAllWindows()));

		KAction* action = file->addAction("reload",mWindow,SLOT(reload()));
		action->setText(i18nc("@action reload the currently viewed image", "Reload"));
		action->setIcon(KIcon("view-refresh"));
		action->setShortcut(Qt::Key_F5);


		mBrowseAction = view->addAction("browse");
		mBrowseAction->setText(i18nc("@action Switch to file list", "Browse"));
		mBrowseAction->setCheckable(true);
		mBrowseAction->setIcon(KIcon("view-list-icons"));

		mViewAction = view->addAction("view");
		mViewAction->setText(i18nc("@action Switch to image view", "View"));
		mViewAction->setIcon(KIcon("view-preview"));
		mViewAction->setCheckable(true);

		mViewModeActionGroup = new QActionGroup(mWindow);
		mViewModeActionGroup->addAction(mBrowseAction);
		mViewModeActionGroup->addAction(mViewAction);

		connect(mViewModeActionGroup, SIGNAL(triggered(QAction*)),
			mWindow, SLOT(setActiveViewModeAction(QAction*)) );

		mFullScreenAction = static_cast<KToggleFullScreenAction*>(view->addAction(KStandardAction::FullScreen,mWindow, SLOT(toggleFullScreen(bool))));
		connect(mDocumentPanel, SIGNAL(toggleFullScreenRequested()),
			mFullScreenAction, SLOT(trigger()) );


		QShortcut* reduceLevelOfDetailsShortcut = new QShortcut(mWindow);
		reduceLevelOfDetailsShortcut->setKey(Qt::Key_Escape);
		connect(reduceLevelOfDetailsShortcut, SIGNAL(activated()),
			mWindow, SLOT(reduceLevelOfDetails()) );

		mGoToPreviousAction = view->addAction("go_previous",mWindow, SLOT(goToPrevious()));
		mGoToPreviousAction->setIcon(KIcon("media-skip-backward"));
		mGoToPreviousAction->setText(i18nc("@action Go to previous image", "Previous"));
		mGoToPreviousAction->setToolTip(i18n("Go to Previous Image"));
		mGoToPreviousAction->setShortcut(Qt::Key_Backspace);

		mGoToNextAction = view->addAction("go_next",mWindow, SLOT(goToNext()));
		mGoToNextAction->setIcon(KIcon("media-skip-forward"));
		mGoToNextAction->setText(i18nc("@action Go to next image", "Next"));
		mGoToNextAction->setToolTip(i18n("Go to Next Image"));
		mGoToNextAction->setShortcut(Qt::Key_Space);


		mGoUpAction = view->addAction(KStandardAction::Up,mWindow, SLOT(goUp()));

		action = view->addAction("go_start_page",mWindow, SLOT(showStartPage()));
		action->setIcon(KIcon("go-home"));
		action->setText(i18nc("@action", "Start Page"));

		mToggleSideBarAction = view->addAction("toggle_sidebar",mWindow, SLOT(toggleSideBar()));
		mToggleSideBarAction->setIcon(KIcon("view-sidetree"));
		mToggleSideBarAction->setShortcut(Qt::Key_F11);

		mToggleSlideShowAction = view->addAction("toggle_slideshow",mWindow, SLOT(toggleSlideShow()));
		mWindow->updateSlideShowAction();
		connect(mSlideShow, SIGNAL(stateChanged(bool)),
			mWindow, SLOT(updateSlideShowAction()) );

		mShowMenuBarAction = static_cast<KToggleAction*>(view->addAction(KStandardAction::ShowMenubar,mWindow, SLOT(toggleMenuBar())));

		view->addAction(KStandardAction::KeyBindings,mWindow->guiFactory(),
			SLOT(configureShortcuts()));

		view->addAction(KStandardAction::Preferences,mWindow,
			SLOT(showConfigDialog()));

		view->addAction(KStandardAction::ConfigureToolbars,mWindow,
			SLOT(configureToolbars()));
	}

	void setupUndoActions() {
		// There is no KUndoGroup similar to KUndoStack. This code basically
		// does the same as KUndoStack, but for the KUndoGroup actions.
		QUndoGroup* undoGroup = DocumentFactory::instance()->undoGroup();
		QAction* action;
		KActionCollection* actionCollection =  mWindow->actionCollection();
		KActionCategory* edit = new KActionCategory(i18nc("@title actions category - means actions changing smth in interface","Edit"), actionCollection);

		action = undoGroup->createRedoAction(actionCollection);
		action->setObjectName(KStandardAction::name(KStandardAction::Redo));
		action->setIcon(KIcon("edit-redo"));
		action->setIconText(i18n("Redo"));
		action->setShortcuts(KStandardShortcut::redo());
		edit->addAction(action->objectName(), action);

		action = undoGroup->createUndoAction(actionCollection);
		action->setObjectName(KStandardAction::name(KStandardAction::Undo));
		action->setIcon(KIcon("edit-undo"));
		action->setIconText(i18n("Undo"));
		action->setShortcuts(KStandardShortcut::undo());
		edit->addAction(action->objectName(), action);
	}


	void setupContextManager() {
		KActionCollection* actionCollection = mWindow->actionCollection();

		mContextManager = new ContextManager(mWindow);
		mContextManager->setSideBar(mSideBar);
		mContextManager->setDirModel(mDirModel);

		mContextManager->addItem(new InfoContextManagerItem(mContextManager));

		#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
		mContextManager->addItem(new SemanticInfoContextManagerItem(mContextManager, actionCollection, mDocumentPanel->documentView()));
		#endif

		mContextManager->addItem(new ImageOpsContextManagerItem(mContextManager, mWindow));

		FileOpsContextManagerItem* fileOpsItem = new FileOpsContextManagerItem(mContextManager, actionCollection);
		mContextManager->addItem(fileOpsItem);
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
		if (!url.isValid()) {
			return QModelIndex();
		}
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
		if (url.isValid()) {
			mUrlNavigator->setUrl(url);
			mGoUpAction->setEnabled(url.path() != "/");
			mGvCore->addUrlToRecentFolders(url);
		} else {
			mGoUpAction->setEnabled(false);
		}
	}

	void setupFullScreenBar() {
		mFullScreenBar = new FullScreenBar(mDocumentPanel);
		mFullScreenContent = new FullScreenContent(
			mFullScreenBar, mWindow->actionCollection(), mSlideShow);

		ThumbnailBarView* view = mFullScreenContent->thumbnailBar();
		view->setModel(mDirModel);
		view->setThumbnailViewHelper(mThumbnailViewHelper);
		view->setSelectionModel(mThumbnailView->selectionModel());

		mFullScreenBar->adjustSize();
	}


	inline void setActionEnabled(const char* name, bool enabled) {
		QAction* action = mWindow->actionCollection()->action(name);
		if (action) {
			action->setEnabled(enabled);
		} else {
			kWarning() << "Action" << name << "not found";
		}
	}


	void setActionsDisabledOnStartPageEnabled(bool enabled) {
		mBrowseAction->setEnabled(enabled);
		mViewAction->setEnabled(enabled);
		mToggleSideBarAction->setEnabled(enabled);
		mFullScreenAction->setEnabled(enabled);
		mToggleSlideShowAction->setEnabled(enabled);

		setActionEnabled("reload", enabled);
		setActionEnabled("go_start_page", enabled);
		setActionEnabled("add_folder_to_places", enabled);
	}


	void updateActions() {
		bool isRasterImage = false;
		bool canSave = false;
		bool isModified = false;
		const KUrl url = currentUrl();

		if (url.isValid()) {
			isRasterImage = mWindow->currentDocumentIsRasterImage();
			canSave = isRasterImage;
			isModified = DocumentFactory::instance()->load(url)->isModified();
			if (!mViewAction->isChecked()) {
				// Saving only makes sense if exactly one image is selected
				QItemSelection selection = mThumbnailView->selectionModel()->selection();
				QModelIndexList indexList = selection.indexes();
				if (indexList.count() != 1) {
					canSave = false;
				}
			}
		}

		KActionCollection* actionCollection = mWindow->actionCollection();
		actionCollection->action("file_save")->setEnabled(canSave && isModified);
		actionCollection->action("file_save_as")->setEnabled(canSave);
		actionCollection->action("file_print")->setEnabled(isRasterImage);
	}

	KUrl currentUrl() const {
		if (mStartPage->isVisible()) {
			return KUrl();
		}
		if (mViewAction->isChecked() && !mDocumentPanel->isEmpty()) {
			return mDocumentPanel->url();
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

	void updateContextDependentComponents() {
		// Gather info
		KFileItemList selectedItemList;
		QItemSelection selection = mThumbnailView->selectionModel()->selection();

		Q_FOREACH(const QModelIndex& index, selection.indexes()) {
			selectedItemList << mDirModel->itemForIndex(index);
		}

		// At least add current url if it's valid (it may not be in
		// selectedItemList if we are viewing a non-browsable url, for example
		// using http protocol)
		const KUrl url = currentUrl();
		if (selectedItemList.isEmpty() && url.isValid()) {
			KFileItem item(KFileItem::Unknown, KFileItem::Unknown, url);
			selectedItemList << item;
		}

		// Update
		mContextManager->setContext(url, selectedItemList);
		mSaveBar->setCurrentUrl(url);
		mSlideShow->setCurrentUrl(url);
	}
};


MainWindow::MainWindow()
: KXmlGuiWindow(),
d(new MainWindow::Private)
{
	d->mWindow = this;
	d->mDirModel = new SortedDirModel(this);
	d->mGvCore = new GvCore(this, d->mDirModel);
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

	createGUI();
	loadConfig();
	connect(DocumentFactory::instance(), SIGNAL(documentChanged(const KUrl&)),
		SLOT(generateThumbnailForUrl(const KUrl&)) );

	connect(DocumentFactory::instance(), SIGNAL(modifiedDocumentListChanged()),
		SLOT(slotModifiedDocumentListChanged()) );

#ifdef KIPI_FOUND
	d->mKIPIInterface = new KIPIInterface(this);
#endif
	setAutoSaveSettings();
}


MainWindow::~MainWindow() {
	delete d;
}

ContextManager* MainWindow::contextManager() const {
	return d->mContextManager;
}

DocumentPanel* MainWindow::documentPanel() const {
	return d->mDocumentPanel;
}


bool MainWindow::currentDocumentIsRasterImage() const {
	if (d->mViewAction->isChecked()) {
		Document::Ptr doc = d->mDocumentPanel->currentDocument();
		if (!doc) {
			return false;
		}
		return doc->kind() == MimeTypeUtils::KIND_RASTER_IMAGE;
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
	// Keep a trace of caption to use it in slotModifiedDocumentListChanged()
	d->mCaption = caption;
	KXmlGuiWindow::setCaption(caption);
}

void MainWindow::setCaption(const QString& caption, bool modified) {
	d->mCaption = caption;
	KXmlGuiWindow::setCaption(caption, modified);
}


void MainWindow::slotModifiedDocumentListChanged() {
	d->updateActions();

	// Update caption
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
		d->mViewStackedWidget->setCurrentWidget(d->mDocumentPanel);
		if (d->mDocumentPanel->isEmpty()) {
			openSelectedDocument();
		}
	} else {
		// Switching to browse mode
		d->mViewStackedWidget->setCurrentWidget(d->mThumbnailViewPanel);
		if (!d->mDocumentPanel->isEmpty()
			&& KProtocolManager::supportsListing(d->mDocumentPanel->url()))
		{
			// Reset the view to spare resources, but don't do it if we can't
			// browse the url, otherwise if the user starts Gwenview this way:
			// gwenview http://example.com/example.png
			// and switch to browse mode, switching back to view mode won't bring
			// his image back.
			d->mDocumentPanel->reset();
		}
		d->mDirModel->setMimeExcludeFilter(QStringList());
		setCaption(QString());
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
	if (!d->mViewAction->isChecked()) {
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
	d->setActionsDisabledOnStartPageEnabled(false);
	d->spreadCurrentDirUrl(KUrl());

	d->mSideBarContainer->hide();
	d->updateToggleSideBarAction();
	d->mViewStackedWidget->setCurrentWidget(d->mStartPage);

	d->updateActions();
	updatePreviousNextActions();
	d->updateContextDependentComponents();
}


void MainWindow::slotStartPageUrlSelected(const KUrl& url) {
	d->setActionsDisabledOnStartPageEnabled(true);

	if (d->mBrowseAction->isChecked()) {
		// Silently uncheck the action so that setInitialUrl() does the right thing
		SignalBlocker blocker(d->mBrowseAction);
		d->mBrowseAction->setChecked(false);
	}

	setInitialUrl(url);
}


void MainWindow::openDirUrl(const KUrl& url) {
	const KUrl currentUrl = d->mContextManager->currentDirUrl();

	if (url.equals(currentUrl, KUrl::CompareWithoutTrailingSlash)) {
		return;
	}

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
	d->mDocumentPanel->reset();
}


void MainWindow::openDocumentUrl(const KUrl& url) {
	if (d->mDocumentPanel->url() == url) {
		return;
	}
	d->mDocumentPanel->openUrl(url);

	d->mFullScreenContent->setCurrentUrl(url);

	Document::Ptr doc = DocumentFactory::instance()->load(url);
	QUndoGroup* undoGroup = DocumentFactory::instance()->undoGroup();
	undoGroup->addStack(doc->undoStack());
	undoGroup->setActiveStack(doc->undoStack());

	d->mUrlToSelect = url;
	d->selectUrlToSelect();
}


void MainWindow::toggleSideBar() {
	d->mSideBarContainer->setVisible(!d->mSideBarContainer->isVisible());
	d->updateToggleSideBarAction();
}


void MainWindow::slotPartCompleted() {
	d->updateActions();
	KUrl url = d->mDocumentPanel->url();
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
	if (d->mViewAction->isChecked()) {
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
	if (!d->mDocumentPanel->isEmpty()) {
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
	if (d->mViewAction->isChecked()) {
		openDocumentUrl(url);
	}
	KUrl dirUrl = url;
	dirUrl.setFileName("");
	if (!dirUrl.equals(d->mContextManager->currentDirUrl(), KUrl::CompareWithoutTrailingSlash)) {
		d->mDirModel->dirLister()->openUrl(dirUrl);
		d->spreadCurrentDirUrl(dirUrl);
	}
	d->mUrlToSelect = url;
	d->selectUrlToSelect();
}


void MainWindow::updatePreviousNextActions() {
	QModelIndex index;

	index = d->getRelativeIndex(-1);
	d->mGoToPreviousAction->setEnabled(index.isValid());

	index = d->getRelativeIndex(1);
	d->mGoToNextAction->setEnabled(index.isValid());
}


void MainWindow::reduceLevelOfDetails() {
	if (d->mFullScreenAction->isChecked()) {
		d->mFullScreenAction->trigger();
	} else if (d->mViewAction->isChecked()) {
		d->mBrowseAction->trigger();
	}
}


void MainWindow::toggleFullScreen(bool checked) {
	setUpdatesEnabled(false);
	if (checked) {
		// Save MainWindow config now, this way if we quit while in
		// fullscreen, we are sure latest MainWindow changes are remembered.
		saveMainWindowSettings(autoSaveConfigGroup());
		resetAutoSaveSettings();

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
		d->mDocumentPanel->setFullScreenMode(true);
		d->mSaveBar->setFullScreenMode(true);
		d->mFullScreenBar->setActivated(true);
	} else {
		setAutoSaveSettings();
		if (d->mStateBeforeFullScreen.mActiveViewModeAction) {
			d->mStateBeforeFullScreen.mActiveViewModeAction->trigger();
		}
		d->mSideBarContainer->setVisible(d->mStateBeforeFullScreen.mSideBarVisible);

		// Back to normal
		d->mDocumentPanel->setFullScreenMode(false);
		d->mSlideShow->stop();
		d->mSaveBar->setFullScreenMode(false);
		d->mFullScreenBar->setActivated(false);
		setWindowState(d->mStateBeforeFullScreen.mWindowState);
		menuBar()->setVisible(d->mShowMenuBarAction->isChecked());
		toolBar()->setVisible(d->mStateBeforeFullScreen.mToolBarVisible);

		// See resizeEvent
		d->mFullScreenLeftAt = QDateTime::currentDateTime();
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
	if (d->mViewAction->isChecked()) {
		d->mDocumentPanel->reload();
	} else {
		d->mDirModel->reload();
	}
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
	QList<KUrl> list = DocumentFactory::instance()->modifiedDocumentList();
	if (list.size() == 0) {
		return true;
	}

	KGuiItem yes(i18n("Save All Changes"), "document-save-all");
	KGuiItem no(i18n("Discard Changes"));
	QString msg = i18np("One image has been modified.", "%1 images have been modified.", list.size())
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


bool MainWindow::queryExit() {
	if (GwenviewConfig::deleteThumbnailCacheOnExit()) {
		const QString dir = ThumbnailLoadJob::thumbnailBaseDir();
		if (QFile::exists(dir)) {
			KIO::NetAccess::del(KUrl::fromPath(dir), this);
		}
	}
	return true;
}


void MainWindow::showConfigDialog() {
	ConfigDialog dialog(this);
	connect(&dialog, SIGNAL(settingsChanged(const QString&)), SLOT(loadConfig()));
	dialog.exec();
}


void MainWindow::toggleMenuBar() {
	if (!d->mFullScreenAction->isChecked()) {
		menuBar()->setVisible(d->mShowMenuBarAction->isChecked());
	}
}


void MainWindow::loadConfig() {
	QColor bgColor = GwenviewConfig::viewBackgroundColor();
	QColor fgColor = bgColor.value() > 128 ? Qt::black : Qt::white;

	QPalette pal = palette();
	pal.setColor(QPalette::Base, bgColor);
	pal.setColor(QPalette::Text, fgColor);

	d->mThumbnailViewPanel->applyPalette(pal);
	d->mStartPage->applyPalette(pal);
	d->mDocumentPanel->setNormalPalette(pal);

	d->mDirModel->setBlackListedExtensions(GwenviewConfig::blackListedExtensions());

	d->mDocumentPanel->loadConfig();
	d->mThumbnailViewPanel->loadConfig();
}


void MainWindow::saveConfig() {
	d->mDocumentPanel->saveConfig();
	d->mThumbnailViewPanel->saveConfig();
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
	KXmlGuiWindow::showEvent(event);
}


void MainWindow::resizeEvent(QResizeEvent* event) {
	KXmlGuiWindow::resizeEvent(event);
	// This is a small hack to execute code after leaving fullscreen, and after
	// the window has been resized back to its former size.
	if (d->mFullScreenLeftAt.isValid() && d->mFullScreenLeftAt.secsTo(QDateTime::currentDateTime()) < 2) {
		if (d->mBrowseAction->isChecked()) {
			d->mThumbnailView->scrollToSelectedIndex();
		}
		d->mFullScreenLeftAt = QDateTime();
	}
}


} // namespace
