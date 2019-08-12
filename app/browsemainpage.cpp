// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "browsemainpage.h"

// Qt
#include <QDropEvent>
#include <QMenu>
#include <QVBoxLayout>

// KDE
#include <KActionCollection>
#include <KActionCategory>
#include <KActionMenu>
#include <KFileItem>
#include <KFilePlacesModel>
#include <KLocalizedString>
#include <KSelectAction>
#include <KIconLoader>
#include <KUrlNavigator>
#include <KUrlMimeData>
#include <KFormat>
#include <KDirModel>

// Local
#include <filtercontroller.h>
#include <gvcore.h>
#include <fileoperations.h>
#include <lib/document/documentfactory.h>
#include <lib/gvdebug.h>
#include <lib/gwenviewconfig.h>
#include <lib/semanticinfo/abstractsemanticinfobackend.h>
#include <lib/semanticinfo/sorteddirmodel.h>
#include <lib/semanticinfo/tagmodel.h>
#include <lib/sorting.h>
#include <lib/archiveutils.h>
#include <lib/thumbnailview/previewitemdelegate.h>
#include <lib/thumbnailview/thumbnailview.h>
#include <ui_browsemainpage.h>
#include <mimetypeutils.h>
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
#include "lib/semanticinfo/semanticinfodirmodel.h"
#endif

namespace Gwenview
{

inline Sorting::Enum sortingFromSortAction(const QAction* action)
{
    Q_ASSERT(action);
    return Sorting::Enum(action->data().toInt());
}

struct BrowseMainPagePrivate : public Ui_BrowseMainPage
{
    BrowseMainPage* q;
    GvCore* mGvCore;
    KFilePlacesModel* mFilePlacesModel;
    KUrlNavigator* mUrlNavigator;
    SortedDirModel* mDirModel;
    int mDocumentCountImages;
    int mDocumentCountVideos;
    KFileItemList* mSelectedMediaItems;
    KActionCollection* mActionCollection;
    FilterController* mFilterController;
    QActionGroup* mSortAction;
    KToggleAction* mSortDescendingAction;
    QActionGroup* mThumbnailDetailsActionGroup;
    PreviewItemDelegate* mDelegate;

    void setupWidgets()
    {
        setupUi(q);
        q->layout()->setContentsMargins(0, 0, 0, 0);

        // mThumbnailView
        mThumbnailView->setModel(mDirModel);

        mDelegate = new PreviewItemDelegate(mThumbnailView);
        mThumbnailView->setItemDelegate(mDelegate);
        mThumbnailView->setSelectionMode(QAbstractItemView::ExtendedSelection);

        // mUrlNavigator (use stupid layouting code because KUrlNavigator ctor
        // can't be used directly from Designer)
        mFilePlacesModel = new KFilePlacesModel(q);
        mUrlNavigator = new KUrlNavigator(mFilePlacesModel, QUrl(), mUrlNavigatorContainer);
        mUrlNavigatorContainer->setAutoFillBackground(true);
        mUrlNavigatorContainer->setBackgroundRole(QPalette::Mid);
        QVBoxLayout* layout = new QVBoxLayout(mUrlNavigatorContainer);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(mUrlNavigator);
        QObject::connect(mUrlNavigator, SIGNAL(urlsDropped(QUrl,QDropEvent*)),
                         q, SLOT(slotUrlsDropped(QUrl,QDropEvent*)));

        // FullScreen Toolbar
        mFullScreenToolBar->setVisible(false);
        mFullScreenToolBar2->setVisible(false);
        mFullScreenToolBar->setAutoFillBackground(true);
        mFullScreenToolBar2->setAutoFillBackground(true);
        mFullScreenToolBar->setBackgroundRole(QPalette::Mid);
        mFullScreenToolBar2->setBackgroundRole(QPalette::Mid);

        // Thumbnail slider
        QObject::connect(mThumbnailSlider, &ZoomSlider::valueChanged,
                         mThumbnailView, &ThumbnailView::setThumbnailWidth);
        QObject::connect(mThumbnailView, &ThumbnailView::thumbnailWidthChanged,
                         mThumbnailSlider, &ZoomSlider::setValue);

        // Document count label
        QMargins labelMargins = mDocumentCountLabel->contentsMargins();
        labelMargins.setLeft(15);
        labelMargins.setRight(15);
        mDocumentCountLabel->setContentsMargins(labelMargins);
    }

    QAction *thumbnailDetailAction(const QString &text, PreviewItemDelegate::ThumbnailDetail detail)
    {
        QAction *action = new QAction(q);
        action->setText(text);
        action->setCheckable(true);
        action->setChecked(GwenviewConfig::thumbnailDetails() & detail);
        action->setData(QVariant(detail));
        mThumbnailDetailsActionGroup->addAction(action);
        QObject::connect(action, SIGNAL(triggered(bool)),
            q, SLOT(updateThumbnailDetails()));
        return action;
    }

    void setupActions(KActionCollection* actionCollection)
    {
        KActionCategory* view = new KActionCategory(i18nc("@title actions category - means actions changing smth in interface", "View"), actionCollection);
        QAction * action = view->addAction("edit_location", q, SLOT(editLocation()));
        action->setText(i18nc("@action:inmenu Navigation Bar", "Edit Location"));
        actionCollection->setDefaultShortcut(action, Qt::Key_F6);

        KActionMenu* sortActionMenu = view->add<KActionMenu>("sort_by");
        sortActionMenu->setText(i18nc("@action:inmenu", "Sort By"));
        
        mSortAction = new QActionGroup(actionCollection);
        action = new QAction(i18nc("@addAction:inmenu", "Name"), mSortAction);
        action->setCheckable(true);
        action->setData(QVariant(Sorting::Name));
        action = new QAction(i18nc("@addAction:inmenu", "Date"), mSortAction);
        action->setCheckable(true);
        action->setData(QVariant(Sorting::Date));
        action = new QAction(i18nc("@addAction:inmenu", "Size"), mSortAction);
        action->setCheckable(true);
        action->setData(QVariant(Sorting::Size));
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
        action = new QAction(i18nc("@addAction:inmenu", "Rating"), mSortAction);
        action->setCheckable(true);
        action->setData(QVariant(Sorting::Rating));
#endif
        QObject::connect(mSortAction, SIGNAL(triggered(QAction*)),
                         q, SLOT(updateSortOrder()));
                
        mSortDescendingAction = view->add<KToggleAction>("sort_desc");
        mSortDescendingAction->setText(i18nc("@action:inmenu Sort", "Descending"));
        QObject::connect(mSortDescendingAction, SIGNAL(toggled(bool)),
                         q, SLOT(updateSortOrder()));
        
        for(auto action : mSortAction->actions()) {
            sortActionMenu->addAction(action);
        }
        sortActionMenu->addSeparator();
        sortActionMenu->addAction(mSortDescendingAction);

        mThumbnailDetailsActionGroup = new QActionGroup(q);
        mThumbnailDetailsActionGroup->setExclusive(false);
        KActionMenu* thumbnailDetailsAction = view->add<KActionMenu>("thumbnail_details");
        thumbnailDetailsAction->setText(i18nc("@action:inmenu", "Thumbnail Details"));
        thumbnailDetailsAction->addAction(thumbnailDetailAction(i18nc("@action:inmenu", "Filename"), PreviewItemDelegate::FileNameDetail));
        thumbnailDetailsAction->addAction(thumbnailDetailAction(i18nc("@action:inmenu", "Date"), PreviewItemDelegate::DateDetail));
        thumbnailDetailsAction->addAction(thumbnailDetailAction(i18nc("@action:inmenu", "Image Size"), PreviewItemDelegate::ImageSizeDetail));
        thumbnailDetailsAction->addAction(thumbnailDetailAction(i18nc("@action:inmenu", "File Size"), PreviewItemDelegate::FileSizeDetail));
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
        thumbnailDetailsAction->addAction(thumbnailDetailAction(i18nc("@action:inmenu", "Rating"), PreviewItemDelegate::RatingDetail));
#endif

        KActionCategory* file = new KActionCategory(i18nc("@title actions category", "File"), actionCollection);
        action = file->addAction("add_folder_to_places", q, SLOT(addFolderToPlaces()));
        action->setText(i18nc("@action:inmenu", "Add Folder to Places"));
        action->setIcon(QIcon::fromTheme("bookmark-new"));
    }

    void setupFilterController()
    {
        QMenu* menu = new QMenu(mAddFilterButton);
        mFilterController = new FilterController(mFilterFrame, mDirModel);
        const auto actionList = mFilterController->actionList();
        for (QAction * action : actionList) {
            menu->addAction(action);
        }
        mAddFilterButton->setMenu(menu);
    }

    void setupFullScreenToolBar()
    {
        mFullScreenToolBar->setIconDimensions(KIconLoader::SizeMedium);
        mFullScreenToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        mFullScreenToolBar->addAction(mActionCollection->action("browse"));
        mFullScreenToolBar->addAction(mActionCollection->action("view"));

        mFullScreenToolBar2->setIconDimensions(KIconLoader::SizeMedium);
        mFullScreenToolBar2->setToolButtonStyle(Qt::ToolButtonIconOnly);
        mFullScreenToolBar2->addAction(mActionCollection->action("leave_fullscreen"));
    }

    void updateSelectedMediaItems(const QItemSelection& selected, const QItemSelection& deselected)
    {
        for (auto index : selected.indexes()) {
            KFileItem item = mDirModel->itemForIndex(index);
            if (!ArchiveUtils::fileItemIsDirOrArchive(item)) {
                mSelectedMediaItems->append(item);
            }
        }
        for (auto index : deselected.indexes()) {
            KFileItem item = mDirModel->itemForIndex(index);
            mSelectedMediaItems->removeOne(item);
        }
    }

    void updateDocumentCountLabel()
    {
        if (mSelectedMediaItems->count() > 1) {
            KIO::filesize_t totalSize = 0;
            for (const auto &item : *mSelectedMediaItems) {
                totalSize += item.size();
            }
            const QString text = i18nc("@info:status %1 number of selected documents, %2 total number of documents, %3 total filesize of selected documents",
                                       "Selected %1 of %2 (%3)",
                                       mSelectedMediaItems->count(),
                                       mDocumentCountImages + mDocumentCountVideos,
                                       KFormat().formatByteSize(totalSize));
            mDocumentCountLabel->setText(text);
        } else {
            const QString imageText = i18ncp("@info:status Image files", "%1 image", "%1 images", mDocumentCountImages);
            const QString videoText = i18ncp("@info:status Video files", "%1 video", "%1 videos", mDocumentCountVideos);
            QString labelText;
            if (mDocumentCountImages > 0 && mDocumentCountVideos == 0) {
                labelText = imageText;
            } else if (mDocumentCountImages == 0 && mDocumentCountVideos > 0) {
                labelText = videoText;
            } else {
                labelText = i18nc("@info:status images, videos", "%1, %2", imageText, videoText);
            }
            mDocumentCountLabel->setText(labelText);
        }
    }

    void documentCountsForIndexRange(const QModelIndex& parent, int start, int end, int& imageCountOut, int& videoCountOut)
    {
        imageCountOut = 0;
        videoCountOut = 0;
        for (int row = start; row <= end; ++row) {
            QModelIndex index = mDirModel->index(row, 0, parent);
            KFileItem item = mDirModel->itemForIndex(index);
            if (!ArchiveUtils::fileItemIsDirOrArchive(item)) {
                MimeTypeUtils::Kind kind = MimeTypeUtils::mimeTypeKind(item.mimetype());
                if (kind == MimeTypeUtils::KIND_RASTER_IMAGE || kind == MimeTypeUtils::KIND_SVG_IMAGE) {
                    imageCountOut++;
                } else if (kind == MimeTypeUtils::KIND_VIDEO) {
                    videoCountOut++;
                }
            }
        }
    }

    void updateContextBarActions()
    {
        PreviewItemDelegate::ContextBarActions actions;
        switch (GwenviewConfig::thumbnailActions())
        {
            case ThumbnailActions::None:
                actions = PreviewItemDelegate::NoAction;
                break;
            case ThumbnailActions::ShowSelectionButtonOnly:
                actions = PreviewItemDelegate::SelectionAction;
                break;
            case ThumbnailActions::AllButtons:
            default:
                actions = PreviewItemDelegate::SelectionAction | PreviewItemDelegate::RotateAction;
                if (!q->window()->isFullScreen()) {
                    actions |= PreviewItemDelegate::FullScreenAction;
                }
                break;
        }
        mDelegate->setContextBarActions(actions);
    }
    
    void applyPalette(bool fullScreenmode)
    {
        q->setPalette(mGvCore->palette(fullScreenmode ? GvCore::FullScreenPalette : GvCore::NormalPalette));
        mThumbnailView->setPalette(mGvCore->palette(fullScreenmode ? GvCore::FullScreenViewPalette : GvCore::NormalViewPalette));
    }
};

BrowseMainPage::BrowseMainPage(QWidget* parent, KActionCollection* actionCollection, GvCore* gvCore)
: QWidget(parent)
, d(new BrowseMainPagePrivate)
{
    d->q = this;
    d->mGvCore = gvCore;
    d->mDirModel = gvCore->sortedDirModel();
    d->mDocumentCountImages = 0;
    d->mDocumentCountVideos = 0;
    d->mSelectedMediaItems = new KFileItemList;
    d->mActionCollection = actionCollection;
    d->setupWidgets();
    d->setupActions(actionCollection);
    d->setupFilterController();
    loadConfig();
    updateSortOrder();
    updateThumbnailDetails();

    // Set up connections for document count
    connect(d->mDirModel, &SortedDirModel::rowsInserted,
            this, &BrowseMainPage::slotDirModelRowsInserted);
    connect(d->mDirModel, &SortedDirModel::rowsAboutToBeRemoved,
            this, &BrowseMainPage::slotDirModelRowsAboutToBeRemoved);
    connect(d->mDirModel, &SortedDirModel::modelReset,
            this, &BrowseMainPage::slotDirModelReset);
    connect(thumbnailView(), &ThumbnailView::selectionChangedSignal,
            this, &BrowseMainPage::slotSelectionChanged);

    installEventFilter(this);
}

BrowseMainPage::~BrowseMainPage()
{
    d->mSelectedMediaItems->clear();
    delete d->mSelectedMediaItems;
    delete d;
}

void BrowseMainPage::loadConfig()
{
    d->applyPalette(window()->isFullScreen());
    d->mUrlNavigator->setUrlEditable(GwenviewConfig::urlNavigatorIsEditable());
    d->mUrlNavigator->setShowFullPath(GwenviewConfig::urlNavigatorShowFullPath());

    d->mThumbnailSlider->setValue(GwenviewConfig::thumbnailSize());
    d->mThumbnailSlider->updateToolTip();
    // If GwenviewConfig::thumbnailSize() returns the current value of
    // mThumbnailSlider, it won't emit valueChanged() and the thumbnail view
    // won't be updated. That's why we do it ourself.
    d->mThumbnailView->setThumbnailAspectRatio(GwenviewConfig::thumbnailAspectRatio());
    d->mThumbnailView->setThumbnailWidth(GwenviewConfig::thumbnailSize());

    const auto actionsList = d->mSortAction->actions();
    for (QAction * action : actionsList) {
        if (sortingFromSortAction(action) == GwenviewConfig::sorting()) {
            action->setChecked(true);
            break;
        }
    }
    d->mSortDescendingAction->setChecked(GwenviewConfig::sortDescending());
    
    d->updateContextBarActions();
}

void BrowseMainPage::saveConfig() const
{
    GwenviewConfig::setUrlNavigatorIsEditable(d->mUrlNavigator->isUrlEditable());
    GwenviewConfig::setUrlNavigatorShowFullPath(d->mUrlNavigator->showFullPath());
    GwenviewConfig::setThumbnailSize(d->mThumbnailSlider->value());
    GwenviewConfig::setSorting(sortingFromSortAction(d->mSortAction->checkedAction()));
    GwenviewConfig::setSortDescending(d->mSortDescendingAction->isChecked());
    GwenviewConfig::setThumbnailDetails(d->mDelegate->thumbnailDetails());
}

bool BrowseMainPage::eventFilter(QObject* watched, QEvent* event)
{
    if (window()->isFullScreen() && event->type() == QEvent::ShortcutOverride) {
        const QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
           d->mActionCollection->action("leave_fullscreen")->trigger();
           event->accept();
        }
    }

    return QWidget::eventFilter(watched, event);
}

void BrowseMainPage::mousePressEvent(QMouseEvent* event)
{
    switch (event->button()) {
    case Qt::ForwardButton:
    case Qt::BackButton:
        return;
    default:
        QWidget::mousePressEvent(event);
    }
}

ThumbnailView* BrowseMainPage::thumbnailView() const
{
    return d->mThumbnailView;
}

KUrlNavigator* BrowseMainPage::urlNavigator() const
{
    return d->mUrlNavigator;
}

void BrowseMainPage::reload()
{
    const QModelIndexList list = d->mThumbnailView->selectionModel()->selectedIndexes();
    for (const QModelIndex & index : list) {
        d->mThumbnailView->reloadThumbnail(index);
    }
    d->mDirModel->reload();
}

void BrowseMainPage::editLocation()
{
    d->mUrlNavigator->setUrlEditable(true);
    d->mUrlNavigator->setFocus();
}

void BrowseMainPage::addFolderToPlaces()
{
    QUrl url = d->mUrlNavigator->locationUrl();
    QString text = url.adjusted(QUrl::StripTrailingSlash).fileName();
    if (text.isEmpty()) {
        text = url.toDisplayString();
    }
    d->mFilePlacesModel->addPlace(text, url);
}

void BrowseMainPage::slotDirModelRowsInserted(const QModelIndex& parent, int start, int end)
{
    int imageCount;
    int videoCount;
    d->documentCountsForIndexRange(parent, start, end, imageCount, videoCount);
    if (imageCount > 0 || videoCount > 0) {
        d->mDocumentCountImages += imageCount;
        d->mDocumentCountVideos += videoCount;
        d->updateDocumentCountLabel();
    }
}

void BrowseMainPage::slotDirModelRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
    int imageCount;
    int videoCount;
    d->documentCountsForIndexRange(parent, start, end, imageCount, videoCount);
    if (imageCount > 0 || videoCount > 0) {
        d->mDocumentCountImages -= imageCount;
        d->mDocumentCountVideos -= videoCount;
        d->updateDocumentCountLabel();
    }
}

void BrowseMainPage::slotDirModelReset()
{
    d->mDocumentCountImages = 0;
    d->mDocumentCountVideos = 0;
    d->mSelectedMediaItems->clear();
    d->updateDocumentCountLabel();
}

void BrowseMainPage::slotSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    d->updateSelectedMediaItems(selected, deselected);
    d->updateDocumentCountLabel();
}

void BrowseMainPage::updateSortOrder()
{
    const QAction* action = d->mSortAction->checkedAction();
    GV_RETURN_IF_FAIL(action);
    
    const Qt::SortOrder order = d->mSortDescendingAction->isChecked() ? Qt::DescendingOrder : Qt::AscendingOrder;
    KDirModel::ModelColumns column;
    int sortRole;

    // Map Sorting::Enum to model columns and sorting roles
    switch (sortingFromSortAction(action)) {
    case Sorting::Name:
        column = KDirModel::Name;
        sortRole = Qt::DisplayRole;
        break;
    case Sorting::Size:
        column = KDirModel::Size;
        sortRole = Qt::DisplayRole;
        break;
    case Sorting::Date:
        column = KDirModel::ModifiedTime;
        sortRole = Qt::DisplayRole;
        break;
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
    case Sorting::Rating:
        column = KDirModel::Name;
        sortRole = SemanticInfoDirModel::RatingRole;
        break;
#endif
    }

    d->mDirModel->setSortRole(sortRole);
    d->mDirModel->sort(column, order);
}

void BrowseMainPage::updateThumbnailDetails()
{
    PreviewItemDelegate::ThumbnailDetails details = 0;
    const auto actionList = d->mThumbnailDetailsActionGroup->actions();
    for (const QAction * action : actionList) {
        if (action->isChecked()) {
            details |= PreviewItemDelegate::ThumbnailDetail(action->data().toInt());
        }
    }
    d->mDelegate->setThumbnailDetails(details);
}

void BrowseMainPage::setFullScreenMode(bool fullScreen)
{
    d->applyPalette(fullScreen);
    d->mUrlNavigatorContainer->setContentsMargins(
        fullScreen ? 6 : 0,
        0, 0, 0);
    d->updateContextBarActions();

    d->mFullScreenToolBar->setVisible(fullScreen);
    d->mFullScreenToolBar2->setVisible(fullScreen);
    if (fullScreen && d->mFullScreenToolBar->actions().isEmpty()) {
        d->setupFullScreenToolBar();
    }
}

void BrowseMainPage::setStatusBarVisible(bool visible)
{
    d->mStatusBarContainer->setVisible(visible);
}

void BrowseMainPage::slotUrlsDropped(const QUrl &destUrl, QDropEvent* event)
{
    const QList<QUrl> urlList = KUrlMimeData::urlsFromMimeData(event->mimeData());
    if (urlList.isEmpty()) {
        return;
    }
    event->acceptProposedAction();

    // We can't call FileOperations::showMenuForDroppedUrls() directly because
    // we need the slot to return so that the drop event is accepted. Otherwise
    // the drop cursor is still visible when the menu is shown.
    QMetaObject::invokeMethod(this, "showMenuForDroppedUrls", Qt::QueuedConnection, Q_ARG(QList<QUrl>, urlList), Q_ARG(QUrl, destUrl));
}

void BrowseMainPage::showMenuForDroppedUrls(const QList<QUrl>& urlList, const QUrl &destUrl)
{
    FileOperations::showMenuForDroppedUrls(d->mUrlNavigator, urlList, destUrl);
}

QToolButton* BrowseMainPage::toggleSideBarButton() const
{
    return d->mToggleSideBarButton;
}

} // namespace
