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
#include "viewmainpage.moc"

// Qt
#include <QApplication>
#include <QCheckBox>
#include <QItemSelectionModel>
#include <QLabel>
#include <QShortcut>
#include <QToolButton>
#include <QVBoxLayout>

// KDE
#include <KActionCollection>
#include <KActionCategory>
#include <KDebug>
#include <KLocale>
#include <KMenu>
#include <KMessageBox>
#include <KToggleAction>

// Local
#include "fileoperations.h"
#include "splitter.h"
#include <lib/document/document.h>
#include <lib/documentview/abstractdocumentviewadapter.h>
#include <lib/documentview/documentview.h>
#include <lib/documentview/documentviewcontainer.h>
#include <lib/documentview/documentviewcontroller.h>
#include <lib/documentview/documentviewsynchronizer.h>
#include <lib/gwenviewconfig.h>
#include <lib/paintutils.h>
#include <lib/semanticinfo/sorteddirmodel.h>
#include <lib/slidecontainer.h>
#include <lib/slideshow.h>
#include <lib/statusbartoolbutton.h>
#include <lib/thumbnailview/thumbnailbarview.h>
#include <lib/zoomwidget.h>

namespace Gwenview
{

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << x
#else
#define LOG(x) ;
#endif

const int ViewMainPage::MaxViewCount = 6;

static QString rgba(const QColor &color)
{
    return QString::fromAscii("rgba(%1, %2, %3, %4)")
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

    bool mFullScreenMode;
    QPalette mNormalPalette;
    bool mCompareMode;
    bool mThumbnailBarVisibleBeforeFullScreen;

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
        Qt::Orientation orientation = mThumbnailBar->orientation();
        QColor bgColor = mNormalPalette.color(QPalette::Normal, QPalette::Window);
        QColor bgSelColor = mNormalPalette.color(QPalette::Normal, QPalette::Highlight);

        // Avoid dark and bright colors
        bgColor.setHsv(bgColor.hue(), bgColor.saturation(), (127 + 3 * bgColor.value()) / 4);

        QColor leftBorderColor = PaintUtils::adjustedHsv(bgColor, 0, 0, qMin(20, 255 - bgColor.value()));
        QColor rightBorderColor = PaintUtils::adjustedHsv(bgColor, 0, 0, -qMin(40, bgColor.value()));
        QColor borderSelColor = PaintUtils::adjustedHsv(bgSelColor, 0, 0, -qMin(60, bgSelColor.value()));

        QString viewCss =
            "#thumbnailBarView {"
            "	background-color: rgba(0, 0, 0, 10%);"
            "}";

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

        QString css = viewCss + itemCss + itemSelCss;
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
        QObject::connect(view, SIGNAL(contextMenuRequested()),
                         q, SLOT(showContextMenu()));

        QObject::connect(view, SIGNAL(completed()),
                         q, SIGNAL(completed()));
        QObject::connect(view, SIGNAL(previousImageRequested()),
                         q, SIGNAL(previousImageRequested()));
        QObject::connect(view, SIGNAL(nextImageRequested()),
                         q, SIGNAL(nextImageRequested()));
        QObject::connect(view, SIGNAL(captionUpdateRequested(QString)),
                         q, SIGNAL(captionUpdateRequested(QString)));
        QObject::connect(view, SIGNAL(toggleFullScreenRequested()),
                         q, SIGNAL(toggleFullScreenRequested()));
        QObject::connect(view, SIGNAL(focused(DocumentView*)),
                         q, SLOT(slotViewFocused(DocumentView*)));
        QObject::connect(view, SIGNAL(hudTrashClicked(DocumentView*)),
                         q, SLOT(trashView(DocumentView*)));
        QObject::connect(view, SIGNAL(hudDeselectClicked(DocumentView*)),
                         q, SLOT(deselectView(DocumentView*)));

        QObject::connect(view, SIGNAL(videoFinished()),
                         mSlideShow, SLOT(resumeAndGoToNextUrl()));

        mDocumentViews << view;

        return view;
    }

    void deleteDocumentView(DocumentView* view)
    {
        mDocumentViewContainer->deleteView(view);
        mDocumentViews.removeOne(view);
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
        }
        view->setCurrent(true);
        mDocumentViewController->setView(view);
        mSynchronizer->setCurrentView(view);

        QModelIndex index = indexForView(view);
        if (!index.isValid()) {
            kWarning() << "No index found for current view";
            return;
        }
        mThumbnailBar->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Current);
    }

    QModelIndex indexForView(DocumentView* view) const
    {
        KUrl url = view->url();
        if (!url.isValid()) {
            kWarning() << "View does not display any document!";
            return QModelIndex();
        }

        // FIXME: Ugly coupling!
        SortedDirModel* model = static_cast<SortedDirModel*>(mThumbnailBar->model());
        return model->indexForUrl(url);
    }
};

ViewMainPage::ViewMainPage(QWidget* parent, SlideShow* slideShow, KActionCollection* actionCollection)
: QWidget(parent)
, d(new ViewMainPagePrivate)
{
    d->q = this;
    d->mSlideShow = slideShow;
    d->mActionCollection = actionCollection;
    d->mFullScreenMode = false;
    d->mCompareMode = false;
    d->mThumbnailBarVisibleBeforeFullScreen = false;

    QShortcut* goToBrowseModeShortcut = new QShortcut(this);
    goToBrowseModeShortcut->setKey(Qt::Key_Return);
    connect(goToBrowseModeShortcut, SIGNAL(activated()), SIGNAL(goToBrowseModeRequested()));

    d->setupToolContainer();
    d->setupStatusBar();

    d->setupAdapterContainer();

    d->setupThumbnailBar();

    d->setupSplitter();

    d->setupDocumentViewController();

    KActionCategory* view = new KActionCategory(i18nc("@title actions category - means actions changing smth in interface", "View"), actionCollection);

    d->mToggleThumbnailBarAction = view->add<KToggleAction>(QString("toggle_thumbnailbar"));
    d->mToggleThumbnailBarAction->setText(i18n("Thumbnail Bar"));
    d->mToggleThumbnailBarAction->setIcon(KIcon("folder-image"));
    d->mToggleThumbnailBarAction->setShortcut(Qt::CTRL | Qt::Key_B);
    d->mToggleThumbnailBarAction->setChecked(GwenviewConfig::thumbnailBarIsVisible());
    connect(d->mToggleThumbnailBarAction, SIGNAL(triggered(bool)),
            this, SLOT(setThumbnailBarVisibility(bool)));
    d->mToggleThumbnailBarButton->setDefaultAction(d->mToggleThumbnailBarAction);

    d->mSynchronizeAction = view->add<KToggleAction>("synchronize_views");
    d->mSynchronizeAction->setText(i18n("Synchronize"));
    d->mSynchronizeAction->setShortcut(Qt::CTRL | Qt::Key_Y);
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
        QPalette pal = QApplication::palette();
        pal.setColor(QPalette::Base, Qt::black);
        d->mDocumentViewContainer->setPalette(pal);
    } else {
        if (d->mThumbnailBarVisibleBeforeFullScreen) {
            d->mToggleThumbnailBarAction->trigger();
        }
        d->mDocumentViewContainer->setPalette(d->mNormalPalette);
    }
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

inline void addActionToMenu(KMenu* menu, KActionCollection* actionCollection, const char* name)
{
    QAction* action = actionCollection->action(name);
    if (action) {
        menu->addAction(action);
    }
}

void ViewMainPage::showContextMenu()
{
    KMenu menu(this);
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

KUrl ViewMainPage::url() const
{
    if (!d->currentView()) {
        LOG("!d->documentView()");
        return KUrl();
    }

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

void ViewMainPage::setNormalPalette(const QPalette& palette)
{
    d->mNormalPalette = palette;
    d->mDocumentViewContainer->setPalette(palette);
    d->setupThumbnailBarStyleSheet();
}

void ViewMainPage::openUrl(const KUrl& url)
{
    openUrls(KUrl::List() << url, url);
}

void ViewMainPage::openUrls(const KUrl::List& allUrls, const KUrl& currentUrl)
{
    bool zoomToFit;
    qreal zoom;
    QPoint position;

    QSet<KUrl> urls = allUrls.toSet();
    d->mCompareMode = urls.count() > 1;

    typedef QMap<KUrl, DocumentView*> ViewForUrlMap;
    ViewForUrlMap viewForUrlMap;

    // Destroy views which show urls we don't care about, remove from "urls" the
    // urls which already have a view.
    Q_FOREACH(DocumentView * view, d->mDocumentViews) {
        KUrl url = view->url();
        if (urls.contains(url)) {
            // view displays an url we must display, keep it
            urls.remove(url);
            viewForUrlMap.insert(url, view);
        } else {
            // Should we remember current zoom settings?
            zoomToFit = view->zoomToFit();
            if (!zoomToFit) {
                zoom = view->zoom();
                position = view->position();
            }

            // view url is not interesting, drop it
            d->deleteDocumentView(view);
        }
    }

    // Create view for remaining urls
    Q_FOREACH(const KUrl & url, urls) {
        if (d->mDocumentViews.count() >= MaxViewCount) {
            kWarning() << "Too many documents to show";
            break;
        }
        DocumentView* view = d->createDocumentView();
        viewForUrlMap.insert(url, view);
    }

    // Set sortKey to match url order
    int sortKey = 0;
    Q_FOREACH(const KUrl& url, allUrls) {
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
        it.value()->openUrl(it.key());
    }

    // Init views
    Q_FOREACH(DocumentView * view, d->mDocumentViews) {
        // Preserve zoom settings from one image to the next
        view->setZoomToFit(zoomToFit);
        if (!zoomToFit) {
            view->setZoom(zoom);
            view->setPosition(position);
        }

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
    Document::Ptr doc = d->currentView()->document();
    if (!doc) {
        kWarning() << "!doc";
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
    d->currentView()->openUrl(doc->url());
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
    KUrl url = view->url();
    deselectView(view);
    FileOperations::trash(KUrl::List() << url, this);
}

void ViewMainPage::deselectView(DocumentView* view)
{
    DocumentView* newCurrentView = 0;
    if (view == d->currentView()) {
        // We need to find a new view to set as current
        int idx = d->mDocumentViews.indexOf(view);
        // Look for the next visible view after the current one
        for (int newIdx = idx + 1; newIdx < d->mDocumentViews.count(); ++newIdx) {
            newCurrentView = d->mDocumentViews.at(newIdx);
            break;
        }
        if (!newCurrentView) {
            // No visible view found after the current one, look before
            for (int newIdx = idx - 1; newIdx >= 0; --newIdx) {
                newCurrentView = d->mDocumentViews.at(newIdx);
                break;
            }
        }
        if (!newCurrentView) {
            kWarning() << "No view found to set as current, this should not happen!";
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
