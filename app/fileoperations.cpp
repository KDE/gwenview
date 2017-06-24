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
#include <QMenu>
#include <QDebug>
#include <QPushButton>
#include <QFileDialog>

// KDE
#include <QInputDialog>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KLocalizedString>
#include <KJobWidgets>

// Local
#include <lib/document/documentfactory.h>
#include <lib/thumbnailprovider/thumbnailprovider.h>
#include <lib/thumbnailprovider/thumbnailprovider.h>
#include <lib/contextmanager.h>
namespace Gwenview
{

namespace FileOperations
{

static void copyMoveOrLink(Operation operation, const QList<QUrl>& urlList, QWidget* parent, ContextManager* contextManager)
{
    Q_ASSERT(!urlList.isEmpty());

  
    
    QFileDialog dialog(parent->nativeParentWidget(), QString());
    dialog.setAcceptMode(QFileDialog::AcceptSave);

    switch (operation) {
    case COPY:
        dialog.setWindowTitle(i18nc("@title:window", "Copy To"));
        dialog.setLabelText(QFileDialog::DialogLabel::Accept, i18nc("@action:button", "Copy"));
        break;
    case MOVE:
        dialog.setWindowTitle(i18nc("@title:window", "Move To"));
        dialog.setLabelText(QFileDialog::DialogLabel::Accept, i18nc("@action:button", "Move"));
        break;
    case LINK:
        dialog.setWindowTitle(i18nc("@title:window", "Link To"));
        dialog.setLabelText(QFileDialog::DialogLabel::Accept, i18nc("@action:button", "Link"));
        break;
    default:
        Q_ASSERT(0);
    }

    if (urlList.count() == 1) {
        dialog.setFileMode(QFileDialog::AnyFile);
        dialog.selectUrl(urlList.first());
    } else {
        dialog.setFileMode(QFileDialog::Directory);
        dialog.setOption(QFileDialog::ShowDirsOnly, true);
    }

    dialog.setDirectoryUrl(contextManager->targetUrl().adjusted(QUrl::RemoveFilename));
    if (!dialog.exec()) {
        return;
    }

    QUrl destUrl = dialog.selectedUrls().first();
    contextManager->setTargetUrl(destUrl);
    
    KIO::CopyJob* job = 0;
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
    job->ui()->setAutoErrorHandlingEnabled(true);
}

static void delOrTrash(KIO::JobUiDelegate::DeletionType deletionType, const QList<QUrl>& urlList, QWidget* parent)
{
    Q_ASSERT(urlList.count() > 0);

    KIO::JobUiDelegate uiDelegate;
    uiDelegate.setWindow(parent);
    if (!uiDelegate.askDeleteConfirmation(urlList, deletionType, KIO::JobUiDelegate::DefaultConfirmation)) {
        return;
    }
    KIO::Job* job = 0;
    switch (deletionType) {
        case KIO::JobUiDelegate::Trash:
            job = KIO::trash(urlList);
            break;
        case KIO::JobUiDelegate::Delete:
            job = KIO::del(urlList);
            break;
        default: // e.g. EmptyTrash
            return;
    }
    Q_ASSERT(job);
    KJobWidgets::setWindow(job,parent);

    Q_FOREACH(const QUrl &url, urlList) {
        DocumentFactory::instance()->forget(url);
    }
}

void copyTo(const QList<QUrl>& urlList, QWidget* parent, ContextManager* contextManager)
{
    copyMoveOrLink(COPY, urlList, parent, contextManager);
}

void moveTo(const QList<QUrl>& urlList, QWidget* parent, ContextManager* contextManager)
{
    copyMoveOrLink(MOVE, urlList, parent, contextManager);
}

void linkTo(const QList<QUrl>& urlList, QWidget* parent, ContextManager* contextManager)
{
    copyMoveOrLink(LINK, urlList, parent, contextManager);
}

void trash(const QList<QUrl>& urlList, QWidget* parent)
{
    delOrTrash(KIO::JobUiDelegate::Trash, urlList, parent);
}

void del(const QList<QUrl>& urlList, QWidget* parent)
{
    delOrTrash(KIO::JobUiDelegate::Delete, urlList, parent);
}

void showMenuForDroppedUrls(QWidget* parent, const QList<QUrl>& urlList, const QUrl &destUrl)
{
    if (urlList.isEmpty()) {
        qWarning() << "urlList is empty!";
        return;
    }

    if (!destUrl.isValid()) {
        qWarning() << "destUrl is not valid!";
        return;
    }

    QMenu menu(parent);
    QAction* moveAction = menu.addAction(
                              QIcon::fromTheme("go-jump"),
                              i18n("Move Here"));
    QAction* copyAction = menu.addAction(
                              QIcon::fromTheme("edit-copy"),
                              i18n("Copy Here"));
    QAction* linkAction = menu.addAction(
                              QIcon::fromTheme("edit-link"),
                              i18n("Link Here"));
    menu.addSeparator();
    menu.addAction(
        QIcon::fromTheme("process-stop"),
        i18n("Cancel"));

    QAction* action = menu.exec(QCursor::pos());

    KIO::Job* job = 0;
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

void rename(const QUrl &oldUrl, QWidget* parent)
{
    QString name = QInputDialog::getText(parent, 
                       i18nc("@title:window", "Rename") /* caption */,
                       xi18n("Rename <filename>%1</filename> to:", oldUrl.fileName()) /* label */,
                       QLineEdit::Normal, oldUrl.fileName() /* value */
                   );
    if (name.isEmpty() || name == oldUrl.fileName()) {
        return;
    }

    QUrl newUrl = oldUrl;
    newUrl = newUrl.adjusted(QUrl::RemoveFilename);
    newUrl.setPath(newUrl.path() + name);
    KIO::SimpleJob* job = KIO::rename(oldUrl, newUrl, KIO::HideProgressInfo);
    KJobWidgets::setWindow(job, parent);
    if (!job->exec()) {
        job->ui()->showErrorMessage();
        return;
    }
    ThumbnailProvider::moveThumbnail(oldUrl, newUrl);
}

} // namespace

} // namespace
