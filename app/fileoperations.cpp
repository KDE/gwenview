// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
#include "fileoperations.h"

// Qt
#include <QFileDialog>
#include <QMenu>
#include <QPushButton>

// KF
#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/DeleteOrTrashJob>
#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KJobWidgets>
#include <KLocalizedString>

// Local
#include "gwenview_app_debug.h"
#include "renamedialog.h"
#include <lib/contextmanager.h>
#include <lib/document/documentfactory.h>
#include <lib/thumbnailprovider/thumbnailprovider.h>

namespace Gwenview
{
namespace FileOperations
{
static void copyMoveOrLink(Operation operation, const QList<QUrl> &urlList, QWidget *parent, ContextManager *contextManager)
{
    Q_ASSERT(!urlList.isEmpty());
    const int numberOfImages = urlList.count();

    auto dialog = new QFileDialog(parent->nativeParentWidget(), QString());
    dialog->setAcceptMode(QFileDialog::AcceptSave);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModal(true);

    // Figure out what the window title and buttons should say,
    // depending on the operation and how many images are selected
    switch (operation) {
    case COPY:
        if (numberOfImages == 1) {
            dialog->setWindowTitle(i18nc("@title:window %1 file name", "Copy %1", urlList.constFirst().fileName()));
        } else {
            dialog->setWindowTitle(i18ncp("@title:window %1 number of images", "Copy %1 image", "Copy %1 images", numberOfImages));
        }
        dialog->setLabelText(QFileDialog::DialogLabel::Accept, i18nc("@action:button", "Copy"));
        break;
    case MOVE:
        if (numberOfImages == 1) {
            dialog->setWindowTitle(i18nc("@title:window %1 file name", "Move %1", urlList.constFirst().fileName()));
        } else {
            dialog->setWindowTitle(i18ncp("@title:window %1 number of images", "Move %1 image", "Move %1 images", numberOfImages));
        }
        dialog->setLabelText(QFileDialog::DialogLabel::Accept, i18nc("@action:button", "Move"));
        break;
    case LINK:
        if (numberOfImages == 1) {
            dialog->setWindowTitle(i18nc("@title:window %1 file name", "Link %1", urlList.constFirst().fileName()));
        } else {
            dialog->setWindowTitle(i18ncp("@title:window %1 number of images", "Link %1 image", "Link %1 images", numberOfImages));
        }
        dialog->setLabelText(QFileDialog::DialogLabel::Accept, i18nc("@action:button", "Link"));
        break;
    default:
        Q_ASSERT(0);
    }

    if (numberOfImages == 1) {
        dialog->setFileMode(QFileDialog::AnyFile);
        dialog->selectUrl(urlList.constFirst());
    } else {
        dialog->setFileMode(QFileDialog::Directory);
        dialog->setOption(QFileDialog::ShowDirsOnly, true);
    }

    QUrl dirUrl = contextManager->targetDirUrl();
    if (!dirUrl.isValid()) {
        dirUrl = urlList.constFirst().adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash);
    }
    dialog->setDirectoryUrl(dirUrl);

    QObject::connect(dialog, &QDialog::accepted, parent, [=]() {
        QUrl destUrl = dialog->selectedUrls().at(0);

        KIO::CopyJob* job = nullptr;
        switch (operation) {
        case COPY:
            job = KIO::copy(urlList, destUrl);
            break;
        case MOVE:
            job = KIO::move(urlList, destUrl);
            break;
        case LINK:
            job = KIO::link(urlList, destUrl);
            break;
        default:
            Q_ASSERT(0);
        }
        KJobWidgets::setWindow(job, parent);
        job->uiDelegate()->setAutoErrorHandlingEnabled(true);

        if (numberOfImages == 1) {
            destUrl = destUrl.adjusted(QUrl::RemoveFilename|QUrl::StripTrailingSlash);
        }
        contextManager->setTargetDirUrl(destUrl);
    });

    dialog->show();
}

static void delOrTrash(KIO::JobUiDelegate::DeletionType deletionType, const QList<QUrl> &urlList, QWidget *parent)
{
    Q_ASSERT(urlList.count() > 0);

    KJob *job = nullptr;
    switch (deletionType) {
    case KIO::JobUiDelegate::Trash:
        job = new KIO::DeleteOrTrashJob(urlList, KIO::AskUserActionInterface::Trash, KIO::AskUserActionInterface::DefaultConfirmation, parent);
        break;
    case KIO::JobUiDelegate::Delete:
        job = new KIO::DeleteOrTrashJob(urlList, KIO::AskUserActionInterface::Delete, KIO::AskUserActionInterface::DefaultConfirmation, parent);
        break;
    default: // e.g. EmptyTrash
        return;
    }
    Q_ASSERT(job);
    KJobWidgets::setWindow(job, parent);
    job->start();

    for (const QUrl &url : urlList) {
        DocumentFactory::instance()->forget(url);
    }
}

void copyTo(const QList<QUrl> &urlList, QWidget *parent, ContextManager *contextManager)
{
    copyMoveOrLink(COPY, urlList, parent, contextManager);
}

void moveTo(const QList<QUrl> &urlList, QWidget *parent, ContextManager *contextManager)
{
    copyMoveOrLink(MOVE, urlList, parent, contextManager);
}

void linkTo(const QList<QUrl> &urlList, QWidget *parent, ContextManager *contextManager)
{
    copyMoveOrLink(LINK, urlList, parent, contextManager);
}

void trash(const QList<QUrl> &urlList, QWidget *parent)
{
    delOrTrash(KIO::JobUiDelegate::Trash, urlList, parent);
}

void del(const QList<QUrl> &urlList, QWidget *parent)
{
    delOrTrash(KIO::JobUiDelegate::Delete, urlList, parent);
}

void showMenuForDroppedUrls(QWidget *parent, const QList<QUrl> &urlList, const QUrl &destUrl)
{
    if (urlList.isEmpty()) {
        qCWarning(GWENVIEW_APP_LOG) << "urlList is empty!";
        return;
    }

    if (!destUrl.isValid()) {
        qCWarning(GWENVIEW_APP_LOG) << "destUrl is not valid!";
        return;
    }

    QMenu menu(parent);
    QAction *moveAction = menu.addAction(QIcon::fromTheme(QStringLiteral("edit-move"), QIcon::fromTheme(QStringLiteral("go-jump"))), i18n("Move Here"));
    QAction *copyAction = menu.addAction(QIcon::fromTheme(QStringLiteral("edit-copy")), i18n("Copy Here"));
    QAction *linkAction = menu.addAction(QIcon::fromTheme(QStringLiteral("edit-link")), i18n("Link Here"));
    menu.addSeparator();
    menu.addAction(QIcon::fromTheme(QStringLiteral("process-stop")), i18n("Cancel"));

    QAction *action = menu.exec(QCursor::pos());

    KIO::Job *job = nullptr;
    if (action == moveAction) {
        job = KIO::move(urlList, destUrl);
    } else if (action == copyAction) {
        job = KIO::copy(urlList, destUrl);
    } else if (action == linkAction) {
        job = KIO::link(urlList, destUrl);
    } else {
        return;
    }
    Q_ASSERT(job);
    KJobWidgets::setWindow(job, parent);
}

void rename(const QUrl &oldUrl, QWidget *parent, ContextManager *contextManager)
{
    auto dialog = new RenameDialog(parent);
    dialog->setFilename(oldUrl.fileName());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModal(true);

    QObject::connect(dialog, &QDialog::accepted, parent, [=]() {
        const QString name = dialog->filename();
        if (name.isEmpty() || name == oldUrl.fileName()) {
            return;
        }

        QUrl newUrl = oldUrl;
        newUrl = newUrl.adjusted(QUrl::RemoveFilename);
        newUrl.setPath(newUrl.path() + name);
        KIO::SimpleJob* job = KIO::rename(oldUrl, newUrl, KIO::HideProgressInfo);
        KJobWidgets::setWindow(job, parent);
        job->uiDelegate()->setAutoErrorHandlingEnabled(true);
        QObject::connect(job, &KJob::result, parent, [contextManager, job, oldUrl, newUrl]() {
            if (!job->error()) {
                contextManager->setCurrentUrl(newUrl);
                ThumbnailProvider::moveThumbnail(oldUrl, newUrl);
            }
        });
    });

    dialog->show();
}

} // namespace

} // namespace
