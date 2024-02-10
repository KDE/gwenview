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
#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDialog>
#include <QFileDialog>
#include <QLineEdit>
#include <QMenuBar>
#include <QMouseEvent>
#include <QPushButton>
#include <QShortcut>
#include <QSplitter>
#include <QStackedWidget>
#include <QTimer>
#include <QUndoGroup>
#include <QUrl>
#include <QVBoxLayout>

#ifdef Q_OS_OSX
#include <QFileOpenEvent>
#endif
#include <QJsonArray>
#include <QJsonObject>

// KF
#include <KActionCategory>
#include <KActionCollection>
#include <KDirLister>
#include <KDirModel>
#include <KFileItem>
#include <KHamburgerMenu>
#include <KLinkItemSelectionModel>
#include <KLocalizedString>
#include <KMessageBox>
#include <KMessageWidget>
#include <KProtocolManager>
#include <KRecentFilesAction>
#include <KStandardShortcut>
#include <KToggleFullScreenAction>
#include <KToolBar>
#include <KToolBarPopupAction>
#include <KUrlComboBox>
#include <KUrlNavigator>
#include <KXMLGUIFactory>
#include <kwidgetsaddons_version.h>
#include <kxmlgui_version.h>
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
#include "lib/semanticinfo/semanticinfodirmodel.h"
#endif
#if HAVE_PURPOSE
#include <Purpose/AlternativesModel>
#include <Purpose/Menu>
#include <purpose_version.h>
#endif

// Local
#include "alignwithsidebarwidgetaction.h"
#include "configdialog.h"
#include "documentinfoprovider.h"
#include "fileopscontextmanageritem.h"
#include "folderviewcontextmanageritem.h"
#include "fullscreencontent.h"
#include "gvcore.h"
#include "gwenview_app_debug.h"
#include "imageopscontextmanageritem.h"
#include "infocontextmanageritem.h"
#include "viewmainpage.h"
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
#include "semanticinfocontextmanageritem.h"
#endif
#include "browsemainpage.h"
#include "preloader.h"
#include "savebar.h"
#include "sidebar.h"
#include "splitter.h"
#include "startmainpage.h"
#include "thumbnailviewhelper.h"
#include <lib/archiveutils.h>
#include <lib/contextmanager.h>
#include <lib/disabledactionshortcutmonitor.h>
#include <lib/document/documentfactory.h>
#include <lib/documentonlyproxymodel.h>
#include <lib/gvdebug.h>
#include <lib/gwenviewconfig.h>
#include <lib/hud/hudbuttonbox.h>
#include <lib/mimetypeutils.h>
#ifdef HAVE_QTDBUS
#include <QDBusConnection>
#include <QDBusPendingReply>
#include <QDBusReply>
#include <lib/mpris2/mpris2service.h>
#endif
#include <lib/print/printhelper.h>
#include <lib/semanticinfo/sorteddirmodel.h>
#include <lib/signalblocker.h>
#include <lib/slideshow.h>
#include <lib/thumbnailprovider/thumbnailprovider.h>
#include <lib/thumbnailview/thumbnailbarview.h>
#include <lib/thumbnailview/thumbnailview.h>
#include <lib/urlutils.h>

namespace Gwenview
{
#undef ENABLE_LOG
#undef LOG
// #define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) qCDebug(GWENVIEW_APP_LOG) << x
#else
#define LOG(x) ;
#endif

static const int BROWSE_PRELOAD_DELAY = 1000;
static const int VIEW_PRELOAD_DELAY = 100;

static const char *SESSION_CURRENT_PAGE_KEY = "Page";
static const char *SESSION_URL_KEY = "Url";

enum MainPageId {
    StartMainPageId,
    BrowseMainPageId,
    ViewMainPageId,
};

struct MainWindowState {
    bool mToolBarVisible;
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
struct MainWindow::Private {
    GvCore *mGvCore = nullptr;
    MainWindow *q = nullptr;
    QSplitter *mCentralSplitter = nullptr;
    QWidget *mContentWidget = nullptr;
    ViewMainPage *mViewMainPage = nullptr;
    KUrlNavigator *mUrlNavigator = nullptr;
    ThumbnailView *mThumbnailView = nullptr;
    ThumbnailView *mActiveThumbnailView = nullptr;
    DocumentInfoProvider *mDocumentInfoProvider = nullptr;
    ThumbnailViewHelper *mThumbnailViewHelper = nullptr;
    QPointer<ThumbnailProvider> mThumbnailProvider;
    BrowseMainPage *mBrowseMainPage = nullptr;
    StartMainPage *mStartMainPage = nullptr;
    SideBar *mSideBar = nullptr;
    KMessageWidget *mSharedMessage = nullptr;
    QStackedWidget *mViewStackedWidget = nullptr;
    FullScreenContent *mFullScreenContent = nullptr;
    SaveBar *mSaveBar = nullptr;
    bool mStartSlideShowWhenDirListerCompleted;
    SlideShow *mSlideShow = nullptr;
#ifdef HAVE_QTDBUS
    Mpris2Service *mMpris2Service = nullptr;
#endif
    Preloader *mPreloader = nullptr;
    bool mPreloadDirectionIsForward;

    QActionGroup *mViewModeActionGroup = nullptr;
    KRecentFilesAction *mFileOpenRecentAction = nullptr;
    QAction *mBrowseAction = nullptr;
    QAction *mViewAction = nullptr;
    QAction *mGoUpAction = nullptr;
    QAction *mGoToPreviousAction = nullptr;
    QAction *mGoToNextAction = nullptr;
    QAction *mGoToFirstAction = nullptr;
    QAction *mGoToLastAction = nullptr;
    KToggleAction *mToggleSideBarAction = nullptr;
    KToggleAction *mToggleOperationsSideBarAction = nullptr;
    QAction *mFullScreenAction = nullptr;
    QAction *mSpotlightModeAction = nullptr;
    QAction *mToggleSlideShowAction = nullptr;
    KToggleAction *mShowMenuBarAction = nullptr;
    KToggleAction *mShowStatusBarAction = nullptr;
    QPointer<HudButtonBox> hudButtonBox = nullptr;
#if HAVE_PURPOSE
    Purpose::Menu *mShareMenu = nullptr;
    KToolBarPopupAction *mShareAction = nullptr;
#endif
    KHamburgerMenu *mHamburgerMenu = nullptr;

    SortedDirModel *mDirModel = nullptr;
    DocumentOnlyProxyModel *mThumbnailBarModel = nullptr;
    KLinkItemSelectionModel *mThumbnailBarSelectionModel = nullptr;
    ContextManager *mContextManager = nullptr;

    MainWindowState mStateBeforeFullScreen;

    QString mCaption;

    MainPageId mCurrentMainPageId;

    QDateTime mFullScreenLeftAt;
    uint screenSaverDbusCookie = 0;

    void setupContextManager()
    {
        mContextManager = new ContextManager(mDirModel, q);
        connect(mContextManager, &ContextManager::selectionChanged, q, &MainWindow::slotSelectionChanged);
        connect(mContextManager, &ContextManager::currentDirUrlChanged, q, &MainWindow::slotCurrentDirUrlChanged);
    }

    void setupWidgets()
    {
        mFullScreenContent = new FullScreenContent(q, mGvCore);
        connect(mContextManager, &ContextManager::currentUrlChanged, mFullScreenContent, &FullScreenContent::setCurrentUrl);

        mCentralSplitter = new Splitter(Qt::Horizontal, q);
        q->setCentralWidget(mCentralSplitter);

        // Left side of splitter
        mSideBar = new SideBar(mCentralSplitter);

        // Right side of splitter
        mContentWidget = new QWidget(mCentralSplitter);

        mSharedMessage = new KMessageWidget(mContentWidget);
        mSharedMessage->setVisible(false);

        mSaveBar = new SaveBar(mContentWidget, q->actionCollection());
        connect(mContextManager, &ContextManager::currentUrlChanged, mSaveBar, &SaveBar::setCurrentUrl);
        mViewStackedWidget = new QStackedWidget(mContentWidget);
        auto layout = new QVBoxLayout(mContentWidget);
        layout->addWidget(mSharedMessage);
        layout->addWidget(mSaveBar);
        layout->addWidget(mViewStackedWidget);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        ////

        mStartSlideShowWhenDirListerCompleted = false;
        mSlideShow = new SlideShow(q);
        connect(mContextManager, &ContextManager::currentUrlChanged, mSlideShow, &SlideShow::setCurrentUrl);

        setupThumbnailView(mViewStackedWidget);
        setupViewMainPage(mViewStackedWidget);
        setupStartMainPage(mViewStackedWidget);
        mViewStackedWidget->addWidget(mBrowseMainPage);
        mViewStackedWidget->addWidget(mViewMainPage);
        mViewStackedWidget->addWidget(mStartMainPage);
        mViewStackedWidget->setCurrentWidget(mBrowseMainPage);

        mCentralSplitter->setStretchFactor(0, 0);
        mCentralSplitter->setStretchFactor(1, 1);
        mCentralSplitter->setChildrenCollapsible(false);

        mThumbnailView->setFocus();

        connect(mSaveBar, &SaveBar::requestSaveAll, mGvCore, &GvCore::saveAll);
        connect(mSaveBar, &SaveBar::goToUrl, q, &MainWindow::goToUrl);

        connect(mSlideShow, &SlideShow::goToUrl, q, &MainWindow::goToUrl);
    }

    void setupThumbnailView(QWidget *parent)
    {
        Q_ASSERT(mContextManager);
        mBrowseMainPage = new BrowseMainPage(parent, q->actionCollection(), mGvCore);

        mThumbnailView = mBrowseMainPage->thumbnailView();
        mThumbnailView->viewport()->installEventFilter(q);
        mThumbnailView->setSelectionModel(mContextManager->selectionModel());
        mUrlNavigator = mBrowseMainPage->urlNavigator();

        mDocumentInfoProvider = new DocumentInfoProvider(mDirModel);
        mThumbnailView->setDocumentInfoProvider(mDocumentInfoProvider);

        mThumbnailViewHelper = new ThumbnailViewHelper(mDirModel, q->actionCollection());
        connect(mContextManager, &ContextManager::currentDirUrlChanged, mThumbnailViewHelper, &ThumbnailViewHelper::setCurrentDirUrl);
        mThumbnailView->setThumbnailViewHelper(mThumbnailViewHelper);

        mThumbnailBarSelectionModel = new KLinkItemSelectionModel(mThumbnailBarModel, mContextManager->selectionModel(), q);

        // Connect thumbnail view
        connect(mThumbnailView, &ThumbnailView::indexActivated, q, &MainWindow::slotThumbnailViewIndexActivated);

        // Connect delegate
        QAbstractItemDelegate *delegate = mThumbnailView->itemDelegate();
        connect(delegate, SIGNAL(saveDocumentRequested(QUrl)), mGvCore, SLOT(save(QUrl)));
        connect(delegate, SIGNAL(rotateDocumentLeftRequested(QUrl)), mGvCore, SLOT(rotateLeft(QUrl)));
        connect(delegate, SIGNAL(rotateDocumentRightRequested(QUrl)), mGvCore, SLOT(rotateRight(QUrl)));
        connect(delegate, SIGNAL(showDocumentInFullScreenRequested(QUrl)), q, SLOT(showDocumentInFullScreen(QUrl)));
        connect(delegate, SIGNAL(setDocumentRatingRequested(QUrl, int)), mGvCore, SLOT(setRating(QUrl, int)));

        // Connect url navigator
        connect(mUrlNavigator, &KUrlNavigator::urlChanged, q, &MainWindow::openDirUrl);
    }

    void setupViewMainPage(QWidget *parent)
    {
        mViewMainPage = new ViewMainPage(parent, mSlideShow, q->actionCollection(), mGvCore);
        connect(mViewMainPage, &ViewMainPage::captionUpdateRequested, q, &MainWindow::slotUpdateCaption);
        connect(mViewMainPage, &ViewMainPage::completed, q, &MainWindow::slotPartCompleted);
        connect(mViewMainPage, &ViewMainPage::previousImageRequested, q, &MainWindow::goToPrevious);
        connect(mViewMainPage, &ViewMainPage::nextImageRequested, q, &MainWindow::goToNext);
        connect(mViewMainPage, &ViewMainPage::openUrlRequested, q, &MainWindow::openUrl);
        connect(mViewMainPage, &ViewMainPage::openDirUrlRequested, q, &MainWindow::openDirUrl);

        setupThumbnailBar(mViewMainPage->thumbnailBar());
    }

    void setupThumbnailBar(ThumbnailView *bar)
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

    void setupStartMainPage(QWidget *parent)
    {
        mStartMainPage = new StartMainPage(parent, mGvCore);
        connect(mStartMainPage, &StartMainPage::urlSelected, q, &MainWindow::slotStartMainPageUrlSelected);
        connect(mStartMainPage, &StartMainPage::recentFileRemoved, [this](const QUrl &url) {
            mFileOpenRecentAction->removeUrl(url);
        });
        connect(mStartMainPage, &StartMainPage::recentFilesCleared, [this]() {
            mFileOpenRecentAction->clear();
        });
    }

    void installDisabledActionShortcutMonitor(QAction *action, const char *slot)
    {
        auto monitor = new DisabledActionShortcutMonitor(action, q);
        connect(monitor, SIGNAL(activated()), q, slot);
    }

    void setupActions()
    {
        KActionCollection *actionCollection = q->actionCollection();
        auto file = new KActionCategory(i18nc("@title actions category", "File"), actionCollection);
        auto view = new KActionCategory(i18nc("@title actions category - means actions changing smth in interface", "View"), actionCollection);

        file->addAction(KStandardAction::Save, q, SLOT(saveCurrent()));
        file->addAction(KStandardAction::SaveAs, q, SLOT(saveCurrentAs()));
        file->addAction(KStandardAction::Open, q, SLOT(openFile()));
        mFileOpenRecentAction = KStandardAction::openRecent(q, SLOT(openUrl(QUrl)), q);
        connect(mFileOpenRecentAction, &KRecentFilesAction::recentListCleared, mGvCore, &GvCore::clearRecentFilesAndFolders);
        auto clearAction = mFileOpenRecentAction->menu()->findChild<QAction *>("clear_action");
        if (clearAction) {
            clearAction->setText(i18nc("@action Open Recent menu", "Clear List"));
        }
        file->addAction("file_open_recent", mFileOpenRecentAction);
        file->addAction(KStandardAction::Print, q, SLOT(print()));
        file->addAction(KStandardAction::PrintPreview, q, SLOT(printPreview()));
        file->addAction(KStandardAction::Quit, qApp, SLOT(closeAllWindows()));

        QAction *action = file->addAction(QStringLiteral("reload"), q, SLOT(reload()));
        action->setText(i18nc("@action reload the currently viewed image", "Reload"));
        action->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
        actionCollection->setDefaultShortcuts(action, KStandardShortcut::reload());

        QAction *replaceLocationAction = actionCollection->addAction(QStringLiteral("replace_location"));
        replaceLocationAction->setText(i18nc("@action:inmenu Navigation Bar", "Replace Location"));
        actionCollection->setDefaultShortcut(replaceLocationAction, Qt::CTRL | Qt::Key_L);
        connect(replaceLocationAction, &QAction::triggered, q, &MainWindow::replaceLocation);

        mBrowseAction = view->addAction(QStringLiteral("browse"));
        mBrowseAction->setText(i18nc("@action:intoolbar Switch to file list", "Browse"));
        mBrowseAction->setToolTip(i18nc("@info:tooltip", "Browse folders for images"));
        mBrowseAction->setCheckable(true);
        mBrowseAction->setIcon(QIcon::fromTheme(QStringLiteral("view-list-icons")));
        actionCollection->setDefaultShortcut(mBrowseAction, Qt::Key_Escape);
        connect(mViewMainPage, &ViewMainPage::goToBrowseModeRequested, mBrowseAction, &QAction::trigger);

        mViewAction = view->addAction(QStringLiteral("view"));
        mViewAction->setText(i18nc("@action:intoolbar Switch to image view", "View"));
        mViewAction->setToolTip(i18nc("@info:tooltip", "View selected images"));
        mViewAction->setIcon(QIcon::fromTheme(QStringLiteral("view-preview")));
        mViewAction->setCheckable(true);

        mViewModeActionGroup = new QActionGroup(q);
        mViewModeActionGroup->addAction(mBrowseAction);
        mViewModeActionGroup->addAction(mViewAction);

        connect(mViewModeActionGroup, &QActionGroup::triggered, q, &MainWindow::setActiveViewModeAction);

        mFullScreenAction = KStandardAction::fullScreen(q, &MainWindow::toggleFullScreen, q, actionCollection);
        QList<QKeySequence> shortcuts = mFullScreenAction->shortcuts();
        shortcuts.append(QKeySequence(Qt::Key_F11));
        actionCollection->setDefaultShortcuts(mFullScreenAction, shortcuts);

        connect(mViewMainPage, &ViewMainPage::toggleFullScreenRequested, mFullScreenAction, &QAction::trigger);

        QAction *leaveFullScreenAction = view->addAction("leave_fullscreen", q, SLOT(leaveFullScreen()));
        leaveFullScreenAction->setIcon(QIcon::fromTheme(QStringLiteral("view-restore")));
        leaveFullScreenAction->setText(i18nc("@action", "Exit Full Screen"));

        mSpotlightModeAction = view->addAction(QStringLiteral("view_toggle_spotlightmode"), q, SLOT(toggleSpotlightMode(bool)));
        mSpotlightModeAction->setCheckable(true);
        mSpotlightModeAction->setText(i18nc("@action", "Spotlight Mode"));
        mSpotlightModeAction->setIcon(QIcon::fromTheme(QStringLiteral("image-navigator-symbolic")));

        mGoToPreviousAction = view->addAction("go_previous", q, SLOT(goToPrevious()));
        mGoToPreviousAction->setPriority(QAction::LowPriority);
        mGoToPreviousAction->setIcon(QIcon::fromTheme(QGuiApplication::layoutDirection() == Qt::LeftToRight ? "go-previous" : "go-previous-symbolic-rtl"));
        mGoToPreviousAction->setText(i18nc("@action Go to previous image", "Previous"));
        mGoToPreviousAction->setToolTip(i18nc("@info:tooltip", "Go to previous image"));
        installDisabledActionShortcutMonitor(mGoToPreviousAction, SLOT(showFirstDocumentReached()));

        mGoToNextAction = view->addAction(QStringLiteral("go_next"), q, SLOT(goToNext()));
        mGoToNextAction->setPriority(QAction::LowPriority);
        mGoToNextAction->setIcon(QIcon::fromTheme(QGuiApplication::layoutDirection() == Qt::LeftToRight ? "go-next" : "go-next-symbolic-rtl"));
        mGoToNextAction->setText(i18nc("@action Go to next image", "Next"));
        mGoToNextAction->setToolTip(i18nc("@info:tooltip", "Go to next image"));
        installDisabledActionShortcutMonitor(mGoToNextAction, SLOT(showLastDocumentReached()));

        mGoToFirstAction = view->addAction(QStringLiteral("go_first"), q, SLOT(goToFirst()));
        mGoToFirstAction->setPriority(QAction::LowPriority);
        mGoToFirstAction->setIcon(QIcon::fromTheme(QStringLiteral("go-first-view")));
        mGoToFirstAction->setText(i18nc("@action Go to first image", "First"));
        mGoToFirstAction->setToolTip(i18nc("@info:tooltip", "Go to first image"));
        actionCollection->setDefaultShortcut(mGoToFirstAction, Qt::Key_Home);

        mGoToLastAction = view->addAction(QStringLiteral("go_last"), q, SLOT(goToLast()));
        mGoToLastAction->setPriority(QAction::LowPriority);
        mGoToLastAction->setIcon(QIcon::fromTheme(QStringLiteral("go-last-view")));
        mGoToLastAction->setText(i18nc("@action Go to last image", "Last"));
        mGoToLastAction->setToolTip(i18nc("@info:tooltip", "Go to last image"));
        actionCollection->setDefaultShortcut(mGoToLastAction, Qt::Key_End);

        mPreloadDirectionIsForward = true;

        mGoUpAction = view->addAction(KStandardAction::Up, q, SLOT(goUp()));

        action = view->addAction("go_start_page", q, SLOT(showStartMainPage()));
        action->setPriority(QAction::LowPriority);
        action->setIcon(QIcon::fromTheme(QStringLiteral("go-home")));
        action->setText(i18nc("@action", "Start Page"));
        action->setToolTip(i18nc("@info:tooltip", "Open the start page"));
        actionCollection->setDefaultShortcuts(action, KStandardShortcut::home());

        mToggleSideBarAction = view->add<KToggleAction>(QStringLiteral("toggle_sidebar"));
        connect(mToggleSideBarAction, &KToggleAction::triggered, q, &MainWindow::toggleSideBar);
        mToggleSideBarAction->setIcon(QIcon::fromTheme(QStringLiteral("view-sidetree")));
        actionCollection->setDefaultShortcut(mToggleSideBarAction, Qt::Key_F4);
        mToggleSideBarAction->setText(i18nc("@action", "Sidebar"));
        connect(mBrowseMainPage->toggleSideBarButton(), &QAbstractButton::clicked, mToggleSideBarAction, &QAction::trigger);
        connect(mViewMainPage->toggleSideBarButton(), &QAbstractButton::clicked, mToggleSideBarAction, &QAction::trigger);

        mToggleOperationsSideBarAction = view->add<KToggleAction>(QStringLiteral("toggle_operations_sidebar"));
        mToggleOperationsSideBarAction->setText(i18nc("@action opens crop, rename, etc.", "Show Editing Tools"));
        mToggleOperationsSideBarAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-image"), QIcon::fromTheme(QStringLiteral("document-edit"))));
        connect(mToggleOperationsSideBarAction, &KToggleAction::triggered, q, &MainWindow::toggleOperationsSideBar);
        connect(mSideBar, &QTabWidget::currentChanged, mToggleOperationsSideBarAction, [=]() {
            mToggleOperationsSideBarAction->setChecked(mSideBar->isVisible() && mSideBar->currentPage() == QLatin1String("operations"));
        });

        mToggleSlideShowAction = view->addAction(QStringLiteral("toggle_slideshow"), q, SLOT(toggleSlideShow()));
        q->updateSlideShowAction();
        connect(mSlideShow, &SlideShow::stateChanged, q, &MainWindow::updateSlideShowAction);

        q->setStandardToolBarMenuEnabled(true);

        mShowMenuBarAction = static_cast<KToggleAction *>(view->addAction(KStandardAction::ShowMenubar, q, SLOT(toggleMenuBar())));
        mShowStatusBarAction = static_cast<KToggleAction *>(view->addAction(KStandardAction::ShowStatusbar, q, SLOT(toggleStatusBar(bool))));

        actionCollection->setDefaultShortcut(mShowStatusBarAction, Qt::Key_F3);

        view->addAction(KStandardAction::name(KStandardAction::KeyBindings),
                        KStandardAction::keyBindings(q, &MainWindow::configureShortcuts, actionCollection));

        connect(q->guiFactory(), &KXMLGUIFactory::shortcutsSaved, q, [this]() {
            q->guiFactory()->refreshActionProperties();
        });

        view->addAction(KStandardAction::Preferences, q, SLOT(showConfigDialog()));

        view->addAction(KStandardAction::ConfigureToolbars, q, SLOT(configureToolbars()));

#if HAVE_PURPOSE
        mShareAction = new KToolBarPopupAction(QIcon::fromTheme(QStringLiteral("document-share")), i18nc("@action Share images", "Share"), q);

        mShareAction->setPopupMode(KToolBarPopupAction::InstantPopup);
        actionCollection->addAction(QStringLiteral("share"), mShareAction);
        mShareMenu = new Purpose::Menu(q);
        mShareAction->setMenu(mShareMenu);

        connect(mShareMenu, &Purpose::Menu::finished, q, [this](const QJsonObject &output, int error, const QString &message) {
            if (error && error != KIO::ERR_USER_CANCELED) {
                mSharedMessage->setText(i18n("Error while sharing: %1", message));
                mSharedMessage->setMessageType(KMessageWidget::MessageType::Error);
                mSharedMessage->animatedShow();
            } else {
                const QString url = output[QStringLiteral("url")].toString();

                if (!url.isEmpty()) {
                    mSharedMessage->setText(i18n("The shared image link (<a href=\"%1\">%1</a>) has been copied to the clipboard.", url));
                    mSharedMessage->setMessageType(KMessageWidget::MessageType::Positive);
                    mSharedMessage->animatedShow();
                    QApplication::clipboard()->setText(url);
                } else {
                    mSharedMessage->setVisible(false);
                }
            }
        });
#endif

        auto alignWithSideBarWidgetAction = new AlignWithSideBarWidgetAction(q);
        alignWithSideBarWidgetAction->setSideBar(mSideBar);
        actionCollection->addAction("align_with_sidebar", alignWithSideBarWidgetAction);

        mHamburgerMenu = KStandardAction::hamburgerMenu(nullptr, nullptr, actionCollection);
        mHamburgerMenu->setShowMenuBarAction(mShowMenuBarAction);
        mHamburgerMenu->setMenuBar(q->menuBar());
        connect(mHamburgerMenu, &KHamburgerMenu::aboutToShowMenu, q, [this]() {
            this->updateHamburgerMenu();
            // Immediately disconnect. We only need to run this once, but on demand.
            // NOTE: The nullptr at the end disconnects all connections between
            // q and mHamburgerMenu's aboutToShowMenu signal.
            disconnect(mHamburgerMenu, &KHamburgerMenu::aboutToShowMenu, q, nullptr);
        });
    }

    void updateHamburgerMenu()
    {
        KActionCollection *actionCollection = q->actionCollection();
        auto menu = new QMenu;
        menu->addAction(actionCollection->action(KStandardAction::name(KStandardAction::Open)));
        menu->addAction(actionCollection->action(KStandardAction::name(KStandardAction::OpenRecent)));
        menu->addAction(actionCollection->action(KStandardAction::name(KStandardAction::Save)));
        menu->addAction(actionCollection->action(KStandardAction::name(KStandardAction::SaveAs)));
        menu->addAction(actionCollection->action(KStandardAction::name(KStandardAction::Print)));
        menu->addAction(actionCollection->action(KStandardAction::name(KStandardAction::PrintPreview)));
        menu->addSeparator();
        menu->addAction(actionCollection->action(KStandardAction::name(KStandardAction::Copy)));
        menu->addAction(actionCollection->action(QStringLiteral("file_trash")));
        menu->addSeparator();
        menu->addAction(mBrowseAction);
        menu->addAction(mViewAction);
        menu->addAction(actionCollection->action(QStringLiteral("sort_by")));
        menu->addAction(mFullScreenAction);
        menu->addAction(mToggleSlideShowAction);
        menu->addSeparator();
#if HAVE_PURPOSE
        menu->addMenu(mShareMenu);
#endif
        auto configureMenu = new QMenu(i18nc("@title:menu submenu for actions that open configuration dialogs", "Configure"));
        configureMenu->addAction(actionCollection->action(QStringLiteral("options_configure_keybinding")));
        configureMenu->addAction(actionCollection->action(QStringLiteral("options_configure_toolbars")));
        configureMenu->addAction(actionCollection->action(QStringLiteral("options_configure")));
        menu->addMenu(configureMenu);
        mHamburgerMenu->setMenu(menu);
    }

    void setupUndoActions()
    {
        // There is no KUndoGroup similar to KUndoStack. This code basically
        // does the same as KUndoStack, but for the KUndoGroup actions.
        QUndoGroup *undoGroup = DocumentFactory::instance()->undoGroup();
        QAction *action;
        KActionCollection *actionCollection = q->actionCollection();
        auto edit = new KActionCategory(i18nc("@title actions category - means actions changing smth in interface", "Edit"), actionCollection);

        action = undoGroup->createRedoAction(actionCollection);
        action->setObjectName(KStandardAction::name(KStandardAction::Redo));
        action->setIcon(QIcon::fromTheme(QStringLiteral("edit-redo")));
        action->setIconText(i18n("Redo"));
        actionCollection->setDefaultShortcuts(action, KStandardShortcut::redo());

        edit->addAction(action->objectName(), action);

        action = undoGroup->createUndoAction(actionCollection);
        action->setObjectName(KStandardAction::name(KStandardAction::Undo));
        action->setIcon(QIcon::fromTheme(QStringLiteral("edit-undo")));
        action->setIconText(i18n("Undo"));
        actionCollection->setDefaultShortcuts(action, KStandardShortcut::undo());
        edit->addAction(action->objectName(), action);
    }

    void setupContextManagerItems()
    {
        Q_ASSERT(mContextManager);
        KActionCollection *actionCollection = q->actionCollection();

        // Create context manager items
        auto folderViewItem = new FolderViewContextManagerItem(mContextManager);
        connect(folderViewItem, &FolderViewContextManagerItem::urlChanged, q, &MainWindow::folderViewUrlChanged);

        auto infoItem = new InfoContextManagerItem(mContextManager);

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
        SemanticInfoContextManagerItem *semanticInfoItem = nullptr;
        semanticInfoItem = new SemanticInfoContextManagerItem(mContextManager, actionCollection, mViewMainPage);
#endif

        auto imageOpsItem = new ImageOpsContextManagerItem(mContextManager, q);

        auto fileOpsItem = new FileOpsContextManagerItem(mContextManager, mThumbnailView, actionCollection, q);

        // Fill sidebar
        SideBarPage *page;
        page = new SideBarPage(QIcon::fromTheme(QStringLiteral("folder")), i18n("Folders"));
        page->setObjectName(QStringLiteral("folders"));
        page->addWidget(folderViewItem->widget());
        page->layout()->setContentsMargins(0, 0, 0, 0);
        mSideBar->addPage(page);

        page = new SideBarPage(QIcon::fromTheme(QStringLiteral("documentinfo")), i18n("Information"));
        page->setObjectName(QStringLiteral("information"));
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
        // If we have semantic info, we want to share the sidebar using a splitter,
        // so the user can dynamically resize the two widgets.
        if (semanticInfoItem) {
            auto splitter = new QSplitter;
            splitter->setObjectName(QStringLiteral("information_splitter")); // This name is used to find it when loading previous sizes.
            splitter->setOrientation(Qt::Vertical);
            splitter->setHandleWidth(5);

            splitter->addWidget(infoItem->widget());
            splitter->setCollapsible(0, false);

            // Cram the semantic info widget and a separator into a separate widget,
            // so that they can be added to the splitter together (layouts can't be added directly).
            // This will give the splitter a visible separator between the two widgets.
            auto separator = new QFrame;
            separator->setFrameShape(QFrame::HLine);
            separator->setLineWidth(1);

            auto container = new QWidget;
            auto containerLayout = new QVBoxLayout(container);
            containerLayout->setContentsMargins(0, 0, 0, 0);
            containerLayout->addWidget(separator);
            containerLayout->addWidget(semanticInfoItem->widget());

            splitter->addWidget(container);
            splitter->setCollapsible(1, false);

            page->addWidget(splitter);
        } else {
            page->addWidget(infoItem->widget());
        }
#else
        page->addWidget(infoItem->widget());
#endif
        mSideBar->addPage(page);

        page = new SideBarPage(QIcon::fromTheme(QStringLiteral("document-edit")), i18n("Operations"));
        page->setObjectName(QStringLiteral("operations"));
        page->addWidget(imageOpsItem->widget());
        auto separator = new QFrame;
        separator->setFrameShape(QFrame::HLine);
        separator->setLineWidth(1);
        page->addWidget(separator);
        page->addWidget(fileOpsItem->widget());
        page->addStretch();
        mSideBar->addPage(page);
    }

    void initDirModel()
    {
        mDirModel->setKindFilter(MimeTypeUtils::KIND_DIR | MimeTypeUtils::KIND_ARCHIVE | MimeTypeUtils::KIND_RASTER_IMAGE | MimeTypeUtils::KIND_SVG_IMAGE
                                 | MimeTypeUtils::KIND_VIDEO);

        connect(mDirModel, &QAbstractItemModel::rowsInserted, q, &MainWindow::slotDirModelNewItems);

        connect(mDirModel, &QAbstractItemModel::rowsRemoved, q, &MainWindow::updatePreviousNextActions);
        connect(mDirModel, &QAbstractItemModel::modelReset, q, &MainWindow::updatePreviousNextActions);

        connect(mDirModel->dirLister(), SIGNAL(completed()), q, SLOT(slotDirListerCompleted()));
    }

    void setupThumbnailBarModel()
    {
        mThumbnailBarModel = new DocumentOnlyProxyModel(q);
        mThumbnailBarModel->setSourceModel(mDirModel);
    }

    bool indexIsDirOrArchive(const QModelIndex &index) const
    {
        Q_ASSERT(index.isValid());
        KFileItem item = mDirModel->itemForIndex(index);
        return ArchiveUtils::fileItemIsDirOrArchive(item);
    }

    void goTo(const QModelIndex &index)
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

    inline void setActionEnabled(const char *name, bool enabled)
    {
        QAction *action = q->actionCollection()->action(name);
        if (action) {
            action->setEnabled(enabled);
        } else {
            qCWarning(GWENVIEW_APP_LOG) << "Action" << name << "not found";
        }
    }

    void setActionsDisabledOnStartMainPageEnabled(bool enabled)
    {
        mBrowseAction->setEnabled(enabled);
        mViewAction->setEnabled(enabled);
        mToggleSideBarAction->setEnabled(enabled);
        mToggleOperationsSideBarAction->setEnabled(enabled);
        mShowStatusBarAction->setEnabled(enabled);
        mFullScreenAction->setEnabled(enabled);
        mToggleSlideShowAction->setEnabled(enabled);

        setActionEnabled("reload", enabled);
        setActionEnabled("go_start_page", enabled);
        setActionEnabled("add_folder_to_places", enabled);
        setActionEnabled("view_toggle_spotlightmode", enabled);
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
            if (mCurrentMainPageId != ViewMainPageId && mContextManager->selectedFileItemList().count() != 1) {
                // Saving only makes sense if exactly one image is selected
                canSave = false;
            }
        }

        KActionCollection *actionCollection = q->actionCollection();
        actionCollection->action(QStringLiteral("file_save"))->setEnabled(canSave && isModified);
        actionCollection->action(QStringLiteral("file_save_as"))->setEnabled(canSave);
        actionCollection->action(QStringLiteral("file_print"))->setEnabled(isRasterImage);
        actionCollection->action(QStringLiteral("file_print_preview"))->setEnabled(isRasterImage);

#if HAVE_PURPOSE
        const KFileItemList selectedFiles = mContextManager->selectedFileItemList();

        if (selectedFiles.isEmpty()) {
            mShareAction->setEnabled(false);
        } else {
            mShareAction->setEnabled(true);

            QJsonArray urls;
            for (const KFileItem &fileItem : selectedFiles) {
                urls << QJsonValue(fileItem.url().toString());
            }

            mShareMenu->model()->setInputData(QJsonObject{{QStringLiteral("mimeType"), MimeTypeUtils::urlMimeType(url)}, {QStringLiteral("urls"), urls}});
            mShareMenu->model()->setPluginType(QStringLiteral("Export"));
            mShareMenu->reload();
        }
#endif
    }

    bool sideBarVisibility() const
    {
        switch (mCurrentMainPageId) {
        case StartMainPageId:
            GV_WARN_AND_RETURN_VALUE(false, "Sidebar not implemented on start page");
            break;
        case BrowseMainPageId:
            return GwenviewConfig::sideBarVisible();
        case ViewMainPageId:
            return q->isFullScreen()              ? GwenviewConfig::sideBarVisibleViewModeFullScreen()
                : GwenviewConfig::spotlightMode() ? GwenviewConfig::sideBarVisibleSpotlightMode()
                                                  : GwenviewConfig::sideBarVisible();
        }
        return false;
    }

    void saveSideBarVisibility(const bool visible)
    {
        switch (mCurrentMainPageId) {
        case StartMainPageId:
            GV_WARN_AND_RETURN("Sidebar not implemented on start page");
            break;
        case BrowseMainPageId:
            GwenviewConfig::setSideBarVisible(visible);
            break;
        case ViewMainPageId:
            q->isFullScreen()                     ? GwenviewConfig::setSideBarVisibleViewModeFullScreen(visible)
                : GwenviewConfig::spotlightMode() ? GwenviewConfig::setSideBarVisibleSpotlightMode(visible)
                                                  : GwenviewConfig::setSideBarVisible(visible);
            break;
        }
    }

    bool statusBarVisibility() const
    {
        switch (mCurrentMainPageId) {
        case StartMainPageId:
            GV_WARN_AND_RETURN_VALUE(false, "Statusbar not implemented on start page");
            break;
        case BrowseMainPageId:
            return GwenviewConfig::statusBarVisibleBrowseMode();
        case ViewMainPageId:
            return q->isFullScreen()              ? GwenviewConfig::statusBarVisibleViewModeFullScreen()
                : GwenviewConfig::spotlightMode() ? GwenviewConfig::statusBarVisibleSpotlightMode()
                                                  : GwenviewConfig::statusBarVisibleViewMode();
        }
        return false;
    }

    void saveStatusBarVisibility(const bool visible)
    {
        switch (mCurrentMainPageId) {
        case StartMainPageId:
            GV_WARN_AND_RETURN("Statusbar not implemented on start page");
            break;
        case BrowseMainPageId:
            GwenviewConfig::setStatusBarVisibleBrowseMode(visible);
            break;
        case ViewMainPageId:
            q->isFullScreen()                     ? GwenviewConfig::setStatusBarVisibleViewModeFullScreen(visible)
                : GwenviewConfig::spotlightMode() ? GwenviewConfig::setStatusBarVisibleSpotlightMode(visible)
                                                  : GwenviewConfig::setStatusBarVisibleViewMode(visible);
            break;
        }
    }

    void loadSplitterConfig()
    {
        const QList<int> sizes = GwenviewConfig::sideBarSplitterSizes();
        if (!sizes.isEmpty()) {
            mCentralSplitter->setSizes(sizes);
        }
    }

    void saveSplitterConfig()
    {
        if (mSideBar->isVisible()) {
            GwenviewConfig::setSideBarSplitterSizes(mCentralSplitter->sizes());
        }
    }

    void loadInformationSplitterConfig()
    {
        const QList<int> sizes = GwenviewConfig::informationSplitterSizes();
        if (!sizes.isEmpty()) {
            // Find the splitter inside the sidebar by objectName.
            auto informationSidebar = mSideBar->findChild<QSplitter *>(QStringLiteral("information_splitter"), Qt::FindChildrenRecursively);
            if (informationSidebar) {
                informationSidebar->setSizes(sizes);
            } else {
                qCWarning(GWENVIEW_APP_LOG) << "Could not find information splitter in sidebar when loading old position.";
            }
        }
    }

    void saveInformationSplitterConfig()
    {
        auto informationSidebar = mSideBar->findChild<QSplitter *>(QStringLiteral("information_splitter"), Qt::FindChildrenRecursively);
        if (informationSidebar) {
            GwenviewConfig::setInformationSplitterSizes(informationSidebar->sizes());
        } else {
            qCWarning(GWENVIEW_APP_LOG) << "Could not find information splitter in sidebar when saving new position.";
        }
    }

    void setScreenSaverEnabled(bool enabled)
    {
#ifdef HAVE_QTDBUS
        if (enabled) {
            QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.ScreenSaver"),
                                                                  QStringLiteral("/ScreenSaver"),
                                                                  QStringLiteral("org.freedesktop.ScreenSaver"),
                                                                  QStringLiteral("Inhibit"));
            message << QGuiApplication::desktopFileName();
            message << i18n("Viewing media");
            QDBusReply<uint> reply = QDBusConnection::sessionBus().call(message);
            if (reply.isValid()) {
                screenSaverDbusCookie = reply.value();
            }
        } else {
            if (screenSaverDbusCookie != 0) {
                QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.ScreenSaver"),
                                                                      QStringLiteral("/ScreenSaver"),
                                                                      QStringLiteral("org.freedesktop.ScreenSaver"),
                                                                      QStringLiteral("UnInhibit"));
                message << static_cast<uint>(screenSaverDbusCookie);
                screenSaverDbusCookie = 0;
                QDBusConnection::sessionBus().send(message);
            }
        }
#endif
    }

    void assignThumbnailProviderToThumbnailView(ThumbnailView *thumbnailView)
    {
        GV_RETURN_IF_FAIL(thumbnailView);
        if (mActiveThumbnailView) {
            mActiveThumbnailView->setThumbnailProvider(nullptr);
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

    enum class ShowPreview { Yes, No };
    void print(ShowPreview showPreview)
    {
        if (!mContextManager->currentUrlIsRasterImage()) {
            return;
        }

        Document::Ptr doc = DocumentFactory::instance()->load(mContextManager->currentUrl());
        PrintHelper printHelper(q);
        if (showPreview == ShowPreview::Yes) {
            printHelper.printPreview(doc);
        } else {
            printHelper.print(doc);
        }
    }

    /// Handles the clicking of links in help texts if the clicked url uses the "gwenview:" scheme.
    class InternalUrlClickedHandler : public QObject
    {
    public:
        InternalUrlClickedHandler(MainWindow *parent)
            : QObject(parent)
        {
            Q_CHECK_PTR(parent);
        }

        inline bool eventFilter(QObject * /* watched */, QEvent *event) override
        {
            if (event->type() != QEvent::WhatsThisClicked) {
                return false;
            }
            const QString linkAddress = static_cast<QWhatsThisClickedEvent *>(event)->href();
            if (!linkAddress.startsWith(QStringLiteral("gwenview:"))) {
                // This eventFilter only handles our internal "gwenview" scheme. Everything else is handled by KXmlGui::KToolTipHelper.
                return false;
            }
            event->accept();
            auto linkPathList = linkAddress.split(QLatin1Char('/'));
            linkPathList.removeFirst(); // remove "gwenview:/"
            Q_ASSERT(!linkPathList.isEmpty());
            Q_ASSERT_X(linkPathList.constFirst() == QStringLiteral("config"), "link resolver", "Handling of this URL is not yet implemented");
            linkPathList.removeFirst(); // remove "config/"
            auto mainWindow = static_cast<MainWindow *>(parent());
            if (linkPathList.isEmpty()) {
                mainWindow->showConfigDialog();
            } else if (linkPathList.constFirst() == QStringLiteral("general")) {
                mainWindow->showConfigDialog(0); // "0" should open General.
            } else if (linkPathList.constFirst() == QStringLiteral("imageview")) {
                mainWindow->showConfigDialog(1); // "1" should open Image View.
            } else if (linkPathList.constFirst() == QStringLiteral("advanced")) {
                mainWindow->showConfigDialog(2); // "2" should open Advanced.
            } else {
                Q_ASSERT_X(false, "config link resolver", "Handling of this config URL is not yet implemented");
            }
            return true;
        }
    };
};

MainWindow::MainWindow()
    : KXmlGuiWindow()
    , d(new MainWindow::Private)
{
    d->q = this;
    d->mCurrentMainPageId = StartMainPageId;
    d->mDirModel = new SortedDirModel(this);
    d->setupContextManager();
    d->setupThumbnailBarModel();
    d->mGvCore = new GvCore(this, d->mDirModel);
    d->mPreloader = new Preloader(this);
    d->mThumbnailProvider = new ThumbnailProvider();
    d->mActiveThumbnailView = nullptr;
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

    connect(DocumentFactory::instance(), &DocumentFactory::modifiedDocumentListChanged, this, &MainWindow::slotModifiedDocumentListChanged);
    connect(qApp, &QApplication::focusChanged, this, &MainWindow::onFocusChanged);

#ifdef HAVE_QTDBUS
    d->mMpris2Service =
        new Mpris2Service(d->mSlideShow, d->mContextManager, d->mToggleSlideShowAction, d->mFullScreenAction, d->mGoToPreviousAction, d->mGoToNextAction, this);
#endif

    auto ratingMenu = static_cast<QMenu *>(guiFactory()->container(QStringLiteral("rating"), this));
    ratingMenu->setIcon(QIcon::fromTheme(QStringLiteral("rating-unrated")));
#ifdef GWENVIEW_SEMANTICINFO_BACKEND_NONE
    if (ratingMenu) {
        ratingMenu->menuAction()->setVisible(false);
    }
#endif
    setAutoSaveSettings();
#ifdef Q_OS_OSX
    qApp->installEventFilter(this);
#endif
    qApp->installEventFilter(new Private::InternalUrlClickedHandler(this));
}

MainWindow::~MainWindow()
{
    delete d->mThumbnailProvider;
    delete d;
}

ContextManager *MainWindow::contextManager() const
{
    return d->mContextManager;
}

ViewMainPage *MainWindow::viewMainPage() const
{
    return d->mViewMainPage;
}

void MainWindow::setCaption(const QString &caption)
{
    // Keep a trace of caption to use it in slotModifiedDocumentListChanged()
    d->mCaption = caption;
    KXmlGuiWindow::setCaption(caption);
}

void MainWindow::setCaption(const QString &caption, bool modified)
{
    d->mCaption = caption;
    KXmlGuiWindow::setCaption(caption, modified);
}

void MainWindow::slotUpdateCaption(const QString &caption)
{
    const QUrl url = d->mContextManager->currentUrl();
    const QList<QUrl> list = DocumentFactory::instance()->modifiedDocumentList();
    setCaption(caption, list.contains(url));
}

void MainWindow::slotModifiedDocumentListChanged()
{
    d->updateActions();
    slotUpdateCaption(d->mCaption);
}

void MainWindow::setInitialUrl(const QUrl &_url)
{
    Q_ASSERT(_url.isValid());
    QUrl url = UrlUtils::fixUserEnteredUrl(_url);
    d->mGvCore->setTrackFileManagerSorting(true);
    syncSortOrder(url);

    if (UrlUtils::urlIsDirectory(url)) {
        d->mBrowseAction->trigger();
        openDirUrl(url);
    } else {
        openUrl(url);
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

void MainWindow::setActiveViewModeAction(QAction *action)
{
    if (action == d->mViewAction) {
        d->mCurrentMainPageId = ViewMainPageId;
        // Switching to view mode
        d->mViewStackedWidget->setCurrentWidget(d->mViewMainPage);
        openSelectedDocuments();
        d->mPreloadDirectionIsForward = true;
        QTimer::singleShot(VIEW_PRELOAD_DELAY, this, &MainWindow::preloadNextUrl);
    } else {
        d->mCurrentMainPageId = BrowseMainPageId;
        // Switching to browse mode
        d->mViewStackedWidget->setCurrentWidget(d->mBrowseMainPage);
        if (!d->mViewMainPage->isEmpty() && KProtocolManager::supportsListing(d->mViewMainPage->url())) {
            // Reset the view to spare resources, but don't do it if we can't
            // browse the url, otherwise if the user starts Gwenview this way:
            // gwenview http://example.com/example.png
            // and switch to browse mode, switching back to view mode won't bring
            // his image back.
            d->mViewMainPage->reset();
        }
        setCaption(d->mUrlNavigator->locationUrl().adjusted(QUrl::RemoveScheme).toString());
    }
    d->autoAssignThumbnailProvider();
    toggleSideBar(d->sideBarVisibility());
    toggleStatusBar(d->statusBarVisibility());

    Q_EMIT viewModeChanged();
}

void MainWindow::slotThumbnailViewIndexActivated(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    KFileItem item = d->mDirModel->itemForIndex(index);
    syncSortOrder(item.url());
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

void MainWindow::openSelectedDocuments()
{
    if (d->mCurrentMainPageId != ViewMainPageId) {
        return;
    }

    int count = 0;
    QList<QUrl> urls;
    const auto list = d->mContextManager->selectedFileItemList();
    for (const auto &item : list) {
        if (!item.isNull() && !ArchiveUtils::fileItemIsDirOrArchive(item)) {
            urls << item.url();

            ++count;
            if (count == ViewMainPage::MaxViewCount) {
                break;
            }
        }
    }

    if (urls.isEmpty()) {
        // Selection contains no fitting items
        // Switch back to browsing mode
        d->mBrowseAction->trigger();
        d->mViewMainPage->reset();
        return;
    }

    QUrl currentUrl = d->mContextManager->currentUrl();
    if (currentUrl.isEmpty() || !urls.contains(currentUrl)) {
        // There is no current URL or it doesn't belong to selection
        // This can happen when user manually selects a group of items
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
    d->mCurrentMainPageId = StartMainPageId;
    d->setActionsDisabledOnStartMainPageEnabled(false);

    d->saveSplitterConfig();
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
        const QString wantedPath = QDir::cleanPath(url.adjusted(QUrl::StripTrailingSlash).path());
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

void MainWindow::folderViewUrlChanged(const QUrl &url)
{
    const QUrl currentUrl = d->mContextManager->currentDirUrl();

    if (url == currentUrl) {
        switch (d->mCurrentMainPageId) {
        case ViewMainPageId:
            d->mBrowseAction->trigger();
            break;
        case BrowseMainPageId:
            d->mViewAction->trigger();
            break;
        case StartMainPageId:
            break;
        }
    } else {
        openDirUrl(url);
    }
}

void MainWindow::syncSortOrder(const QUrl &url)
{
    if (!d->mGvCore->trackFileManagerSorting()) {
        return;
    }

#ifdef HAVE_QTDBUS
    QDBusMessage message = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.FileManager1"),
                                                          QStringLiteral("/org/freedesktop/FileManager1"),
                                                          QStringLiteral("org.freedesktop.FileManager1"),
                                                          QStringLiteral("SortOrderForUrl"));

    QUrl dirUrl = url;
    dirUrl = dirUrl.adjusted(QUrl::RemoveFilename);
    message << dirUrl.toString();

    QDBusPendingCall call = QDBusConnection::sessionBus().asyncCall(message);
    auto watcher = new QDBusPendingCallWatcher(call, this);

    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *call) {
        QDBusPendingReply<QString, QString> reply = *call;
        // Fail silently
        if (!reply.isError()) {
            QString columnName = reply.argumentAt<0>();
            QString orderName = reply.argumentAt<1>();

            int column = -1;
            int sortRole = -1;
            Qt::SortOrder order = orderName == QStringLiteral("descending") ? Qt::DescendingOrder : Qt::AscendingOrder;

            if (columnName == "text") {
                column = KDirModel::Name;
                sortRole = Qt::DisplayRole;
                qCDebug(GWENVIEW_APP_LOG) << "Sorting according to file manager: text";
            } else if (columnName == "modificationtime") {
                column = KDirModel::ModifiedTime;
                sortRole = Qt::DisplayRole;
                qCDebug(GWENVIEW_APP_LOG) << "Sorting according to file manager: modtime";
            } else if (columnName == "size") {
                column = KDirModel::Size;
                sortRole = Qt::DisplayRole;
                qCDebug(GWENVIEW_APP_LOG) << "Sorting according to file manager: size";
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
            } else if (columnName == "rating") {
                column = KDirModel::Name;
                sortRole = SemanticInfoDirModel::RatingRole;
                qCDebug(GWENVIEW_APP_LOG) << "Sorting according to file manager: rating";
#endif
            }

            if (column >= 0 && sortRole >= 0) {
                d->mDirModel->setSortRole(sortRole);
                d->mDirModel->sort(column, order);
            }
        }
        call->deleteLater();
    });
#endif
}

void MainWindow::toggleSideBar(bool visible)
{
    d->saveSplitterConfig();
    d->mToggleSideBarAction->setChecked(visible);
    d->mToggleOperationsSideBarAction->setChecked(visible && d->mSideBar->currentPage() == QLatin1String("operations"));
    d->saveSideBarVisibility(visible);
    d->mSideBar->setVisible(visible);

    const QString iconName = QApplication::isRightToLeft() ? (visible ? "sidebar-collapse-right" : "sidebar-expand-right")
                                                           : (visible ? "sidebar-collapse-left" : "sidebar-expand-left");
    const QString toolTip = visible ? i18nc("@info:tooltip", "Hide sidebar") : i18nc("@info:tooltip", "Show sidebar");

    const QList<QToolButton *> buttonList{d->mBrowseMainPage->toggleSideBarButton(), d->mViewMainPage->toggleSideBarButton()};
    for (auto button : buttonList) {
        button->setIcon(QIcon::fromTheme(iconName));
        button->setToolTip(toolTip);
    }
}

void MainWindow::toggleOperationsSideBar(bool visible)
{
    if (visible) {
        d->mSideBar->setCurrentPage(QLatin1String("operations"));
    }
    toggleSideBar(visible);
}

void MainWindow::toggleStatusBar(bool visible)
{
    d->mShowStatusBarAction->setChecked(visible);
    d->saveStatusBarVisibility(visible);

    d->mViewMainPage->setStatusBarVisible(visible);
    d->mBrowseMainPage->setStatusBarVisible(visible);
}

void MainWindow::slotPartCompleted()
{
    d->updateActions();
    const QUrl url = d->mContextManager->currentUrl();
    if (!url.isEmpty() && GwenviewConfig::historyEnabled()) {
        d->mFileOpenRecentAction->addUrl(url);
        d->mGvCore->addUrlToRecentFiles(url);
    }

    if (KProtocolManager::supportsListing(url)) {
        const QUrl dirUrl = d->mContextManager->currentDirUrl();
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
        QUndoGroup *undoGroup = DocumentFactory::instance()->undoGroup();
        if (!item.isNull() && !ArchiveUtils::fileItemIsDirOrArchive(item)) {
            QUrl url = item.url();
            Document::Ptr doc = DocumentFactory::instance()->load(url);
            undoGroup->addStack(doc->undoStack());
            undoGroup->setActiveStack(doc->undoStack());
        } else {
            undoGroup->setActiveStack(nullptr);
        }
    }

    // Update UI
    d->updateActions();
    updatePreviousNextActions();

    // Start preloading
    int preloadDelay = d->mCurrentMainPageId == ViewMainPageId ? VIEW_PRELOAD_DELAY : BROWSE_PRELOAD_DELAY;
    QTimer::singleShot(preloadDelay, this, &MainWindow::preloadNextUrl);
}

void MainWindow::slotCurrentDirUrlChanged(const QUrl &url)
{
    if (url.isValid()) {
        d->mUrlNavigator->setLocationUrl(url);
        d->mGoUpAction->setEnabled(url.path() != "/");
        if (d->mCurrentMainPageId == BrowseMainPageId) {
            setCaption(d->mUrlNavigator->locationUrl().toDisplayString(QUrl::PreferLocalFile));
        }
    } else {
        d->mGoUpAction->setEnabled(false);
    }
}

void MainWindow::slotDirModelNewItems()
{
    if (d->mContextManager->selectionModel()->hasSelection()) {
        updatePreviousNextActions();
    }
}

void MainWindow::slotDirListerCompleted()
{
    if (d->mStartSlideShowWhenDirListerCompleted) {
        d->mStartSlideShowWhenDirListerCompleted = false;
        QTimer::singleShot(0, d->mToggleSlideShowAction, &QAction::trigger);
    }
    if (d->mContextManager->selectionModel()->hasSelection()) {
        updatePreviousNextActions();
    } else if (!d->mContextManager->currentUrl().isValid()) {
        d->goToFirstDocument();

        // Try to select the first directory in case there are no images to select
        if (!d->mContextManager->selectionModel()->hasSelection()) {
            const QModelIndex index = d->mThumbnailView->model()->index(0, 0);
            if (index.isValid()) {
                d->mThumbnailView->setCurrentIndex(index);
            }
        }
    }
    d->mThumbnailView->scrollToSelectedIndex();
    d->mViewMainPage->thumbnailBar()->scrollToSelectedIndex();
    d->mFullScreenContent->thumbnailBar()->scrollToSelectedIndex();
}

void MainWindow::goToPrevious()
{
    const QModelIndex currentIndex = d->mContextManager->selectionModel()->currentIndex();
    QModelIndex previousIndex = d->mDirModel->index(currentIndex.row(), 0);

    constexpr QFlags<MimeTypeUtils::Kind> allowedKinds = MimeTypeUtils::KIND_RASTER_IMAGE | MimeTypeUtils::KIND_SVG_IMAGE | MimeTypeUtils::KIND_VIDEO;
    KFileItem fileItem;

    // Besides images also folders and archives are listed as well,
    // we need to skip them in the slideshow
    do {
        previousIndex = d->mDirModel->index(previousIndex.row() - 1, 0);
        fileItem = previousIndex.data(KDirModel::FileItemRole).value<KFileItem>();
    } while (previousIndex.isValid() && !(allowedKinds & MimeTypeUtils::fileItemKind(fileItem)));

    if (!previousIndex.isValid()
        && (GwenviewConfig::navigationEndNotification() == SlideShow::NeverWarn
            || (GwenviewConfig::navigationEndNotification() == SlideShow::WarnOnSlideshow && !d->mSlideShow->isRunning() && !d->mFullScreenAction->isChecked())
            || d->hudButtonBox)) {
        d->goToLastDocument();
    } else if (!previousIndex.isValid()) {
        showFirstDocumentReached();
    } else {
        d->goTo(-1);
    }
}

void MainWindow::goToNext()
{
    const QModelIndex currentIndex = d->mContextManager->selectionModel()->currentIndex();
    QModelIndex nextIndex = d->mDirModel->index(currentIndex.row(), 0);

    constexpr QFlags<MimeTypeUtils::Kind> allowedKinds = MimeTypeUtils::KIND_RASTER_IMAGE | MimeTypeUtils::KIND_SVG_IMAGE | MimeTypeUtils::KIND_VIDEO;
    KFileItem fileItem;

    // Besides images also folders and archives are listed as well,
    // we need to skip them in the slideshow
    do {
        nextIndex = d->mDirModel->index(nextIndex.row() + 1, 0);
        fileItem = nextIndex.data(KDirModel::FileItemRole).value<KFileItem>();
    } while (nextIndex.isValid() && !(allowedKinds & MimeTypeUtils::fileItemKind(fileItem)));

    if (!nextIndex.isValid()
        && (GwenviewConfig::navigationEndNotification() == SlideShow::NeverWarn
            || (GwenviewConfig::navigationEndNotification() == SlideShow::WarnOnSlideshow && !d->mSlideShow->isRunning() && !d->mFullScreenAction->isChecked())
            || d->hudButtonBox)) {
        d->goToFirstDocument();
    } else if (!nextIndex.isValid()) {
        showLastDocumentReached();
    } else {
        d->goTo(1);
    }
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
        hasPrevious = GwenviewConfig::navigationEndNotification() != SlideShow::AlwaysWarn || (prevIndex.isValid() && !d->indexIsDirOrArchive(prevIndex));
        hasNext = GwenviewConfig::navigationEndNotification() != SlideShow::AlwaysWarn || (nextIndex.isValid() && !d->indexIsDirOrArchive(nextIndex));
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
    if (checked) {
        leaveSpotlightMode();
        // Save MainWindow config now, this way if we quit while in
        // fullscreen, we are sure latest MainWindow changes are remembered.
        KConfigGroup saveConfigGroup = autoSaveConfigGroup();
        if (!isFullScreen()) {
            // Save state if window manager did not already switch to fullscreen.
            saveMainWindowSettings(saveConfigGroup);
            d->mStateBeforeFullScreen.mToolBarVisible = toolBar()->isVisible();
        }
        setAutoSaveSettings(saveConfigGroup, false);
        resetAutoSaveSettings();

        // Go full screen
        KToggleFullScreenAction::setFullScreen(this, true);
        menuBar()->hide();
        toolBar()->hide();

        qApp->setProperty("KDE_COLOR_SCHEME_PATH", d->mGvCore->fullScreenPaletteName());
        QApplication::setPalette(d->mGvCore->palette(GvCore::FullScreenPalette));

        d->setScreenSaverEnabled(false);
    } else {
        setAutoSaveSettings();

        // Back to normal
        qApp->setProperty("KDE_COLOR_SCHEME_PATH", QVariant());
        QApplication::setPalette(d->mGvCore->palette(GvCore::NormalPalette));

        d->mSlideShow->pause();
        KToggleFullScreenAction::setFullScreen(this, false);
        menuBar()->setVisible(d->mShowMenuBarAction->isChecked());
        toolBar()->setVisible(d->mStateBeforeFullScreen.mToolBarVisible);

        d->setScreenSaverEnabled(true);

        // See resizeEvent
        d->mFullScreenLeftAt = QDateTime::currentDateTime();
    }

    d->mFullScreenContent->setFullScreenMode(checked);
    d->mBrowseMainPage->setFullScreenMode(checked);
    d->mViewMainPage->setFullScreenMode(checked);
    d->mSaveBar->setFullScreenMode(checked);

    toggleSideBar(d->sideBarVisibility());
    toggleStatusBar(d->statusBarVisibility());

    setUpdatesEnabled(true);
    d->autoAssignThumbnailProvider();
}

void MainWindow::leaveSpotlightMode()
{
    if (d->mSpotlightModeAction->isChecked()) {
        d->mSpotlightModeAction->trigger();
    }
}

void MainWindow::toggleSpotlightMode(bool checked)
{
    setUpdatesEnabled(false);
    if (checked) {
        leaveFullScreen();
        KConfigGroup saveConfigGroup = autoSaveConfigGroup();
        saveMainWindowSettings(saveConfigGroup);
        d->mStateBeforeFullScreen.mToolBarVisible = toolBar()->isVisible();

        setAutoSaveSettings(saveConfigGroup, false);
        resetAutoSaveSettings();

        menuBar()->hide();
        toolBar()->hide();
    } else {
        setAutoSaveSettings();

        menuBar()->setVisible(d->mShowMenuBarAction->isChecked());
        toolBar()->setVisible(d->mStateBeforeFullScreen.mToolBarVisible);
    }

    d->mViewMainPage->setSpotlightMode(checked);

    toggleSideBar(d->sideBarVisibility());
    toggleStatusBar(d->statusBarVisibility());
    setUpdatesEnabled(true);
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
    const QUrl dirUrl = d->mContextManager->currentDirUrl();

    auto dialog = new QFileDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModal(true);
    dialog->setDirectoryUrl(dirUrl);
    dialog->setWindowTitle(i18nc("@title:window", "Open Image"));
    dialog->setMimeTypeFilters(MimeTypeUtils::imageMimeTypes());
    dialog->setAcceptMode(QFileDialog::AcceptOpen);
    connect(dialog, &QDialog::accepted, this, [this, dialog]() {
        if (!dialog->selectedUrls().isEmpty()) {
            openUrl(dialog->selectedUrls().at(0));
        }
    });

    dialog->show();
}

void MainWindow::openUrl(const QUrl &url)
{
    d->setActionsDisabledOnStartMainPageEnabled(true);
    d->mContextManager->setUrlToSelect(url);
    d->mViewAction->trigger();
}

void MainWindow::showDocumentInFullScreen(const QUrl &url)
{
    d->mContextManager->setUrlToSelect(url);
    d->mViewAction->trigger();
    d->mFullScreenAction->trigger();
}

void MainWindow::toggleSlideShow()
{
    if (d->mSlideShow->isRunning()) {
        d->mSlideShow->pause();
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
        d->mToggleSlideShowAction->setText(i18n("Pause Slideshow"));
        d->mToggleSlideShowAction->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-pause")));
    } else {
        d->mToggleSlideShowAction->setText(d->mFullScreenAction->isChecked() ? i18n("Resume Slideshow") : i18n("Start Slideshow"));
        d->mToggleSlideShowAction->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
    }
}

bool MainWindow::queryClose()
{
    saveConfig();
    QList<QUrl> list = DocumentFactory::instance()->modifiedDocumentList();
    if (list.size() == 0) {
        return true;
    }

    KGuiItem yes(i18n("Save All Changes"), "document-save-all");
    KGuiItem no(i18n("Discard Changes"), "delete");
    QString msg =
        i18np("One image has been modified.", "%1 images have been modified.", list.size()) + '\n' + i18n("If you quit now, your changes will be lost.");
    int answer = KMessageBox::warningTwoActionsCancel(this, msg, QString() /* caption */, yes, no);

    switch (answer) {
    case KMessageBox::PrimaryAction:
        d->mGvCore->saveAll();
        // We need to wait a bit because the DocumentFactory is notified about
        // saved documents through a queued connection.
        qApp->processEvents();
        return DocumentFactory::instance()->modifiedDocumentList().isEmpty();

    case KMessageBox::SecondaryAction:
        return true;

    default: // cancel
        return false;
    }
}

void MainWindow::showConfigDialog(int page)
{
    // Save first so changes like thumbnail zoom level are not lost when reloading config
    saveConfig();

    auto dialog = new ConfigDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModal(true);
    connect(dialog, &KConfigDialog::settingsChanged, this, &MainWindow::loadConfig);
    dialog->setCurrentPage(page);
    dialog->show();
}

void MainWindow::configureShortcuts()
{
    guiFactory()->showConfigureShortcutsDialog();
}

void MainWindow::toggleMenuBar()
{
    if (!d->mFullScreenAction->isChecked()) {
        if (!d->mShowMenuBarAction->isChecked() && (!toolBar()->isVisible() || !toolBar()->actions().contains(d->mHamburgerMenu))) {
            const QString accel = d->mShowMenuBarAction->shortcut().toString();
            KMessageBox::information(this,
                                     i18n("This will hide the menu bar completely."
                                          " You can show it again by typing %1.",
                                          accel),
                                     i18n("Hide menu bar"),
                                     QLatin1String("HideMenuBarWarning"));
        }
        menuBar()->setVisible(d->mShowMenuBarAction->isChecked());
    }
}

void MainWindow::loadConfig()
{
    d->mDirModel->setBlackListedExtensions(GwenviewConfig::blackListedExtensions());
    d->mDirModel->adjustKindFilter(MimeTypeUtils::KIND_VIDEO, GwenviewConfig::listVideos());

    if (GwenviewConfig::historyEnabled()) {
        d->mFileOpenRecentAction->loadEntries(KConfigGroup(KSharedConfig::openConfig(), "Recent Files"));
        const auto mFileOpenRecentActionUrls = d->mFileOpenRecentAction->urls();
        for (const QUrl &url : mFileOpenRecentActionUrls) {
            d->mGvCore->addUrlToRecentFiles(url);
        }
    } else {
        d->mFileOpenRecentAction->clear();
    }
    d->mFileOpenRecentAction->setVisible(GwenviewConfig::historyEnabled());

    d->mStartMainPage->loadConfig();
    d->mViewMainPage->loadConfig();
    d->mBrowseMainPage->loadConfig();
    d->mContextManager->loadConfig();
    d->mSideBar->loadConfig();
    d->loadSplitterConfig();
    d->loadInformationSplitterConfig();
}

void MainWindow::saveConfig()
{
    d->mFileOpenRecentAction->saveEntries(KConfigGroup(KSharedConfig::openConfig(), "Recent Files"));
    d->mViewMainPage->saveConfig();
    d->mBrowseMainPage->saveConfig();
    d->mContextManager->saveConfig();
    d->saveSplitterConfig();
    d->saveInformationSplitterConfig();
    GwenviewConfig::setFullScreenModeActive(isFullScreen());
    GwenviewConfig::setSpotlightMode(d->mSpotlightModeAction->isChecked());
    // Save the last used version when Gwenview closes so we know which settings/features the user
    // is aware of which is needed for migration. The version format is: two digits each for
    // major minor bugfix version. Never decrease this number. Increase it when needed.
    GwenviewConfig::setLastUsedVersion(210800);
}

void MainWindow::print()
{
    d->print(Private::ShowPreview::No);
}

void MainWindow::printPreview()
{
    d->print(Private::ShowPreview::Yes);
}

void MainWindow::preloadNextUrl()
{
    static bool disablePreload = qgetenv("GV_MAX_UNREFERENCED_IMAGES") == "0";
    if (disablePreload) {
        qCDebug(GWENVIEW_APP_LOG) << "Preloading disabled";
        return;
    }
    QItemSelection selection = d->mContextManager->selectionModel()->selection();
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

// Set a sane initial window size
QSize MainWindow::sizeHint() const
{
    return KXmlGuiWindow::sizeHint().expandedTo(QSize(1020, 700));
}

void MainWindow::showEvent(QShowEvent *event)
{
    // We need to delay initializing the action state until the menu bar has
    // been initialized, that's why it's done only in the showEvent()
    if (GwenviewConfig::lastUsedVersion() == -1 && toolBar()->actions().contains(d->mHamburgerMenu)) {
        menuBar()->hide();
    }
    d->mShowMenuBarAction->setChecked(menuBar()->isVisible());
    KXmlGuiWindow::showEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
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

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);
    Q_UNUSED(event);
#ifdef Q_OS_OSX
    /**
     * handle Mac OS X file open events (only exist on OS X)
     */
    if (event->type() == QEvent::FileOpen) {
        QFileOpenEvent *fileOpenEvent = static_cast<QFileOpenEvent *>(event);
        openUrl(fileOpenEvent->url());
        return true;
    }
#endif
    if (obj == d->mThumbnailView->viewport()) {
        switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
            mouseButtonNavigate(static_cast<QMouseEvent *>(event));
            break;
        default:;
        }
    }
    return false;
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    mouseButtonNavigate(event);
    KXmlGuiWindow::mousePressEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    mouseButtonNavigate(event);
    KXmlGuiWindow::mouseDoubleClickEvent(event);
}

void MainWindow::mouseButtonNavigate(QMouseEvent *event)
{
    switch (event->button()) {
    case Qt::ForwardButton:
        if (d->mGoToNextAction->isEnabled()) {
            d->mGoToNextAction->trigger();
            return;
        }
        break;
    case Qt::BackButton:
        if (d->mGoToPreviousAction->isEnabled()) {
            d->mGoToPreviousAction->trigger();
            return;
        }
        break;
    default:;
    }
}

void MainWindow::setDistractionFreeMode(bool value)
{
    d->mFullScreenContent->setDistractionFreeMode(value);
}

void MainWindow::saveProperties(KConfigGroup &group)
{
    group.writeEntry(SESSION_CURRENT_PAGE_KEY, int(d->mCurrentMainPageId));
    group.writeEntry(SESSION_URL_KEY, d->mContextManager->currentUrl().toString());
}

void MainWindow::readProperties(const KConfigGroup &group)
{
    const QUrl url = group.readEntry(SESSION_URL_KEY, QUrl());
    if (url.isValid()) {
        goToUrl(url);
    }

    MainPageId pageId = MainPageId(group.readEntry(SESSION_CURRENT_PAGE_KEY, int(StartMainPageId)));
    if (pageId == StartMainPageId) {
        showStartMainPage();
    } else if (pageId == BrowseMainPageId) {
        d->mBrowseAction->trigger();
    } else {
        d->mViewAction->trigger();
    }
}

void MainWindow::showFirstDocumentReached()
{
    if (d->hudButtonBox || d->mCurrentMainPageId != ViewMainPageId) {
        return;
    }
    d->hudButtonBox = new HudButtonBox;
    d->hudButtonBox->setText(i18n("You reached the first document, what do you want to do?"));
    d->hudButtonBox->addButton(i18n("Stay There"));
    d->hudButtonBox->addAction(d->mGoToLastAction, i18n("Go to the Last Document"));
    d->hudButtonBox->addAction(d->mBrowseAction, i18n("Go Back to the Document List"));
    d->hudButtonBox->addCountDown(15000);
    d->mViewMainPage->showMessageWidget(d->hudButtonBox, Qt::AlignCenter);
}

void MainWindow::showLastDocumentReached()
{
    if (d->hudButtonBox || d->mCurrentMainPageId != ViewMainPageId) {
        return;
    }
    d->hudButtonBox = new HudButtonBox;
    d->hudButtonBox->setText(i18n("You reached the last document, what do you want to do?"));
    d->hudButtonBox->addButton(i18n("Stay There"));
    d->hudButtonBox->addAction(d->mGoToFirstAction, i18n("Go to the First Document"));
    d->hudButtonBox->addAction(d->mBrowseAction, i18n("Go Back to the Document List"));
    d->hudButtonBox->addCountDown(15000);
    d->mViewMainPage->showMessageWidget(d->hudButtonBox, Qt::AlignCenter);
}

void MainWindow::replaceLocation()
{
    QLineEdit *lineEdit = d->mUrlNavigator->editor()->lineEdit();

    // If the text field currently has focus and everything is selected,
    // pressing the keyboard shortcut returns the whole thing to breadcrumb mode
    if (d->mUrlNavigator->isUrlEditable() && lineEdit->hasFocus() && lineEdit->selectedText() == lineEdit->text()) {
        d->mUrlNavigator->setUrlEditable(false);
    } else {
        d->mUrlNavigator->setUrlEditable(true);
        d->mUrlNavigator->setFocus();
        lineEdit->selectAll();
    }
}

void MainWindow::onFocusChanged(QWidget *old, QWidget *now)
{
    if (old == nullptr) {
        if (now != nullptr && isFullScreen()) {
            d->setScreenSaverEnabled(false);
        }
    } else {
        if (now == nullptr) {
            d->setScreenSaverEnabled(true);
        }
    }
}

} // namespace

#include "moc_mainwindow.cpp"
