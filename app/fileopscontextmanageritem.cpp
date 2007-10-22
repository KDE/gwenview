// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
#include "fileopscontextmanageritem.h"

// Qt
#include <QAction>

// KDE
#include <kfiledialog.h>
#include <kfileitem.h>
#include <kguiitem.h>
#include <kio/copyjob.h>
#include <kio/deletejob.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpropertiesdialog.h>
#include <kdebug.h>

// Local
#include "contextmanager.h"
#include "sidebar.h"

namespace Gwenview {

static KUrl::List urlListFromKFileItemList(const KFileItemList list) {
	KUrl::List urlList;
	Q_FOREACH(KFileItem item, list) {
		urlList << item.url();
	}
	return urlList;
}

// Taken from KonqOperations::askDeleteConfirmation
// Forked because KonqOperations is in kdebase/apps/lib/konq, and kdegraphics
// can't depend on kdebase.
enum Operation { TRASH, DEL, COPY, MOVE, LINK, EMPTYTRASH, STAT, MKDIR, RESTORE, UNKNOWN };
enum ConfirmationType { DEFAULT_CONFIRMATION, SKIP_CONFIRMATION, FORCE_CONFIRMATION };
static bool askDeleteConfirmation( const KUrl::List & selectedUrls, Operation operation, ConfirmationType confirmation, QWidget* widget )
{
	if ( confirmation == SKIP_CONFIRMATION ) {
		return true;
	}

	QString keyName;
	if ( confirmation == DEFAULT_CONFIRMATION ) {
		KConfig config( "konquerorrc", KConfig::NoGlobals );
		keyName = ( operation == DEL ? "ConfirmDelete" : "ConfirmTrash" );
		bool ask = config.group("Trash").readEntry( keyName, true );
		if (!ask) {
			return true;
		}
	}

	KUrl::List::ConstIterator it = selectedUrls.begin();
	QStringList prettyList;
	for ( ; it != selectedUrls.end(); ++it ) {
		if ( (*it).protocol() == "trash" ) {
			QString path = (*it).path();
			// HACK (#98983): remove "0-foo". Note that it works better than
			// displaying KFileItem::name(), for files under a subdir.
			prettyList.append( path.remove(QRegExp("^/[0-9]*-")) );
		} else
			prettyList.append( (*it).pathOrUrl() );
	}

	int result;
	if (operation == DEL) {
		result = KMessageBox::warningContinueCancelList(
			widget,
			i18np( "Do you really want to delete this item?", "Do you really want to delete these %1 items?", prettyList.count()),
			prettyList,
			i18n( "Delete Files" ),
			KStandardGuiItem::del(),
			KStandardGuiItem::cancel(),
			keyName, KMessageBox::Notify | KMessageBox::Dangerous);
	} else {
		result = KMessageBox::warningContinueCancelList(
			widget,
			i18np( "Do you really want to move this item to the trash?", "Do you really want to move these %1 items to the trash?", prettyList.count()),
			prettyList,
			i18n( "Move to Trash" ),
			KGuiItem( i18nc( "Verb", "&Trash" ), "user-trash"),
			KStandardGuiItem::cancel(),
			keyName, KMessageBox::Notify | KMessageBox::Dangerous);
	}
	if (!keyName.isEmpty()) {
		// Check kmessagebox setting... erase & copy to konquerorrc.
		KSharedConfig::Ptr config = KGlobal::config();
		KConfigGroup saver(config, "Notification Messages");
		if (!saver.readEntry(keyName, QVariant(true)).toBool()) {
			saver.writeEntry(keyName, true);
			saver.sync();
			KConfig konq_config("konquerorrc", KConfig::NoGlobals);
			konq_config.group("Trash").writeEntry( keyName, false );
		}
	}
	return (result == KMessageBox::Continue);
}


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


	void copyMoveOrLink(Operation operation) {
		KFileItemList list = mContextManagerItem->contextManager()->selection();
		Q_ASSERT(list.count() > 0);
		KUrl::List urlList = urlListFromKFileItemList(list);

		KFileDialog dialog(
			KUrl("kfiledialog:///<copyMoveOrLink>"),
			QString() /* filter */,
			mSideBar);
		switch (operation) {
		case COPY:
			dialog.setCaption(i18n("Copy To"));
			break;
		case MOVE:
			dialog.setCaption(i18n("Move To"));
			break;
		case LINK:
			dialog.setCaption(i18n("Link To"));
			break;
		default:
			Q_ASSERT(0);
		}
		dialog.setOperationMode(KFileDialog::Saving);
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
		switch (operation) {
		case COPY:
			KIO::copy(urlList, destUrl);
			break;

		case MOVE:
			KIO::move(urlList, destUrl);
			break;

		case LINK:
			KIO::link(urlList, destUrl);
			break;

		default:
			Q_ASSERT(0);
		}
	}


	void delOrTrash(Operation operation) {
		KFileItemList list = mContextManagerItem->contextManager()->selection();
		Q_ASSERT(list.count() > 0);
		KUrl::List urlList = urlListFromKFileItemList(list);

		if (!askDeleteConfirmation(urlList, operation, DEFAULT_CONFIRMATION, mSideBar)) {
			return;
		}

		switch (operation) {
		case TRASH:
			KIO::trash(urlList);
			break;

		case DEL:
			KIO::del(urlList);
			break;

		default:
			kWarning() << "Unknown operation " << operation ;
			break;
		}
	}
};


FileOpsContextManagerItem::FileOpsContextManagerItem(ContextManager* manager)
: AbstractContextManagerItem(manager)
, d(new FileOpsContextManagerItemPrivate) {
	d->mContextManagerItem = this;

	connect(contextManager(), SIGNAL(selectionChanged()),
		SLOT(updateActions()) );
	connect(contextManager(), SIGNAL(currentDirUrlChanged()),
		SLOT(updateActions()) );

	d->mCopyToAction = new QAction(this);
	d->mCopyToAction->setText(i18nc("Verb", "Copy To"));
	connect(d->mCopyToAction, SIGNAL(triggered()),
		SLOT(copyTo()) );

	d->mMoveToAction = new QAction(this);
	d->mMoveToAction->setText(i18nc("Verb", "Move To"));
	connect(d->mMoveToAction, SIGNAL(triggered()),
		SLOT(moveTo()) );

	d->mLinkToAction = new QAction(this);
	d->mLinkToAction->setText(i18nc("Verb", "Link To"));
	connect(d->mLinkToAction, SIGNAL(triggered()),
		SLOT(linkTo()) );

	d->mTrashAction = new QAction(this);
	d->mTrashAction->setText(i18nc("Verb", "Trash"));
	d->mTrashAction->setIcon(KIcon("user-trash"));
	d->mTrashAction->setShortcut(Qt::Key_Delete);
	connect(d->mTrashAction, SIGNAL(triggered()),
		SLOT(trash()) );

	d->mDelAction = new QAction(this);
	d->mDelAction->setText(i18n("Delete"));
	d->mDelAction->setIcon(KIcon("edit-delete"));
	d->mDelAction->setShortcut(Qt::ShiftModifier | Qt::Key_Delete);
	connect(d->mDelAction, SIGNAL(triggered()),
		SLOT(del()) );

	d->mShowPropertiesAction = new QAction(this);
	d->mShowPropertiesAction->setText(i18n("Properties"));
	d->mShowPropertiesAction->setIcon(KIcon("document-properties"));
	connect(d->mShowPropertiesAction, SIGNAL(triggered()),
		SLOT(showProperties()) );
}


FileOpsContextManagerItem::~FileOpsContextManagerItem() {
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


void FileOpsContextManagerItem::updateActions() {
	KFileItemList list = contextManager()->selection();
	bool selectionNotEmpty = list.count() > 0;

	d->mCopyToAction->setEnabled(selectionNotEmpty);
	d->mMoveToAction->setEnabled(selectionNotEmpty);
	d->mLinkToAction->setEnabled(selectionNotEmpty);
	d->mTrashAction->setEnabled(selectionNotEmpty);
	d->mDelAction->setEnabled(selectionNotEmpty);

	updateSideBarContent();
}


inline void addIfEnabled(SideBarGroup* group, QAction* action) {
	if (action->isEnabled()) {
		group->addAction(action);
	}
}

void FileOpsContextManagerItem::updateSideBarContent() {
	if (!d->mSideBar->isVisible()) {
		return;
	}

	d->mGroup->clear();
	addIfEnabled(d->mGroup, d->mCopyToAction);
	addIfEnabled(d->mGroup, d->mMoveToAction);
	addIfEnabled(d->mGroup, d->mLinkToAction);
	addIfEnabled(d->mGroup, d->mTrashAction);
	addIfEnabled(d->mGroup, d->mDelAction);
	addIfEnabled(d->mGroup, d->mShowPropertiesAction);
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
	d->delOrTrash(TRASH);
}


void FileOpsContextManagerItem::del() {
	d->delOrTrash(DEL);
}


void FileOpsContextManagerItem::copyTo() {
	d->copyMoveOrLink(COPY);
}


void FileOpsContextManagerItem::moveTo() {
	d->copyMoveOrLink(MOVE);
}


void FileOpsContextManagerItem::linkTo() {
	d->copyMoveOrLink(LINK);
}


} // namespace
