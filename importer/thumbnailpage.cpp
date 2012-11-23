// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
#include "thumbnailpage.moc"

// Qt
#include <QMenu>
#include <QPushButton>
#include <QTreeView>
#include <QWidgetAction>

// KDE
#include <KDebug>
#include <KDirLister>
#include <KDirModel>
#include <KDirSelectDialog>
#include <KIconLoader>
#include <KIO/NetAccess>

// Local
#include <lib/archiveutils.h>
#include <lib/kindproxymodel.h>
#include <lib/gwenviewconfig.h>
#include <lib/recursivedirmodel.h>
#include <lib/semanticinfo/sorteddirmodel.h>
#include <lib/thumbnailview/abstractthumbnailviewhelper.h>
#include <lib/thumbnailview/previewitemdelegate.h>
#include <documentdirfinder.h>
#include <importerconfigdialog.h>
#include <serializedurlmap.h>
#include <ui_thumbnailpage.h>

namespace Gwenview
{

static const int DEFAULT_THUMBNAIL_SIZE = 128;
static const qreal DEFAULT_THUMBNAIL_ASPECT_RATIO = 3. / 2.;

static const char* URL_FOR_BASE_URL_GROUP = "UrlForBaseUrl";

class ImporterThumbnailViewHelper : public AbstractThumbnailViewHelper
{
public:
    ImporterThumbnailViewHelper(QObject* parent)
        : AbstractThumbnailViewHelper(parent)
    {}

    void showContextMenu(QWidget*)
    {}

    void showMenuForUrlDroppedOnViewport(QWidget*, const KUrl::List&)
    {}

    void showMenuForUrlDroppedOnDir(QWidget*, const KUrl::List&, const KUrl&)
    {}
};

inline KFileItem itemForIndex(const QModelIndex& index)
{
    return index.data(KDirModel::FileItemRole).value<KFileItem>();
}

struct ThumbnailPagePrivate : public Ui_ThumbnailPage
{
    ThumbnailPage* q;
    QMenu* mSrcUrlMenu;
    SerializedUrlMap mUrlMap;

    ThumbnailPagePrivate()
    {
        mUrlMap.setConfigGroup(KConfigGroup(KGlobal::config(), URL_FOR_BASE_URL_GROUP));
    }

    KUrl mSrcBaseUrl;
    KUrl mSrcUrl;

    RecursiveDirModel* mRecursiveDirModel;
    QAbstractItemModel* mFinalModel;

    QPushButton* mImportSelectedButton;
    QPushButton* mImportAllButton;
    KUrl::List mUrlList;

    void setupDirModel()
    {
        mRecursiveDirModel = new RecursiveDirModel(q);

        KindProxyModel* kindProxyModel = new KindProxyModel(q);
        kindProxyModel->setKindFilter(
            MimeTypeUtils::KIND_RASTER_IMAGE
            | MimeTypeUtils::KIND_SVG_IMAGE
            | MimeTypeUtils::KIND_VIDEO);
        kindProxyModel->setSourceModel(mRecursiveDirModel);

        QSortFilterProxyModel *sortModel = new QSortFilterProxyModel(q);
        sortModel->setDynamicSortFilter(true);
        sortModel->setSourceModel(kindProxyModel);
        sortModel->sort(0);

        mFinalModel = sortModel;

        QObject::connect(
            mFinalModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
            q, SLOT(updateImportButtons()));
        QObject::connect(
            mFinalModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            q, SLOT(updateImportButtons()));
        QObject::connect(
            mFinalModel, SIGNAL(modelReset()),
            q, SLOT(updateImportButtons()));
    }

    void setupIcons()
    {
        const KIconLoader::Group group = KIconLoader::NoGroup;
        const int size = KIconLoader::SizeHuge;
        mSrcIconLabel->setPixmap(KIconLoader::global()->loadIcon("camera-photo", group, size));
        mDstIconLabel->setPixmap(KIconLoader::global()->loadIcon("computer", group, size));
    }

    void setupSrcUrlButton()
    {
        mSrcUrlMenu = new QMenu(q);
        QObject::connect(mSrcUrlMenu, SIGNAL(aboutToShow()), q, SLOT(initSrcUrlMenu()));
        mSrcUrlButton->setMenu(mSrcUrlMenu);
    }

    void setupDstUrlRequester()
    {
        mDstUrlRequester->setMode(KFile::Directory | KFile::LocalOnly);
    }

    void setupThumbnailView()
    {
        mThumbnailView->setModel(mFinalModel);

        mThumbnailView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        mThumbnailView->setThumbnailViewHelper(new ImporterThumbnailViewHelper(q));

        PreviewItemDelegate* delegate = new PreviewItemDelegate(mThumbnailView);
        delegate->setThumbnailDetails(PreviewItemDelegate::FileNameDetail);
        delegate->setContextBarActions(PreviewItemDelegate::SelectionAction);
        mThumbnailView->setItemDelegate(delegate);

        // Colors
        int value = GwenviewConfig::viewBackgroundValue();
        QColor bgColor = QColor::fromHsv(0, 0, value);
        QColor fgColor = value > 128 ? Qt::black : Qt::white;

        QPalette pal = mThumbnailView->palette();
        pal.setColor(QPalette::Base, bgColor);
        pal.setColor(QPalette::Text, fgColor);

        mThumbnailView->setPalette(pal);

        QObject::connect(mSlider, SIGNAL(valueChanged(int)),
                         mThumbnailView, SLOT(setThumbnailWidth(int)));
        QObject::connect(mThumbnailView, SIGNAL(thumbnailWidthChanged(int)),
                         mSlider, SLOT(setValue(int)));
        int thumbnailSize = DEFAULT_THUMBNAIL_SIZE;
        mSlider->setValue(thumbnailSize);
        mSlider->updateToolTip();
        mThumbnailView->setThumbnailAspectRatio(DEFAULT_THUMBNAIL_ASPECT_RATIO);
        mThumbnailView->setThumbnailWidth(thumbnailSize);

        QObject::connect(
            mThumbnailView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            q, SLOT(updateImportButtons()));
    }

    void setupButtonBox()
    {
        QObject::connect(mConfigureButton, SIGNAL(clicked()),
                         q, SLOT(showConfigDialog()));

        mImportSelectedButton = mButtonBox->addButton(
                                    i18n("Import Selected"), QDialogButtonBox::AcceptRole,
                                    q, SLOT(slotImportSelected()));

        mImportAllButton = mButtonBox->addButton(
                               i18n("Import All"), QDialogButtonBox::AcceptRole,
                               q, SLOT(slotImportAll()));

        QObject::connect(
            mButtonBox, SIGNAL(rejected()),
            q, SIGNAL(rejected()));
    }

    KUrl urlForBaseUrl() const
    {
        KUrl url = mUrlMap.value(mSrcBaseUrl);
        if (!url.isValid()) {
            return KUrl();
        }

        KIO::UDSEntry entry;
        bool ok = KIO::NetAccess::stat(url, entry, q);
        if (!ok) {
            return KUrl();
        }
        KFileItem item(entry, url, true /* delayedMimeTypes */);
        return item.isDir() ? url : KUrl();
    }

    void rememberUrl(const KUrl& url)
    {
        mUrlMap.insert(mSrcBaseUrl, url);
    }
};

ThumbnailPage::ThumbnailPage()
: d(new ThumbnailPagePrivate)
{
    d->q = this;
    d->setupUi(this);
    d->setupIcons();
    d->setupDirModel();
    d->setupSrcUrlButton();
    d->setupDstUrlRequester();
    d->setupThumbnailView();
    d->setupButtonBox();
    updateImportButtons();
}

ThumbnailPage::~ThumbnailPage()
{
    delete d;
}

void ThumbnailPage::setSourceUrl(const KUrl& srcBaseUrl)
{
    d->mSrcBaseUrl = srcBaseUrl;
    KUrl url = d->urlForBaseUrl();

    if (url.isValid()) {
        openUrl(url);
    } else {
        DocumentDirFinder* finder = new DocumentDirFinder(srcBaseUrl);
        connect(finder, SIGNAL(done(KUrl,DocumentDirFinder::Status)),
                SLOT(slotDocumentDirFinderDone(KUrl,DocumentDirFinder::Status)));
        finder->start();
    }
}

void ThumbnailPage::slotDocumentDirFinderDone(const KUrl& url, DocumentDirFinder::Status /*status*/)
{
    d->rememberUrl(url);
    openUrl(url);
}

void ThumbnailPage::openUrl(const KUrl& url)
{
    d->mSrcUrl = url;
    d->mSrcUrlButton->setText(url.pathOrUrl());
    d->mRecursiveDirModel->setUrl(url);
}

KUrl::List ThumbnailPage::urlList() const
{
    return d->mUrlList;
}

void ThumbnailPage::setDestinationUrl(const KUrl& url)
{
    d->mDstUrlRequester->setUrl(url);
}

KUrl ThumbnailPage::destinationUrl() const
{
    return d->mDstUrlRequester->url();
}

void ThumbnailPage::slotImportSelected()
{
    importList(d->mThumbnailView->selectionModel()->selectedIndexes());
}

void ThumbnailPage::slotImportAll()
{
    QModelIndexList list;
    QAbstractItemModel* model = d->mThumbnailView->model();
    for (int row = model->rowCount() - 1; row >= 0; --row) {
        list << model->index(row, 0);
    }
    importList(list);
}

void ThumbnailPage::importList(const QModelIndexList& list)
{
    d->mUrlList.clear();
    Q_FOREACH(const QModelIndex & index, list) {
        KFileItem item = itemForIndex(index);
        if (!ArchiveUtils::fileItemIsDirOrArchive(item)) {
            d->mUrlList << item.url();
        }
        // FIXME: Handle dirs (do we want to import recursively?)
    }
    emit importRequested();
}

void ThumbnailPage::updateImportButtons()
{
    d->mImportSelectedButton->setEnabled(d->mThumbnailView->selectionModel()->hasSelection());
    d->mImportAllButton->setEnabled(d->mThumbnailView->model()->rowCount(QModelIndex()) > 0);
}

void ThumbnailPage::showConfigDialog()
{
    ImporterConfigDialog dialog(this);
    dialog.exec();
}

void ThumbnailPage::initSrcUrlMenu()
{
    if (!d->mSrcUrlMenu->isEmpty()) {
        return;
    }

    KDirModel* model = new KDirModel(this);
    model->dirLister()->setDirOnlyMode(true);
    model->dirLister()->openUrl(d->mSrcBaseUrl);

    QSortFilterProxyModel *sortModel = new QSortFilterProxyModel(this);
    sortModel->setDynamicSortFilter(true);
    sortModel->setSourceModel(model);
    sortModel->sort(0);

    QTreeView* view = new QTreeView;
    view->resize(200, 300);
    view->setEditTriggers(QTreeView::NoEditTriggers);
    view->setModel(sortModel);
    for(int i = 1; i < model->columnCount(); ++i) {
        view->hideColumn(i);
    }
    view->setHeaderHidden(true);
    connect(view, SIGNAL(activated(QModelIndex)), SLOT(openUrlFromIndex(QModelIndex)));
    connect(view, SIGNAL(clicked(QModelIndex)), SLOT(openUrlFromIndex(QModelIndex)));

    QWidgetAction* viewAction = new QWidgetAction(d->mSrcUrlMenu);
    viewAction->setDefaultWidget(view);

    d->mSrcUrlMenu->addAction(viewAction);
}

void ThumbnailPage::openUrlFromIndex(const QModelIndex& index)
{
    KFileItem item = itemForIndex(index);
    if (item.isNull()) {
        return;
    }
    KUrl url = item.url();
    d->rememberUrl(url);
    openUrl(url);
}

} // namespace
