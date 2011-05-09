// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
#include <QMimeData>

// KDE
#include <kaction.h>
#include <kactioncollection.h>
#include <kactioncategory.h>
#include <kdebug.h>
#include <kfileitem.h>
#include <kfileitemlistproperties.h>
#include <kio/paste.h>
#include <klocale.h>
#include <kmimetypetrader.h>
#include <kopenwithdialog.h>
#include <kpropertiesdialog.h>
#include <krun.h>
#include <kservice.h>

// libkonq
#include <konq_operations.h>

// Local
#include <lib/eventwatcher.h>
#include "contextmanager.h"
#include "fileoperations.h"
#include "sidebar.h"

namespace Gwenview {

// copied from libkonq (konqmimedata.cpp)
static const char* CUT_SELECTION_DATA = "application/x-kde-cutselection";

static void addCutSelection(QMimeData* mimeData) {
	mimeData->setData(CUT_SELECTION_DATA, "1");
}

static bool decodeIsCutSelection(const QMimeData* mimeData) {
	QByteArray data = mimeData->data(CUT_SELECTION_DATA);
	return data.isEmpty() ? false : data.at(0) == '1';
}

// copied from libkonq (konq_operations.cpp)
static QPair<bool, QString> pasteInfo(const KUrl& targetUrl) {
    QPair<bool, QString> ret;
    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();

    const bool canPasteData = KIO::canPasteMimeSource(mimeData);
    KUrl::List urls = KUrl::List::fromMimeData(mimeData);
    if (!urls.isEmpty() || canPasteData) {
        // disable the paste action if no writing is supported
        KFileItem item(KFileItem::Unknown, KFileItem::Unknown, targetUrl);
        ret.first = KFileItemListProperties(KFileItemList() << item).supportsWriting();

        if (urls.count() == 1) {
            const KFileItem item(KFileItem::Unknown, KFileItem::Unknown, urls.first(), true);
            ret.second = item.isDir() ? i18nc("@action:inmenu", "Paste One Folder") :
                                        i18nc("@action:inmenu", "Paste One File");

        } else if (!urls.isEmpty()) {
            ret.second = i18ncp("@action:inmenu", "Paste One Item", "Paste %1 Items", urls.count());
        } else {
            ret.second = i18nc("@action:inmenu", "Paste Clipboard Contents...");
        }
    } else {
        ret.first = false;
        ret.second = i18nc("@action:inmenu", "Paste");
    }

    return ret;
}
// /copied

struct FileOpsContextManagerItemPrivate {
	FileOpsContextManagerItem* mContextManagerItem;
	QListView* mThumbnailView;
	SideBarGroup* mGroup;
	KAction* mCutAction;
	KAction* mCopyAction;
	KAction* mPasteAction;
	KAction* mCopyToAction;
	KAction* mMoveToAction;
	KAction* mLinkToAction;
	KAction* mRenameAction;
	KAction* mTrashAction;
	KAction* mDelAction;
	KAction* mRestoreAction;
	KAction* mShowPropertiesAction;
	KAction* mCreateFolderAction;
	KAction* mOpenWithAction;
	QMap<QString, KService::Ptr> mServiceForName;

	KUrl::List urlList() const {
		KUrl::List urlList;

		KFileItemList list = mContextManagerItem->contextManager()->selectedFileItemList();
		if (list.count() > 0) {
			urlList = list.urlList();
		} else {
			KUrl url = mContextManagerItem->contextManager()->currentUrl();
			Q_ASSERT(url.isValid());
			urlList << url;
		}
		return urlList;
	}


	void updateServiceForName() {
		// This code is inspired from
		// kdebase/apps/lib/konq/konq_menuactions.cpp

		// Get list of all distinct mimetypes in selection
		QStringList mimeTypes;
		Q_FOREACH(const KFileItem& item, mContextManagerItem->contextManager()->selectedFileItemList()) {
			const QString mimeType = item.mimetype();
			if (!mimeTypes.contains(mimeType)) {
				mimeTypes << mimeType;
			}
		}

		// Query trader
		const QString firstMimeType = mimeTypes.takeFirst();
		const QString constraintTemplate = "'%1' in ServiceTypes";
		QStringList constraints;
		Q_FOREACH(const QString& mimeType, mimeTypes) {
			constraints << constraintTemplate.arg(mimeType);
		}

		KService::List services = KMimeTypeTrader::self()->query(
			firstMimeType, "Application",
			constraints.join(" and "));

		// Update map
		mServiceForName.clear();
		Q_FOREACH(const KService::Ptr &service, services) {
			mServiceForName[service->name()] = service;
		}
	}


	QMimeData* selectionMimeData() {
		QMimeData* mimeData = new QMimeData;
		KFileItemList list = mContextManagerItem->contextManager()->selectedFileItemList();
		list.urlList().populateMimeData(mimeData);
		return mimeData;
	}


	KUrl pasteTargetUrl() const {
		// If only one folder is selected, paste inside it, otherwise paste in
		// current
		KFileItemList list = mContextManagerItem->contextManager()->selectedFileItemList();
		if (list.count() == 1 && list.first().isDir()) {
			return list.first().url();
		} else {
			return mContextManagerItem->contextManager()->currentDirUrl();
		}
	}
};


FileOpsContextManagerItem::FileOpsContextManagerItem(ContextManager* manager, QListView* thumbnailView, KActionCollection* actionCollection)
: AbstractContextManagerItem(manager)
, d(new FileOpsContextManagerItemPrivate) {
	d->mContextManagerItem = this;
	d->mThumbnailView = thumbnailView;
	d->mGroup = new SideBarGroup(i18n("File Operations"));
	setWidget(d->mGroup);
	EventWatcher::install(d->mGroup, QEvent::Show, this, SLOT(updateSideBarContent()));

	connect(contextManager(), SIGNAL(selectionChanged()),
		SLOT(updateActions()) );
	connect(contextManager(), SIGNAL(currentDirUrlChanged()),
		SLOT(updateActions()) );

	KActionCategory* file = new KActionCategory(i18nc("@title actions category","File"), actionCollection);
	KActionCategory* edit = new KActionCategory(i18nc("@title actions category","Edit"), actionCollection);

	d->mCutAction = edit->addAction(KStandardAction::Cut, this, SLOT(cut()));

	// Copied from Dolphin:
    // need to remove shift+del from cut action, else the shortcut for deletejob
    // doesn't work
	KShortcut cutShortcut = d->mCutAction->shortcut();
	cutShortcut.remove(Qt::SHIFT + Qt::Key_Delete, KShortcut::KeepEmpty);
	d->mCutAction->setShortcut(cutShortcut);

	d->mCopyAction = edit->addAction(KStandardAction::Copy, this, SLOT(copy()));
	d->mPasteAction = edit->addAction(KStandardAction::Paste, this, SLOT(paste()));

	d->mCopyToAction = file->addAction("file_copy_to",this,SLOT(copyTo()));
	d->mCopyToAction->setText(i18nc("Verb", "Copy To..."));
	d->mCopyToAction->setShortcut(Qt::Key_F7);

	d->mMoveToAction = file->addAction("file_move_to",this,SLOT(moveTo()));
	d->mMoveToAction->setText(i18nc("Verb", "Move To..."));
	d->mMoveToAction->setShortcut(Qt::Key_F8);

	d->mLinkToAction = file->addAction("file_link_to",this,SLOT(linkTo()));
	d->mLinkToAction->setText(i18nc("Verb: create link to the file where user wants", "Link To..."));
	d->mLinkToAction->setShortcut(Qt::Key_F9);

	d->mRenameAction = file->addAction("file_rename",this,SLOT(rename()));
	d->mRenameAction->setText(i18nc("Verb", "Rename..."));
	d->mRenameAction->setShortcut(Qt::Key_F2);

	d->mTrashAction = file->addAction("file_trash",this,SLOT(trash()));
	d->mTrashAction->setText(i18nc("Verb", "Trash"));
	d->mTrashAction->setIcon(KIcon("user-trash"));
	d->mTrashAction->setShortcut(Qt::Key_Delete);

	d->mDelAction = file->addAction("file_delete",this,SLOT(del()));
	d->mDelAction->setText(i18n("Delete"));
	d->mDelAction->setIcon(KIcon("edit-delete"));
	d->mDelAction->setShortcut(QKeySequence(Qt::ShiftModifier | Qt::Key_Delete));

	d->mRestoreAction = file->addAction("file_restore", this, SLOT(restore()));
	d->mRestoreAction->setText(i18n("Restore"));

	d->mShowPropertiesAction = file->addAction("file_show_properties",this,SLOT(showProperties()));
	d->mShowPropertiesAction->setText(i18n("Properties"));
	d->mShowPropertiesAction->setIcon(KIcon("document-properties"));

	d->mCreateFolderAction = file->addAction("file_create_folder",this,SLOT(createFolder()));
	d->mCreateFolderAction->setText(i18n("Create Folder..."));
	d->mCreateFolderAction->setIcon(KIcon("folder-new"));

	d->mOpenWithAction = file->addAction("file_open_with");
	d->mOpenWithAction->setText(i18n("Open With"));
	QMenu* menu = new QMenu;
	d->mOpenWithAction->setMenu(menu);
	connect(menu, SIGNAL(aboutToShow()),
		SLOT(populateOpenMenu()) );
	connect(menu, SIGNAL(triggered(QAction*)),
		SLOT(openWith(QAction*)) );
	updateActions();

	connect(QApplication::clipboard(), SIGNAL(dataChanged()),
		SLOT(updatePasteAction()));
	updatePasteAction();
}


FileOpsContextManagerItem::~FileOpsContextManagerItem() {
	delete d->mOpenWithAction->menu();
	delete d;
}


void FileOpsContextManagerItem::updateActions() {
	const int count = contextManager()->selectedFileItemList().count();
	const bool selectionNotEmpty = count > 0;
	const bool urlIsValid = contextManager()->currentUrl().isValid();
	const bool dirUrlIsValid = contextManager()->currentDirUrl().isValid();

	d->mCutAction->setEnabled(selectionNotEmpty);
	d->mCopyAction->setEnabled(selectionNotEmpty);
	d->mCopyToAction->setEnabled(selectionNotEmpty);
	d->mMoveToAction->setEnabled(selectionNotEmpty);
	d->mLinkToAction->setEnabled(selectionNotEmpty);
	d->mTrashAction->setEnabled(selectionNotEmpty);
	d->mDelAction->setEnabled(selectionNotEmpty);
	d->mOpenWithAction->setEnabled(selectionNotEmpty);
	d->mRenameAction->setEnabled(count == 1);

	d->mCreateFolderAction->setEnabled(dirUrlIsValid);
	d->mShowPropertiesAction->setEnabled(dirUrlIsValid || urlIsValid);

	updateSideBarContent();
}


void FileOpsContextManagerItem::updatePasteAction() {
	QPair<bool, QString> info = pasteInfo(d->pasteTargetUrl());
	d->mPasteAction->setEnabled(info.first);
	d->mPasteAction->setText(info.second);
}


inline void addIfEnabled(SideBarGroup* group, QAction* action) {
	if (action->isEnabled()) {
		group->addAction(action);
	}
}

void FileOpsContextManagerItem::updateSideBarContent() {
	if (!d->mGroup->isVisible()) {
		return;
	}

	d->mGroup->clear();
	addIfEnabled(d->mGroup, d->mRenameAction);
	addIfEnabled(d->mGroup, d->mTrashAction);
	addIfEnabled(d->mGroup, d->mDelAction);
	addIfEnabled(d->mGroup, d->mCopyToAction);
	addIfEnabled(d->mGroup, d->mMoveToAction);
	addIfEnabled(d->mGroup, d->mLinkToAction);
	addIfEnabled(d->mGroup, d->mShowPropertiesAction);
	addIfEnabled(d->mGroup, d->mCreateFolderAction);
	addIfEnabled(d->mGroup, d->mOpenWithAction);
}


void FileOpsContextManagerItem::showProperties() {
	KFileItemList list = contextManager()->selectedFileItemList();
	if (list.count() > 0) {
		KPropertiesDialog::showDialog(list, d->mGroup);
	} else {
		KUrl url = contextManager()->currentDirUrl();
		KPropertiesDialog::showDialog(url, d->mGroup);
	}
}


void FileOpsContextManagerItem::cut() {
	QMimeData* mimeData = d->selectionMimeData();
	addCutSelection(mimeData);
	QApplication::clipboard()->setMimeData(mimeData);
}


void FileOpsContextManagerItem::copy() {
	QMimeData* mimeData = d->selectionMimeData();
	QApplication::clipboard()->setMimeData(mimeData);
}


void FileOpsContextManagerItem::paste() {
	const bool move = decodeIsCutSelection(QApplication::clipboard()->mimeData());
	KIO::pasteClipboard(d->pasteTargetUrl(), d->mGroup, move);
}


void FileOpsContextManagerItem::trash() {
	FileOperations::trash(d->urlList(), d->mGroup);
}


void FileOpsContextManagerItem::del() {
	FileOperations::del(d->urlList(), d->mGroup);
}


void FileOpsContextManagerItem::restore() {
	KonqOperations::restoreTrashedItems(d->urlList(), d->mGroup);
}


void FileOpsContextManagerItem::copyTo() {
	FileOperations::copyTo(d->urlList(), d->mGroup);
}


void FileOpsContextManagerItem::moveTo() {
	FileOperations::moveTo(d->urlList(), d->mGroup);
}


void FileOpsContextManagerItem::linkTo() {
	FileOperations::linkTo(d->urlList(), d->mGroup);
}


void FileOpsContextManagerItem::rename() {
	if (d->mThumbnailView->isVisible()) {
		QModelIndex index = d->mThumbnailView->currentIndex();
		d->mThumbnailView->edit(index);
	} else {
		FileOperations::rename(d->urlList().first(), d->mGroup);
	}
}


void FileOpsContextManagerItem::createFolder() {
	KUrl url = contextManager()->currentDirUrl();
	FileOperations::createFolder(url, d->mGroup);
}


void FileOpsContextManagerItem::populateOpenMenu() {
	QMenu* openMenu = d->mOpenWithAction->menu();
	qDeleteAll(openMenu->actions());

	d->updateServiceForName();

	Q_FOREACH(const KService::Ptr &service, d->mServiceForName) {
		QString text = service->name().replace( '&', "&&" );
		QAction* action = openMenu->addAction(text);
		action->setIcon(KIcon(service->icon()));
		action->setData(service->name());
	}

	openMenu->addSeparator();
	openMenu->addAction(i18n("Other Application..."));
}


void FileOpsContextManagerItem::openWith(QAction* action) {
	Q_ASSERT(action);
	KService::Ptr service;
	KUrl::List list = d->urlList();

	QString name = action->data().toString();
	if (name.isEmpty()) {
		// Other Application...
		KOpenWithDialog dlg(list, d->mGroup);
		if (!dlg.exec()) {
			return;
		}
		service = dlg.service();

		if (!service) {
			// User entered a custom command
			Q_ASSERT(!dlg.text().isEmpty());
			KRun::run(dlg.text(), list, d->mGroup);
			return;
		}
	} else {
		service = d->mServiceForName[name];
	}

	Q_ASSERT(service);
	KRun::run(*service, list, d->mGroup);
}


} // namespace
