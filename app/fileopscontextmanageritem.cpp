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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "fileopscontextmanageritem.h"

// Qt
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QListView>
#include <QMenu>
#include <QShortcut>

// KF
#include <kio_version.h>
#include <KActionCollection>
#include <KActionCategory>
#include <KFileItem>
#include <KFileItemActions>
#include <KIO/Paste>
#include <KIO/PasteJob>
#include <KIO/RestoreJob>
#include <KIO/JobUiDelegate>
#include <KIOCore/KFileItemListProperties>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KOpenWithDialog>
#include <KPropertiesDialog>
#include <KXMLGUIClient>
#include <KUrlMimeData>
#include <KFileItemListProperties>
#include <KIO/OpenFileManagerWindowJob>
#if KIO_VERSION >= QT_VERSION_CHECK(5, 71, 0)
#include <KIO/ApplicationLauncherJob>
#include <KIO/JobUiDelegate>
#else
#include <KRun>
#endif

// Local
#include "dialogguard.h"
#include <lib/contextmanager.h>
#include <lib/eventwatcher.h>
#include <lib/gvdebug.h>
#include <lib/mimetypeutils.h>
#include "fileoperations.h"
#include "sidebar.h"

namespace Gwenview
{


QList<QUrl> FileOpsContextManagerItem::urlList() const
{
    return contextManager()->selectedFileItemList().urlList();
}

void FileOpsContextManagerItem::updateServiceList()
{
    // This code is inspired from
    // kdebase/apps/lib/konq/konq_menuactions.cpp

    // Get list of all distinct mimetypes in selection
    QStringList mimeTypes;
    const auto selectedFileItemList = contextManager()->selectedFileItemList();
    for (const KFileItem & item : selectedFileItemList) {
        const QString mimeType = item.mimetype();
        if (!mimeTypes.contains(mimeType)) {
            mimeTypes << mimeType;
        }
    }

    // Query trader
    mServiceList = KFileItemActions::associatedApplications(mimeTypes, QString());
}

QMimeData* FileOpsContextManagerItem::selectionMimeData()
{
    KFileItemList selectedFiles;

    // In Compare mode, restrict the returned mimedata to the focused image
    if (!mThumbnailView->isVisible()) {
        selectedFiles << KFileItem(contextManager()->currentUrl());
    } else {
        selectedFiles = contextManager()->selectedFileItemList();
    }

    return MimeTypeUtils::selectionMimeData(selectedFiles, MimeTypeUtils::ClipboardTarget);
}

QUrl FileOpsContextManagerItem::pasteTargetUrl() const
{
    // If only one folder is selected, paste inside it, otherwise paste in
    // current
    KFileItemList list = contextManager()->selectedFileItemList();
    if (list.count() == 1 && list.first().isDir()) {
        return list.first().url();
    } else {
        return contextManager()->currentDirUrl();
    }
}

static QAction* createSeparator(QObject* parent)
{
    QAction* action = new QAction(parent);
    action->setSeparator(true);
    return action;
}

FileOpsContextManagerItem::FileOpsContextManagerItem(ContextManager* manager, QListView* thumbnailView, KActionCollection* actionCollection, KXMLGUIClient* client)
: AbstractContextManagerItem(manager)
{
    mThumbnailView = thumbnailView;
    mXMLGUIClient = client;
    mGroup = new SideBarGroup(i18n("File Operations"));
    setWidget(mGroup);
    EventWatcher::install(mGroup, QEvent::Show, this, SLOT(updateSideBarContent()));

    mInTrash = false;
    mNewFileMenu = new KNewFileMenu(Q_NULLPTR, QString(), this);

    connect(contextManager(), &ContextManager::selectionChanged,
            this, &FileOpsContextManagerItem::updateActions);
    connect(contextManager(), &ContextManager::currentDirUrlChanged,
            this, &FileOpsContextManagerItem::updateActions);

    KActionCategory* file = new KActionCategory(i18nc("@title actions category", "File"), actionCollection);
    KActionCategory* edit = new KActionCategory(i18nc("@title actions category", "Edit"), actionCollection);

    mCutAction = edit->addAction(KStandardAction::Cut, this, SLOT(cut()));
    mCopyAction = edit->addAction(KStandardAction::Copy, this, SLOT(copy()));
    mPasteAction = edit->addAction(KStandardAction::Paste, this, SLOT(paste()));

    mCopyToAction = file->addAction(QStringLiteral("file_copy_to"), this, SLOT(copyTo()));
    mCopyToAction->setText(i18nc("Verb", "Copy To..."));
    mCopyToAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
    actionCollection->setDefaultShortcut(mCopyToAction, Qt::Key_F7);

    mMoveToAction = file->addAction(QStringLiteral("file_move_to"), this, SLOT(moveTo()));
    mMoveToAction->setText(i18nc("Verb", "Move To..."));
    mMoveToAction->setIcon(QIcon::fromTheme(QStringLiteral("go-jump")));
    actionCollection->setDefaultShortcut(mMoveToAction, Qt::Key_F8);

    mLinkToAction = file->addAction(QStringLiteral("file_link_to"), this, SLOT(linkTo()));
    mLinkToAction->setText(i18nc("Verb: create link to the file where user wants", "Link To..."));
    mLinkToAction->setIcon(QIcon::fromTheme(QStringLiteral("link")));
    actionCollection->setDefaultShortcut(mLinkToAction, Qt::Key_F9);

    mRenameAction = file->addAction(QStringLiteral("file_rename"), this, SLOT(rename()));
    mRenameAction->setText(i18nc("Verb", "Rename..."));
    mRenameAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-rename")));
    actionCollection->setDefaultShortcut(mRenameAction, Qt::Key_F2);

    mTrashAction = file->addAction(QStringLiteral("file_trash"), this, SLOT(trash()));
    mTrashAction->setText(i18nc("Verb", "Trash"));
    mTrashAction->setIcon(QIcon::fromTheme(QStringLiteral("user-trash")));
    actionCollection->setDefaultShortcut(mTrashAction, Qt::Key_Delete);

    mDelAction = file->addAction(KStandardAction::DeleteFile, this, SLOT(del()));

    mRestoreAction = file->addAction(QStringLiteral("file_restore"), this, SLOT(restore()));
    mRestoreAction->setText(i18n("Restore"));
    mRestoreAction->setIcon(QIcon::fromTheme(QStringLiteral("restoration")));

    mShowPropertiesAction = file->addAction(QStringLiteral("file_show_properties"), this, SLOT(showProperties()));
    mShowPropertiesAction->setText(i18n("Properties"));
    mShowPropertiesAction->setIcon(QIcon::fromTheme(QStringLiteral("document-properties")));
    actionCollection->setDefaultShortcut(mShowPropertiesAction, QKeySequence(Qt::ALT | Qt::Key_Return));

    mCreateFolderAction = file->addAction(QStringLiteral("file_create_folder"), this, SLOT(createFolder()));
    mCreateFolderAction->setText(i18n("Create Folder..."));
    mCreateFolderAction->setIcon(QIcon::fromTheme(QStringLiteral("folder-new")));

    mOpenWithAction = file->addAction(QStringLiteral("file_open_with"));
    mOpenWithAction->setText(i18n("Open With"));
    QMenu* menu = new QMenu;
    mOpenWithAction->setMenu(menu);
    connect(menu, &QMenu::aboutToShow, this, &FileOpsContextManagerItem::populateOpenMenu);
    connect(menu, &QMenu::triggered, this, &FileOpsContextManagerItem::openWith);

    mOpenContainingFolderAction = file->addAction(QStringLiteral("file_open_containing_folder"), this, SLOT(openContainingFolder()));
    mOpenContainingFolderAction->setText(i18n("Open Containing Folder"));
    mOpenContainingFolderAction->setIcon(QIcon::fromTheme(QStringLiteral("document-open-folder")));

    mRegularFileActionList
            << mRenameAction
            << mTrashAction
            << mDelAction
            << createSeparator(this)
            << mCopyToAction
            << mMoveToAction
            << mLinkToAction
            << createSeparator(this)
            << mOpenWithAction
            << mOpenContainingFolderAction
            << mShowPropertiesAction
            << createSeparator(this)
            << mCreateFolderAction
            ;

    mTrashFileActionList
            << mRestoreAction
            << mDelAction
            << createSeparator(this)
            << mShowPropertiesAction
            ;

    connect(QApplication::clipboard(), &QClipboard::dataChanged,
            this, &FileOpsContextManagerItem::updatePasteAction);

    updatePasteAction();
    // Delay action update because it must happen *after* main window has called
    // createGUI(), otherwise calling mXMLGUIClient->plugActionList() will
    // fail.
    QMetaObject::invokeMethod(this, &FileOpsContextManagerItem::updateActions, Qt::QueuedConnection);
}

FileOpsContextManagerItem::~FileOpsContextManagerItem()
{
    delete mOpenWithAction->menu();
}

void FileOpsContextManagerItem::updateActions()
{
    const int count = contextManager()->selectedFileItemList().count();
    const bool selectionNotEmpty = count > 0;
    const bool urlIsValid = contextManager()->currentUrl().isValid();
    const bool dirUrlIsValid = contextManager()->currentDirUrl().isValid();

    mInTrash = contextManager()->currentDirUrl().scheme() == "trash";

    mCutAction->setEnabled(selectionNotEmpty);
    mCopyAction->setEnabled(selectionNotEmpty);
    mCopyToAction->setEnabled(selectionNotEmpty);
    mMoveToAction->setEnabled(selectionNotEmpty);
    mLinkToAction->setEnabled(selectionNotEmpty);
    mTrashAction->setEnabled(selectionNotEmpty);
    mRestoreAction->setEnabled(selectionNotEmpty);
    mDelAction->setEnabled(selectionNotEmpty);
    mOpenWithAction->setEnabled(selectionNotEmpty);
    mRenameAction->setEnabled(count == 1);
    mOpenContainingFolderAction->setEnabled(selectionNotEmpty);

    mCreateFolderAction->setEnabled(dirUrlIsValid);
    mShowPropertiesAction->setEnabled(dirUrlIsValid || urlIsValid);

    mXMLGUIClient->unplugActionList("file_action_list");
    QList<QAction*>& list = mInTrash ? mTrashFileActionList : mRegularFileActionList;
    mXMLGUIClient->plugActionList("file_action_list", list);

    updateSideBarContent();
}

void FileOpsContextManagerItem::updatePasteAction()
{
    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
    bool enable;
    KFileItem destItem(pasteTargetUrl());
    const QString text = KIO::pasteActionText(mimeData, &enable, destItem);
    mPasteAction->setEnabled(enable);
    mPasteAction->setText(text);
}

void FileOpsContextManagerItem::updateSideBarContent()
{
    if (!mGroup->isVisible()) {
        return;
    }

    mGroup->clear();
    QList<QAction*>& list = mInTrash ? mTrashFileActionList : mRegularFileActionList;
    for (QAction * action : qAsConst(list)) {
        if (action->isEnabled() && !action->isSeparator()) {
            mGroup->addAction(action);
        }
    }
}

void FileOpsContextManagerItem::showProperties()
{
    KFileItemList list = contextManager()->selectedFileItemList();
    if (list.count() > 0) {
        KPropertiesDialog::showDialog(list, mGroup);
    } else {
        QUrl url = contextManager()->currentDirUrl();
        KPropertiesDialog::showDialog(url, mGroup);
    }
}

void FileOpsContextManagerItem::cut()
{
    QMimeData* mimeData = selectionMimeData();
    KIO::setClipboardDataCut(mimeData, true);
    QApplication::clipboard()->setMimeData(mimeData);
}

void FileOpsContextManagerItem::copy()
{
    QMimeData* mimeData = selectionMimeData();
    KIO::setClipboardDataCut(mimeData, false);
    QApplication::clipboard()->setMimeData(mimeData);
}

void FileOpsContextManagerItem::paste()
{
    KIO::Job *job = KIO::paste(QApplication::clipboard()->mimeData(), pasteTargetUrl());
    KJobWidgets::setWindow(job, mGroup);
}

void FileOpsContextManagerItem::trash()
{
    FileOperations::trash(urlList(), mGroup);
}

void FileOpsContextManagerItem::del()
{
    FileOperations::del(urlList(), mGroup);
}

void FileOpsContextManagerItem::restore()
{
    KIO::RestoreJob *job = KIO::restoreFromTrash(urlList());
    KJobWidgets::setWindow(job, mGroup);
    job->uiDelegate()->setAutoErrorHandlingEnabled(true);
}

void FileOpsContextManagerItem::copyTo()
{
    FileOperations::copyTo(urlList(), widget(), contextManager());
}

void FileOpsContextManagerItem::moveTo()
{
    FileOperations::moveTo(urlList(), widget(), contextManager());
}

void FileOpsContextManagerItem::linkTo()
{
    FileOperations::linkTo(urlList(), widget(), contextManager());
}

void FileOpsContextManagerItem::rename()
{
    if (mThumbnailView->isVisible()) {
        QModelIndex index = mThumbnailView->currentIndex();
        mThumbnailView->edit(index);
    } else {
        FileOperations::rename(urlList().constFirst(), mGroup, contextManager());
        contextManager()->slotSelectionChanged();
    }
}

void FileOpsContextManagerItem::createFolder()
{
    QUrl url = contextManager()->currentDirUrl();
    mNewFileMenu->setParentWidget(mGroup);
    mNewFileMenu->setPopupFiles(QList<QUrl>() << url);
    mNewFileMenu->createDirectory();
}

void FileOpsContextManagerItem::populateOpenMenu()
{
    QMenu* openMenu = mOpenWithAction->menu();
    qDeleteAll(openMenu->actions());

    updateServiceList();

    int idx = 0;
    for (const KService::Ptr & service : qAsConst(mServiceList)) {
        QString text = service->name().replace('&', "&&");
        QAction* action = openMenu->addAction(text);
        action->setIcon(QIcon::fromTheme(service->icon()));
        action->setData(idx);
        ++idx;
    }

    openMenu->addSeparator();
    QAction* action = openMenu->addAction(i18n("Other Application..."));
    action->setData(-1);
}

void FileOpsContextManagerItem::openWith(QAction* action)
{
    Q_ASSERT(action);
    KService::Ptr service;
    QList<QUrl> list = urlList();

    bool ok;
    int idx = action->data().toInt(&ok);
    GV_RETURN_IF_FAIL(ok);
#if KIO_VERSION >= QT_VERSION_CHECK(5, 71, 0)
    if (idx != -1) {
        service = mServiceList.at(idx);
    }
    // If service is null, ApplicationLauncherJob will invoke the open-with dialog
    auto *job = new KIO::ApplicationLauncherJob(service);
    job->setUrls(list);
    job->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, mGroup));
    job->start();
#else
    if (idx == -1) {
        // Other Application...
        DialogGuard<KOpenWithDialog> dlg(list, mGroup);
        if (!dlg->exec()) {
            return;
        }
        service = dlg->service();

        if (!service) {
            // User entered a custom command
            Q_ASSERT(!dlg->text().isEmpty());
            KRun::run(dlg->text(), list, mGroup);
            return;
        }
    } else {
        service = mServiceList.at(idx);
    }

    Q_ASSERT(service);
    KRun::runService(*service, list, mGroup);
#endif
}

void FileOpsContextManagerItem::openContainingFolder()
{
    KIO::highlightInFileManager(urlList());
}

} // namespace
