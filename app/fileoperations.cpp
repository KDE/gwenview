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

// KDE
#include <KDebug>
#include <KFileDialog>
#include <KFileItem>
#include <KMenu>
#include <KInputDialog>
#include <KPushButton>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KIO/NetAccess>
#include <KLocale>
#include <KJobWidgets>
#include <konq_operations.h>

// Local
#include <lib/document/documentfactory.h>
#include <lib/thumbnailprovider/thumbnailprovider.h>
#include <KJobWidgets/KJobWidgets>

namespace Gwenview
{

namespace FileOperations
{

static void copyMoveOrLink(KonqOperations::Operation operation, const KUrl::List& urlList, QWidget* parent)
{
    Q_ASSERT(urlList.count() > 0);

    KFileDialog dialog(
        KUrl("kfiledialog:///<copyMoveOrLink>"),
        QString() /* filter */,
        parent);
    dialog.setOperationMode(KFileDialog::Saving);
    switch (operation) {
    case KonqOperations::COPY:
        dialog.setCaption(i18nc("@title:window", "Copy To"));
        dialog.okButton()->setText(i18nc("@action:button", "Copy"));
        break;
    case KonqOperations::MOVE:
        dialog.setCaption(i18nc("@title:window", "Move To"));
        dialog.okButton()->setText(i18nc("@action:button", "Move"));
        break;
    case KonqOperations::LINK:
        dialog.setCaption(i18nc("@title:window", "Link To"));
        dialog.okButton()->setText(i18nc("@action:button", "Link"));
        break;
    default:
        Q_ASSERT(0);
    }
    if (urlList.count() == 1) {
        dialog.setMode(KFile::File);
        dialog.setSelection(urlList[0].fileName());
    } else {
        dialog.setMode(KFile::ExistingOnly | KFile::Directory);
    }
    if (!dialog.exec()) {
        return;
    }

    KUrl destUrl = dialog.selectedUrl();
    KonqOperations::copy(parent, operation, urlList, destUrl);
}

static void delOrTrash(KonqOperations::Operation operation, const KUrl::List& urlList, QWidget* parent)
{
    Q_ASSERT(urlList.count() > 0);

    if (!KonqOperations::askDeleteConfirmation(urlList, operation, KonqOperations::DEFAULT_CONFIRMATION, parent)) {
        return;
    }

    KIO::Job* job = 0;
    // KonqOperations::delOrTrash() handles the confirmation and does not provide
    // a way to know if the deletion has been accepted.
    // We need to know about the confirmation so that DocumentFactory can forget
    // about the deleted urls. That's why we can't use KonqOperations::delOrTrash()
    switch (operation) {
    case KonqOperations::TRASH:
        job = KIO::trash(urlList);
        break;

    case KonqOperations::DEL:
        job = KIO::del(urlList);
        break;

    default:
        kWarning() << "Unknown operation" << operation;
        return;
    }
    Q_ASSERT(job);
    KJobWidgets::setWindow(job,parent);

    Q_FOREACH(const KUrl & url, urlList) {
        DocumentFactory::instance()->forget(url);
    }
}

void copyTo(const KUrl::List& urlList, QWidget* parent)
{
    copyMoveOrLink(KonqOperations::COPY, urlList, parent);
}

void moveTo(const KUrl::List& urlList, QWidget* parent)
{
    copyMoveOrLink(KonqOperations::MOVE, urlList, parent);
}

void linkTo(const KUrl::List& urlList, QWidget* parent)
{
    copyMoveOrLink(KonqOperations::LINK, urlList, parent);
}

void trash(const KUrl::List& urlList, QWidget* parent)
{
    delOrTrash(KonqOperations::TRASH, urlList, parent);
}

void del(const KUrl::List& urlList, QWidget* parent)
{
    delOrTrash(KonqOperations::DEL, urlList, parent);
}

void showMenuForDroppedUrls(QWidget* parent, const KUrl::List& urlList, const KUrl& destUrl)
{
    if (urlList.isEmpty()) {
        kWarning() << "urlList is empty!";
        return;
    }

    if (!destUrl.isValid()) {
        kWarning() << "destUrl is not valid!";
        return;
    }

    KMenu menu(parent);
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

void rename(const KUrl& oldUrl, QWidget* parent)
{
    QString name = KInputDialog::getText(
                       i18nc("@title:window", "Rename") /* caption */,
                       i18n("Rename <filename>%1</filename> to:", oldUrl.fileName()) /* label */,
                       oldUrl.fileName() /* value */,
                       0 /* ok */,
                       parent
                   );
    if (name.isEmpty() || name == oldUrl.fileName()) {
        return;
    }

    KUrl newUrl = oldUrl;
    newUrl.setFileName(name);
    KIO::SimpleJob* job = KIO::rename(oldUrl, newUrl, KIO::HideProgressInfo);
    if (!KIO::NetAccess::synchronousRun(job, parent)) {
        job->ui()->showErrorMessage();
        return;
    }
    ThumbnailProvider::moveThumbnail(oldUrl, newUrl);
}

} // namespace

} // namespace
