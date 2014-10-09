/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
#include "mainwindow.h"
#include <config-gwenview.h>

// Qt
#include <QApplication>
#include <QDateTime>
#include <QPushButton>
#include <QShortcut>
#include <QSplitter>
#include <QStackedWidget>
#include <QTimer>
#include <QUndoGroup>
#include <QVBoxLayout>
#include <QMenuBar>


// KDE
#include <KIO/NetAccess>
#include <KActionCategory>
#include <KActionCollection>
#include <QAction>
#include <KApplication>
#include <KFileDialog>
#include <KFileItem>
#include <KLocale>
#include <KMessageBox>
#include <KNotificationRestrictions>
#include <KProtocolManager>
#include <KLinkItemSelectionModel>
#include <KRecentFilesAction>
#include <KStandardShortcut>
#include <KToggleFullScreenAction>
#include <QUrl>
#include <KUrlNavigator>
#include <KToolBar>
#include <KXMLGUIFactory>
#include <KDirLister>

// Local
#include "configdialog.h"
#include "documentinfoprovider.h"
#include "viewmainpage.h"
#include "fileopscontextmanageritem.h"
#include "folderviewcontextmanageritem.h"
#include "fullscreencontent.h"
#include "gvcore.h"
#include "imageopscontextmanageritem.h"
#include "infocontextmanageritem.h"
#ifdef KIPI_FOUND
#include "kipiexportaction.h"
#include "kipiinterface.h"
#endif
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
#include "semanticinfocontextmanageritem.h"
#endif
#include "preloader.h"
#include "savebar.h"
#include "sidebar.h"
#include "splitter.h"
#include "startmainpage.h"
#include "thumbnailviewhelper.h"
#include "browsemainpage.h"
#include <lib/hud/hudbuttonbox.h>
#include <lib/archiveutils.h>
#include <lib/contextmanager.h>
#include <lib/disabledactionshortcutmonitor.h>
#include <lib/document/documentfactory.h>
#include <lib/documentonlyproxymodel.h>
#include <lib/eventwatcher.h>
#include <lib/gvdebug.h>
#include <lib/gwenviewconfig.h>
#include <lib/mimetypeutils.h>
#include <lib/print/printhelper.h>
#include <lib/slideshow.h>
#include <lib/signalblocker.h>
#include <lib/semanticinfo/sorteddirmodel.h>
#include <lib/thumbnailprovider/thumbnailprovider.h>
#include <lib/thumbnailview/thumbnailbarview.h>
#include <lib/thumbnailview/thumbnailview.h>
#include <lib/urlutils.h>

namespace Gwenview
{

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) qDebug() << x
#else
#define LOG(x) ;
#endif

static const int BROWSE_PRELOAD_DELAY = 1000;
static const int VIEW_PRELOAD_DELAY = 100;

static const char* BROWSE_MODE_SIDE_BAR_GROUP = "SideBar-BrowseMode";
static const char* VIEW_MODE_SIDE_BAR_GROUP = "SideBar-ViewMode";
static const char* FULLSCREEN_MODE_SIDE_BAR_GROUP = "SideBar-FullScreenMode";
static const char* SIDE_BAR_IS_VISIBLE_KEY = "IsVisible";

static const char* SESSION_CURRENT_PAGE_KEY = "Page";
static const char* SESSION_URL_KEY = "Url";

enum MainPageId {
    StartMainPageId,
    BrowseMainPageId,
    ViewMainPageId
};

struct MainWindowState
{
    bool mToolBarVisible;
    Qt::WindowStates mWindowState;
};

/*

Layout of the main window looks like this:

.-mCentralSplitter-----------------------------.
|.-mSideBar--. .-mContentWidget---------------.|
||           | |.-mSaveBar-------------------.||
||           | ||                            |||
||           | |'----------------------------'||
||           | |.-mViewStackedWidget---------.||
||           | ||                            |||
||           | ||                            |||
||           | ||                            |||
||           | ||                            |||
||           | |'----------------------------'||
|'-----------' '------------------------------'|
'----------------------------------------------'

*/
struct MainWindow::Private
{
    GvCore* mGvCore;
    MainWindow* q;
    QSplitter* mCentralSplitter;
    QWidget* mContentWidget;
    ViewMainPage* mViewMainPage;
    KUrlNavigator* mUrlNavigator;
    ThumbnailView* mThumbnailView;
    ThumbnailView* mActiveThumbnailView;
    DocumentInfoProvider* mDocumentInfoProvider;
    ThumbnailViewHelper* mThumbnailViewHelper;
    QPointer<ThumbnailProvider> mThumbnailProvider;
    BrowseMainPage* mBrowseMainPage;
    StartMainPage* mStartMainPage;
    SideBar* mSideBar;
    QStackedWidget* mViewStackedWidget;
    FullScreenContent* mFullScreenContent;
    SaveBar* mSaveBar;
    bool mStartSlideShowWhenDirListerCompleted;
    SlideShow* mSlideShow;
    Preloader* mPreloader;
    bool mPreloadDirectionIsForward;
#ifdef KIPI_FOUND
    KIPIInterface* mKIPIInterface;
#endif

    QActionGroup* mViewModeActionGroup;
    KRecentFilesAction* mFileOpenRecentAction;
    QAction * mBrowseAction;
    QAction * mViewAction;
    QAction * mGoUpAction;
    QAction * mGoToPreviousAction;
    QAction * mGoToNextAction;
    QAction * mGoToFirstAction;
    QAction * mGoToLastAction;
    KToggleAction* mToggleSideBarAction;
    KToggleFullScreenAction* mFullScreenAction;
    QAction * mToggleSlideShowAction;
    KToggleAction* mShowMenuBarAction;
#ifdef KIPI_FOUND
    KIPIExportAction* mKIPIExportAction;
#endif

    SortedDirModel* mDirModel;
    DocumentOnlyProxyModel* mThumbnailBarModel;
    KLinkItemSelectionModel* mThumbnailBarSelectionModel;
    ContextManager* mContextManager;

    MainWindowState mStateBeforeFullScreen;

    QString mCaption;

    MainPageId mCurrentMainPageId;

    QDateTime mFullScreenLeftAt;
    KNotificationRestrictions* mNotificationRestrictions;

    void setupContextManager()
    {
        mContextManager = new ContextManager(mDirModel, q);
        connect(mContextManager, SIGNAL(selectionChanged()),
            q, SLOT(slotSelectionChanged()));
        connect(mContextManager, SIGNAL(currentDirUrlChanged(QUrl)),
            q, SLOT(slotCurrentDirUrlChanged(QUrl)));
    }

    void setupWidgets()
    {
        mFullScreenContent = new FullScreenContent(q);
        connect(mContextManager, SIGNAL(currentUrlChanged(QUrl)), mFullScreenContent, SLOT(setCurrentUrl(QUrl)));

        mCentralSplitter = new Splitter(Qt::Horizontal, q);
        q->setCentralWidget(mCentralSplitter);

        // Left side of splitter
        mSideBar = new SideBar(mCentralSplitter);
        EventWatcher::install(mSideBar, QList<QEvent::Type>() << QEvent::Show << QEvent::Hide,
                              q, SLOT(updateToggleSideBarAction()));

        // Right side of splitter
        mContentWidget = new QWidget(mCentralSplitter);

        mSaveBar = new SaveBar(mContentWidget, q->actionCollection());
        connect(mContextManager, SIGNAL(currentUrlChanged(QUrl)), mSaveBar, SLOT(setCurrentUrl(QUrl)));
        mViewStackedWidget = new QStackedWidget(mContentWidget);
        QVBoxLayout* layout = new QVBoxLayout(mContentWidget);
        layout->addWidget(mSaveBar);
        layout->addWidget(mViewStackedWidget);
        layout->setMargin(0);
        layout->setSpacing(0);
        ////

        mStartSlideShowWhenDirListerCompleted = false;
        mSlideShow = new SlideShow(q);
        connect(mContextManager, SIGNAL(currentUrlChanged(QUrl)), mSlideShow, SLOT(setCurrentUrl(QUrl)));

        setupThumbnailView(mViewStackedWidget);
        setupViewMainPage(mViewStackedWidget);
        setupStartMainPage(mViewStackedWidget);
        mViewStackedWidget->addWidget(mBrowseMainPage);
        mViewStackedWidget->addWidget(mViewMainPage);
        mViewStackedWidget->addWidget(mStartMainPage);
        mViewStackedWidget->setCurrentWidget(mBrowseMainPage);

        mCentralSplitter->setStretchFactor(0, 0);
        mCentralSplitter->setStretchFactor(1, 1);

        connect(mSaveBar, SIGNAL(requestSaveAll()),
                mGvCore, SLOT(saveAll()));
        connect(mSaveBar, SIGNAL(goToUrl(QUrl)),
                q, SLOT(goToUrl(QUrl)));

        connect(mSlideShow, SIGNAL(goToUrl(QUrl)),
                q, SLOT(goToUrl(QUrl)));
    }

    void setupThumbnailView(QWidget* parent)
    {
        Q_ASSERT(mContextManager);
        mBrowseMainPage = new BrowseMainPage(parent, q->actionCollection(), mGvCore);

        mThumbnailView = mBrowseMainPage->thumbnailView();
        mThumbnailView->setSelectionModel(mContextManager->selectionModel());
        mUrlNavigator = mBrowseMainPage->urlNavigator();

        mDocumentInfoProvider = new DocumentInfoProvider(mDirModel);
        mThumbnailView->setDocumentInfoProvider(mDocumentInfoProvider);

        mThumbnailViewHelper = new ThumbnailViewHelper(mDirModel, q->actionCollection());
        connect(mContextManager, SIGNAL(currentDirUrlChanged(QUrl)),
            mThumbnailViewHelper, SLOT(setCurrentDirUrl(QUrl)));
        mThumbnailView->setThumbnailViewHelper(mThumbnailViewHelper);

        mThumbnailBarSelectionModel = new KLinkItemSelectionModel(mThumbnailBarModel, mContextManager->selectionModel(), q);

        // Connect thumbnail view
        connect(mThumbnailView, SIGNAL(indexActivated(QModelIndex)),
                q, SLOT(slotThumbnailViewIndexActivated(QModelIndex)));

        // Connect delegate
        QAbstractItemDelegate* delegate = mThumbnailView->itemDelegate();
        connect(delegate, SIGNAL(saveDocumentRequested(QUrl)),
                mGvCore, SLOT(save(QUrl)));
        connect(delegate, SIGNAL(rotateDocumentLeftRequested(QUrl)),
                mGvCore, SLOT(rotateLeft(QUrl)));
        connect(delegate, SIGNAL(rotateDocumentRightRequested(QUrl)),
                mGvCore, SLOT(rotateRight(QUrl)));
        connect(delegate, SIGNAL(showDocumentInFullScreenRequested(QUrl)),
                q, SLOT(showDocumentInFullScreen(QUrl)));
        connect(delegate, SIGNAL(setDocumentRatingRequested(QUrl,int)),
                mGvCore, SLOT(setRating(QUrl,int)));

        // Connect url navigator
        connect(mUrlNavigator, SIGNAL(urlChanged(QUrl)),
                q, SLOT(openDirUrl(QUrl)));
    }

    void setupViewMainPage(QWidget* parent)
    {
        mViewMainPage = new ViewMainPage(parent, mSlideShow, q->actionCollection(), mGvCore);
        connect(mViewMainPage, SIGNAL(captionUpdateRequested(QString)),
                q, SLOT(setWindowTitle(QString)));
        connect(mViewMainPage, SIGNAL(completed()),
                q, SLOT(slotPartCompleted()));
        connect(mViewMainPage, SIGNAL(previousImageRequested()),
                q, SLOT(goToPrevious()));
        connect(mViewMainPage, SIGNAL(nextImageRequested()),
                q, SLOT(goToNext()));

        setupThumbnailBar(mViewMainPage->thumbnailBar());
    }

    void setupThumbnailBar(ThumbnailView* bar)
    {
        Q_ASSERT(mThumbnailBarModel);
        Q_ASSERT(mThumbnailBarSelectionModel);
        Q_ASSERT(mDocumentInfoProvider);
        Q_ASSERT(mThumbnailViewHelper);
        bar->setModel(mThumbnailBarModel);
        bar->setSelectionModel(mThumbnailBarSelectionModel);
        bar->setDocumentInfoProvider(mDocumentInfoProvider);
        bar->setThumbnailViewHelper(mThumbnailViewHelper);
    }

    void setupStartMainPage(QWidget* parent)
    {
        mStartMainPage = new StartMainPage(parent, mGvCore);
        connect(mStartMainPage, SIGNAL(urlSelected(QUrl)),
                q, SLOT(slotStartMainPageUrlSelected(QUrl)));
    }

    void installDisabledActionShortcutMonitor(QAction* action, const char* slot)
    {
        DisabledActionShortcutMonitor* monitor = new DisabledActionShortcutMonitor(action, q);
        connect(monitor, SIGNAL(activated()), q, slot);
    }

    void setupActions()
    {
        KActionCollection* actionCollection = q->actionCollection();
        KActionCategory* file = new KActionCategory(i18nc("@title actions category", "File"), actionCollection);
        KActionCategory* view = new KActionCategory(i18nc("@title actions category - means actions changing smth in interface", "View"), actionCollection);

        file->addAction(KStandardAction::Save, q, SLOT(saveCurrent()));
        file->addAction(KStandardAction::SaveAs, q, SLOT(saveCurrentAs()));
        file->addAction(KStandardAction::Open, q, SLOT(openFile()));
        mFileOpenRecentAction = KStandardAction::openRecent(q, SLOT(openUrl(QUrl)), q);
        file->addAction("file_open_recent", mFileOpenRecentAction);
        file->addAction(KStandardAction::Print, q, SLOT(print()));
        file->addAction(KStandardAction::Quit, KApplication::kApplication(), SLOT(closeAllWindows()));

        QAction * action = file->addAction("reload", q, SLOT(reload()));
        action->setText(i18nc("@action reload the currently viewed image", "Reload"));
        action->setIcon(QIcon::fromTheme("view-refresh"));
        action->setShortcut(Qt::Key_F5);

        mBrowseAction = view->addAction("browse");
        mBrowseAction->setText(i18nc("@action:intoolbar Switch to file list", "Browse"));
        mBrowseAction->setToolTip(i18nc("@info:tooltip", "Browse folders for images"));
        mBrowseAction->setCheckable(true);
        mBrowseAction->setIcon(QIcon::fromTheme("view-list-icons"));
        mBrowseAction->setShortcut(Qt::Key_Escape);
        connect(mViewMainPage, SIGNAL(goToBrowseModeRequested()),
            mBrowseAction, SLOT(trigger()));

        mViewAction = view->addAction("view");
        mViewAction->setText(i18nc("@action:intoolbar Switch to image view", "View"));
        mViewAction->setToolTip(i18nc("@info:tooltip", "View selected images"));
        mViewAction->setIcon(QIcon::fromTheme("view-preview"));
        mViewAction->setCheckable(true);

        mViewModeActionGroup = new QActionGroup(q);
        mViewModeActionGroup->addAction(mBrowseAction);
        mViewModeActionGroup->addAction(mViewAction);

        connect(mViewModeActionGroup, SIGNAL(triggered(QAction*)),
                q, SLOT(setActiveViewModeAction(QAction*)));

        mFullScreenAction = static_cast<KToggleFullScreenAction*>(view->addAction(KStandardAction::FullScreen, q, SLOT(toggleFullScreen(bool))));
        QList<QKeySequence> shortcuts = mFullScreenAction->shortcuts();
        shortcuts.append(QKeySequence(Qt::Key_F11));
        mFullScreenAction->setShortcuts(shortcuts);

        connect(mViewMainPage, SIGNAL(toggleFullScreenRequested()),
                mFullScreenAction, SLOT(trigger()));

        QAction * leaveFullScreenAction = view->addAction("leave_fullscreen", q, SLOT(leaveFullScreen()));
        leaveFullScreenAction->setIcon(QIcon::fromTheme("view-restore"));
        leaveFullScreenAction->setPriority(QAction::LowPriority);
        leaveFullScreenAction->setText(i18nc("@action", "Leave Fullscreen Mode"));

        mGoToPreviousAction = view->addAction("go_previous", q, SLOT(goToPrevious()));
        mGoToPreviousAction->setPriority(QAction::LowPriority);
        mGoToPreviousAction->setIcon(QIcon::fromTheme("media-skip-backward"));
        mGoToPreviousAction->setText(i18nc("@action Go to previous image", "Previous"));
        mGoToPreviousAction->setToolTip(i18nc("@info:tooltip", "Go to previous image"));
        mGoToPreviousAction->setShortcut(Qt::Key_Backspace);
        installDisabledActionShortcutMonitor(mGoToPreviousAction, SLOT(showFirstDocumentReached()));

        mGoToNextAction = view->addAction("go_next", q, SLOT(goToNext()));
        mGoToNextAction->setPriority(QAction::LowPriority);
        mGoToNextAction->setIcon(QIcon::fromTheme("media-skip-forward"));
        mGoToNextAction->setText(i18nc("@action Go to next image", "Next"));
        mGoToNextAction->setToolTip(i18nc("@info:tooltip", "Go to next image"));
        mGoToNextAction->setShortcut(Qt::Key_Space);
        installDisabledActionShortcutMonitor(mGoToNextAction, SLOT(showLastDocumentReached()));

        mGoToFirstAction = view->addAction("go_first", q, SLOT(goToFirst()));
        mGoToFirstAction->setPriority(QAction::LowPriority);
        mGoToFirstAction->setText(i18nc("@action Go to first image", "First"));
        mGoToFirstAction->setToolTip(i18nc("@info:tooltip", "Go to first image"));
        mGoToFirstAction->setShortcut(Qt::Key_Home);

        mGoToLastAction = view->addAction("go_last", q, SLOT(goToLast()));
        mGoToLastAction->setPriority(QAction::LowPriority);
        mGoToLastAction->setText(i18nc("@action Go to last image", "Last"));
        mGoToLastAction->setToolTip(i18nc("@info:tooltip", "Go to last image"));
        mGoToLastAction->setShortcut(Qt::Key_End);

        mPreloadDirectionIsForward = true;

        mGoUpAction = view->addAction(KStandardAction::Up, q, SLOT(goUp()));

        action = view->addAction("go_start_page", q, SLOT(showStartMainPage()));
        action->setPriority(QAction::LowPriority);
        action->setIcon(QIcon::fromTheme("go-home"));
        action->setText(i18nc("@action", "Start Page"));
        action->setToolTip(i18nc("@info:tooltip", "Open the start page"));

        mToggleSideBarAction = view->add<KToggleAction>("toggle_sidebar");
        connect(mToggleSideBarAction, SIGNAL(toggled(bool)),
                q, SLOT(toggleSideBar(bool)));
        mToggleSideBarAction->setIcon(QIcon::fromTheme("view-sidetree"));
        mToggleSideBarAction->setShortcut(Qt::Key_F4);
        mToggleSideBarAction->setText(i18nc("@action", "Sidebar"));
        connect(mBrowseMainPage->toggleSideBarButton(), SIGNAL(clicked()),
                mToggleSideBarAction, SLOT(trigger()));
        connect(mViewMainPage->toggleSideBarButton(), SIGNAL(clicked()),
                mToggleSideBarAction, SLOT(trigger()));

        mToggleSlideShowAction = view->addAction("toggle_slideshow", q, SLOT(toggleSlideShow()));
        q->updateSlideShowAction();
        connect(mSlideShow, SIGNAL(stateChanged(bool)),
                q, SLOT(updateSlideShowAction()));

        mShowMenuBarAction = static_cast<KToggleAction*>(view->addAction(KStandardAction::ShowMenubar, q, SLOT(toggleMenuBar())));

        view->addAction(KStandardAction::KeyBindings, q->guiFactory(),
                        SLOT(configureShortcuts()));

        view->addAction(KStandardAction::Preferences, q,
                        SLOT(showConfigDialog()));

        view->addAction(KStandardAction::ConfigureToolbars, q,
                        SLOT(configureToolbars()));

#ifdef KIPI_FOUND
        mKIPIExportAction = new KIPIExportAction(q);
        actionCollection->addAction("kipi_export", mKIPIExportAction);
#endif
    }

    void setupUndoActions()
    {
        // There is no KUndoGroup similar to KUndoStack. This code basically
        // does the same as KUndoStack, but for the KUndoGroup actions.
        QUndoGroup* undoGroup = DocumentFactory::instance()->undoGroup();
        QAction* action;
        KActionCollection* actionCollection =  q->actionCollection();
        KActionCategory* edit = new KActionCategory(i18nc("@title actions category - means actions changing smth in interface", "Edit"), actionCollection);

        action = undoGroup->createRedoAction(actionCollection);
        action->setObjectName(KStandardAction::name(KStandardAction::Redo));
        action->setIcon(QIcon::fromTheme("edit-redo"));
        action->setIconText(i18n("Redo"));
        action->setShortcuts(KStandardShortcut::redo());
        edit->addAction(action->objectName(), action);

        action = undoGroup->createUndoAction(actionCollection);
        action->setObjectName(KStandardAction::name(KStandardAction::Undo));
        action->setIcon(QIcon::fromTheme("edit-undo"));
        action->setIconText(i18n("Undo"));
        action->setShortcuts(KStandardShortcut::undo());
        edit->addAction(action->objectName(), action);
    }

    void setupContextManagerItems()
    {
        Q_ASSERT(mContextManager);
        KActionCollection* actionCollection = q->actionCollection();

        // Create context manager items
        FolderViewContextManagerItem* folderViewItem = new FolderViewContextManagerItem(mContextManager);
        connect(folderViewItem, SIGNAL(urlChanged(QUrl)),
                q, SLOT(openDirUrl(QUrl)));

        InfoContextManagerItem* infoItem = new InfoContextManagerItem(mContextManager);

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
        SemanticInfoContextManagerItem* semanticInfoItem = 0;
        semanticInfoItem = new SemanticInfoContextManagerItem(mContextManager, actionCollection, mViewMainPage);
#endif

        ImageOpsContextManagerItem* imageOpsItem =
            new ImageOpsContextManagerItem(mContextManager, q);

        FileOpsContextManagerItem* fileOpsItem = new FileOpsContextManagerItem(mContextManager, mThumbnailView, actionCollection, q);

        // Fill sidebar
        SideBarPage* page;
        page = new SideBarPage(i18n("Folders"));
        page->setObjectName(QLatin1String("folders"));
        page->addWidget(folderViewItem->widget());
        page->layout()->setMargin(0);
        mSideBar->addPage(page);

        page = new SideBarPage(i18n("Information"));
        page->setObjectName(QLatin1String("information"));
        page->addWidget(infoItem->widget());
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
        if (semanticInfoItem) {
            page->addWidget(semanticInfoItem->widget());
        }
#endif
        mSideBar->addPage(page);

        page = new SideBarPage(i18n("Operations"));
        page->setObjectName(QLatin1String("operations"));
        page->addWidget(imageOpsItem->widget());
        page->addWidget(fileOpsItem->widget());
        page->addStretch();
        mSideBar->addPage(page);
    }

    void initDirModel()
    {
        mDirModel->setKindFilter(
            MimeTypeUtils::KIND_DIR
            | MimeTypeUtils::KIND_ARCHIVE
            | MimeTypeUtils::KIND_RASTER_IMAGE
            | MimeTypeUtils::KIND_SVG_IMAGE
            | MimeTypeUtils::KIND_VIDEO);

        connect(mDirModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
                q, SLOT(slotDirModelNewItems()));

        connect(mDirModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
                q, SLOT(updatePreviousNextActions()));
        connect(mDirModel, SIGNAL(modelReset()),
                q, SLOT(updatePreviousNextActions()));

        connect(mDirModel->dirLister(), SIGNAL(completed()),
                q, SLOT(slotDirListerCompleted()));
    }

    void setupThumbnailBarModel()
    {
        mThumbnailBarModel = new DocumentOnlyProxyModel(q);
        mThumbnailBarModel->setSourceModel(mDirModel);
    }

    bool indexIsDirOrArchive(const QModelIndex& index) const
    {
        Q_ASSERT(index.isValid());
        KFileItem item = mDirModel->itemForIndex(index);
        return ArchiveUtils::fileItemIsDirOrArchive(item);
    }

    void goTo(const QModelIndex& index)
    {
        if (!index.isValid()) {
            return;
        }
        mThumbnailView->setCurrentIndex(index);
        mThumbnailView->scrollTo(index);
    }

    void goTo(int offset)
    {
        mPreloadDirectionIsForward = offset > 0;
        QModelIndex index = mContextManager->selectionModel()->currentIndex();
        index = mDirModel->index(index.row() + offset, 0);
        if (index.isValid() && !indexIsDirOrArchive(index)) {
            goTo(index);
        }
    }

    void goToFirstDocument()
    {
        QModelIndex index;
        for (int row = 0;; ++row) {
            index = mDirModel->index(row, 0);
            if (!index.isValid()) {
                return;
            }

            if (!indexIsDirOrArchive(index)) {
                break;
            }
        }
        goTo(index);
    }

    void goToLastDocument()
    {
        QModelIndex index = mDirModel->index(mDirModel->rowCount() - 1, 0);
        goTo(index);
    }

    void setupFullScreenContent()
    {
        mFullScreenContent->init(q->actionCollection(), mViewMainPage, mSlideShow);
        setupThumbnailBar(mFullScreenContent->thumbnailBar());
    }

    inline void setActionEnabled(const char* name, bool enabled)
    {
        QAction* action = q->actionCollection()->action(name);
        if (action) {
            action->setEnabled(enabled);
        } else {
            qWarning() << "Action" << name << "not found";
        }
    }

    void setActionsDisabledOnStartMainPageEnabled(bool enabled)
    {
        mBrowseAction->setEnabled(enabled);
        mViewAction->setEnabled(enabled);
        mToggleSideBarAction->setEnabled(enabled);
        mFullScreenAction->setEnabled(enabled);
        mToggleSlideShowAction->setEnabled(enabled);

        setActionEnabled("reload", enabled);
        setActionEnabled("go_start_page", enabled);
        setActionEnabled("add_folder_to_places", enabled);
    }

    void updateActions()
    {
        bool isRasterImage = false;
        bool canSave = false;
        bool isModified = false;
        const QUrl url = mContextManager->currentUrl();

        if (url.isValid()) {
            isRasterImage = mContextManager->currentUrlIsRasterImage();
            canSave = isRasterImage;
            isModified = DocumentFactory::instance()->load(url)->isModified();
            if (mCurrentMainPageId != ViewMainPageId) {
                // Saving only makes sense if exactly one image is selected
                QItemSelection selection = mThumbnailView->selectionModel()->selection();
                QModelIndexList indexList = selection.indexes();
                if (indexList.count() != 1) {
                    canSave = false;
                }
            }
        }

        KActionCollection* actionCollection = q->actionCollection();
        actionCollection->action("file_save")->setEnabled(canSave && isModified);
        actionCollection->action("file_save_as")->setEnabled(canSave);
        actionCollection->action("file_print")->setEnabled(isRasterImage);
    }

    const char* sideBarConfigGroupName() const
    {
        const char* name = 0;
        switch (mCurrentMainPageId) {
        case StartMainPageId:
            GV_WARN_AND_RETURN_VALUE(BROWSE_MODE_SIDE_BAR_GROUP, "mCurrentMainPageId == 'StartMainPageId'");
            break;
        case BrowseMainPageId:
            name = BROWSE_MODE_SIDE_BAR_GROUP;
            break;
        case ViewMainPageId:
            name = mViewMainPage->isFullScreenMode()
                   ? FULLSCREEN_MODE_SIDE_BAR_GROUP
                   : VIEW_MODE_SIDE_BAR_GROUP;
            break;
        }
        return name;
    }

    void loadSideBarConfig()
    {
        static QMap<const char*, bool> defaultVisibility;
        if (defaultVisibility.isEmpty()) {
            defaultVisibility[BROWSE_MODE_SIDE_BAR_GROUP]     = true;
            defaultVisibility[VIEW_MODE_SIDE_BAR_GROUP]       = true;
            defaultVisibility[FULLSCREEN_MODE_SIDE_BAR_GROUP] = false;
        }

        const char* name = sideBarConfigGroupName();
        KConfigGroup group(KSharedConfig::openConfig(), name);
        mSideBar->setVisible(group.readEntry(SIDE_BAR_IS_VISIBLE_KEY, defaultVisibility[name]));
        mSideBar->setCurrentPage(GwenviewConfig::sideBarPage());
        q->updateToggleSideBarAction();
    }

    void saveSideBarConfig() const
    {
        KConfigGroup group(KSharedConfig::openConfig(), sideBarConfigGroupName());
        group.writeEntry(SIDE_BAR_IS_VISIBLE_KEY, mSideBar->isVisible());
        GwenviewConfig::setSideBarPage(mSideBar->currentPage());
    }

    void setScreenSaverEnabled(bool enabled)
    {
        // Always delete mNotificationRestrictions, it does not hurt
        delete mNotificationRestrictions;
        if (!enabled) {
            mNotificationRestrictions = new KNotificationRestrictions(KNotificationRestrictions::ScreenSaver, q);
        } else {
            mNotificationRestrictions = 0;
        }
    }

    void assignThumbnailProviderToThumbnailView(ThumbnailView* thumbnailView)
    {
        GV_RETURN_IF_FAIL(thumbnailView);
        if (mActiveThumbnailView) {
            mActiveThumbnailView->setThumbnailProvider(0);
        }
        thumbnailView->setThumbnailProvider(mThumbnailProvider);
        mActiveThumbnailView = thumbnailView;
        if (mActiveThumbnailView->isVisible()) {
            mThumbnailProvider->stop();
            mActiveThumbnailView->generateThumbnailsForItems();
        }
    }

    void autoAssignThumbnailProvider()
    {
        if (mCurrentMainPageId == ViewMainPageId) {
            if (q->windowState() & Qt::WindowFullScreen) {
                assignThumbnailProviderToThumbnailView(mFullScreenContent->thumbnailBar());
            } else {
                assignThumbnailProviderToThumbnailView(mViewMainPage->thumbnailBar());
            }
        } else if (mCurrentMainPageId == BrowseMainPageId) {
            assignThumbnailProviderToThumbnailView(mThumbnailView);
        } else if (mCurrentMainPageId == StartMainPageId) {
            assignThumbnailProviderToThumbnailView(mStartMainPage->recentFoldersView());
        }
    }
};

MainWindow::MainWindow()
: KXmlGuiWindow(),
      d(new MainWindow::Private)
{
    d->q = this;
    d->mCurrentMainPageId = StartMainPageId;
    d->mDirModel = new SortedDirModel(this);
    d->setupContextManager();
    d->setupThumbnailBarModel();
    d->mGvCore = new GvCore(this, d->mDirModel);
    d->mPreloader = new Preloader(this);
    d->mNotificationRestrictions = 0;
    d->mThumbnailProvider = new ThumbnailProvider();
    d->mActiveThumbnailView = 0;
    d->initDirModel();
    d->setupWidgets();
    d->setupActions();
    d->setupUndoActions();
    d->setupContextManagerItems();
    d->setupFullScreenContent();
    d->updateActions();
    updatePreviousNextActions();
    d->mSaveBar->initActionDependentWidgets();

    createGUI();
    loadConfig();

    connect(DocumentFactory::instance(), SIGNAL(modifiedDocumentListChanged()),
            SLOT(slotModifiedDocumentListChanged()));

#ifdef KIPI_FOUND
    d->mKIPIInterface = new KIPIInterface(this);
    d->mKIPIExportAction->setKIPIInterface(d->mKIPIInterface);
#endif
    setAutoSaveSettings();
}

MainWindow::~MainWindow()
{
    if (GwenviewConfig::deleteThumbnailCacheOnExit()) {
        const QString dir = ThumbnailProvider::thumbnailBaseDir();
        if (QFile::exists(dir)) {
            KIO::NetAccess::del(QUrl::fromLocalFile(dir), this);
        }
    }
    delete d->mThumbnailProvider;
    delete d;
}

ContextManager* MainWindow::contextManager() const
{
    return d->mContextManager;
}

ViewMainPage* MainWindow::viewMainPage() const
{
    return d->mViewMainPage;
}

void MainWindow::setCaption(const QString& caption)
{
    // Keep a trace of caption to use it in slotModifiedDocumentListChanged()
    d->mCaption = caption;
    KXmlGuiWindow::setCaption(caption);
}

void MainWindow::setCaption(const QString& caption, bool modified)
{
    d->mCaption = caption;
    KXmlGuiWindow::setCaption(caption, modified);
}

void MainWindow::slotModifiedDocumentListChanged()
{
    d->updateActions();

    // Update caption
    QList<QUrl> list = DocumentFactory::instance()->modifiedDocumentList();
    bool modified = list.count() > 0;
    setCaption(d->mCaption, modified);
}

void MainWindow::setInitialUrl(const QUrl &_url)
{
    Q_ASSERT(_url.isValid());
    QUrl url = UrlUtils::fixUserEnteredUrl(_url);
    if (url.scheme() == "http" || url.scheme() == "https") {
        d->mGvCore->addUrlToRecentUrls(url);
    }
    if (UrlUtils::urlIsDirectory(url)) {
        d->mBrowseAction->trigger();
        openDirUrl(url);
    } else {
        d->mViewAction->trigger();
        d->mViewMainPage->openUrl(url);
        d->mContextManager->setUrlToSelect(url);
    }
}

void MainWindow::startSlideShow()
{
    d->mViewAction->trigger();
    // We need to wait until we have listed all images in the dirlister to
    // start the slideshow because the SlideShow objects needs an image list to
    // work.
    d->mStartSlideShowWhenDirListerCompleted = true;
}

void MainWindow::setActiveViewModeAction(QAction* action)
{
    if (d->mCurrentMainPageId != StartMainPageId) {
        d->saveSideBarConfig();
    }
    if (action == d->mViewAction) {
        d->mCurrentMainPageId = ViewMainPageId;
        // Switching to view mode
        d->mViewStackedWidget->setCurrentWidget(d->mViewMainPage);
        if (d->mViewMainPage->isEmpty()) {
            openSelectedDocuments();
        }
        d->mPreloadDirectionIsForward = true;
        QTimer::singleShot(VIEW_PRELOAD_DELAY, this, SLOT(preloadNextUrl()));
    } else {
        d->mCurrentMainPageId = BrowseMainPageId;
        // Switching to browse mode
        d->mViewStackedWidget->setCurrentWidget(d->mBrowseMainPage);
        if (!d->mViewMainPage->isEmpty()
                && KProtocolManager::supportsListing(d->mViewMainPage->url())) {
            // Reset the view to spare resources, but don't do it if we can't
            // browse the url, otherwise if the user starts Gwenview this way:
            // gwenview http://example.com/example.png
            // and switch to browse mode, switching back to view mode won't bring
            // his image back.
            d->mViewMainPage->reset();
        }
        setCaption(QString());
    }
    d->loadSideBarConfig();
    d->autoAssignThumbnailProvider();

    emit viewModeChanged();
}

void MainWindow::slotThumbnailViewIndexActivated(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }

    KFileItem item = d->mDirModel->itemForIndex(index);
    if (item.isDir()) {
        // Item is a dir, open it
        openDirUrl(item.url());
    } else {
        QString protocol = ArchiveUtils::protocolForMimeType(item.mimetype());
        if (!protocol.isEmpty()) {
            // Item is an archive, tweak url then open it
            QUrl url = item.url();
            url.setScheme(protocol);
            openDirUrl(url);
        } else {
            // Item is a document, switch to view mode
            d->mViewAction->trigger();
        }
    }
}

static bool indexRowLessThan(const QModelIndex& i1, const QModelIndex& i2)
{
    return i1.row() < i2.row();
}

void MainWindow::openSelectedDocuments()
{
    if (d->mCurrentMainPageId != ViewMainPageId) {
        return;
    }

    QModelIndex currentIndex = d->mThumbnailView->currentIndex();
    if (!currentIndex.isValid()) {
        return;
    }

    int count = 0;

    QList<QUrl> urls;
    QUrl currentUrl;
    QModelIndex firstDocumentIndex;
    QModelIndexList list = d->mThumbnailView->selectionModel()->selectedIndexes();
    // Make 'list' follow the same order as 'mThumbnailView'
    qSort(list.begin(), list.end(), indexRowLessThan);

    Q_FOREACH(const QModelIndex& index, list) {
        KFileItem item = d->mDirModel->itemForIndex(index);
        if (!item.isNull() && !ArchiveUtils::fileItemIsDirOrArchive(item)) {
            QUrl url = item.url();
            if (!firstDocumentIndex.isValid()) {
                firstDocumentIndex = index;
            }
            urls << url;
            if (index == currentIndex) {
                currentUrl = url;
            }
            ++count;
            if (count == ViewMainPage::MaxViewCount) {
                break;
            }
        }
    }
    if (urls.isEmpty()) {
        // No image to display
        return;
    }
    if (currentUrl.isEmpty()) {
        // Current index is not selected, or it is not a document: set
        // firstDocumentIndex as current
        GV_RETURN_IF_FAIL(firstDocumentIndex.isValid());
        d->mThumbnailView->selectionModel()->setCurrentIndex(firstDocumentIndex, QItemSelectionModel::Current);
        currentUrl = urls.first();
    }

    d->mViewMainPage->openUrls(urls, currentUrl);
}

void MainWindow::goUp()
{
    if (d->mCurrentMainPageId == BrowseMainPageId) {
        QUrl url = d->mContextManager->currentDirUrl();
        url = KIO::upUrl(url);
        openDirUrl(url);
    } else {
        d->mBrowseAction->trigger();
    }
}

void MainWindow::showStartMainPage()
{
    if (d->mCurrentMainPageId != StartMainPageId) {
        d->saveSideBarConfig();
        d->mCurrentMainPageId = StartMainPageId;
    }
    d->setActionsDisabledOnStartMainPageEnabled(false);

    d->mSideBar->hide();
    d->mViewStackedWidget->setCurrentWidget(d->mStartMainPage);

    d->updateActions();
    updatePreviousNextActions();
    d->mContextManager->setCurrentDirUrl(QUrl());
    d->mContextManager->setCurrentUrl(QUrl());

    d->autoAssignThumbnailProvider();
}

void MainWindow::slotStartMainPageUrlSelected(const QUrl &url)
{
    d->setActionsDisabledOnStartMainPageEnabled(true);

    if (d->mBrowseAction->isChecked()) {
        // Silently uncheck the action so that setInitialUrl() does the right thing
        SignalBlocker blocker(d->mBrowseAction);
        d->mBrowseAction->setChecked(false);
    }

    setInitialUrl(url);
}

void MainWindow::openDirUrl(const QUrl &url)
{
    const QUrl currentUrl = d->mContextManager->currentDirUrl();

    if (url == currentUrl) {
        return;
    }

    if (url.isParentOf(currentUrl)) {
        // Keep first child between url and currentUrl selected
        // If currentPath is      "/home/user/photos/2008/event"
        // and wantedPath is      "/home/user/photos"
        // pathToSelect should be "/home/user/photos/2008"

        // To anyone considering using QUrl::toLocalFile() instead of
        // QUrl::path() here. Please don't, using QUrl::path() is the right
        // thing to do here.
        const QString currentPath = QDir::cleanPath(currentUrl.adjusted(QUrl::StripTrailingSlash).path());
        const QString wantedPath  = QDir::cleanPath(url.adjusted(QUrl::StripTrailingSlash).path());
        const QChar separator('/');
        const int slashCount = wantedPath.count(separator);
        const QString pathToSelect = currentPath.section(separator, 0, slashCount + 1);
        QUrl urlToSelect = url;
        urlToSelect.setPath(pathToSelect);
        d->mContextManager->setUrlToSelect(urlToSelect);
    }
    d->mThumbnailProvider->stop();
    d->mContextManager->setCurrentDirUrl(url);
    d->mGvCore->addUrlToRecentFolders(url);
    d->mViewMainPage->reset();
}

void MainWindow::toggleSideBar(bool on)
{
    d->mSideBar->setVisible(on);
}

void MainWindow::updateToggleSideBarAction()
{
    SignalBlocker blocker(d->mToggleSideBarAction);
    bool visible = d->mSideBar->isVisible();
    d->mToggleSideBarAction->setChecked(visible);

    QString text;
    if (QApplication::isRightToLeft()) {
        text = QString::fromUtf8(visible ? "▮→" : "▮←");
    } else {
        text = QString::fromUtf8(visible ? "▮←" : "▮→");
    }
    QString toolTip = visible ? i18nc("@info:tooltip", "Hide sidebar") : i18nc("@info:tooltip", "Show sidebar");

    QList<QToolButton*> lst;
    lst << d->mBrowseMainPage->toggleSideBarButton()
        << d->mViewMainPage->toggleSideBarButton();
    Q_FOREACH(QToolButton * button, lst) {
        button->setText(text);
        button->setToolTip(toolTip);
    }
}

void MainWindow::slotPartCompleted()
{
    d->updateActions();
    QUrl url = d->mViewMainPage->url();
    if (!url.isEmpty()) {
        d->mFileOpenRecentAction->addUrl(url);
    }
    if (!KProtocolManager::supportsListing(url)) {
        return;
    }

    QUrl dirUrl = url;
    dirUrl = dirUrl.adjusted(QUrl::RemoveFilename);
    dirUrl.setPath(dirUrl.path() + QString());
    if (dirUrl.matches(d->mContextManager->currentDirUrl(), QUrl::StripTrailingSlash)) {
        QModelIndex index = d->mDirModel->indexForUrl(url);
        QItemSelectionModel* selectionModel = d->mThumbnailView->selectionModel();
        if (index.isValid() && !selectionModel->isSelected(index)) {
            // FIXME: QGV Reactivating this line prevents navigation to prev/next image
            //selectionModel->select(index, QItemSelectionModel::SelectCurrent);
        }
    } else {
        d->mContextManager->setCurrentDirUrl(dirUrl);
        d->mGvCore->addUrlToRecentFolders(dirUrl);
    }
}

void MainWindow::slotSelectionChanged()
{
    if (d->mCurrentMainPageId == ViewMainPageId) {
        // The user selected a new file in the thumbnail view, since the
        // document view is visible, let's show it
        openSelectedDocuments();
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
            QUrl url = item.url();
            Document::Ptr doc = DocumentFactory::instance()->load(url);
            undoGroup->addStack(doc->undoStack());
            undoGroup->setActiveStack(doc->undoStack());
        } else {
            undoGroup->setActiveStack(0);
        }
    }

    // Update UI
    d->updateActions();
    updatePreviousNextActions();

    // Start preloading
    int preloadDelay = d->mCurrentMainPageId == ViewMainPageId ? VIEW_PRELOAD_DELAY : BROWSE_PRELOAD_DELAY;
    QTimer::singleShot(preloadDelay, this, SLOT(preloadNextUrl()));
}

void MainWindow::slotCurrentDirUrlChanged(const QUrl &url)
{
    if (url.isValid()) {
        d->mUrlNavigator->setLocationUrl(url);
        d->mGoUpAction->setEnabled(url.path() != "/");
    } else {
        d->mGoUpAction->setEnabled(false);
    }
}

void MainWindow::slotDirModelNewItems()
{
    if (d->mThumbnailView->selectionModel()->hasSelection()) {
        updatePreviousNextActions();
    }
}

void MainWindow::slotDirListerCompleted()
{
    if (d->mStartSlideShowWhenDirListerCompleted) {
        d->mStartSlideShowWhenDirListerCompleted = false;
        QTimer::singleShot(0, d->mToggleSlideShowAction, SLOT(trigger()));
    }
    if (d->mThumbnailView->selectionModel()->hasSelection()) {
        updatePreviousNextActions();
    } else if (!d->mContextManager->urlToSelect().isValid()) {
        d->goToFirstDocument();
    }
    d->mThumbnailView->scrollToSelectedIndex();
    d->mViewMainPage->thumbnailBar()->scrollToSelectedIndex();
}

void MainWindow::goToPrevious()
{
    d->goTo(-1);
}

void MainWindow::goToNext()
{
    d->goTo(1);
}

void MainWindow::goToFirst()
{
    d->goToFirstDocument();
}

void MainWindow::goToLast()
{
    d->goToLastDocument();
}

void MainWindow::goToUrl(const QUrl &url)
{
    if (d->mCurrentMainPageId == ViewMainPageId) {
        d->mViewMainPage->openUrl(url);
    }
    QUrl dirUrl = url;
    dirUrl = dirUrl.adjusted(QUrl::RemoveFilename);
    dirUrl.setPath(dirUrl.path() + "");
    if (dirUrl != d->mContextManager->currentDirUrl()) {
        d->mContextManager->setCurrentDirUrl(dirUrl);
        d->mGvCore->addUrlToRecentFolders(dirUrl);
    }
    d->mContextManager->setUrlToSelect(url);
}

void MainWindow::updatePreviousNextActions()
{
    bool hasPrevious;
    bool hasNext;
    QModelIndex currentIndex = d->mContextManager->selectionModel()->currentIndex();
    if (currentIndex.isValid() && !d->indexIsDirOrArchive(currentIndex)) {
        int row = currentIndex.row();
        QModelIndex prevIndex = d->mDirModel->index(row - 1, 0);
        QModelIndex nextIndex = d->mDirModel->index(row + 1, 0);
        hasPrevious = prevIndex.isValid() && !d->indexIsDirOrArchive(prevIndex);
        hasNext = nextIndex.isValid() && !d->indexIsDirOrArchive(nextIndex);
    } else {
        hasPrevious = false;
        hasNext = false;
    }

    d->mGoToPreviousAction->setEnabled(hasPrevious);
    d->mGoToNextAction->setEnabled(hasNext);
    d->mGoToFirstAction->setEnabled(hasPrevious);
    d->mGoToLastAction->setEnabled(hasNext);
}

void MainWindow::leaveFullScreen()
{
    if (d->mFullScreenAction->isChecked()) {
        d->mFullScreenAction->trigger();
    }
}

void MainWindow::toggleFullScreen(bool checked)
{
    setUpdatesEnabled(false);
    d->saveSideBarConfig();
    if (checked) {
        // Save MainWindow config now, this way if we quit while in
        // fullscreen, we are sure latest MainWindow changes are remembered.
        KConfigGroup saveConfigGroup = autoSaveConfigGroup();
        saveMainWindowSettings(saveConfigGroup);
        resetAutoSaveSettings();

        // Save state
        d->mStateBeforeFullScreen.mToolBarVisible = toolBar()->isVisible();
        d->mStateBeforeFullScreen.mWindowState = windowState();

        // Go full screen
        setWindowState(windowState() | Qt::WindowFullScreen);
        menuBar()->hide();
        toolBar()->hide();

        QApplication::setPalette(d->mGvCore->palette(GvCore::FullScreenPalette));
        d->mFullScreenContent->setFullScreenMode(true);
        d->mBrowseMainPage->setFullScreenMode(true);
        d->mViewMainPage->setFullScreenMode(true);
        d->mSaveBar->setFullScreenMode(true);
        d->setScreenSaverEnabled(false);

        // HACK: Only load sidebar config now, because it looks at
        // ViewMainPage fullScreenMode property to determine the sidebar
        // config group.
        d->loadSideBarConfig();
    } else {
        setAutoSaveSettings();

        // Back to normal
        QApplication::setPalette(d->mGvCore->palette(GvCore::NormalPalette));
        d->mFullScreenContent->setFullScreenMode(false);
        d->mBrowseMainPage->setFullScreenMode(false);
        d->mViewMainPage->setFullScreenMode(false);
        d->mSlideShow->stop();
        d->mSaveBar->setFullScreenMode(false);
        setWindowState(d->mStateBeforeFullScreen.mWindowState);
        menuBar()->setVisible(d->mShowMenuBarAction->isChecked());
        toolBar()->setVisible(d->mStateBeforeFullScreen.mToolBarVisible);

        d->setScreenSaverEnabled(true);

        // Keep this after mViewMainPage->setFullScreenMode(false).
        // See call to loadSideBarConfig() above.
        d->loadSideBarConfig();

        // See resizeEvent
        d->mFullScreenLeftAt = QDateTime::currentDateTime();
    }
    setUpdatesEnabled(true);
    d->autoAssignThumbnailProvider();
}

void MainWindow::saveCurrent()
{
    d->mGvCore->save(d->mContextManager->currentUrl());
}

void MainWindow::saveCurrentAs()
{
    d->mGvCore->saveAs(d->mContextManager->currentUrl());
}

void MainWindow::reload()
{
    if (d->mCurrentMainPageId == ViewMainPageId) {
        d->mViewMainPage->reload();
    } else {
        d->mBrowseMainPage->reload();
    }
}

void MainWindow::openFile()
{
    QUrl dirUrl = d->mContextManager->currentDirUrl();

    KFileDialog dialog(dirUrl, QString(), this);
    dialog.setWindowTitle(i18nc("@title:window", "Open Image"));
    const QStringList mimeFilter = MimeTypeUtils::imageMimeTypes();
    dialog.setMimeFilter(mimeFilter);
    dialog.setOperationMode(KFileDialog::Opening);
    if (!dialog.exec()) {
        return;
    }

    openUrl(dialog.selectedUrl());
}

void MainWindow::openUrl(const QUrl& url)
{
    d->setActionsDisabledOnStartMainPageEnabled(true);
    d->mViewAction->trigger();
    d->mViewMainPage->openUrl(url);
    d->mContextManager->setUrlToSelect(url);
}

void MainWindow::showDocumentInFullScreen(const QUrl &url)
{
    d->mViewMainPage->openUrl(url);
    d->mContextManager->setUrlToSelect(url);
    d->mFullScreenAction->trigger();
}

void MainWindow::toggleSlideShow()
{
    if (d->mSlideShow->isRunning()) {
        d->mSlideShow->stop();
    } else {
        if (!d->mViewAction->isChecked()) {
            d->mViewAction->trigger();
        }
        if (!d->mFullScreenAction->isChecked()) {
            d->mFullScreenAction->trigger();
        }
        QList<QUrl> list;
        for (int pos = 0; pos < d->mDirModel->rowCount(); ++pos) {
            QModelIndex index = d->mDirModel->index(pos, 0);
            KFileItem item = d->mDirModel->itemForIndex(index);
            MimeTypeUtils::Kind kind = MimeTypeUtils::fileItemKind(item);
            switch (kind) {
            case MimeTypeUtils::KIND_SVG_IMAGE:
            case MimeTypeUtils::KIND_RASTER_IMAGE:
            case MimeTypeUtils::KIND_VIDEO:
                list << item.url();
                break;
            default:
                break;
            }
        }
        d->mSlideShow->start(list);
    }
    updateSlideShowAction();
}

void MainWindow::updateSlideShowAction()
{
    if (d->mSlideShow->isRunning()) {
        d->mToggleSlideShowAction->setText(i18n("Stop Slideshow"));
        d->mToggleSlideShowAction->setIcon(QIcon::fromTheme("media-playback-pause"));
    } else {
        d->mToggleSlideShowAction->setText(i18n("Start Slideshow"));
        d->mToggleSlideShowAction->setIcon(QIcon::fromTheme("media-playback-start"));
    }
}

bool MainWindow::queryClose()
{
    saveConfig();
    d->saveSideBarConfig();
    QList<QUrl> list = DocumentFactory::instance()->modifiedDocumentList();
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
        // We need to wait a bit because the DocumentFactory is notified about
        // saved documents through a queued connection.
        qApp->processEvents();
        return DocumentFactory::instance()->modifiedDocumentList().isEmpty();

    case KMessageBox::No:
        return true;

    default: // cancel
        return false;
    }
}

void MainWindow::showConfigDialog()
{
    ConfigDialog dialog(this);
    connect(&dialog, SIGNAL(settingsChanged(QString)), SLOT(loadConfig()));
    dialog.exec();
}

void MainWindow::toggleMenuBar()
{
    if (!d->mFullScreenAction->isChecked()) {
        menuBar()->setVisible(d->mShowMenuBarAction->isChecked());
    }
}

void MainWindow::loadConfig()
{
    d->mDirModel->setBlackListedExtensions(GwenviewConfig::blackListedExtensions());
    d->mDirModel->adjustKindFilter(MimeTypeUtils::KIND_VIDEO, GwenviewConfig::listVideos());

    d->mFileOpenRecentAction->loadEntries(KConfigGroup(KSharedConfig::openConfig(), "Recent Files"));
    d->mStartMainPage->loadConfig();
    d->mViewMainPage->loadConfig();
    d->mBrowseMainPage->loadConfig();
}

void MainWindow::saveConfig()
{
    d->mFileOpenRecentAction->saveEntries(KConfigGroup(KSharedConfig::openConfig(), "Recent Files"));
    d->mViewMainPage->saveConfig();
    d->mBrowseMainPage->saveConfig();
}

void MainWindow::print()
{
    if (!d->mContextManager->currentUrlIsRasterImage()) {
        return;
    }

    Document::Ptr doc = DocumentFactory::instance()->load(d->mContextManager->currentUrl());
    PrintHelper printHelper(this);
    printHelper.print(doc);
}

void MainWindow::preloadNextUrl()
{
    static bool disablePreload = qgetenv("GV_MAX_UNREFERENCED_IMAGES") == "0";
    if (disablePreload) {
        qDebug() << "Preloading disabled";
        return;
    }
    QItemSelection selection = d->mThumbnailView->selectionModel()->selection();
    if (selection.size() != 1) {
        return;
    }

    QModelIndexList indexList = selection.indexes();
    if (indexList.isEmpty()) {
        return;
    }

    QModelIndex index = indexList.at(0);
    if (!index.isValid()) {
        return;
    }

    if (d->mCurrentMainPageId == ViewMainPageId) {
        // If we are in view mode, preload the next url, otherwise preload the
        // selected one
        int offset = d->mPreloadDirectionIsForward ? 1 : -1;
        index = d->mDirModel->sibling(index.row() + offset, index.column(), index);
        if (!index.isValid()) {
            return;
        }
    }

    KFileItem item = d->mDirModel->itemForIndex(index);
    if (!ArchiveUtils::fileItemIsDirOrArchive(item)) {
        QUrl url = item.url();
        if (url.isLocalFile()) {
            QSize size = d->mViewStackedWidget->size();
            d->mPreloader->preload(url, size);
        }
    }
}

QSize MainWindow::sizeHint() const
{
    return KXmlGuiWindow::sizeHint().expandedTo(QSize(750, 500));
}

void MainWindow::showEvent(QShowEvent *event)
{
    // We need to delay initializing the action state until the menu bar has
    // been initialized, that's why it's done only in the showEvent()
    d->mShowMenuBarAction->setChecked(menuBar()->isVisible());
    KXmlGuiWindow::showEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    KXmlGuiWindow::resizeEvent(event);
    // This is a small hack to execute code after leaving fullscreen, and after
    // the window has been resized back to its former size.
    if (d->mFullScreenLeftAt.isValid() && d->mFullScreenLeftAt.secsTo(QDateTime::currentDateTime()) < 2) {
        if (d->mCurrentMainPageId == BrowseMainPageId) {
            d->mThumbnailView->scrollToSelectedIndex();
        }
        d->mFullScreenLeftAt = QDateTime();
    }
}

void MainWindow::setDistractionFreeMode(bool value)
{
    d->mFullScreenContent->setDistractionFreeMode(value);
}

void MainWindow::saveProperties(KConfigGroup& group)
{
    group.writeEntry(SESSION_CURRENT_PAGE_KEY, int(d->mCurrentMainPageId));
    group.writeEntry(SESSION_URL_KEY, d->mContextManager->currentUrl().toString());
}

void MainWindow::readProperties(const KConfigGroup& group)
{
    MainPageId pageId = MainPageId(group.readEntry(SESSION_CURRENT_PAGE_KEY, int(StartMainPageId)));
    if (pageId == StartMainPageId) {
        d->mCurrentMainPageId = StartMainPageId;
        showStartMainPage();
    } else if (pageId == BrowseMainPageId) {
        d->mBrowseAction->trigger();
    } else {
        d->mViewAction->trigger();
    }
    QUrl url = group.readEntry(SESSION_URL_KEY, QUrl());
    if (!url.isValid()) {
        qWarning() << "Invalid url!";
        return;
    }
    goToUrl(url);
}

void MainWindow::showFirstDocumentReached()
{
    if (d->mCurrentMainPageId != ViewMainPageId) {
        return;
    }
    HudButtonBox* dlg = new HudButtonBox;
    dlg->setText(i18n("You reached the first document, what do you want to do?"));
    dlg->addButton(i18n("Stay There"));
    dlg->addAction(d->mGoToLastAction, i18n("Go to the Last Document"));
    dlg->addAction(d->mBrowseAction, i18n("Go Back to the Document List"));
    dlg->addCountDown(15000);
    d->mViewMainPage->showMessageWidget(dlg, Qt::AlignCenter);
}

void MainWindow::showLastDocumentReached()
{
    if (d->mCurrentMainPageId != ViewMainPageId) {
        return;
    }
    HudButtonBox* dlg = new HudButtonBox;
    dlg->setText(i18n("You reached the last document, what do you want to do?"));
    dlg->addButton(i18n("Stay There"));
    dlg->addAction(d->mGoToFirstAction, i18n("Go to the First Document"));
    dlg->addAction(d->mBrowseAction, i18n("Go Back to the Document List"));
    dlg->addCountDown(15000);
    d->mViewMainPage->showMessageWidget(dlg, Qt::AlignCenter);
}

} // namespace
