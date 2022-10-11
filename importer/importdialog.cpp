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
#include "importdialog.h"

// Qt
#include <QApplication>
#include <QDate>
#include <QStackedWidget>
#include <QStandardPaths>

// KF
#include <KIO/ApplicationLauncherJob>
#include <KIO/DeleteJob>
#include <KLocalizedString>
#include <KMessageBox>
#include <KProtocolInfo>
#include <KService>
#include <KStandardGuiItem>
#include <Solid/Device>
#include <kio_version.h>
#if KIO_VERSION >= QT_VERSION_CHECK(5, 98, 0)
#include <KIO/JobUiDelegateFactory>
#else
#include <KIO/JobUiDelegate>
#endif

// Local
#include "dialogpage.h"
#include "gwenview_importer_debug.h"
#include "importer.h"
#include "importerconfig.h"
#include "progresspage.h"
#include "thumbnailpage.h"

namespace Gwenview
{
class ImportDialogPrivate
{
public:
    ImportDialog *q = nullptr;
    QStackedWidget *mCentralWidget = nullptr;
    ThumbnailPage *mThumbnailPage = nullptr;
    ProgressPage *mProgressPage = nullptr;
    DialogPage *mDialogPage = nullptr;
    Importer *mImporter = nullptr;

    void checkForFailedUrls()
    {
        // First check for errors on file imports or subfolder creation
        const QList<QUrl> failedUrls = mImporter->failedUrlList();
        const QList<QUrl> failedSubFolders = mImporter->failedSubFolderList();
        const int failedUrlCount = failedUrls.count();
        const int failedSubFolderCount = failedSubFolders.count();
        if (failedUrlCount + failedSubFolderCount > 0) {
            QStringList files, dirs;
            for (int i = 0; i < failedUrlCount; i++) {
                files << failedUrls[i].toString(QUrl::PreferLocalFile);
            }
            for (int i = 0; i < failedSubFolderCount; i++) {
                dirs << failedSubFolders[i].toString(QUrl::PreferLocalFile);
            }
            Q_EMIT q->showErrors(files, dirs);
        }
    }

    void deleteImportedUrls()
    {
        const QList<QUrl> importedUrls = mImporter->importedUrlList();
        const QList<QUrl> skippedUrls = mImporter->skippedUrlList();
        const int importedCount = importedUrls.count();
        const int skippedCount = skippedUrls.count();

        if (importedCount == 0 && skippedCount == 0) {
            return;
        }

        QStringList message;
        message << i18np("One document has been imported.", "%1 documents have been imported.", importedCount);
        if (skippedCount > 0) {
            message << i18np("One document has been skipped because it had already been imported.",
                             "%1 documents have been skipped because they had already been imported.",
                             skippedCount);
        }

        if (mImporter->renamedCount() > 0) {
            message[0].append(QStringLiteral("*"));
            message << QStringLiteral("<small>* ")
                    + i18np("One of them has been renamed because another document with the same name had already been imported.",
                            "%1 of them have been renamed because other documents with the same name had already been imported.",
                            mImporter->renamedCount())
                    + "</small>";
        }

        message << QString();
        if (skippedCount == 0) {
            message << i18np("Delete the imported document from the device?", "Delete the %1 imported documents from the device?", importedCount);
        } else if (importedCount == 0) {
            message << i18np("Delete the skipped document from the device?", "Delete the %1 skipped documents from the device?", skippedCount);
        } else {
            message << i18ncp("Singular sentence is actually never used.",
                              "Delete the imported or skipped document from the device?",
                              "Delete the %1 imported and skipped documents from the device?",
                              importedCount + skippedCount);
        }

        int answer = KMessageBox::questionYesNo(mCentralWidget,
                                                QStringLiteral("<qt>") + message.join(QStringLiteral("<br/>")) + QStringLiteral("</qt>"),
                                                i18nc("@title:window", "Import Finished"),
                                                KGuiItem(i18n("Keep")),
                                                KStandardGuiItem::del());
        if (answer == KMessageBox::Yes) {
            return;
        }
        QList<QUrl> urls = importedUrls + skippedUrls;
        while (true) {
            KIO::Job *job = KIO::del(urls);
            if (job->exec()) {
                break;
            }
            // Deleting failed
            int answer =
                KMessageBox::warningYesNo(mCentralWidget,
                                          i18np("Failed to delete the document:\n%2", "Failed to delete documents:\n%2", urls.count(), job->errorString()),
                                          QString(),
                                          KGuiItem(i18n("Retry")),
                                          KGuiItem(i18n("Ignore")));
            if (answer != KMessageBox::Yes) {
                // Ignore
                break;
            }
        }
    }

    void startGwenview()
    {
        KService::Ptr service = KService::serviceByDesktopName(QStringLiteral("org.kde.gwenview"));
        if (!service) {
            qCCritical(GWENVIEW_IMPORTER_LOG) << "Could not find gwenview";
        } else {
            auto job = new KIO::ApplicationLauncherJob(service);
            job->setUrls({mThumbnailPage->destinationUrl()});
#if KIO_VERSION >= QT_VERSION_CHECK(5, 98, 0)
            job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));
#else
            job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, nullptr));
#endif
            job->start();
        }
    }

    void showWhatNext()
    {
        mCentralWidget->setCurrentWidget(mDialogPage);
        mDialogPage->setText(i18n("What do you want to do now?"));
        mDialogPage->removeButtons();
        int gwenview = mDialogPage->addButton(KGuiItem(i18n("View Imported Documents with Gwenview"), QStringLiteral("gwenview")));
        int importMore = mDialogPage->addButton(KGuiItem(i18n("Import more Documents")));
        mDialogPage->addButton(KGuiItem(i18n("Quit"), QStringLiteral("dialog-cancel")));

        int answer = mDialogPage->exec();
        if (answer == gwenview) {
            startGwenview();
            qApp->quit();
        } else if (answer == importMore) {
            mCentralWidget->setCurrentWidget(mThumbnailPage);
        } else { /* quit */
            qApp->quit();
        }
    }
};

ImportDialog::ImportDialog()
    : d(new ImportDialogPrivate)
{
    d->q = this;
    d->mImporter = new Importer(this);
    connect(d->mImporter, &Importer::error, this, &ImportDialog::showImportError);
    d->mThumbnailPage = new ThumbnailPage;

    QUrl url = ImporterConfig::destinationUrl();
    if (!url.isValid()) {
        url = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
        const int year = QDate::currentDate().year();
        url.setPath(url.path() + '/' + QString::number(year));
    }
    d->mThumbnailPage->setDestinationUrl(url);

    d->mProgressPage = new ProgressPage(d->mImporter);

    d->mDialogPage = new DialogPage;

    d->mCentralWidget = new QStackedWidget;
    setCentralWidget(d->mCentralWidget);
    d->mCentralWidget->addWidget(d->mThumbnailPage);
    d->mCentralWidget->addWidget(d->mProgressPage);
    d->mCentralWidget->addWidget(d->mDialogPage);

    connect(d->mThumbnailPage, &ThumbnailPage::importRequested, this, &ImportDialog::startImport);
    connect(d->mThumbnailPage, &ThumbnailPage::rejected, this, &QWidget::close);
    connect(d->mImporter, &Importer::importFinished, this, &ImportDialog::slotImportFinished);
    connect(this, &ImportDialog::showErrors, d->mDialogPage, &DialogPage::slotShowErrors);

    d->mCentralWidget->setCurrentWidget(d->mThumbnailPage);

    setWindowIcon(QIcon::fromTheme(QStringLiteral("gwenview")));
    setAutoSaveSettings();
}

ImportDialog::~ImportDialog()
{
    delete d;
}

QSize ImportDialog::sizeHint() const
{
    return QSize(700, 500);
}

void ImportDialog::setSourceUrl(const QUrl &url, const QString &deviceUdi)
{
    QString name, iconName;
    if (deviceUdi.isEmpty()) {
        name = url.url(QUrl::PreferLocalFile);
        iconName = KProtocolInfo::icon(url.scheme());
        if (iconName.isEmpty()) {
            iconName = QStringLiteral("folder");
        }
    } else {
        Solid::Device device(deviceUdi);
        name = device.vendor() + QLatin1Char(' ') + device.product();
        iconName = device.icon();
    }
    d->mThumbnailPage->setSourceUrl(url, iconName, name);
}

void ImportDialog::startImport()
{
    const QUrl url = d->mThumbnailPage->destinationUrl();
    ImporterConfig::setDestinationUrl(url);
    ImporterConfig::self()->save();

    d->mCentralWidget->setCurrentWidget(d->mProgressPage);
    d->mImporter->setAutoRenameFormat(ImporterConfig::autoRename() ? ImporterConfig::autoRenameFormat() : QString());
    d->mImporter->start(d->mThumbnailPage->urlList(), url);
}

void ImportDialog::slotImportFinished()
{
    d->checkForFailedUrls();
    d->deleteImportedUrls();
    d->showWhatNext();
}

void ImportDialog::showImportError(const QString &message)
{
    KMessageBox::error(this, message);
    d->mCentralWidget->setCurrentWidget(d->mThumbnailPage);
}

} // namespace
