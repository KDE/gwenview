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
#include "thumbnailpage.h"
#include "dialogguard.h"

// Qt
#include <QDebug>
#include <QDir>
#include <QIcon>
#include <QPushButton>
#include <QTreeView>

// KDE
#include <KAcceleratorManager>
#include <KDirLister>
#include <KDirModel>
#include <KIconLoader>
#include <kio/global.h>
#include <kmodelindexproxymapper.h>
#include <KJobWidgets>

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
#include <lib/thumbnailprovider/thumbnailprovider.h>

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

    void showContextMenu(QWidget*) Q_DECL_OVERRIDE
    {}

    void showMenuForUrlDroppedOnViewport(QWidget*, const QList<QUrl>&) Q_DECL_OVERRIDE
    {}

    void showMenuForUrlDroppedOnDir(QWidget*, const QList<QUrl>&, const QUrl&) Q_DECL_OVERRIDE
    {}
};

inline KFileItem itemForIndex(const QModelIndex& index)
{
    return index.data(KDirModel::FileItemRole).value<KFileItem>();
}

struct ThumbnailPagePrivate : public Ui_ThumbnailPage
{
    ThumbnailPage* q;
    SerializedUrlMap mUrlMap;

    QIcon mSrcBaseIcon;
    QString mSrcBaseName;
    QUrl mSrcBaseUrl;
    QUrl mSrcUrl;
    KModelIndexProxyMapper* mSrcUrlModelProxyMapper;

    RecursiveDirModel* mRecursiveDirModel;
    QAbstractItemModel* mFinalModel;

    ThumbnailProvider mThumbnailProvider;

    QPushButton* mImportSelectedButton;
    QPushButton* mImportAllButton;
    QList<QUrl> mUrlList;

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

    void setupSrcUrlWidgets()
    {
        mSrcUrlModelProxyMapper = 0;
        QObject::connect(mSrcUrlButton, SIGNAL(clicked()), q, SLOT(setupSrcUrlTreeView()));
        QObject::connect(mSrcUrlButton, SIGNAL(clicked()), q, SLOT(toggleSrcUrlTreeView()));
        mSrcUrlTreeView->hide();
        KAcceleratorManager::setNoAccel(mSrcUrlButton);
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
        mThumbnailView->setThumbnailProvider(&mThumbnailProvider);

        QObject::connect(
            mThumbnailView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            q, SLOT(updateImportButtons()));
    }

    void setupButtonBox()
    {
        QObject::connect(mConfigureButton, SIGNAL(clicked()),
                         q, SLOT(showConfigDialog()));

        mImportSelectedButton = mButtonBox->addButton(i18n("Import Selected"), QDialogButtonBox::AcceptRole);
        QObject::connect(mImportSelectedButton, SIGNAL(clicked(bool)), q, SLOT(slotImportSelected()));

        mImportAllButton = mButtonBox->addButton(i18n("Import All"), QDialogButtonBox::AcceptRole);
        QObject::connect(mImportAllButton, SIGNAL(clicked(bool)), q, SLOT(slotImportAll()));

        QObject::connect(
            mButtonBox, SIGNAL(rejected()),
            q, SIGNAL(rejected()));
    }

    QUrl urlForBaseUrl() const
    {
        QUrl url = mUrlMap.value(mSrcBaseUrl);
        if (!url.isValid()) {
            return QUrl();
        }

        KIO::StatJob *job = KIO::stat(url);
        KJobWidgets::setWindow(job, q);
        if (!job->exec()) {
            return QUrl();
        }
        KFileItem item(job->statResult(), url, true /* delayedMimeTypes */);
        return item.isDir() ? url : QUrl();
    }

    void rememberUrl(const QUrl& url)
    {
        mUrlMap.insert(mSrcBaseUrl, url);
    }
};

ThumbnailPage::ThumbnailPage()
: d(new ThumbnailPagePrivate)
{
    d->q = this;
    d->mUrlMap.setConfigGroup(KConfigGroup(KSharedConfig::openConfig(), URL_FOR_BASE_URL_GROUP));
    d->setupUi(this);
    d->setupIcons();
    d->setupDirModel();
    d->setupSrcUrlWidgets();
    d->setupDstUrlRequester();
    d->setupThumbnailView();
    d->setupButtonBox();
    updateImportButtons();
}

ThumbnailPage::~ThumbnailPage()
{
    delete d;
}

void ThumbnailPage::setSourceUrl(const QUrl& srcBaseUrl, const QString& iconName, const QString& name)
{
    d->mSrcBaseIcon = QIcon::fromTheme(iconName);
    d->mSrcBaseName = name;

    const int size = KIconLoader::SizeHuge;
    d->mSrcIconLabel->setPixmap(d->mSrcBaseIcon.pixmap(size));

    d->mSrcBaseUrl = srcBaseUrl;
    if (!d->mSrcBaseUrl.path().endsWith('/')) {
        d->mSrcBaseUrl.setPath(d->mSrcBaseUrl.path() + '/');
    }
    QUrl url = d->urlForBaseUrl();

    if (url.isValid()) {
        openUrl(url);
    } else {
        DocumentDirFinder* finder = new DocumentDirFinder(srcBaseUrl);
        connect(finder, SIGNAL(done(QUrl,DocumentDirFinder::Status)),
                SLOT(slotDocumentDirFinderDone(QUrl,DocumentDirFinder::Status)));
        finder->start();
    }
}

void ThumbnailPage::slotDocumentDirFinderDone(const QUrl& url, DocumentDirFinder::Status /*status*/)
{
    d->rememberUrl(url);
    openUrl(url);
}

void ThumbnailPage::openUrl(const QUrl& url)
{
    d->mSrcUrl = url;
    QString path = QDir(d->mSrcBaseUrl.path()).relativeFilePath(d->mSrcUrl.path());
    QString text;
    if (path.isEmpty() || path == ".") {
        text = d->mSrcBaseName;
    } else {
        path = QUrl::fromPercentEncoding(path.toUtf8());
        path.replace("/", QString::fromUtf8(" › "));
        text = QString::fromUtf8("%1 › %2").arg(d->mSrcBaseName).arg(path);
    }
    d->mSrcUrlButton->setText(text);
    d->mRecursiveDirModel->setUrl(url);
}

QList<QUrl> ThumbnailPage::urlList() const
{
    return d->mUrlList;
}

void ThumbnailPage::setDestinationUrl(const QUrl& url)
{
    d->mDstUrlRequester->setUrl(url);
}

QUrl ThumbnailPage::destinationUrl() const
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
    DialogGuard<ImporterConfigDialog> dialog(this);
    dialog->exec();
}

/**
 * This model allows only the url passed in the constructor to appear at the root
 * level. This makes it possible to select the url, but not its siblings.
 * It also provides custom role values for the root item.
 */
class OnlyBaseUrlProxyModel : public QSortFilterProxyModel
{
public:
    OnlyBaseUrlProxyModel(const QUrl& url, const QIcon& icon, const QString& name, QObject* parent)
    : QSortFilterProxyModel(parent)
    , mUrl(url)
    , mIcon(icon)
    , mName(name)
    {}

    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const // reimp
    Q_DECL_OVERRIDE {
        if (sourceParent.isValid()) {
            return true;
        }
        QModelIndex index = sourceModel()->index(sourceRow, 0);
        KFileItem item = itemForIndex(index);
        return item.url().matches(mUrl, QUrl::StripTrailingSlash);
    }

    QVariant data(const QModelIndex& index, int role) const // reimp
    Q_DECL_OVERRIDE {
        if (index.parent().isValid()) {
            return QSortFilterProxyModel::data(index, role);
        }
        switch (role) {
        case Qt::DisplayRole:
            return mName;
        case Qt::DecorationRole:
            return mIcon;
        case Qt::ToolTipRole:
            return mUrl.toDisplayString(QUrl::PreferLocalFile);
        default:
            return QSortFilterProxyModel::data(index, role);
        }
    }

private:
    QUrl mUrl;
    QIcon mIcon;
    QString mName;
};

void ThumbnailPage::setupSrcUrlTreeView()
{
    if (d->mSrcUrlTreeView->model()) {
        // Already initialized
        return;
    }
    KDirModel* dirModel = new KDirModel(this);
    dirModel->dirLister()->setDirOnlyMode(true);
    dirModel->dirLister()->openUrl(KIO::upUrl(d->mSrcBaseUrl));

    OnlyBaseUrlProxyModel* onlyBaseUrlModel = new OnlyBaseUrlProxyModel(d->mSrcBaseUrl, d->mSrcBaseIcon, d->mSrcBaseName, this);
    onlyBaseUrlModel->setSourceModel(dirModel);

    QSortFilterProxyModel* sortModel = new QSortFilterProxyModel(this);
    sortModel->setDynamicSortFilter(true);
    sortModel->setSourceModel(onlyBaseUrlModel);
    sortModel->sort(0);

    d->mSrcUrlModelProxyMapper = new KModelIndexProxyMapper(dirModel, sortModel, this);

    d->mSrcUrlTreeView->setModel(sortModel);
    for(int i = 1; i < dirModel->columnCount(); ++i) {
        d->mSrcUrlTreeView->hideColumn(i);
    }
    connect(d->mSrcUrlTreeView, SIGNAL(activated(QModelIndex)), SLOT(openUrlFromIndex(QModelIndex)));
    connect(d->mSrcUrlTreeView, SIGNAL(clicked(QModelIndex)), SLOT(openUrlFromIndex(QModelIndex)));

    dirModel->expandToUrl(d->mSrcUrl);
    connect(dirModel, SIGNAL(expand(QModelIndex)), SLOT(slotSrcUrlModelExpand(QModelIndex)));
}

void ThumbnailPage::slotSrcUrlModelExpand(const QModelIndex& index)
{
    QModelIndex viewIndex = d->mSrcUrlModelProxyMapper->mapLeftToRight(index);
    d->mSrcUrlTreeView->expand(viewIndex);
    KFileItem item = itemForIndex(index);
    if (item.url() == d->mSrcUrl) {
        d->mSrcUrlTreeView->selectionModel()->select(viewIndex, QItemSelectionModel::ClearAndSelect);
    }
}

void ThumbnailPage::toggleSrcUrlTreeView()
{
    d->mSrcUrlTreeView->setVisible(!d->mSrcUrlTreeView->isVisible());
}

void ThumbnailPage::openUrlFromIndex(const QModelIndex& index)
{
    KFileItem item = itemForIndex(index);
    if (item.isNull()) {
        return;
    }
    QUrl url = item.url();
    d->rememberUrl(url);
    openUrl(url);
}

} // namespace
