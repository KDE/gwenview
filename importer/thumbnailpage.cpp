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

// Qt
#include <QDesktopServices>
#include <QDir>
#include <QGraphicsOpacityEffect>
#include <QIcon>
#include <QProcess>
#include <QPushButton>
#include <QTreeView>

// KF
#include <KAcceleratorManager>
#include <KDirLister>
#include <KDirModel>
#include <KIO/DesktopExecParser>
#include <KIconLoader>
#include <KJobWidgets>
#include <KMessageBox>
#include <KModelIndexProxyMapper>
#include <kio/global.h>

// Local
#include "gwenview_importer_debug.h"
#include <documentdirfinder.h>
#include <importerconfigdialog.h>
#include <lib/archiveutils.h>
#include <lib/gwenviewconfig.h>
#include <lib/kindproxymodel.h>
#include <lib/recursivedirmodel.h>
#include <lib/semanticinfo/sorteddirmodel.h>
#include <lib/thumbnailprovider/thumbnailprovider.h>
#include <lib/thumbnailview/abstractthumbnailviewhelper.h>
#include <lib/thumbnailview/previewitemdelegate.h>
#include <serializedurlmap.h>
#include <ui_thumbnailpage.h>

namespace Gwenview
{
static const int DEFAULT_THUMBNAIL_SIZE = 128;
static const qreal DEFAULT_THUMBNAIL_ASPECT_RATIO = 3. / 2.;

static const char *URL_FOR_BASE_URL_GROUP = "UrlForBaseUrl";

class ImporterThumbnailViewHelper : public AbstractThumbnailViewHelper
{
public:
    ImporterThumbnailViewHelper(QObject *parent)
        : AbstractThumbnailViewHelper(parent)
    {
    }

    void showContextMenu(QWidget *) override
    {
    }

    void showMenuForUrlDroppedOnViewport(QWidget *, const QList<QUrl> &) override
    {
    }

    void showMenuForUrlDroppedOnDir(QWidget *, const QList<QUrl> &, const QUrl &) override
    {
    }
};

inline KFileItem itemForIndex(const QModelIndex &index)
{
    return index.data(KDirModel::FileItemRole).value<KFileItem>();
}

struct ThumbnailPagePrivate : public Ui_ThumbnailPage {
    ThumbnailPage *q = nullptr;
    SerializedUrlMap mUrlMap;

    QIcon mSrcBaseIcon;
    QString mSrcBaseName;
    QUrl mSrcBaseUrl;
    QUrl mSrcUrl;
    KModelIndexProxyMapper *mSrcUrlModelProxyMapper = nullptr;

    RecursiveDirModel *mRecursiveDirModel = nullptr;
    QAbstractItemModel *mFinalModel = nullptr;

    ThumbnailProvider mThumbnailProvider;

    // Placeholder view
    QLabel *mPlaceHolderIconLabel = nullptr;
    QLabel *mPlaceHolderLabel = nullptr;
    QLabel *mRequireRestartLabel = nullptr;
    QPushButton *mInstallProtocolSupportButton = nullptr;
    QVBoxLayout *mPlaceHolderLayout = nullptr;
    QWidget *mPlaceHolderWidget = nullptr; // To avoid clipping in gridLayout

    QPushButton *mImportSelectedButton;
    QPushButton *mImportAllButton;
    QList<QUrl> mUrlList;

    void setupDirModel()
    {
        mRecursiveDirModel = new RecursiveDirModel(q);

        auto kindProxyModel = new KindProxyModel(q);
        kindProxyModel->setKindFilter(MimeTypeUtils::KIND_RASTER_IMAGE | MimeTypeUtils::KIND_SVG_IMAGE | MimeTypeUtils::KIND_VIDEO);
        kindProxyModel->setSourceModel(mRecursiveDirModel);

        auto sortModel = new QSortFilterProxyModel(q);
        sortModel->setDynamicSortFilter(true);
        sortModel->setSourceModel(kindProxyModel);
        sortModel->sort(0);

        mFinalModel = sortModel;

        QObject::connect(mFinalModel, &QAbstractItemModel::rowsInserted, q, &ThumbnailPage::updateImportButtons);
        QObject::connect(mFinalModel, &QAbstractItemModel::rowsRemoved, q, &ThumbnailPage::updateImportButtons);
        QObject::connect(mFinalModel, &QAbstractItemModel::modelReset, q, &ThumbnailPage::updateImportButtons);
    }

    void setupIcons()
    {
        const int size = KIconLoader::SizeHuge;
        mSrcIconLabel->setPixmap(QIcon::fromTheme(QStringLiteral("camera-photo")).pixmap(size));
        mDstIconLabel->setPixmap(QIcon::fromTheme(QStringLiteral("computer")).pixmap(size));
    }

    void setupSrcUrlWidgets()
    {
        mSrcUrlModelProxyMapper = nullptr;
        QObject::connect(mSrcUrlButton, &QAbstractButton::clicked, q, &ThumbnailPage::setupSrcUrlTreeView);
        QObject::connect(mSrcUrlButton, &QAbstractButton::clicked, q, &ThumbnailPage::toggleSrcUrlTreeView);
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

        auto delegate = new PreviewItemDelegate(mThumbnailView);
        delegate->setThumbnailDetails(PreviewItemDelegate::FileNameDetail);
        PreviewItemDelegate::ContextBarActions actions;
        switch (GwenviewConfig::thumbnailActions()) {
        case ThumbnailActions::None:
            actions = PreviewItemDelegate::NoAction;
            break;
        case ThumbnailActions::ShowSelectionButtonOnly:
        case ThumbnailActions::AllButtons:
            actions = PreviewItemDelegate::SelectionAction;
            break;
        }
        delegate->setContextBarActions(actions);
        mThumbnailView->setItemDelegate(delegate);

        QObject::connect(mSlider, &ZoomSlider::valueChanged, mThumbnailView, &ThumbnailView::setThumbnailWidth);
        QObject::connect(mThumbnailView, &ThumbnailView::thumbnailWidthChanged, mSlider, &ZoomSlider::setValue);
        int thumbnailSize = DEFAULT_THUMBNAIL_SIZE;
        mSlider->setValue(thumbnailSize);
        mSlider->updateToolTip();
        mThumbnailView->setThumbnailAspectRatio(DEFAULT_THUMBNAIL_ASPECT_RATIO);
        mThumbnailView->setThumbnailWidth(thumbnailSize);
        mThumbnailView->setThumbnailProvider(&mThumbnailProvider);

        QObject::connect(mThumbnailView->selectionModel(), &QItemSelectionModel::selectionChanged, q, &ThumbnailPage::updateImportButtons);
    }

    void setupPlaceHolderView()
    {
        mPlaceHolderWidget = new QWidget(q);
        // Use QSizePolicy::MinimumExpanding to avoid clipping
        mPlaceHolderWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

        // Icon
        mPlaceHolderIconLabel = new QLabel(mPlaceHolderWidget);
        const QSize iconSize(KIconLoader::SizeHuge, KIconLoader::SizeHuge);
        mPlaceHolderIconLabel->setMinimumSize(iconSize);
        mPlaceHolderIconLabel->setPixmap(QIcon::fromTheme(QStringLiteral("edit-none")).pixmap(iconSize));
        auto iconEffect = new QGraphicsOpacityEffect(mPlaceHolderIconLabel);
        iconEffect->setOpacity(0.5);
        mPlaceHolderIconLabel->setGraphicsEffect(iconEffect);

        // Label: see dolphin/src/views/dolphinview.cpp
        const QSizePolicy labelSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred, QSizePolicy::Label);
        mPlaceHolderLabel = new QLabel(mPlaceHolderWidget);
        mPlaceHolderLabel->setSizePolicy(labelSizePolicy);
        QFont placeholderLabelFont;
        // To match the size of a level 2 Heading/KTitleWidget
        placeholderLabelFont.setPointSize(qRound(placeholderLabelFont.pointSize() * 1.3));
        mPlaceHolderLabel->setFont(placeholderLabelFont);
        mPlaceHolderLabel->setTextInteractionFlags(Qt::NoTextInteraction);
        mPlaceHolderLabel->setWordWrap(true);
        mPlaceHolderLabel->setAlignment(Qt::AlignCenter);
        // Match opacity of QML placeholder label component
        auto effect = new QGraphicsOpacityEffect(mPlaceHolderLabel);
        effect->setOpacity(0.5);
        mPlaceHolderLabel->setGraphicsEffect(effect);
        // Show more friendly text when the protocol is "camera" (which is the usual case)
        const QString scheme(mSrcBaseUrl.scheme());
        // Truncate long protocol name
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        const QString truncatedScheme(scheme.length() <= 10 ? scheme : scheme.leftRef(5) + QStringLiteral("…") + scheme.rightRef(5));
#else
        const QString truncatedScheme(
            scheme.length() <= 10 ? scheme : QStringView(scheme).left(5).toString() + QStringLiteral("…") + QStringView(scheme).right(5).toString());
#endif
        // clang-format off
        if (scheme == QLatin1String("camera")) {
            mPlaceHolderLabel->setText(i18nc("@info above install button when Kamera is not installed", "Support for your camera is not installed."));
        } else {
            mPlaceHolderLabel->setText(i18nc("@info above install button, %1 protocol name", "The protocol support library for \"%1\" is not installed.", truncatedScheme));
        }

        // Label to guide the user to restart the wizard after installing the protocol support library
        mRequireRestartLabel = new QLabel(mPlaceHolderWidget);
        mRequireRestartLabel->setSizePolicy(labelSizePolicy);
        mRequireRestartLabel->setTextInteractionFlags(Qt::NoTextInteraction);
        mRequireRestartLabel->setWordWrap(true);
        mRequireRestartLabel->setAlignment(Qt::AlignCenter);
        auto effect2 = new QGraphicsOpacityEffect(mRequireRestartLabel);
        effect2->setOpacity(0.5);
        mRequireRestartLabel->setGraphicsEffect(effect2);
        mRequireRestartLabel->setText(i18nc("@info:usagetip above install button", "After finishing the installation process, restart this Importer to continue."));

        // Button
        // Check if Discover is installed
        q->mDiscoverAvailable = !QStandardPaths::findExecutable("plasma-discover").isEmpty();
        QIcon buttonIcon(q->mDiscoverAvailable ? QIcon::fromTheme("plasmadiscover") : QIcon::fromTheme("install"));
        QString buttonText, whatsThisText;
        QString tooltipText(i18nc("@info:tooltip for a button, %1 protocol name", "Launch Discover to install the protocol support library for \"%1\"", scheme));
        if (scheme == QLatin1String("camera")) {
            buttonText = i18nc("@action:button", "Install Support for this Camera…");
            whatsThisText = i18nc("@info:whatsthis for a button when Kamera is not installed", "You need Kamera installed on your system to read from the camera. Click here to launch Discover to install Kamera to enable protocol support for \"camera:/\" on your system.");
        } else {
            if (q->mDiscoverAvailable) {
                buttonText = i18nc("@action:button %1 protocol name", "Install Protocol Support for \"%1\"…", truncatedScheme);
                whatsThisText = i18nc("@info:whatsthis for a button, %1 protocol name", "Click here to launch Discover to install the missing protocol support library to enable the protocol support for \"%1:/\" on your system.", scheme);
            } else {
                // If Discover is not found on the system, guide the user to search the web.
                buttonIcon = QIcon::fromTheme("internet-web-browser");
                buttonText = i18nc("@action:button %1 protocol name", "Search the Web for How to Install Protocol Support for \"%1\"…", truncatedScheme);
                tooltipText.clear();
            }
        }
        mInstallProtocolSupportButton = new QPushButton(buttonIcon, buttonText, mPlaceHolderWidget);
        mInstallProtocolSupportButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        mInstallProtocolSupportButton->setToolTip(tooltipText);
        mInstallProtocolSupportButton->setWhatsThis(whatsThisText);
        // Highlight the button so the user can notice it more easily.
        mInstallProtocolSupportButton->setDefault(true);
        mInstallProtocolSupportButton->setFocus();
        // Button action
        if (q->mDiscoverAvailable || scheme == QLatin1String("camera")) {
            QObject::connect(mInstallProtocolSupportButton, &QAbstractButton::clicked, q, &ThumbnailPage::installProtocolSupport);
        } else {
            QObject::connect(mInstallProtocolSupportButton, &QAbstractButton::clicked, q, [scheme]() {
                const QString searchKeyword(QUrl::toPercentEncoding(i18nc("@info this text will be used as a search term in an online search engine, %1 protocol name", "How to install protocol support for \"%1\" on Linux", scheme)).constData());
                const QString searchEngineURL(i18nc("search engine URL, %1 search keyword, and translators can replace duckduckgo with other search engines", "https://duckduckgo.com/?q=%1", searchKeyword));
                QDesktopServices::openUrl(QUrl(searchEngineURL));
            });
        }
        // clang-format on

        // VBoxLayout
        mPlaceHolderLayout = new QVBoxLayout(mPlaceHolderWidget);
        mPlaceHolderLayout->addStretch();
        mPlaceHolderLayout->addWidget(mPlaceHolderIconLabel, 0, Qt::AlignCenter);
        mPlaceHolderLayout->addWidget(mPlaceHolderLabel);
        mPlaceHolderLayout->addWidget(mRequireRestartLabel);
        mPlaceHolderLayout->addWidget(mInstallProtocolSupportButton, 0, Qt::AlignCenter); // Do not stretch the button
        mPlaceHolderLayout->addStretch();

        // Hide other controls
        gridLayout->removeItem(verticalLayout);
        gridLayout->removeItem(verticalLayout_2);
        gridLayout->removeWidget(label);
        gridLayout->removeWidget(label_2);
        gridLayout->removeWidget(widget);
        gridLayout->removeWidget(widget);
        gridLayout->removeWidget(mDstUrlRequester);
        mDstIconLabel->hide();
        mSrcIconLabel->hide();
        label->hide();
        label_2->hide();
        mDstUrlRequester->hide();
        widget->hide();
        widget_2->hide();
        mConfigureButton->hide();
        mImportSelectedButton->hide();
        mImportAllButton->hide();

        gridLayout->addWidget(mPlaceHolderWidget, 0, 0, 1, 0);
    }

    void setupButtonBox()
    {
        QObject::connect(mConfigureButton, &QAbstractButton::clicked, q, &ThumbnailPage::showConfigDialog);

        mImportSelectedButton = mButtonBox->addButton(i18n("Import Selected"), QDialogButtonBox::AcceptRole);
        QObject::connect(mImportSelectedButton, &QAbstractButton::clicked, q, &ThumbnailPage::slotImportSelected);

        mImportAllButton = mButtonBox->addButton(i18n("Import All"), QDialogButtonBox::AcceptRole);
        QObject::connect(mImportAllButton, &QAbstractButton::clicked, q, &ThumbnailPage::slotImportAll);

        QObject::connect(mButtonBox, &QDialogButtonBox::rejected, q, &ThumbnailPage::rejected);
    }

    QUrl urlForBaseUrl() const
    {
        QUrl url = mUrlMap.value(mSrcBaseUrl);
        if (!url.isValid()) {
            return {};
        }

        KIO::StatJob *job = KIO::stat(url);
        KJobWidgets::setWindow(job, q);
        if (!job->exec()) {
            return {};
        }
        KFileItem item(job->statResult(), url, true /* delayedMimeTypes */);
        return item.isDir() ? url : QUrl();
    }

    void rememberUrl(const QUrl &url)
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

void ThumbnailPage::setSourceUrl(const QUrl &srcBaseUrl, const QString &iconName, const QString &name)
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
        auto finder = new DocumentDirFinder(srcBaseUrl);
        connect(finder, &DocumentDirFinder::done, this, &ThumbnailPage::slotDocumentDirFinderDone);
        connect(finder, &DocumentDirFinder::protocollNotSupportedError, this, [this]() {
            d->setupPlaceHolderView();
        });
        finder->start();
    }
}

void ThumbnailPage::slotDocumentDirFinderDone(const QUrl &url, DocumentDirFinder::Status /*status*/)
{
    d->rememberUrl(url);
    openUrl(url);
}

void ThumbnailPage::openUrl(const QUrl &url)
{
    d->mSrcUrl = url;
    QString path = QDir(d->mSrcBaseUrl.path()).relativeFilePath(d->mSrcUrl.path());
    QString text;
    if (path.isEmpty() || path == QLatin1String(".")) {
        text = d->mSrcBaseName;
    } else {
        path = QUrl::fromPercentEncoding(path.toUtf8());
        path.replace('/', QString::fromUtf8(" › "));
        text = QString::fromUtf8("%1 › %2").arg(d->mSrcBaseName, path);
    }
    d->mSrcUrlButton->setText(text);
    d->mRecursiveDirModel->setUrl(url);
}

QList<QUrl> ThumbnailPage::urlList() const
{
    return d->mUrlList;
}

void ThumbnailPage::setDestinationUrl(const QUrl &url)
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
    QAbstractItemModel *model = d->mThumbnailView->model();
    for (int row = model->rowCount() - 1; row >= 0; --row) {
        list << model->index(row, 0);
    }
    importList(list);
}

void ThumbnailPage::importList(const QModelIndexList &list)
{
    d->mUrlList.clear();
    for (const QModelIndex &index : list) {
        KFileItem item = itemForIndex(index);
        if (!ArchiveUtils::fileItemIsDirOrArchive(item)) {
            d->mUrlList << item.url();
        }
        // FIXME: Handle dirs (do we want to import recursively?)
    }
    Q_EMIT importRequested();
}

void ThumbnailPage::updateImportButtons()
{
    d->mImportSelectedButton->setEnabled(d->mThumbnailView->selectionModel()->hasSelection());
    d->mImportAllButton->setEnabled(d->mThumbnailView->model()->rowCount(QModelIndex()) > 0);
}

void ThumbnailPage::showConfigDialog()
{
    auto dialog = new ImporterConfigDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModal(true);
    dialog->show();
}

void ThumbnailPage::installProtocolSupport() const
{
    const QString scheme(d->mSrcBaseUrl.scheme());
    // clang-format off
    if (scheme == QLatin1String("camera")) {
        const QUrl kameraInstallUrl("appstream://org.kde.kamera");
        if (KIO::DesktopExecParser::hasSchemeHandler(kameraInstallUrl)) {
            QDesktopServices::openUrl(kameraInstallUrl);
        } else {
            KMessageBox::error(d->widget, xi18nc("@info when failing to open the appstream URL", "Opening Discover failed.<nl/>Please check if Discover is installed on your system, or use your system's package manager to install \"Kamera\" package."));
        }
    } else if (!QProcess::startDetached(QStringLiteral("plasma-discover"), QStringList({"--search", scheme}))) {
        // For other protocols, search for the protocol name in Discover.
        KMessageBox::error(d->widget, xi18nc("@info when failing to launch plasma-discover, %1 protocol name", "Opening Discover failed.<nl/>Please check if Discover is installed on your system, or use your system's package manager to install the protocol support library for \"%1\".", scheme));
    }
    // clang-format on
}

/**
 * This model allows only the url passed in the constructor to appear at the root
 * level. This makes it possible to select the url, but not its siblings.
 * It also provides custom role values for the root item.
 */
class OnlyBaseUrlProxyModel : public QSortFilterProxyModel
{
public:
    OnlyBaseUrlProxyModel(const QUrl &url, const QIcon &icon, const QString &name, QObject *parent)
        : QSortFilterProxyModel(parent)
        , mUrl(url)
        , mIcon(icon)
        , mName(name)
    {
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        if (sourceParent.isValid()) {
            return true;
        }
        QModelIndex index = sourceModel()->index(sourceRow, 0);
        KFileItem item = itemForIndex(index);
        return item.url().matches(mUrl, QUrl::StripTrailingSlash);
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
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
    auto dirModel = new KDirModel(this);
    dirModel->dirLister()->setDirOnlyMode(true);
    dirModel->dirLister()->openUrl(KIO::upUrl(d->mSrcBaseUrl));

    auto onlyBaseUrlModel = new OnlyBaseUrlProxyModel(d->mSrcBaseUrl, d->mSrcBaseIcon, d->mSrcBaseName, this);
    onlyBaseUrlModel->setSourceModel(dirModel);

    auto sortModel = new QSortFilterProxyModel(this);
    sortModel->setDynamicSortFilter(true);
    sortModel->setSourceModel(onlyBaseUrlModel);
    sortModel->sort(0);

    d->mSrcUrlModelProxyMapper = new KModelIndexProxyMapper(dirModel, sortModel, this);

    d->mSrcUrlTreeView->setModel(sortModel);
    for (int i = 1; i < dirModel->columnCount(); ++i) {
        d->mSrcUrlTreeView->hideColumn(i);
    }
    connect(d->mSrcUrlTreeView, &QAbstractItemView::activated, this, &ThumbnailPage::openUrlFromIndex);
    connect(d->mSrcUrlTreeView, &QAbstractItemView::clicked, this, &ThumbnailPage::openUrlFromIndex);

    dirModel->expandToUrl(d->mSrcUrl);
    connect(dirModel, &KDirModel::expand, this, &ThumbnailPage::slotSrcUrlModelExpand);
}

void ThumbnailPage::slotSrcUrlModelExpand(const QModelIndex &index)
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

void ThumbnailPage::openUrlFromIndex(const QModelIndex &index)
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
