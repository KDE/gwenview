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
#include "gvcore.h"

// Qt
#include <QApplication>
#include <QHBoxLayout>
#include <QImageWriter>
#include <QLabel>
#include <QMimeDatabase>
#include <QSpacerItem>
#include <QSpinBox>
#include <QUrl>

// KF
#include <KColorScheme>
#include <KColorUtils>
#include <KFileCustomDialog>
#include <KFileWidget>
#include <KLocalizedString>
#include <KMessageBox>

// Local
#include "gwenview_app_debug.h"
#include <lib/backgroundcolorwidget/backgroundcolorwidget.h>
#include <lib/binder.h>
#include <lib/document/documentfactory.h>
#include <lib/document/documentjob.h>
#include <lib/document/savejob.h>
#include <lib/gwenviewconfig.h>
#include <lib/historymodel.h>
#include <lib/hud/hudbutton.h>
#include <lib/hud/hudmessagebubble.h>
#include <lib/mimetypeutils.h>
#include <lib/recentfilesmodel.h>
#include <lib/semanticinfo/semanticinfodirmodel.h>
#include <lib/semanticinfo/sorteddirmodel.h>
#include <lib/transformimageoperation.h>
#include <mainwindow.h>
#include <saveallhelper.h>
#include <viewmainpage.h>

namespace Gwenview
{
struct GvCorePrivate {
    GvCore *q;
    MainWindow *mMainWindow;
    SortedDirModel *mDirModel;
    HistoryModel *mRecentFoldersModel;
    RecentFilesModel *mRecentFilesModel;
    QPalette mPalettes[4];
    QString mFullScreenPaletteName;
    int configFileJPEGQualityValue = GwenviewConfig::jPEGQuality();

    KFileCustomDialog *createSaveAsDialog(const QUrl &url)
    {
        // Build the JPEG quality chooser custom widget
        auto *JPEGQualityChooserWidget = new QWidget;
        JPEGQualityChooserWidget->setVisible(false); // shown only for JPEGs

        auto *JPEGQualityChooserLabel = new QLabel;
        JPEGQualityChooserLabel->setText(i18n("Image quality:"));
        JPEGQualityChooserLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

        auto *JPEGQualityChooserSpinBox = new QSpinBox;
        JPEGQualityChooserSpinBox->setMinimum(1);
        JPEGQualityChooserSpinBox->setMaximum(100);
        JPEGQualityChooserSpinBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        JPEGQualityChooserSpinBox->setSuffix(i18nc("Spinbox suffix; percentage 1 - 100", "%"));
        configFileJPEGQualityValue = GwenviewConfig::jPEGQuality();
        JPEGQualityChooserSpinBox->setValue(configFileJPEGQualityValue);

        // Temporarily change JPEG quality value
        QObject::connect(JPEGQualityChooserSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), JPEGQualityChooserSpinBox, [=](int value) {
            GwenviewConfig::setJPEGQuality(value);
        });

        auto *horizontalSpacer = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Fixed);

        auto *JPEGQualityChooserLayout = new QHBoxLayout(JPEGQualityChooserWidget);
        JPEGQualityChooserLayout->setContentsMargins(0, 0, 0, 0);
        JPEGQualityChooserLayout->addWidget(JPEGQualityChooserLabel);
        JPEGQualityChooserLayout->addWidget(JPEGQualityChooserSpinBox);
        JPEGQualityChooserLayout->addItem(horizontalSpacer);

        // Set up the dialog
        auto *dialog = new KFileCustomDialog(mMainWindow);
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setModal(true);
        KFileWidget *fileWidget = dialog->fileWidget();
        dialog->setCustomWidget(JPEGQualityChooserWidget);
        fileWidget->setConfirmOverwrite(true);
        dialog->setOperationMode(KFileWidget::Saving);
        dialog->setWindowTitle(i18nc("@title:window", "Save Image"));
        // Temporary workaround for selectUrl() not setting the
        // initial directory to url (removed in D4193)
        dialog->setUrl(url.adjusted(QUrl::RemoveFilename));
        fileWidget->setSelectedUrl(url);

        QStringList supportedMimetypes;
        for (const QByteArray &mimeName : QImageWriter::supportedMimeTypes()) {
            supportedMimetypes.append(QString::fromLocal8Bit(mimeName));
        }

        fileWidget->setMimeFilter(supportedMimetypes, MimeTypeUtils::urlMimeType(url));

        // Only show the lossy image quality chooser when saving a lossy image
        QObject::connect(fileWidget, &KFileWidget::filterChanged, JPEGQualityChooserWidget, [=](const QString &filter) {
            JPEGQualityChooserWidget->setVisible(filter.contains(QLatin1String("jpeg")) || filter.contains(QLatin1String("jxl"))
                                                 || filter.contains(QLatin1String("webp")) || filter.contains(QLatin1String("avif"))
                                                 || filter.contains(QLatin1String("heif")) || filter.contains(QLatin1String("heic")));
        });

        return dialog;
    }

    void setupPalettes()
    {
        // Normal
        KSharedConfigPtr config = KSharedConfig::openConfig();
        mPalettes[GvCore::NormalPalette] = KColorScheme::createApplicationPalette(config);
        QPalette viewPalette = mPalettes[GvCore::NormalPalette];

        BackgroundColorWidget::ColorMode colorMode = GwenviewConfig::backgroundColorMode();
        bool usingLightTheme = BackgroundColorWidget::usingLightTheme();

        if ((usingLightTheme && colorMode == BackgroundColorWidget::Dark) || (!usingLightTheme && colorMode == BackgroundColorWidget::Light)) {
            viewPalette.setColor(QPalette::Base, mPalettes[GvCore::NormalPalette].color(QPalette::Text));
            viewPalette.setColor(QPalette::Text, mPalettes[GvCore::NormalPalette].color(QPalette::Base));
            viewPalette.setColor(QPalette::Window, mPalettes[GvCore::NormalPalette].color(QPalette::WindowText));
            viewPalette.setColor(QPalette::WindowText, mPalettes[GvCore::NormalPalette].color(QPalette::Window));
            viewPalette.setColor(QPalette::Button, mPalettes[GvCore::NormalPalette].color(QPalette::ButtonText));
            viewPalette.setColor(QPalette::ButtonText, mPalettes[GvCore::NormalPalette].color(QPalette::Button));
            viewPalette.setColor(QPalette::ToolTipBase, mPalettes[GvCore::NormalPalette].color(QPalette::ToolTipText));
            viewPalette.setColor(QPalette::ToolTipText, mPalettes[GvCore::NormalPalette].color(QPalette::ToolTipBase));
        } else if (colorMode == BackgroundColorWidget::Neutral) {
            QColor base = KColorUtils::mix(mPalettes[GvCore::NormalPalette].color(QPalette::Base), mPalettes[GvCore::NormalPalette].color(QPalette::Text), 0.5);
            QColor window =
                KColorUtils::mix(mPalettes[GvCore::NormalPalette].color(QPalette::Window), mPalettes[GvCore::NormalPalette].color(QPalette::WindowText), 0.5);
            QColor button =
                KColorUtils::mix(mPalettes[GvCore::NormalPalette].color(QPalette::Button), mPalettes[GvCore::NormalPalette].color(QPalette::ButtonText), 0.5);
            QColor toolTipBase = KColorUtils::mix(mPalettes[GvCore::NormalPalette].color(QPalette::ToolTipBase),
                                                  mPalettes[GvCore::NormalPalette].color(QPalette::ToolTipText),
                                                  0.5);
            viewPalette.setColor(QPalette::Base, base);
            viewPalette.setColor(QPalette::Text, base.lightnessF() > 0.5 ? Qt::black : Qt::white);
            viewPalette.setColor(QPalette::Window, window);
            viewPalette.setColor(QPalette::WindowText, base.lightnessF() > 0.5 ? Qt::black : Qt::white);
            viewPalette.setColor(QPalette::Button, button);
            viewPalette.setColor(QPalette::ButtonText, base.lightnessF() > 0.5 ? Qt::black : Qt::white);
            viewPalette.setColor(QPalette::ToolTipBase, toolTipBase);
            viewPalette.setColor(QPalette::ToolTipText, base.lightnessF() > 0.5 ? Qt::black : Qt::white);
        }

        mPalettes[GvCore::NormalViewPalette] = viewPalette;

        // Fullscreen
        QString name = GwenviewConfig::fullScreenColorScheme();
        if (name.isEmpty()) {
            // Default color scheme
            mFullScreenPaletteName = QStandardPaths::locate(QStandardPaths::AppDataLocation, "color-schemes/fullscreen.colors");
            config = KSharedConfig::openConfig(mFullScreenPaletteName);
        } else if (name.contains('/')) {
            // Full path to a .colors file
            mFullScreenPaletteName = name;
            config = KSharedConfig::openConfig(mFullScreenPaletteName);
        } else {
            // Standard KDE color scheme
            mFullScreenPaletteName = QStringLiteral("color-schemes/%1.colors").arg(name);
            config = KSharedConfig::openConfig(mFullScreenPaletteName, KConfig::FullConfig, QStandardPaths::AppDataLocation);
        }
        mPalettes[GvCore::FullScreenPalette] = KColorScheme::createApplicationPalette(config);

        // If we are using the default palette, adjust it to match the system color scheme
        if (name.isEmpty()) {
            adjustDefaultFullScreenPalette();
        }

        // FullScreenView has either a solid black color or a textured background
        viewPalette = mPalettes[GvCore::FullScreenPalette];
        QPixmap bgTexture(256, 256);
        if (Gwenview::GwenviewConfig::fullScreenBackground() == Gwenview::FullScreenBackground::Black) {
            bgTexture.fill(Qt::black);
        } else {
            QString path = QStandardPaths::locate(QStandardPaths::AppDataLocation, "images/background.png");
            bgTexture = path;
        }
        viewPalette.setBrush(QPalette::Base, bgTexture);
        mPalettes[GvCore::FullScreenViewPalette] = viewPalette;
    }

    void adjustDefaultFullScreenPalette()
    {
        // The Fullscreen palette by default does not use the system color scheme, and therefore uses an 'accent' color
        // of blue. So for every color group/role combination that uses the accent color, we use a muted version of the
        // Normal palette. We also use the normal HighlightedText color so it properly contrasts with Highlight.
        const QPalette normalPal = mPalettes[GvCore::NormalPalette];
        QPalette fullscreenPal = mPalettes[GvCore::FullScreenPalette];

        // Colors from the normal palette (source of the system theme's accent color)
        const QColor normalToolTipBase = normalPal.color(QPalette::Normal, QPalette::ToolTipBase);
        const QColor normalToolTipText = normalPal.color(QPalette::Normal, QPalette::ToolTipText);
        const QColor normalHighlight = normalPal.color(QPalette::Normal, QPalette::Highlight);
        const QColor normalHighlightedText = normalPal.color(QPalette::Normal, QPalette::HighlightedText);
        const QColor normalLink = normalPal.color(QPalette::Normal, QPalette::Link);
        const QColor normalActiveToolTipBase = normalPal.color(QPalette::Active, QPalette::ToolTipBase);
        const QColor normalActiveToolTipText = normalPal.color(QPalette::Active, QPalette::ToolTipText);
        const QColor normalActiveHighlight = normalPal.color(QPalette::Active, QPalette::Highlight);
        const QColor normalActiveHighlightedText = normalPal.color(QPalette::Active, QPalette::HighlightedText);
        const QColor normalActiveLink = normalPal.color(QPalette::Active, QPalette::Link);
        const QColor normalDisabledToolTipBase = normalPal.color(QPalette::Disabled, QPalette::ToolTipBase);
        const QColor normalDisabledToolTipText = normalPal.color(QPalette::Disabled, QPalette::ToolTipText);
        // Note: Disabled Highlight missing as they do not use the accent color
        const QColor normalDisabledLink = normalPal.color(QPalette::Disabled, QPalette::Link);
        const QColor normalInactiveToolTipBase = normalPal.color(QPalette::Inactive, QPalette::ToolTipBase);
        const QColor normalInactiveToolTipText = normalPal.color(QPalette::Inactive, QPalette::ToolTipText);
        const QColor normalInactiveHighlight = normalPal.color(QPalette::Inactive, QPalette::Highlight);
        const QColor normalInactiveHighlightedText = normalPal.color(QPalette::Inactive, QPalette::HighlightedText);
        const QColor normalInactiveLink = normalPal.color(QPalette::Inactive, QPalette::Link);

        // Colors of the fullscreen palette which we will be modifying
        QColor fullScreenToolTipBase = fullscreenPal.color(QPalette::Normal, QPalette::ToolTipBase);
        QColor fullScreenToolTipText = fullscreenPal.color(QPalette::Normal, QPalette::ToolTipText);
        QColor fullScreenHighlight = fullscreenPal.color(QPalette::Normal, QPalette::Highlight);
        QColor fullScreenLink = fullscreenPal.color(QPalette::Normal, QPalette::Link);
        QColor fullScreenActiveToolTipBase = fullscreenPal.color(QPalette::Active, QPalette::ToolTipBase);
        QColor fullScreenActiveToolTipText = fullscreenPal.color(QPalette::Active, QPalette::ToolTipText);
        QColor fullScreenActiveHighlight = fullscreenPal.color(QPalette::Active, QPalette::Highlight);
        QColor fullScreenActiveLink = fullscreenPal.color(QPalette::Active, QPalette::Link);
        QColor fullScreenDisabledToolTipBase = fullscreenPal.color(QPalette::Disabled, QPalette::ToolTipBase);
        QColor fullScreenDisabledToolTipText = fullscreenPal.color(QPalette::Disabled, QPalette::ToolTipText);
        QColor fullScreenDisabledLink = fullscreenPal.color(QPalette::Disabled, QPalette::Link);
        QColor fullScreenInactiveToolTipBase = fullscreenPal.color(QPalette::Inactive, QPalette::ToolTipBase);
        QColor fullScreenInactiveToolTipText = fullscreenPal.color(QPalette::Inactive, QPalette::ToolTipText);
        QColor fullScreenInactiveHighlight = fullscreenPal.color(QPalette::Inactive, QPalette::Highlight);
        QColor fullScreenInactiveLink = fullscreenPal.color(QPalette::Inactive, QPalette::Link);

        // Adjust the value of the normal color so it's not too dark/bright, and apply to the respective fullscreen color
        fullScreenToolTipBase.setHsv(normalToolTipBase.hue(), normalToolTipBase.saturation(), (127 + 2 * normalToolTipBase.value()) / 3);
        fullScreenToolTipText.setHsv(normalToolTipText.hue(), normalToolTipText.saturation(), (127 + 2 * normalToolTipText.value()) / 3);
        fullScreenHighlight.setHsv(normalHighlight.hue(), normalHighlight.saturation(), (127 + 2 * normalHighlight.value()) / 3);
        fullScreenLink.setHsv(normalLink.hue(), normalLink.saturation(), (127 + 2 * normalLink.value()) / 3);
        fullScreenActiveToolTipBase.setHsv(normalActiveToolTipBase.hue(),
                                           normalActiveToolTipBase.saturation(),
                                           (127 + 2 * normalActiveToolTipBase.value()) / 3);
        fullScreenActiveToolTipText.setHsv(normalActiveToolTipText.hue(),
                                           normalActiveToolTipText.saturation(),
                                           (127 + 2 * normalActiveToolTipText.value()) / 3);
        fullScreenActiveHighlight.setHsv(normalActiveHighlight.hue(), normalActiveHighlight.saturation(), (127 + 2 * normalActiveHighlight.value()) / 3);
        fullScreenActiveLink.setHsv(normalActiveLink.hue(), normalActiveLink.saturation(), (127 + 2 * normalActiveLink.value()) / 3);
        fullScreenDisabledToolTipBase.setHsv(normalDisabledToolTipBase.hue(),
                                             normalDisabledToolTipBase.saturation(),
                                             (127 + 2 * normalDisabledToolTipBase.value()) / 3);
        fullScreenDisabledToolTipText.setHsv(normalDisabledToolTipText.hue(),
                                             normalDisabledToolTipText.saturation(),
                                             (127 + 2 * normalDisabledToolTipText.value()) / 3);
        fullScreenDisabledLink.setHsv(normalDisabledLink.hue(), normalDisabledLink.saturation(), (127 + 2 * normalDisabledLink.value()) / 3);
        fullScreenInactiveToolTipBase.setHsv(normalInactiveToolTipBase.hue(),
                                             normalInactiveToolTipBase.saturation(),
                                             (127 + 2 * normalInactiveToolTipBase.value()) / 3);
        fullScreenInactiveToolTipText.setHsv(normalInactiveToolTipText.hue(),
                                             normalInactiveToolTipText.saturation(),
                                             (127 + 2 * normalInactiveToolTipText.value()) / 3);
        fullScreenInactiveHighlight.setHsv(normalInactiveHighlight.hue(),
                                           normalInactiveHighlight.saturation(),
                                           (127 + 2 * normalInactiveHighlight.value()) / 3);
        fullScreenInactiveLink.setHsv(normalInactiveLink.hue(), normalInactiveLink.saturation(), (127 + 2 * normalInactiveLink.value()) / 3);

        // Apply the modified colors to the fullscreen palette
        fullscreenPal.setColor(QPalette::Normal, QPalette::ToolTipBase, fullScreenToolTipBase);
        fullscreenPal.setColor(QPalette::Normal, QPalette::ToolTipText, fullScreenToolTipText);
        fullscreenPal.setColor(QPalette::Normal, QPalette::Highlight, fullScreenHighlight);
        fullscreenPal.setColor(QPalette::Normal, QPalette::Link, fullScreenLink);
        fullscreenPal.setColor(QPalette::Active, QPalette::ToolTipBase, fullScreenActiveToolTipBase);
        fullscreenPal.setColor(QPalette::Active, QPalette::ToolTipText, fullScreenActiveToolTipText);
        fullscreenPal.setColor(QPalette::Active, QPalette::Highlight, fullScreenActiveHighlight);
        fullscreenPal.setColor(QPalette::Active, QPalette::Link, fullScreenActiveLink);
        fullscreenPal.setColor(QPalette::Disabled, QPalette::ToolTipBase, fullScreenDisabledToolTipBase);
        fullscreenPal.setColor(QPalette::Disabled, QPalette::ToolTipText, fullScreenDisabledToolTipText);
        fullscreenPal.setColor(QPalette::Disabled, QPalette::Link, fullScreenDisabledLink);
        fullscreenPal.setColor(QPalette::Inactive, QPalette::ToolTipBase, fullScreenInactiveToolTipBase);
        fullscreenPal.setColor(QPalette::Inactive, QPalette::ToolTipText, fullScreenInactiveToolTipText);
        fullscreenPal.setColor(QPalette::Inactive, QPalette::Highlight, fullScreenInactiveHighlight);
        fullscreenPal.setColor(QPalette::Inactive, QPalette::Link, fullScreenInactiveLink);

        // Since we use an adjusted version of the normal highlight color, we need to use the normal version of the
        // text color so it contrasts
        fullscreenPal.setColor(QPalette::Normal, QPalette::HighlightedText, normalHighlightedText);
        fullscreenPal.setColor(QPalette::Active, QPalette::HighlightedText, normalActiveHighlightedText);
        fullscreenPal.setColor(QPalette::Inactive, QPalette::HighlightedText, normalInactiveHighlightedText);

        mPalettes[GvCore::FullScreenPalette] = fullscreenPal;
    }
};

GvCore::GvCore(MainWindow *mainWindow, SortedDirModel *dirModel)
    : QObject(mainWindow)
    , d(new GvCorePrivate)
{
    d->q = this;
    d->mMainWindow = mainWindow;
    d->mDirModel = dirModel;
    d->mRecentFoldersModel = nullptr;
    d->mRecentFilesModel = nullptr;

    d->setupPalettes();

    connect(GwenviewConfig::self(), SIGNAL(configChanged()), SLOT(slotConfigChanged()));
    connect(qApp, &QApplication::paletteChanged, this, [this]() {
        d->setupPalettes();
    });
}

GvCore::~GvCore()
{
    delete d;
}

QAbstractItemModel *GvCore::recentFoldersModel() const
{
    if (!d->mRecentFoldersModel) {
        d->mRecentFoldersModel =
            new HistoryModel(const_cast<GvCore *>(this), QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/recentfolders/");
    }
    return d->mRecentFoldersModel;
}

QAbstractItemModel *GvCore::recentFilesModel() const
{
    if (!d->mRecentFilesModel) {
        d->mRecentFilesModel = new RecentFilesModel(const_cast<GvCore *>(this));
    }
    return d->mRecentFilesModel;
}

AbstractSemanticInfoBackEnd *GvCore::semanticInfoBackEnd() const
{
    return d->mDirModel->semanticInfoBackEnd();
}

SortedDirModel *GvCore::sortedDirModel() const
{
    return d->mDirModel;
}

void GvCore::addUrlToRecentFolders(QUrl url)
{
    if (!GwenviewConfig::historyEnabled()) {
        return;
    }
    if (!url.isValid()) {
        return;
    }

    // For "sftp://localhost", "/" is a different path than "" (bug #312060)
    if (!url.path().isEmpty() && !url.path().endsWith('/')) {
        url.setPath(url.path() + '/');
    }

    recentFoldersModel();
    d->mRecentFoldersModel->addUrl(url);
}

void GvCore::addUrlToRecentFiles(const QUrl &url)
{
    if (!GwenviewConfig::historyEnabled()) {
        return;
    }
    recentFilesModel();
    d->mRecentFilesModel->addUrl(url);
}

void GvCore::saveAll()
{
    SaveAllHelper helper(d->mMainWindow);
    helper.save();
}

void GvCore::save(const QUrl &url)
{
    Document::Ptr doc = DocumentFactory::instance()->load(url);
    QByteArray format = doc->format();
    const QByteArrayList availableTypes = QImageWriter::supportedImageFormats();
    if (availableTypes.contains(format)) {
        DocumentJob *job = doc->save(url, format);
        connect(job, SIGNAL(result(KJob *)), SLOT(slotSaveResult(KJob *)));
    } else {
        // We don't know how to save in 'format', ask the user for a format we can
        // write to.
        KGuiItem saveUsingAnotherFormat = KStandardGuiItem::saveAs();
        saveUsingAnotherFormat.setText(i18n("Save using another format"));
        int result = KMessageBox::warningContinueCancel(d->mMainWindow,
                                                        i18n("Gwenview cannot save images in '%1' format.", QString(format)),
                                                        QString() /* caption */,
                                                        saveUsingAnotherFormat);
        if (result == KMessageBox::Continue) {
            saveAs(url);
        }
    }
}

void GvCore::saveAs(const QUrl &url)
{
    KFileCustomDialog *dialog = d->createSaveAsDialog(url);

    connect(dialog, &QDialog::accepted, this, [=]() {
        KFileWidget *fileWidget = dialog->fileWidget();

        const QList<QUrl> files = fileWidget->selectedUrls();
        if (files.isEmpty()) {
            return;
        }

        const QString filename = files.at(0).fileName();

        const QMimeType mimeType = QMimeDatabase().mimeTypeForFile(filename, QMimeDatabase::MatchExtension);
        QByteArray format;
        if (mimeType.isValid()) {
            format = mimeType.preferredSuffix().toLocal8Bit();
        } else {
            KMessageBox::sorry(d->mMainWindow, i18nc("@info", "Gwenview cannot save images as %1.", QFileInfo(filename).suffix()));
        }

        QUrl saveAsUrl = fileWidget->selectedUrls().constFirst();

        if (format == "jpg") {
            // Gwenview code assumes JPEG images have "jpeg" format, so if the
            // dialog returned the format "jpg", use "jpeg" instead
            // This does not affect the actual filename extension
            format = "jpeg";
        }

        // Start save
        Document::Ptr doc = DocumentFactory::instance()->load(url);
        KJob *job = doc->save(saveAsUrl, format);
        if (!job) {
            const QString saveName = saveAsUrl.fileName();
            const QString name = !saveName.isEmpty() ? saveName : saveAsUrl.toDisplayString();
            const QString msg = xi18nc("@info", "<emphasis strong='true'>Saving <filename>%1</filename> failed:</emphasis><nl />%2", name, doc->errorString());
            KMessageBox::sorry(QApplication::activeWindow(), msg);
        } else {
            // Regardless of job result, reset JPEG config value if it was changed by
            // the Save As dialog
            connect(job, &KJob::result, this, [=]() {
                if (GwenviewConfig::jPEGQuality() != d->configFileJPEGQualityValue) {
                    GwenviewConfig::setJPEGQuality(d->configFileJPEGQualityValue);
                }
            });

            connect(job, &KJob::result, this, &GvCore::slotSaveResult);
        }
    });

    dialog->show();
}

static void applyTransform(const QUrl &url, Orientation orientation)
{
    auto *op = new TransformImageOperation(orientation);
    Document::Ptr doc = DocumentFactory::instance()->load(url);
    op->applyToDocument(doc);
}

void GvCore::slotSaveResult(KJob *_job)
{
    auto *job = static_cast<SaveJob *>(_job);
    QUrl oldUrl = job->oldUrl();
    QUrl newUrl = job->newUrl();

    if (job->error()) {
        QString name = newUrl.fileName().isEmpty() ? newUrl.toDisplayString() : newUrl.fileName();
        const QString msg =
            xi18nc("@info", "<emphasis strong='true'>Saving <filename>%1</filename> failed:</emphasis><nl />%2", name, kxi18n(qPrintable(job->errorString())));

        int result = KMessageBox::warningContinueCancel(d->mMainWindow, msg, QString() /* caption */, KStandardGuiItem::saveAs());

        if (result == KMessageBox::Continue) {
            saveAs(oldUrl);
        }
        return;
    }

    if (oldUrl != newUrl) {
        d->mMainWindow->goToUrl(newUrl);

        ViewMainPage *page = d->mMainWindow->viewMainPage();
        if (page->isVisible()) {
            auto *bubble = new HudMessageBubble();
            bubble->setText(i18n("You are now viewing the new document."));
            KGuiItem item = KStandardGuiItem::back();
            item.setText(i18n("Go back to the original"));
            HudButton *button = bubble->addButton(item);

            BinderRef<MainWindow, QUrl>::bind(button, SIGNAL(clicked()), d->mMainWindow, &MainWindow::goToUrl, oldUrl);
            connect(button, SIGNAL(clicked()), bubble, SLOT(deleteLater()));

            page->showMessageWidget(bubble);
        }
    }
}

void GvCore::rotateLeft(const QUrl &url)
{
    applyTransform(url, ROT_270);
}

void GvCore::rotateRight(const QUrl &url)
{
    applyTransform(url, ROT_90);
}

void GvCore::setRating(const QUrl &url, int rating)
{
    QModelIndex index = d->mDirModel->indexForUrl(url);
    if (!index.isValid()) {
        qCWarning(GWENVIEW_APP_LOG) << "invalid index!";
        return;
    }
    d->mDirModel->setData(index, rating, SemanticInfoDirModel::RatingRole);
}

static void clearModel(QAbstractItemModel *model)
{
    model->removeRows(0, model->rowCount());
}

void GvCore::clearRecentFilesAndFolders()
{
    clearModel(recentFilesModel());
    clearModel(recentFoldersModel());
}

void GvCore::slotConfigChanged()
{
    if (!GwenviewConfig::historyEnabled()) {
        clearRecentFilesAndFolders();
    }
    d->setupPalettes();
}

QPalette GvCore::palette(GvCore::PaletteType type) const
{
    return d->mPalettes[type];
}

QString GvCore::fullScreenPaletteName() const
{
    return d->mFullScreenPaletteName;
}

} // namespace
