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
#include "browsemainpage.moc"

// Qt
#include <QDropEvent>
#include <QMenu>
#include <QSlider>
#include <QToolTip>
#include <QVBoxLayout>

// KDE
#include <KActionCollection>
#include <KActionCategory>
#include <KActionMenu>
#include <KDebug>
#include <KFileItem>
#include <KFilePlacesModel>
#include <KLineEdit>
#include <KLocale>
#include <KSelectAction>
#include <KStatusBar>
#include <KToggleAction>
#include <KUrlNavigator>

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
    int mDocumentCount;
    KActionCollection* mActionCollection;
    FilterController* mFilterController;
    KSelectAction* mSortAction;
    QActionGroup* mThumbnailDetailsActionGroup;
    PreviewItemDelegate* mDelegate;

    void setupWidgets()
    {
        setupUi(q);
        q->layout()->setMargin(0);

        // mThumbnailView
        mThumbnailView->setModel(mDirModel);

        mDelegate = new PreviewItemDelegate(mThumbnailView);
        mThumbnailView->setItemDelegate(mDelegate);
        mThumbnailView->setSelectionMode(QAbstractItemView::ExtendedSelection);

        // mUrlNavigator (use stupid layouting code because KUrlNavigator ctor
        // can't be used directly from Designer)
        mFilePlacesModel = new KFilePlacesModel(q);
        mUrlNavigator = new KUrlNavigator(mFilePlacesModel, KUrl(), mUrlNavigatorContainer);
        mUrlNavigatorContainer->setAutoFillBackground(true);
        QVBoxLayout* layout = new QVBoxLayout(mUrlNavigatorContainer);
        layout->setMargin(0);
        layout->addWidget(mUrlNavigator);
        QObject::connect(mUrlNavigator, SIGNAL(urlsDropped(KUrl,QDropEvent*)),
                         q, SLOT(slotUrlsDropped(KUrl,QDropEvent*)));
        updateUrlNavigatorBackgroundColor();

        // Thumbnail slider
        QObject::connect(mThumbnailSlider, SIGNAL(valueChanged(int)),
                         mThumbnailView, SLOT(setThumbnailWidth(int)));
        QObject::connect(mThumbnailView, SIGNAL(thumbnailWidthChanged(int)),
                         mThumbnailSlider, SLOT(setValue(int)));
    }

    void setupActions(KActionCollection* actionCollection)
    {
        KActionCategory* view = new KActionCategory(i18nc("@title actions category - means actions changing smth in interface", "View"), actionCollection);
        KAction* action = view->addAction("edit_location", q, SLOT(editLocation()));
        action->setText(i18nc("@action:inmenu Navigation Bar", "Edit Location"));
        action->setShortcut(Qt::Key_F6);

        mSortAction = view->add<KSelectAction>("sort_order");
        mSortAction->setText(i18nc("@action:inmenu", "Sort By"));
        action = mSortAction->addAction(i18nc("@addAction:inmenu", "Name"));
        action->setData(QVariant(Sorting::Name));
        action = mSortAction->addAction(i18nc("@addAction:inmenu", "Date"));
        action->setData(QVariant(Sorting::Date));
        action = mSortAction->addAction(i18nc("@addAction:inmenu", "Size"));
        action->setData(QVariant(Sorting::Size));
        QObject::connect(mSortAction, SIGNAL(triggered(QAction*)),
                         q, SLOT(updateSortOrder()));

        mThumbnailDetailsActionGroup = new QActionGroup(q);
        mThumbnailDetailsActionGroup->setExclusive(false);
        KActionMenu* thumbnailDetailsAction = view->add<KActionMenu>("thumbnail_details");
        thumbnailDetailsAction->setText(i18nc("@action:inmenu", "Thumbnail Details"));
#define addAction(text, detail) \
    action = new KAction(q); \
    thumbnailDetailsAction->addAction(action); \
    action->setText(text); \
    action->setCheckable(true); \
    action->setChecked(GwenviewConfig::thumbnailDetails() & detail); \
    action->setData(QVariant(detail)); \
    mThumbnailDetailsActionGroup->addAction(action); \
    QObject::connect(action, SIGNAL(triggered(bool)), \
                     q, SLOT(updateThumbnailDetails()));
        addAction(i18nc("@action:inmenu", "Filename"), PreviewItemDelegate::FileNameDetail);
        addAction(i18nc("@action:inmenu", "Date"), PreviewItemDelegate::DateDetail);
        addAction(i18nc("@action:inmenu", "Image Size"), PreviewItemDelegate::ImageSizeDetail);
        addAction(i18nc("@action:inmenu", "File Size"), PreviewItemDelegate::FileSizeDetail);
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
        addAction(i18nc("@action:inmenu", "Rating"), PreviewItemDelegate::RatingDetail);
#endif
#undef addAction

        KActionCategory* file = new KActionCategory(i18nc("@title actions category", "File"), actionCollection);
        action = file->addAction("add_folder_to_places", q, SLOT(addFolderToPlaces()));
        action->setText(i18nc("@action:inmenu", "Add Folder to Places"));
    }

    void setupFilterController()
    {
        QMenu* menu = new QMenu(mAddFilterButton);
        mFilterController = new FilterController(mFilterFrame, mDirModel);
        Q_FOREACH(QAction * action, mFilterController->actionList()) {
            menu->addAction(action);
        }
        mAddFilterButton->setMenu(menu);
    }

    void setupFullScreenToolBar()
    {
        mFullScreenToolBar->setIconDimensions(KIconLoader::SizeMedium);
        mFullScreenToolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        #define addAction(name) mFullScreenToolBar->addAction(mActionCollection->action(name))
        addAction("browse");
        addAction("view");
        #undef addAction

        mFullScreenToolBar2->setIconDimensions(KIconLoader::SizeMedium);
        mFullScreenToolBar2->setToolButtonStyle(Qt::ToolButtonIconOnly);
        mFullScreenToolBar2->addAction(mActionCollection->action("leave_fullscreen"));
    }

    void updateDocumentCountLabel()
    {
        QString text = i18ncp("@label", "%1 document", "%1 documents", mDocumentCount);
        mDocumentCountLabel->setText(text);
    }

    void setupDocumentCountConnections()
    {
        QObject::connect(mDirModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
                         q, SLOT(slotDirModelRowsInserted(QModelIndex,int,int)));

        QObject::connect(mDirModel, SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)),
                         q, SLOT(slotDirModelRowsAboutToBeRemoved(QModelIndex,int,int)));

        QObject::connect(mDirModel, SIGNAL(modelReset()),
                         q, SLOT(slotDirModelReset()));
    }

    int documentCountInIndexRange(const QModelIndex& parent, int start, int end)
    {
        int count = 0;
        for (int row = start; row <= end; ++row) {
            QModelIndex index = mDirModel->index(row, 0, parent);
            KFileItem item = mDirModel->itemForIndex(index);
            if (!ArchiveUtils::fileItemIsDirOrArchive(item)) {
                ++count;
            }
        }
        return count;
    }

    void updateUrlNavigatorBackgroundColor()
    {
       QPalette pal(q->palette());
       pal.setColor(QPalette::Window, pal.color(QPalette::Window).dark(110));
       mUrlNavigatorContainer->setPalette(pal);
    }
};

BrowseMainPage::BrowseMainPage(QWidget* parent, SortedDirModel* dirModel, KActionCollection* actionCollection, GvCore* gvCore)
: QWidget(parent)
, d(new BrowseMainPagePrivate)
{
    d->q = this;
    d->mDirModel = dirModel;
    d->mDocumentCount = 0;
    d->mActionCollection = actionCollection;
    d->mGvCore = gvCore;
    d->setupWidgets();
    d->setupActions(actionCollection);
    d->setupFilterController();
    d->setupDocumentCountConnections();
    loadConfig();
    updateSortOrder();
    updateThumbnailDetails();
}

BrowseMainPage::~BrowseMainPage()
{
    delete d;
}

void BrowseMainPage::loadConfig()
{
    setPalette(d->mGvCore->palette(GvCore::NormalPalette));
    d->mThumbnailView->setPalette(d->mGvCore->palette(GvCore::NormalViewPalette));
    d->mUrlNavigator->setUrlEditable(GwenviewConfig::urlNavigatorIsEditable());
    d->mUrlNavigator->setShowFullPath(GwenviewConfig::urlNavigatorShowFullPath());

    d->mThumbnailSlider->setValue(GwenviewConfig::thumbnailSize());
    d->mThumbnailSlider->updateToolTip();
    // If GwenviewConfig::thumbnailSize() returns the current value of
    // mThumbnailSlider, it won't emit valueChanged() and the thumbnail view
    // won't be updated. That's why we do it ourself.
    d->mThumbnailView->setThumbnailAspectRatio(GwenviewConfig::thumbnailAspectRatio());
    d->mThumbnailView->setThumbnailWidth(GwenviewConfig::thumbnailSize());

    Q_FOREACH(QAction * action, d->mSortAction->actions()) {
        if (sortingFromSortAction(action) == GwenviewConfig::sorting()) {
            d->mSortAction->setCurrentAction(action);
            break;
        }
    }
}

void BrowseMainPage::saveConfig() const
{
    GwenviewConfig::setUrlNavigatorIsEditable(d->mUrlNavigator->isUrlEditable());
    GwenviewConfig::setUrlNavigatorShowFullPath(d->mUrlNavigator->showFullPath());
    GwenviewConfig::setThumbnailSize(d->mThumbnailSlider->value());
    GwenviewConfig::setSorting(sortingFromSortAction(d->mSortAction->currentAction()));
    GwenviewConfig::setThumbnailDetails(d->mDelegate->thumbnailDetails());
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
    QModelIndexList list = d->mThumbnailView->selectionModel()->selectedIndexes();
    Q_FOREACH(const QModelIndex & index, list) {
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
    KUrl url = d->mUrlNavigator->locationUrl();
    QString text = url.fileName();
    if (text.isEmpty()) {
        text = url.pathOrUrl();
    }
    d->mFilePlacesModel->addPlace(text, url);
}

void BrowseMainPage::slotDirModelRowsInserted(const QModelIndex& parent, int start, int end)
{
    int count = d->documentCountInIndexRange(parent, start, end);
    if (count > 0) {
        d->mDocumentCount += count;
        d->updateDocumentCountLabel();
    }
}

void BrowseMainPage::slotDirModelRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
    int count = d->documentCountInIndexRange(parent, start, end);
    if (count > 0) {
        d->mDocumentCount -= count;
        d->updateDocumentCountLabel();
    }
}

void BrowseMainPage::slotDirModelReset()
{
    d->mDocumentCount = 0;
    d->updateDocumentCountLabel();
}

void BrowseMainPage::updateSortOrder()
{
    const QAction* action = d->mSortAction->currentAction();
    GV_RETURN_IF_FAIL(action);

    // This works because for now Sorting::Enum maps to KDirModel::ModelColumns
    d->mDirModel->setSortRole(sortingFromSortAction(action));
}

void BrowseMainPage::updateThumbnailDetails()
{
    PreviewItemDelegate::ThumbnailDetails details = 0;
    Q_FOREACH(const QAction * action, d->mThumbnailDetailsActionGroup->actions()) {
        if (action->isChecked()) {
            details |= PreviewItemDelegate::ThumbnailDetail(action->data().toInt());
        }
    }
    d->mDelegate->setThumbnailDetails(details);
}

void BrowseMainPage::setFullScreenMode(bool fullScreen)
{
    setPalette(d->mGvCore->palette(fullScreen ? GvCore::FullScreenPalette : GvCore::NormalPalette));
    d->mThumbnailView->setPalette(d->mGvCore->palette(fullScreen ? GvCore::FullScreenViewPalette : GvCore::NormalViewPalette));
    d->updateUrlNavigatorBackgroundColor();
    d->mUrlNavigatorContainer->setContentsMargins(
        fullScreen ? 6 : 0,
        0, 0, 0);
    PreviewItemDelegate::ContextBarActions actions = PreviewItemDelegate::SelectionAction | PreviewItemDelegate::RotateAction;
    if (!fullScreen) {
        actions |= PreviewItemDelegate::FullScreenAction;
    }
    d->mDelegate->setContextBarActions(actions);

    d->mFullScreenToolBar->setVisible(fullScreen);
    d->mFullScreenToolBar2->setVisible(fullScreen);
    if (fullScreen && d->mFullScreenToolBar->actions().isEmpty()) {
        d->setupFullScreenToolBar();
    }
}

void BrowseMainPage::slotUrlsDropped(const KUrl& destUrl, QDropEvent* event)
{
    const KUrl::List urlList = KUrl::List::fromMimeData(event->mimeData());
    if (urlList.isEmpty()) {
        return;
    }
    event->acceptProposedAction();

    // We can't call FileOperations::showMenuForDroppedUrls() directly because
    // we need the slot to return so that the drop event is accepted. Otherwise
    // the drop cursor is still visible when the menu is shown.
    QMetaObject::invokeMethod(this, "showMenuForDroppedUrls", Qt::QueuedConnection, Q_ARG(KUrl::List, urlList), Q_ARG(KUrl, destUrl));
}

void BrowseMainPage::showMenuForDroppedUrls(const KUrl::List& urlList, const KUrl& destUrl)
{
    FileOperations::showMenuForDroppedUrls(d->mUrlNavigator, urlList, destUrl);
}

QToolButton* BrowseMainPage::toggleSideBarButton() const
{
    return d->mToggleSideBarButton;
}

} // namespace
