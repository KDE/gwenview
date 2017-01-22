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
#include "viewmainpage.h"

// Qt
#include <QCheckBox>
#include <QItemSelectionModel>
#include <QShortcut>
#include <QToolButton>
#include <QVBoxLayout>
#include <QDebug>
#include <QMenu>

// KDE
#include <KActionCollection>
#include <KActionCategory>
#include <KLocalizedString>
#include <KMessageBox>
#include <KModelIndexProxyMapper>
#include <KToggleAction>
#include <KActivities/ResourceInstance>

// Local
#include "fileoperations.h"
#include <gvcore.h>
#include "splitter.h"
#include <lib/document/document.h>
#include <lib/documentview/abstractdocumentviewadapter.h>
#include <lib/documentview/documentview.h>
#include <lib/documentview/documentviewcontainer.h>
#include <lib/documentview/documentviewcontroller.h>
#include <lib/documentview/documentviewsynchronizer.h>
#include <lib/gvdebug.h>
#include <lib/gwenviewconfig.h>
#include <lib/paintutils.h>
#include <lib/semanticinfo/sorteddirmodel.h>
#include <lib/slidecontainer.h>
#include <lib/slideshow.h>
#include <lib/statusbartoolbutton.h>
#include <lib/thumbnailview/thumbnailbarview.h>
#include <lib/zoomwidget.h>
#include <lib/zoommode.h>

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

const int ViewMainPage::MaxViewCount = 6;

static QString rgba(const QColor &color)
{
    return QString::fromLocal8Bit("rgba(%1, %2, %3, %4)")
           .arg(color.red())
           .arg(color.green())
           .arg(color.blue())
           .arg(color.alpha());
}

static QString gradient(Qt::Orientation orientation, const QColor &color, int value)
{
    int x2, y2;
    if (orientation == Qt::Horizontal) {
        x2 = 0;
        y2 = 1;
    } else {
        x2 = 1;
        y2 = 0;
    }
    QString grad =
        "qlineargradient(x1:0, y1:0, x2:%1, y2:%2,"
        "stop:0 %3, stop: 1 %4)";
    return grad
           .arg(x2)
           .arg(y2)
           .arg(rgba(PaintUtils::adjustedHsv(color, 0, 0, qMin(255 - color.value(), value / 2))))
           .arg(rgba(PaintUtils::adjustedHsv(color, 0, 0, -qMin(color.value(), value / 2))))
           ;
}

/*
 * Layout of mThumbnailSplitter is:
 *
 * +-mThumbnailSplitter------------------------------------------------+
 * |+-mAdapterContainer-----------------------------------------------+|
 * ||+-mDocumentViewContainer----------------------------------------+||
 * |||+-DocumentView----------------++-DocumentView-----------------+|||
 * ||||                             ||                              ||||
 * ||||                             ||                              ||||
 * ||||                             ||                              ||||
 * ||||                             ||                              ||||
 * ||||                             ||                              ||||
 * ||||                             ||                              ||||
 * |||+-----------------------------++------------------------------+|||
 * ||+---------------------------------------------------------------+||
 * ||+-mToolContainer------------------------------------------------+||
 * |||                                                               |||
 * ||+---------------------------------------------------------------+||
 * ||+-mStatusBarContainer-------------------------------------------+||
 * |||[mToggleSideBarButton][mToggleThumbnailBarButton] [mZoomWidget]|||
 * ||+---------------------------------------------------------------+||
 * |+-----------------------------------------------------------------+|
 * |===================================================================|
 * |+-mThumbnailBar---------------------------------------------------+|
 * ||                                                                 ||
 * ||                                                                 ||
 * |+-----------------------------------------------------------------+|
 * +-------------------------------------------------------------------+
 */
struct ViewMainPagePrivate
{
    ViewMainPage* q;
    SlideShow* mSlideShow;
    KActionCollection* mActionCollection;
    GvCore* mGvCore;
    KModelIndexProxyMapper* mDirModelToBarModelProxyMapper;
    QSplitter *mThumbnailSplitter;
    QWidget* mAdapterContainer;
    DocumentViewController* mDocumentViewController;
    QList<DocumentView*> mDocumentViews;
    DocumentViewSynchronizer* mSynchronizer;
    QToolButton* mToggleSideBarButton;
    QToolButton* mToggleThumbnailBarButton;
    ZoomWidget* mZoomWidget;
    DocumentViewContainer* mDocumentViewContainer;
    SlideContainer* mToolContainer;
    QWidget* mStatusBarContainer;
    ThumbnailBarView* mThumbnailBar;
    KToggleAction* mToggleThumbnailBarAction;
    KToggleAction* mSynchronizeAction;
    QCheckBox* mSynchronizeCheckBox;

    // Activity Resource events reporting needs to be above KPart,
    // in the shell itself, to avoid problems with other MDI applications
    // that use this KPart
    QHash<DocumentView*, KActivities::ResourceInstance*> mActivityResources;

    bool mFullScreenMode;
    bool mCompareMode;
    bool mThumbnailBarVisibleBeforeFullScreen;
    ZoomMode::Enum mZoomMode;

    void setupThumbnailBar()
    {
        mThumbnailBar = new ThumbnailBarView;
        ThumbnailBarItemDelegate* delegate = new ThumbnailBarItemDelegate(mThumbnailBar);
        mThumbnailBar->setItemDelegate(delegate);
        mThumbnailBar->setVisible(GwenviewConfig::thumbnailBarIsVisible());
        mThumbnailBar->setSelectionMode(QAbstractItemView::ExtendedSelection);
    }

    void setupThumbnailBarStyleSheet()
    {
        QPalette pal = mGvCore->palette(GvCore::NormalViewPalette);
        mThumbnailBar->setPalette(pal);
        Qt::Orientation orientation = mThumbnailBar->orientation();
        QColor bgColor = pal.color(QPalette::Normal, QPalette::Base);
        QColor bgSelColor = pal.color(QPalette::Normal, QPalette::Highlight);

        // Avoid dark and bright colors
        bgColor.setHsv(bgColor.hue(), bgColor.saturation(), (127 + 3 * bgColor.value()) / 4);

        QColor leftBorderColor = PaintUtils::adjustedHsv(bgColor, 0, 0, qMin(20, 255 - bgColor.value()));
        QColor rightBorderColor = PaintUtils::adjustedHsv(bgColor, 0, 0, -qMin(40, bgColor.value()));
        QColor borderSelColor = PaintUtils::adjustedHsv(bgSelColor, 0, 0, -qMin(60, bgSelColor.value()));

        QString itemCss =
            "QListView::item {"
            "	background-color: %1;"
            "	border-left: 1px solid %2;"
            "	border-right: 1px solid %3;"
            "}";
        itemCss = itemCss.arg(
                      gradient(orientation, bgColor, 46),
                      gradient(orientation, leftBorderColor, 36),
                      gradient(orientation, rightBorderColor, 26));

        QString itemSelCss =
            "QListView::item:selected {"
            "	background-color: %1;"
            "	border-left: 1px solid %2;"
            "	border-right: 1px solid %2;"
            "}";
        itemSelCss = itemSelCss.arg(
                         gradient(orientation, bgSelColor, 56),
                         rgba(borderSelColor));

        QString css = itemCss + itemSelCss;
        if (orientation == Qt::Vertical) {
            css.replace("left", "top").replace("right", "bottom");
        }

        mThumbnailBar->setStyleSheet(css);
    }

    void setupAdapterContainer()
    {
        mAdapterContainer = new QWidget;

        QVBoxLayout* layout = new QVBoxLayout(mAdapterContainer);
        layout->setMargin(0);
        layout->setSpacing(0);
        mDocumentViewContainer = new DocumentViewContainer;
        mDocumentViewContainer->setAutoFillBackground(true);
        mDocumentViewContainer->setBackgroundRole(QPalette::Base);
        layout->addWidget(mDocumentViewContainer);
        layout->addWidget(mToolContainer);
        layout->addWidget(mStatusBarContainer);
    }

    void setupDocumentViewController()
    {
        mDocumentViewController = new DocumentViewController(mActionCollection, q);
        mDocumentViewController->setZoomWidget(mZoomWidget);
        mDocumentViewController->setToolContainer(mToolContainer);
        mSynchronizer = new DocumentViewSynchronizer(&mDocumentViews, q);
    }

    DocumentView* createDocumentView()
    {
        DocumentView* view = mDocumentViewContainer->createView();

        // Connect context menu
        // If you need to connect another view signal, make sure it is disconnected in deleteDocumentView
        QObject::connect(view, &DocumentView::contextMenuRequested, q, &ViewMainPage::showContextMenu);

        QObject::connect(view, &DocumentView::completed, q, &ViewMainPage::completed);
        QObject::connect(view, &DocumentView::previousImageRequested, q, &ViewMainPage::previousImageRequested);
        QObject::connect(view, &DocumentView::nextImageRequested, q, &ViewMainPage::nextImageRequested);
        QObject::connect(view, &DocumentView::captionUpdateRequested, q, &ViewMainPage::captionUpdateRequested);
        QObject::connect(view, &DocumentView::toggleFullScreenRequested, q, &ViewMainPage::toggleFullScreenRequested);
        QObject::connect(view, &DocumentView::focused, q, &ViewMainPage::slotViewFocused);
        QObject::connect(view, &DocumentView::hudTrashClicked, q, &ViewMainPage::trashView);
        QObject::connect(view, &DocumentView::hudDeselectClicked, q, &ViewMainPage::deselectView);

        QObject::connect(view, &DocumentView::videoFinished, mSlideShow, &SlideShow::resumeAndGoToNextUrl);

        mDocumentViews << view;
        mActivityResources.insert(view, new KActivities::ResourceInstance(q->window()->winId(), view));

        return view;
    }

    void deleteDocumentView(DocumentView* view)
    {
        if (mDocumentViewController->view() == view) {
            mDocumentViewController->setView(0);
        }

        // Make sure we do not get notified about this view while it is going away.
        // mDocumentViewController->deleteView() animates the view deletion so
        // the view still exists for a short while when we come back to the
        // event loop)
        QObject::disconnect(view, 0, q, 0);
        QObject::disconnect(view, 0, mSlideShow, 0);

        mDocumentViews.removeOne(view);
        mActivityResources.remove(view);
        mDocumentViewContainer->deleteView(view);
    }

    void setupToolContainer()
    {
        mToolContainer = new SlideContainer;
    }

    void setupStatusBar()
    {
        mStatusBarContainer = new QWidget;
        mToggleSideBarButton = new StatusBarToolButton;
        mToggleThumbnailBarButton = new StatusBarToolButton;
        mZoomWidget = new ZoomWidget;
        mSynchronizeCheckBox = new QCheckBox(i18n("Synchronize"));
        mSynchronizeCheckBox->hide();

        QHBoxLayout* layout = new QHBoxLayout(mStatusBarContainer);
        layout->setMargin(0);
        layout->setSpacing(0);
        layout->addWidget(mToggleSideBarButton);
        layout->addWidget(mToggleThumbnailBarButton);
        layout->addStretch();
        layout->addWidget(mSynchronizeCheckBox);
        layout->addStretch();
        layout->addWidget(mZoomWidget);
    }

    void setupSplitter()
    {
        Qt::Orientation orientation = GwenviewConfig::thumbnailBarOrientation();
        mThumbnailSplitter = new Splitter(orientation == Qt::Horizontal ? Qt::Vertical : Qt::Horizontal, q);
        mThumbnailBar->setOrientation(orientation);
        mThumbnailBar->setThumbnailAspectRatio(GwenviewConfig::thumbnailAspectRatio());
        mThumbnailBar->setRowCount(GwenviewConfig::thumbnailBarRowCount());
        mThumbnailSplitter->addWidget(mAdapterContainer);
        mThumbnailSplitter->addWidget(mThumbnailBar);
        mThumbnailSplitter->setSizes(GwenviewConfig::thumbnailSplitterSizes());

        QVBoxLayout* layout = new QVBoxLayout(q);
        layout->setMargin(0);
        layout->addWidget(mThumbnailSplitter);
    }

    void saveSplitterConfig()
    {
        if (mThumbnailBar->isVisible()) {
            GwenviewConfig::setThumbnailSplitterSizes(mThumbnailSplitter->sizes());
        }
    }

    DocumentView* currentView() const
    {
        return mDocumentViewController->view();
    }

    void setCurrentView(DocumentView* view)
    {
        DocumentView* oldView = currentView();
        if (view == oldView) {
            return;
        }
        if (oldView) {
            oldView->setCurrent(false);
            Q_ASSERT(mActivityResources.contains(oldView));
            mActivityResources.value(oldView)->notifyFocusedOut();
        }
        view->setCurrent(true);
        mDocumentViewController->setView(view);
        mSynchronizer->setCurrentView(view);

        QModelIndex index = indexForView(view);
        if (index.isValid()) {
            // Index may be invalid when Gwenview is started as
            // `gwenview /foo/image.png` because in this situation it loads image.png
            // *before* listing /foo (because it matters less to the user)
            mThumbnailBar->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Current);
        }

        Q_ASSERT(mActivityResources.contains(view));
        mActivityResources.value(view)->notifyFocusedIn();
    }

    QModelIndex indexForView(DocumentView* view) const
    {
        QUrl url = view->url();
        if (!url.isValid()) {
            qWarning() << "View does not display any document!";
            return QModelIndex();
        }

        SortedDirModel* dirModel = mGvCore->sortedDirModel();
        QModelIndex srcIndex = dirModel->indexForUrl(url);
        if (!mDirModelToBarModelProxyMapper) {
            // Delay the initialization of the mapper to its first use because
            // mThumbnailBar->model() is not set after ViewMainPage ctor is
            // done.
            const_cast<ViewMainPagePrivate*>(this)->mDirModelToBarModelProxyMapper = new KModelIndexProxyMapper(dirModel, mThumbnailBar->model(), q);
        }
        QModelIndex index = mDirModelToBarModelProxyMapper->mapLeftToRight(srcIndex);
        return index;
    }

    void applyPalette(bool fullScreenMode)
    {
        mDocumentViewContainer->setPalette(mGvCore->palette(fullScreenMode ? GvCore::FullScreenViewPalette : GvCore::NormalViewPalette));
        setupThumbnailBarStyleSheet();
    }
};

ViewMainPage::ViewMainPage(QWidget* parent, SlideShow* slideShow, KActionCollection* actionCollection, GvCore* gvCore)
: QWidget(parent)
, d(new ViewMainPagePrivate)
{
    d->q = this;
    d->mDirModelToBarModelProxyMapper = 0; // Initialized later
    d->mSlideShow = slideShow;
    d->mActionCollection = actionCollection;
    d->mGvCore = gvCore;
    d->mFullScreenMode = false;
    d->mCompareMode = false;
    d->mThumbnailBarVisibleBeforeFullScreen = false;

    QShortcut* goToBrowseModeShortcut = new QShortcut(this);
    goToBrowseModeShortcut->setKey(Qt::Key_Return);
    connect(goToBrowseModeShortcut, &QShortcut::activated, this, &ViewMainPage::goToBrowseModeRequested);

    d->setupToolContainer();
    d->setupStatusBar();

    d->setupAdapterContainer();

    d->setupThumbnailBar();

    d->setupSplitter();

    d->setupDocumentViewController();

    KActionCategory* view = new KActionCategory(i18nc("@title actions category - means actions changing smth in interface", "View"), actionCollection);

    d->mToggleThumbnailBarAction = view->add<KToggleAction>(QString("toggle_thumbnailbar"));
    d->mToggleThumbnailBarAction->setText(i18n("Thumbnail Bar"));
    d->mToggleThumbnailBarAction->setIcon(QIcon::fromTheme("folder-image"));
    actionCollection->setDefaultShortcut(d->mToggleThumbnailBarAction, Qt::CTRL + Qt::Key_B);
    d->mToggleThumbnailBarAction->setChecked(GwenviewConfig::thumbnailBarIsVisible());
    connect(d->mToggleThumbnailBarAction, &KToggleAction::triggered, this, &ViewMainPage::setThumbnailBarVisibility);
    d->mToggleThumbnailBarButton->setDefaultAction(d->mToggleThumbnailBarAction);

    d->mSynchronizeAction = view->add<KToggleAction>("synchronize_views");
    d->mSynchronizeAction->setText(i18n("Synchronize"));
    actionCollection->setDefaultShortcut(d->mSynchronizeAction, Qt::CTRL + Qt::Key_Y);
    connect(d->mSynchronizeAction, SIGNAL(toggled(bool)),
            d->mSynchronizer, SLOT(setActive(bool)));
    // Ensure mSynchronizeAction and mSynchronizeCheckBox are in sync
    connect(d->mSynchronizeAction, SIGNAL(toggled(bool)),
            d->mSynchronizeCheckBox, SLOT(setChecked(bool)));
    connect(d->mSynchronizeCheckBox, SIGNAL(toggled(bool)),
            d->mSynchronizeAction, SLOT(setChecked(bool)));
}

ViewMainPage::~ViewMainPage()
{
    delete d;
}

void ViewMainPage::loadConfig()
{
    d->applyPalette(false /* fullScreenMode */);

    // FIXME: Not symetric with saveConfig(). Check if it matters.
    Q_FOREACH(DocumentView * view, d->mDocumentViews) {
        view->loadAdapterConfig();
    }

    Qt::Orientation orientation = GwenviewConfig::thumbnailBarOrientation();
    d->mThumbnailSplitter->setOrientation(orientation == Qt::Horizontal ? Qt::Vertical : Qt::Horizontal);
    d->mThumbnailBar->setOrientation(orientation);
    d->setupThumbnailBarStyleSheet();

    int oldRowCount = d->mThumbnailBar->rowCount();
    int newRowCount = GwenviewConfig::thumbnailBarRowCount();
    if (oldRowCount != newRowCount) {
        d->mThumbnailBar->setUpdatesEnabled(false);
        int gridSize = d->mThumbnailBar->gridSize().width();

        d->mThumbnailBar->setRowCount(newRowCount);

        // Adjust splitter to ensure thumbnail size remains the same
        int delta = (newRowCount - oldRowCount) * gridSize;
        QList<int> sizes = d->mThumbnailSplitter->sizes();
        Q_ASSERT(sizes.count() == 2);
        sizes[0] -= delta;
        sizes[1] += delta;
        d->mThumbnailSplitter->setSizes(sizes);

        d->mThumbnailBar->setUpdatesEnabled(true);
    }

    d->mZoomMode = GwenviewConfig::zoomMode();
}

void ViewMainPage::saveConfig()
{
    d->saveSplitterConfig();
    GwenviewConfig::setThumbnailBarIsVisible(d->mToggleThumbnailBarAction->isChecked());
}

void ViewMainPage::setThumbnailBarVisibility(bool visible)
{
    d->saveSplitterConfig();
    d->mThumbnailBar->setVisible(visible);
}

int ViewMainPage::statusBarHeight() const
{
    return d->mStatusBarContainer->height();
}

void ViewMainPage::setFullScreenMode(bool fullScreenMode)
{
    d->mFullScreenMode = fullScreenMode;
    d->mStatusBarContainer->setVisible(!fullScreenMode);

    if (fullScreenMode) {
        d->mThumbnailBarVisibleBeforeFullScreen = d->mToggleThumbnailBarAction->isChecked();
        if (d->mThumbnailBarVisibleBeforeFullScreen) {
            d->mToggleThumbnailBarAction->trigger();
        }
    } else {
        if (d->mThumbnailBarVisibleBeforeFullScreen) {
            d->mToggleThumbnailBarAction->trigger();
        }
    }
    d->applyPalette(fullScreenMode);
    d->mToggleThumbnailBarAction->setEnabled(!fullScreenMode);
}

bool ViewMainPage::isFullScreenMode() const
{
    return d->mFullScreenMode;
}

ThumbnailBarView* ViewMainPage::thumbnailBar() const
{
    return d->mThumbnailBar;
}

inline void addActionToMenu(QMenu* menu, KActionCollection* actionCollection, const char* name)
{
    QAction* action = actionCollection->action(name);
    if (action) {
        menu->addAction(action);
    }
}

void ViewMainPage::showContextMenu()
{
    QMenu menu(this);
    addActionToMenu(&menu, d->mActionCollection, "fullscreen");
    menu.addSeparator();
    addActionToMenu(&menu, d->mActionCollection, "go_previous");
    addActionToMenu(&menu, d->mActionCollection, "go_next");
    if (d->currentView()->canZoom()) {
        menu.addSeparator();
        addActionToMenu(&menu, d->mActionCollection, "view_actual_size");
        addActionToMenu(&menu, d->mActionCollection, "view_zoom_to_fit");
        addActionToMenu(&menu, d->mActionCollection, "view_zoom_in");
        addActionToMenu(&menu, d->mActionCollection, "view_zoom_out");
    }
    if (d->mCompareMode) {
        menu.addSeparator();
        addActionToMenu(&menu, d->mActionCollection, "synchronize_views");
    }

    menu.addSeparator();
    addActionToMenu(&menu, d->mActionCollection, "file_copy_to");
    addActionToMenu(&menu, d->mActionCollection, "file_move_to");
    addActionToMenu(&menu, d->mActionCollection, "file_link_to");
    menu.addSeparator();
    addActionToMenu(&menu, d->mActionCollection, "file_open_with");
    menu.exec(QCursor::pos());
}

QSize ViewMainPage::sizeHint() const
{
    return QSize(400, 300);
}

QUrl ViewMainPage::url() const
{
    GV_RETURN_VALUE_IF_FAIL(d->currentView(), QUrl());
    return d->currentView()->url();
}

Document::Ptr ViewMainPage::currentDocument() const
{
    if (!d->currentView()) {
        LOG("!d->documentView()");
        return Document::Ptr();
    }

    return d->currentView()->document();
}

bool ViewMainPage::isEmpty() const
{
    return !currentDocument();
}

RasterImageView* ViewMainPage::imageView() const
{
    if (!d->currentView()) {
        return 0;
    }
    return d->currentView()->imageView();
}

DocumentView* ViewMainPage::documentView() const
{
    return d->currentView();
}

void ViewMainPage::openUrl(const QUrl &url)
{
    openUrls(QList<QUrl>() << url, url);
}

void ViewMainPage::openUrls(const QList<QUrl>& allUrls, const QUrl &currentUrl)
{
    DocumentView::Setup setup;

    QSet<QUrl> urls = allUrls.toSet();
    d->mCompareMode = urls.count() > 1;

    typedef QMap<QUrl, DocumentView*> ViewForUrlMap;
    ViewForUrlMap viewForUrlMap;

    if (!d->mDocumentViews.isEmpty()) {
        d->mDocumentViewContainer->updateSetup(d->mDocumentViews.last());
    }
    if (d->mDocumentViews.isEmpty() || d->mZoomMode == ZoomMode::Autofit) {
        setup.valid = true;
        setup.zoomToFit = true;
    } else {
        setup = d->mDocumentViews.last()->setup();
    }
    // Destroy views which show urls we don't care about, remove from "urls" the
    // urls which already have a view.
    Q_FOREACH(DocumentView * view, d->mDocumentViews) {
        QUrl url = view->url();
        if (urls.contains(url)) {
            // view displays an url we must display, keep it
            urls.remove(url);
            viewForUrlMap.insert(url, view);
        } else {
            // view url is not interesting, drop it
            d->deleteDocumentView(view);
        }
    }

    // Create view for remaining urls
    Q_FOREACH(const QUrl &url, urls) {
        if (d->mDocumentViews.count() >= MaxViewCount) {
            qWarning() << "Too many documents to show";
            break;
        }
        DocumentView* view = d->createDocumentView();
        viewForUrlMap.insert(url, view);
    }

    // Set sortKey to match url order
    int sortKey = 0;
    Q_FOREACH(const QUrl &url, allUrls) {
        viewForUrlMap[url]->setSortKey(sortKey);
        ++sortKey;
    }

    d->mDocumentViewContainer->updateLayout();

    // Load urls for new views. Do it only now because the view must have the
    // correct size before it starts loading its url. Do not do it later because
    // view->url() needs to be set for the next loop.
    ViewForUrlMap::ConstIterator
        it = viewForUrlMap.constBegin(),
        end = viewForUrlMap.constEnd();
    for (; it != end; ++it) {
        QUrl url = it.key();
        DocumentView* view = it.value();
        DocumentView::Setup savedSetup = d->mDocumentViewContainer->savedSetup(url);
        view->openUrl(url, d->mZoomMode == ZoomMode::Individual && savedSetup.valid ? savedSetup : setup);
        d->mActivityResources.value(view)->setUri(url);
    }

    // Init views
    Q_FOREACH(DocumentView * view, d->mDocumentViews) {
        view->setCompareMode(d->mCompareMode);
        if (view->url() == currentUrl) {
            d->setCurrentView(view);
        } else {
            view->setCurrent(false);
        }
    }

    d->mSynchronizeCheckBox->setVisible(d->mCompareMode);
    if (d->mCompareMode) {
        d->mSynchronizer->setActive(d->mSynchronizeCheckBox->isChecked());
    } else {
        d->mSynchronizer->setActive(false);
    }
}

void ViewMainPage::reload()
{
    DocumentView *view = d->currentView();
    if (!view) {
        return;
    }
    Document::Ptr doc = view->document();
    if (!doc) {
        qWarning() << "!doc";
        return;
    }
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
    // Call openUrl again because DocumentView may need to switch to a new
    // adapter (for example because document was broken and it is not anymore)
    d->currentView()->openUrl(doc->url(), d->currentView()->setup());
}

void ViewMainPage::reset()
{
    d->mDocumentViewController->setView(0);
    d->mDocumentViewContainer->reset();
    d->mDocumentViews.clear();
}

void ViewMainPage::slotViewFocused(DocumentView* view)
{
    d->setCurrentView(view);
}

void ViewMainPage::trashView(DocumentView* view)
{
    QUrl url = view->url();
    deselectView(view);
    FileOperations::trash(QList<QUrl>() << url, this);
}

void ViewMainPage::deselectView(DocumentView* view)
{
    DocumentView* newCurrentView = 0;
    if (view == d->currentView()) {
        // We need to find a new view to set as current
        int idx = d->mDocumentViews.indexOf(view);
        if (idx + 1 < d->mDocumentViews.count()) {
            newCurrentView = d->mDocumentViews.at(idx + 1);
        } else if (idx > 0) {
            newCurrentView = d->mDocumentViews.at(idx - 1);
        } else {
            GV_WARN_AND_RETURN("No view found to set as current");
        }
    }

    QModelIndex index = d->indexForView(view);
    QItemSelectionModel* selectionModel = d->mThumbnailBar->selectionModel();
    selectionModel->select(index, QItemSelectionModel::Deselect);

    if (newCurrentView) {
        d->setCurrentView(newCurrentView);
    }
}

QToolButton* ViewMainPage::toggleSideBarButton() const
{
    return d->mToggleSideBarButton;
}

void ViewMainPage::showMessageWidget(QGraphicsWidget* widget, Qt::Alignment align)
{
    d->mDocumentViewContainer->showMessageWidget(widget, align);
}

} // namespace
