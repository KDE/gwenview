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
#include <QPushButton>

// KDE
#include <KDebug>
#include <KDirLister>
#include <KDirModel>
#include <KIconLoader>
#include <KUrlNavigator>

// Local
#include <lib/archiveutils.h>
#include <lib/kindproxymodel.h>
#include <lib/gwenviewconfig.h>
#include <lib/recursivedirmodel.h>
#include <lib/semanticinfo/sorteddirmodel.h>
#include <lib/thumbnailview/abstractthumbnailviewhelper.h>
#include <lib/thumbnailview/previewitemdelegate.h>
#include "documentdirfinder.h"
#include "importerconfigdialog.h"
#include <ui_thumbnailpage.h>

namespace Gwenview
{

static const int DEFAULT_THUMBNAIL_SIZE = 128;

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
    KUrlNavigator* mUrlNavigator;
    SortedDirModel* mDirModel;

    RecursiveDirModel* mRecursiveDirModel;
    KindProxyModel* mKindProxyModel;

    QPushButton* mImportSelectedButton;
    QPushButton* mImportAllButton;
    KUrl::List mUrlList;
    bool mRecurse;

    void setupDirModel()
    {
        mDirModel = new SortedDirModel(q);
        mDirModel->setKindFilter(
            MimeTypeUtils::KIND_DIR
            | MimeTypeUtils::KIND_ARCHIVE
            | MimeTypeUtils::KIND_RASTER_IMAGE
            | MimeTypeUtils::KIND_SVG_IMAGE
            | MimeTypeUtils::KIND_VIDEO
        );

        QObject::connect(
            mDirModel, SIGNAL(rowsInserted(QModelIndex,int,int)),
            q, SLOT(updateImportButtons()));
        QObject::connect(
            mDirModel, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            q, SLOT(updateImportButtons()));
        QObject::connect(
            mDirModel, SIGNAL(modelReset()),
            q, SLOT(updateImportButtons()));

        mRecursiveDirModel = new RecursiveDirModel(q);
        mKindProxyModel = new KindProxyModel(q);
        mKindProxyModel->setKindFilter(
            MimeTypeUtils::KIND_RASTER_IMAGE
            | MimeTypeUtils::KIND_SVG_IMAGE
            | MimeTypeUtils::KIND_VIDEO);
        mKindProxyModel->setSourceModel(mRecursiveDirModel);
    }

    void setupIcons()
    {
        const KIconLoader::Group group = KIconLoader::NoGroup;
        const int size = KIconLoader::SizeHuge;
        mSrcIconLabel->setPixmap(KIconLoader::global()->loadIcon("camera-photo", group, size));
        mDstIconLabel->setPixmap(KIconLoader::global()->loadIcon("computer", group, size));
    }

    void setupListingModeRadioButtons()
    {
        QObject::connect(mRecurseRadioButton, SIGNAL(toggled(bool)), q, SLOT(setListRecursively(bool)));
    }

    void setupDstUrlRequester()
    {
        mDstUrlRequester->setMode(KFile::Directory | KFile::LocalOnly);
    }

    void setupUrlNavigator()
    {
        mUrlNavigator = new KUrlNavigator(0 /* KFilePlacesModel */, KUrl(), mUrlNavigatorContainer);
        mUrlNavigator->setShowFullPath(true);
        QVBoxLayout* layout = new QVBoxLayout(mUrlNavigatorContainer);
        layout->setMargin(0);
        layout->addWidget(mUrlNavigator);

        QObject::connect(mUrlNavigator, SIGNAL(urlChanged(KUrl)),
                         q, SLOT(openUrl(KUrl)));
    }

    void setupThumbnailView()
    {
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
                         mThumbnailView, SLOT(setThumbnailSize(int)));
        int thumbnailSize = DEFAULT_THUMBNAIL_SIZE;
        mSlider->setValue(thumbnailSize);
        mSlider->updateToolTip();
        mThumbnailView->setThumbnailSize(thumbnailSize);

        QObject::connect(mThumbnailView, SIGNAL(indexActivated(QModelIndex)),
                         q, SLOT(slotThumbnailViewIndexActivated(QModelIndex)));
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
};

ThumbnailPage::ThumbnailPage()
: d(new ThumbnailPagePrivate)
{
    d->q = this;
    d->setupUi(this);
    d->setupIcons();
    d->setupListingModeRadioButtons();
    d->setupDirModel();
    d->setupDstUrlRequester();
    d->setupUrlNavigator();
    d->setupThumbnailView();
    d->setupButtonBox();
    setListRecursively(false);
    updateImportButtons();
}

ThumbnailPage::~ThumbnailPage()
{
    delete d;
}

void ThumbnailPage::setSourceUrl(const KUrl& url)
{
    DocumentDirFinder* finder = new DocumentDirFinder(url);
    connect(finder, SIGNAL(done(KUrl,DocumentDirFinder::Status)),
            SLOT(slotDocumentDirFinderDone(KUrl,DocumentDirFinder::Status)));
    finder->start();
}

void ThumbnailPage::slotDocumentDirFinderDone(const KUrl& url, DocumentDirFinder::Status status)
{
    kDebug() << url << "status:" << status;
    kDebug() << "FIXME: Handle different status";
    openUrl(url);
}

void ThumbnailPage::openUrl(const KUrl& url)
{
    d->mUrlNavigator->setLocationUrl(url);
    if (d->mRecurse) {
        d->mDirModel->dirLister()->openUrl(url);
    } else {
        d->mRecursiveDirModel->setUrl(url);
    }
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

void ThumbnailPage::slotThumbnailViewIndexActivated(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }

    KFileItem item = itemForIndex(index);
    if (item.isDir()) {
        // Item is a dir, open it
        openUrl(item.url());
    } else {
        QString protocol = ArchiveUtils::protocolForMimeType(item.mimetype());
        if (!protocol.isEmpty()) {
            // Item is an archive, tweak url then open it
            KUrl url = item.url();
            url.setProtocol(protocol);
            openUrl(url);
        }
    }
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
    d->mImportAllButton->setEnabled(d->mRecurse
        ? d->mThumbnailView->model()->rowCount(QModelIndex()) > 0
        : d->mDirModel->hasDocuments());
}

void ThumbnailPage::showConfigDialog()
{
    ImporterConfigDialog dialog(this);
    dialog.exec();
}

void ThumbnailPage::setListRecursively(bool recurse)
{
    d->mRecurse = recurse;
    KUrl url = d->mUrlNavigator->locationUrl();
    if (recurse) {
        if (url.isValid()) {
            d->mRecursiveDirModel->setUrl(url);
        }
        d->mThumbnailView->setModel(d->mKindProxyModel);
    } else {
        if (url.isValid()) {
            d->mDirModel->dirLister()->openUrl(url);
        }
        d->mThumbnailView->setModel(d->mDirModel);
    }

    connect(
        d->mThumbnailView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
        SLOT(updateImportButtons()));
}

} // namespace
