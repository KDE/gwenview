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
#include <QStandardItemModel>

// KDE
#include <KFileDialog>
#include <KGlobalSettings>
#include <KImageIO>
#include <KIO/NetAccess>
#include <KLocale>
#include <KMessageBox>
#include <KStandardDirs>
#include <KUrl>

// Local
#include <lib/binder.h>
#include <lib/document/documentfactory.h>
#include <lib/document/documentjob.h>
#include <lib/document/savejob.h>
#include <lib/graphicshudbutton.h>
#include <lib/gwenviewconfig.h>
#include <lib/historymodel.h>
#include <lib/messagebubble.h>
#include <lib/mimetypeutils.h>
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
    HistoryModel* mRecentUrlsModel;
    QPalette mPalettes[4];

    bool showSaveAsDialog(const KUrl& url, KUrl* outUrl, QByteArray* format)
    {
        KFileDialog dialog(url, QString(), mMainWindow);
        dialog.setOperationMode(KFileDialog::Saving);
        dialog.setSelection(url.fileName());
        dialog.setMimeFilter(
            KImageIO::mimeTypes(KImageIO::Writing), // List
            MimeTypeUtils::urlMimeType(url)         // Default
        );

        // Show dialog
        do {
            if (!dialog.exec()) {
                return false;
            }

            const QString mimeType = dialog.currentMimeFilter();
            if (mimeType.isEmpty()) {
                KMessageBox::sorry(
                    mMainWindow,
                    i18nc("@info",
                          "No image format selected.")
                );
                continue;
            }

            const QStringList typeList = KImageIO::typeForMime(mimeType);
            if (typeList.count() > 0) {
                *format = typeList[0].toAscii();
                break;
            }
            KMessageBox::sorry(
                mMainWindow,
                i18nc("@info",
                      "Gwenview cannot save images as %1.", mimeType)
            );
        } while (true);

        *outUrl = dialog.selectedUrl();
        return true;
    }

    void setupPalettes()
    {
        QPalette pal;
        int value = GwenviewConfig::viewBackgroundValue();
        QColor fgColor = value > 128 ? Qt::black : Qt::white;

        // Normal
        mPalettes[GvCore::NormalPalette] = KGlobalSettings::createApplicationPalette();

        pal = mPalettes[GvCore::NormalPalette];
        pal.setColor(QPalette::Base, QColor::fromHsv(0, 0, value));
        pal.setColor(QPalette::Text, fgColor);
        mPalettes[GvCore::NormalViewPalette] = pal;

        // Fullscreen
        KSharedConfigPtr config;
        QString name = GwenviewConfig::fullScreenColorScheme();
        if (name.isEmpty()) {
            // Default color scheme
            QString path = KStandardDirs::locate("data", "gwenview/color-schemes/fullscreen.colors");
            config = KSharedConfig::openConfig(path);
        } else if (name.contains('/')) {
            // Full path to a .colors file
            config = KSharedConfig::openConfig(name);
        } else {
            // Standard KDE color scheme
            config = KSharedConfig::openConfig(QString("color-schemes/%1.colors").arg(name), KConfig::FullConfig, "data");
        }
        mPalettes[GvCore::FullScreenPalette] = KGlobalSettings::createApplicationPalette(config);

        pal = mPalettes[GvCore::FullScreenPalette];
        QString path = KStandardDirs::locate("data", "gwenview/images/background.png");
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
    d->mRecentUrlsModel = 0;

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
        d->mRecentFoldersModel = new HistoryModel(const_cast<GvCore*>(this), KStandardDirs::locateLocal("appdata", "recentfolders/"));
    }
    return d->mRecentFoldersModel;
}

QAbstractItemModel* GvCore::recentUrlsModel() const
{
    if (!d->mRecentUrlsModel) {
        d->mRecentUrlsModel = new HistoryModel(const_cast<GvCore*>(this), KStandardDirs::locateLocal("appdata", "recenturls/"));
    }
    return d->mRecentUrlsModel;
}

AbstractSemanticInfoBackEnd* GvCore::semanticInfoBackEnd() const
{
    return d->mDirModel->semanticInfoBackEnd();
}

void GvCore::addUrlToRecentFolders(const KUrl& url)
{
    if (!GwenviewConfig::historyEnabled()) {
        return;
    }
    recentFoldersModel();
    d->mRecentFoldersModel->addUrl(url);
}

void GvCore::addUrlToRecentUrls(const KUrl& url)
{
    if (!GwenviewConfig::historyEnabled()) {
        return;
    }
    recentUrlsModel();
    d->mRecentUrlsModel->addUrl(url);
}

void GvCore::saveAll()
{
    SaveAllHelper helper(d->mMainWindow);
    helper.save();
}

void GvCore::save(const KUrl& url)
{
    Document::Ptr doc = DocumentFactory::instance()->load(url);
    QByteArray format = doc->format();
    const QStringList availableTypes = KImageIO::types(KImageIO::Writing);
    if (availableTypes.contains(QString(format))) {
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

void GvCore::saveAs(const KUrl& url)
{
    QByteArray format;
    KUrl saveAsUrl;
    if (!d->showSaveAsDialog(url, &saveAsUrl, &format)) {
        return;
    }

    // Check for overwrite
    if (KIO::NetAccess::exists(saveAsUrl, KIO::NetAccess::DestinationSide, d->mMainWindow)) {
        int answer = KMessageBox::warningContinueCancel(
                         d->mMainWindow,
                         i18nc("@info",
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
        const QString name = saveAsUrl.fileName().isEmpty() ? saveAsUrl.pathOrUrl() : saveAsUrl.fileName();
        const QString msg = i18nc("@info", "<b>Saving <filename>%1</filename> failed:</b><br>%2",
                                  name, doc->errorString());
        KMessageBox::sorry(QApplication::activeWindow(), msg);
    } else {
        connect(job, SIGNAL(result(KJob*)), SLOT(slotSaveResult(KJob*)));
    }
}

static void applyTransform(const KUrl& url, Orientation orientation)
{
    TransformImageOperation* op = new TransformImageOperation(orientation);
    Document::Ptr doc = DocumentFactory::instance()->load(url);
    op->applyToDocument(doc);
}

void GvCore::slotSaveResult(KJob* _job)
{
    SaveJob* job = static_cast<SaveJob*>(_job);
    KUrl oldUrl = job->oldUrl();
    KUrl newUrl = job->newUrl();

    if (job->error()) {
        QString name = newUrl.fileName().isEmpty() ? newUrl.pathOrUrl() : newUrl.fileName();
        QString msg = i18nc("@info", "<b>Saving <filename>%1</filename> failed:</b><br>%2",
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
            MessageBubble* bubble = new MessageBubble();
            bubble->setText(i18n("You are now viewing the new document."));
            KGuiItem item = KStandardGuiItem::back();
            item.setText(i18n("Go back to the original"));
            GraphicsHudButton* button = bubble->addButton(item);

            BinderRef<MainWindow, KUrl>::bind(button, SIGNAL(clicked()), d->mMainWindow, &MainWindow::goToUrl, oldUrl);
            connect(button, SIGNAL(clicked()),
                bubble, SLOT(deleteLater()));

            page->showMessageWidget(bubble);
        }
    }
}

void GvCore::rotateLeft(const KUrl& url)
{
    applyTransform(url, ROT_270);
}

void GvCore::rotateRight(const KUrl& url)
{
    applyTransform(url, ROT_90);
}

void GvCore::setRating(const KUrl& url, int rating)
{
    QModelIndex index = d->mDirModel->indexForUrl(url);
    if (!index.isValid()) {
        kWarning() << "invalid index!";
        return;
    }
    d->mDirModel->setData(index, rating, SemanticInfoDirModel::RatingRole);
}

bool GvCore::ensureDocumentIsEditable(const KUrl& url)
{
    // FIXME: Replace with a CheckEditableJob?
    // This way we can factorize the error message
    Document::Ptr doc = DocumentFactory::instance()->load(url);
    doc->startLoadingFullImage();
    doc->waitUntilLoaded();
    if (doc->isEditable()) {
        return true;
    }

    KMessageBox::sorry(
        QApplication::activeWindow(),
        i18nc("@info", "Gwenview cannot edit this kind of image.")
    );
    return false;
}

static void clearModel(QAbstractItemModel* model)
{
    model->removeRows(0, model->rowCount());
}

void GvCore::slotConfigChanged()
{
    if (!GwenviewConfig::historyEnabled()) {
        clearModel(recentFoldersModel());
        clearModel(recentUrlsModel());
    }
    d->setupPalettes();
}

QPalette GvCore::palette(GvCore::PaletteType type) const
{
    return d->mPalettes[type];
}

} // namespace
