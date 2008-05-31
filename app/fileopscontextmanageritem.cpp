// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#include <QMenu>

// KDE
#include <kaction.h>
#include <kactioncollection.h>
#include <kdebug.h>
#include <kfileitem.h>
#include <klocale.h>
#include <kmimetypetrader.h>
#include <kopenwithdialog.h>
#include <kpropertiesdialog.h>
#include <krun.h>
#include <kservice.h>

// Local
#include "contextmanager.h"
#include "fileoperations.h"
#include "sidebar.h"

namespace Gwenview {

struct FileOpsContextManagerItemPrivate {
	FileOpsContextManagerItem* mContextManagerItem;
	SideBar* mSideBar;
	SideBarGroup* mGroup;
	QAction* mCopyToAction;
	QAction* mMoveToAction;
	QAction* mLinkToAction;
	QAction* mTrashAction;
	QAction* mDelAction;
	QAction* mShowPropertiesAction;
	QAction* mCreateFolderAction;
	QAction* mOpenWithAction;
	QMap<QString, KService::Ptr> mServiceForName;

	KUrl::List urlList() const {
		KUrl::List urlList;

		KFileItemList list = mContextManagerItem->contextManager()->selection();
		if (list.count() > 0) {
			urlList = list.urlList();
		} else {
			KUrl url = mContextManagerItem->contextManager()->currentUrl();
			Q_ASSERT(url.isValid());
			urlList << url;
		}
		return urlList;
	}
};


FileOpsContextManagerItem::FileOpsContextManagerItem(ContextManager* manager, KActionCollection* actionCollection)
: AbstractContextManagerItem(manager)
, d(new FileOpsContextManagerItemPrivate) {
	d->mContextManagerItem = this;
	d->mSideBar = 0;
	d->mGroup = 0;

	connect(contextManager(), SIGNAL(selectionChanged()),
		SLOT(updateActions()) );
	connect(contextManager(), SIGNAL(currentDirUrlChanged()),
		SLOT(updateActions()) );

	d->mCopyToAction = actionCollection->addAction("file_copy_to");
	d->mCopyToAction->setText(i18nc("Verb", "Copy To..."));
	connect(d->mCopyToAction, SIGNAL(triggered()),
		SLOT(copyTo()) );

	d->mMoveToAction = actionCollection->addAction("file_move_to");
	d->mMoveToAction->setText(i18nc("Verb", "Move To..."));
	connect(d->mMoveToAction, SIGNAL(triggered()),
		SLOT(moveTo()) );

	d->mLinkToAction = actionCollection->addAction("file_link_to");
	d->mLinkToAction->setText(i18nc("Verb", "Link To..."));
	connect(d->mLinkToAction, SIGNAL(triggered()),
		SLOT(linkTo()) );

	d->mTrashAction = actionCollection->addAction("file_trash");
	d->mTrashAction->setText(i18nc("Verb", "Trash"));
	d->mTrashAction->setIcon(KIcon("user-trash"));
	d->mTrashAction->setShortcut(Qt::Key_Delete);
	connect(d->mTrashAction, SIGNAL(triggered()),
		SLOT(trash()) );

	d->mDelAction = actionCollection->addAction("file_delete");
	d->mDelAction->setText(i18n("Delete"));
	d->mDelAction->setIcon(KIcon("edit-delete"));
	d->mDelAction->setShortcut(Qt::ShiftModifier | Qt::Key_Delete);
	connect(d->mDelAction, SIGNAL(triggered()),
		SLOT(del()) );

	d->mShowPropertiesAction = actionCollection->addAction("file_show_properties");
	d->mShowPropertiesAction->setText(i18n("Properties"));
	d->mShowPropertiesAction->setIcon(KIcon("document-properties"));
	connect(d->mShowPropertiesAction, SIGNAL(triggered()),
		SLOT(showProperties()) );

	d->mCreateFolderAction = actionCollection->addAction("file_create_folder");
	d->mCreateFolderAction->setText(i18n("Create Folder..."));
	d->mCreateFolderAction->setIcon(KIcon("folder-new"));
	connect(d->mCreateFolderAction, SIGNAL(triggered()),
		SLOT(createFolder()) );

	d->mOpenWithAction = actionCollection->addAction("file_open_with");
	d->mOpenWithAction->setText(i18n("Open With..."));
	QMenu* menu = new QMenu;
	d->mOpenWithAction->setMenu(menu);
	connect(menu, SIGNAL(aboutToShow()),
		SLOT(populateOpenMenu()) );
	connect(menu, SIGNAL(triggered(QAction*)),
		SLOT(openWith(QAction*)) );
        updateActions();
}


FileOpsContextManagerItem::~FileOpsContextManagerItem() {
	delete d->mOpenWithAction->menu();
	delete d;
}


void FileOpsContextManagerItem::setSideBar(SideBar* sideBar) {
	d->mSideBar = sideBar;
	connect(sideBar, SIGNAL(aboutToShow()),
		SLOT(updateSideBarContent()) );

	d->mGroup = sideBar->createGroup(i18n("File Operations"));
}


QAction* FileOpsContextManagerItem::copyToAction() const {
	return d->mCopyToAction;
}


QAction* FileOpsContextManagerItem::moveToAction() const {
	return d->mMoveToAction;
}


QAction* FileOpsContextManagerItem::linkToAction() const {
	return d->mLinkToAction;
}


QAction* FileOpsContextManagerItem::trashAction() const {
	return d->mTrashAction;
}


QAction* FileOpsContextManagerItem::delAction() const {
	return d->mDelAction;
}


QAction* FileOpsContextManagerItem::showPropertiesAction() const {
	return d->mShowPropertiesAction;
}


QAction* FileOpsContextManagerItem::createFolderAction() const {
	return d->mCreateFolderAction;
}


void FileOpsContextManagerItem::updateActions() {
	KFileItemList list = contextManager()->selection();
	bool selectionNotEmpty = list.count() > 0;

	d->mCopyToAction->setEnabled(selectionNotEmpty);
	d->mMoveToAction->setEnabled(selectionNotEmpty);
	d->mLinkToAction->setEnabled(selectionNotEmpty);
	d->mTrashAction->setEnabled(selectionNotEmpty);
	d->mDelAction->setEnabled(selectionNotEmpty);

	KUrl url = contextManager()->currentUrl();
	d->mOpenWithAction->setEnabled(url.isValid());

	updateSideBarContent();
}


inline void addIfEnabled(SideBarGroup* group, QAction* action) {
	if (action->isEnabled()) {
		group->addAction(action);
	}
}

void FileOpsContextManagerItem::updateSideBarContent() {
	if (!d->mSideBar ||  !d->mSideBar->isVisible()) {
		return;
	}

	d->mGroup->clear();
	addIfEnabled(d->mGroup, d->mCreateFolderAction);
	addIfEnabled(d->mGroup, d->mCopyToAction);
	addIfEnabled(d->mGroup, d->mMoveToAction);
	addIfEnabled(d->mGroup, d->mLinkToAction);
	addIfEnabled(d->mGroup, d->mTrashAction);
	addIfEnabled(d->mGroup, d->mDelAction);
	addIfEnabled(d->mGroup, d->mShowPropertiesAction);
	addIfEnabled(d->mGroup, d->mOpenWithAction);
}


void FileOpsContextManagerItem::showProperties() {
	KFileItemList list = contextManager()->selection();
	if (list.count() > 0) {
		KPropertiesDialog::showDialog(list, d->mSideBar);
	} else {
		KUrl url = contextManager()->currentDirUrl();
		KPropertiesDialog::showDialog(url, d->mSideBar);
	}
}


void FileOpsContextManagerItem::trash() {
	FileOperations::trash(d->urlList(), d->mSideBar);
}


void FileOpsContextManagerItem::del() {
	FileOperations::del(d->urlList(), d->mSideBar);
}


void FileOpsContextManagerItem::copyTo() {
	FileOperations::copyTo(d->urlList(), d->mSideBar);
}


void FileOpsContextManagerItem::moveTo() {
	FileOperations::moveTo(d->urlList(), d->mSideBar);
}


void FileOpsContextManagerItem::linkTo() {
	FileOperations::linkTo(d->urlList(), d->mSideBar);
}


void FileOpsContextManagerItem::createFolder() {
	KUrl url = contextManager()->currentDirUrl();
	FileOperations::createFolder(url, d->mSideBar);
}


void FileOpsContextManagerItem::populateOpenMenu() {
	QMenu* openMenu = d->mOpenWithAction->menu();
	QList<QAction*> currentActions = openMenu->actions();
	qDeleteAll(currentActions);

	QString mimeType = contextManager()->currentUrlMimeType();
	KService::List services = KMimeTypeTrader::self()->query(mimeType);

	d->mServiceForName.clear();
	Q_FOREACH(const KService::Ptr &service, services) {
		d->mServiceForName[service->name()] = service;
	}

	Q_FOREACH(const KService::Ptr &service, d->mServiceForName) {
		QString text = service->name().replace( "&", "&&" );
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
		KOpenWithDialog dlg(list, d->mSideBar);
		if (!dlg.exec()) {
			return;
		}
		service = dlg.service();

		if (!service) {
			// User entered a custom command
			Q_ASSERT(!dlg.text().isEmpty());
			KRun::run(dlg.text(), list, d->mSideBar);
			return;
		}
	} else {
		service = d->mServiceForName[name];
	}

	Q_ASSERT(service);
	KRun::run(*service, list, d->mSideBar);
}


} // namespace
