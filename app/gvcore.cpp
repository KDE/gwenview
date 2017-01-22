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
#include <QUrl>
#include <QImageWriter>
#include <QMimeDatabase>

// KDE
#include <QFileDialog>
#include <KColorScheme>
#include <KIO/StatJob>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageBox>

// Local
#include <lib/binder.h>
#include <lib/document/documentfactory.h>
#include <lib/document/documentjob.h>
#include <lib/document/savejob.h>
#include <lib/hud/hudbutton.h>
#include <lib/gwenviewconfig.h>
#include <lib/historymodel.h>
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

struct GvCorePrivate
{
    GvCore* q;
    MainWindow* mMainWindow;
    SortedDirModel* mDirModel;
    HistoryModel* mRecentFoldersModel;
    RecentFilesModel* mRecentFilesModel;
    QPalette mPalettes[4];

    bool showSaveAsDialog(const QUrl &url, QUrl* outUrl, QByteArray* format)
    {
        QFileDialog dialog(mMainWindow);
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        dialog.selectUrl(url);

        QStringList supportedMimetypes;
        for (const QByteArray &mimeName : QImageWriter::supportedMimeTypes()) {
            supportedMimetypes.append(QString::fromLocal8Bit(mimeName));
        }

        dialog.setMimeTypeFilters(supportedMimetypes);
        dialog.selectMimeTypeFilter(MimeTypeUtils::urlMimeType(url));

        // Show dialog
        do {
            if (!dialog.exec()) {
                return false;
            }

            QList<QUrl> files = dialog.selectedUrls();
            if (files.isEmpty()) {
                return false;
            }

            QString filename = files.first().fileName();

            const QStringList typeList = QMimeDatabase().mimeTypeForName(filename).suffixes();
            if (typeList.count() > 0) {
                *format = typeList.first().toLocal8Bit();
                break;
            }
            KMessageBox::sorry(
                mMainWindow,
                i18nc("@info",
                      "Gwenview cannot save images as %1.", QFileInfo(filename).suffix())
            );
        } while (true);

        *outUrl = dialog.selectedUrls().first();
        return true;
    }

    void setupPalettes()
    {
        QPalette pal;
        int value = GwenviewConfig::viewBackgroundValue();
        QColor fgColor = value > 128 ? Qt::black : Qt::white;

        // Normal
        KSharedConfigPtr config = KSharedConfig::openConfig();
        mPalettes[GvCore::NormalPalette] = KColorScheme::createApplicationPalette(config);

        pal = mPalettes[GvCore::NormalPalette];
        pal.setColor(QPalette::Base, QColor::fromHsv(0, 0, value));
        pal.setColor(QPalette::Text, fgColor);
        mPalettes[GvCore::NormalViewPalette] = pal;

        // Fullscreen
        QString name = GwenviewConfig::fullScreenColorScheme();
        if (name.isEmpty()) {
            // Default color scheme
            QString path = QStandardPaths::locate(QStandardPaths::AppDataLocation, "color-schemes/fullscreen.colors");
            config = KSharedConfig::openConfig(path);
        } else if (name.contains('/')) {
            // Full path to a .colors file
            config = KSharedConfig::openConfig(name);
        } else {
            // Standard KDE color scheme
            config = KSharedConfig::openConfig(QString("color-schemes/%1.colors").arg(name), KConfig::FullConfig, QStandardPaths::AppDataLocation);
        }
        mPalettes[GvCore::FullScreenPalette] = KColorScheme::createApplicationPalette(config);

        pal = mPalettes[GvCore::FullScreenPalette];
        QString path = QStandardPaths::locate(QStandardPaths::AppDataLocation, "images/background.png");
        QPixmap bgTexture(path);
        pal.setBrush(QPalette::Base, bgTexture);
        mPalettes[GvCore::FullScreenViewPalette] = pal;
    }
};

GvCore::GvCore(MainWindow* mainWindow, SortedDirModel* dirModel)
: QObject(mainWindow)
, d(new GvCorePrivate)
{
    d->q = this;
    d->mMainWindow = mainWindow;
    d->mDirModel = dirModel;
    d->mRecentFoldersModel = 0;
    d->mRecentFilesModel = 0;

    d->setupPalettes();

    connect(GwenviewConfig::self(), SIGNAL(configChanged()),
            SLOT(slotConfigChanged()));
}

GvCore::~GvCore()
{
    delete d;
}

QAbstractItemModel* GvCore::recentFoldersModel() const
{
    if (!d->mRecentFoldersModel) {
        d->mRecentFoldersModel = new HistoryModel(const_cast<GvCore*>(this), QStandardPaths::locate(QStandardPaths::AppLocalDataLocation, "recentfolders/"));
    }
    return d->mRecentFoldersModel;
}

QAbstractItemModel* GvCore::recentFilesModel() const
{
    if (!d->mRecentFilesModel) {
        d->mRecentFilesModel = new RecentFilesModel(const_cast<GvCore*>(this));
    }
    return d->mRecentFilesModel;
}

AbstractSemanticInfoBackEnd* GvCore::semanticInfoBackEnd() const
{
    return d->mDirModel->semanticInfoBackEnd();
}

SortedDirModel* GvCore::sortedDirModel() const
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
    if (url.path() != "") { // This check is a workaround for bug #312060
        url.setPath(url.path()+'/');
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
        DocumentJob* job = doc->save(url, format);
        connect(job, SIGNAL(result(KJob*)), SLOT(slotSaveResult(KJob*)));
    } else {
        // We don't know how to save in 'format', ask the user for a format we can
        // write to.
        KGuiItem saveUsingAnotherFormat = KStandardGuiItem::saveAs();
        saveUsingAnotherFormat.setText(i18n("Save using another format"));
        int result = KMessageBox::warningContinueCancel(
                         d->mMainWindow,
                         i18n("Gwenview cannot save images in '%1' format.", QString(format)),
                         QString() /* caption */,
                         saveUsingAnotherFormat
                     );
        if (result == KMessageBox::Continue) {
            saveAs(url);
        }
    }
}

void GvCore::saveAs(const QUrl &url)
{
    QByteArray format;
    QUrl saveAsUrl;
    if (!d->showSaveAsDialog(url, &saveAsUrl, &format)) {
        return;
    }

    // Check for overwrite
    KIO::StatJob *statJob = KIO::stat(saveAsUrl, KIO::StatJob::DestinationSide, 0);
    KJobWidgets::setWindow(statJob, d->mMainWindow);
    if (statJob->exec()) {
        int answer = KMessageBox::warningContinueCancel(
                         d->mMainWindow,
                         xi18nc("@info",
                                "A file named <filename>%1</filename> already exists.\n"
                                "Are you sure you want to overwrite it?",
                                saveAsUrl.fileName()),
                         QString(),
                         KStandardGuiItem::overwrite());
        if (answer == KMessageBox::Cancel) {
            return;
        }
    }

    // Start save
    Document::Ptr doc = DocumentFactory::instance()->load(url);
    KJob* job = doc->save(saveAsUrl, format.data());
    if (!job) {
        const QString name = saveAsUrl.fileName().isEmpty() ? saveAsUrl.toDisplayString() : saveAsUrl.fileName();
        const QString msg = xi18nc("@info", "<b>Saving <filename>%1</filename> failed:</b><br />%2",
                                   name, doc->errorString());
        KMessageBox::sorry(QApplication::activeWindow(), msg);
    } else {
        connect(job, SIGNAL(result(KJob*)), SLOT(slotSaveResult(KJob*)));
    }
}

static void applyTransform(const QUrl &url, Orientation orientation)
{
    TransformImageOperation* op = new TransformImageOperation(orientation);
    Document::Ptr doc = DocumentFactory::instance()->load(url);
    op->applyToDocument(doc);
}

void GvCore::slotSaveResult(KJob* _job)
{
    SaveJob* job = static_cast<SaveJob*>(_job);
    QUrl oldUrl = job->oldUrl();
    QUrl newUrl = job->newUrl();

    if (job->error()) {
        QString name = newUrl.fileName().isEmpty() ? newUrl.toDisplayString() : newUrl.fileName();
        QString msg = xi18nc("@info", "<b>Saving <filename>%1</filename> failed:</b><br />%2",
                             name, job->errorString());

        int result = KMessageBox::warningContinueCancel(
                         d->mMainWindow, msg,
                         QString() /* caption */,
                         KStandardGuiItem::saveAs());

        if (result == KMessageBox::Continue) {
            saveAs(newUrl);
        }
        return;
    }

    if (oldUrl != newUrl) {
        d->mMainWindow->goToUrl(newUrl);

        ViewMainPage* page = d->mMainWindow->viewMainPage();
        if (page->isVisible()) {
            HudMessageBubble* bubble = new HudMessageBubble();
            bubble->setText(i18n("You are now viewing the new document."));
            KGuiItem item = KStandardGuiItem::back();
            item.setText(i18n("Go back to the original"));
            HudButton* button = bubble->addButton(item);

            BinderRef<MainWindow, QUrl>::bind(button, SIGNAL(clicked()), d->mMainWindow, &MainWindow::goToUrl, oldUrl);
            connect(button, SIGNAL(clicked()),
                bubble, SLOT(deleteLater()));

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
        qWarning() << "invalid index!";
        return;
    }
    d->mDirModel->setData(index, rating, SemanticInfoDirModel::RatingRole);
}

static void clearModel(QAbstractItemModel* model)
{
    model->removeRows(0, model->rowCount());
}

void GvCore::slotConfigChanged()
{
    if (!GwenviewConfig::historyEnabled()) {
        clearModel(recentFoldersModel());
        clearModel(recentFilesModel());
    }
    d->setupPalettes();
}

QPalette GvCore::palette(GvCore::PaletteType type) const
{
    return d->mPalettes[type];
}

} // namespace
