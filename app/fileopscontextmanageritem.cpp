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
#include <kfileitem.h>
#include <kguiitem.h>
#include <kio/copyjob.h>
#include <kio/deletejob.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpropertiesdialog.h>

// Local
#include "contextmanager.h"
#include "sidebar.h"

namespace Gwenview {

static KUrl::List urlListFromKFileItemList(const QList<KFileItem> list) {
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
			KGuiItem( i18nc( "Verb", "&Trash" ), "edit-trash"),
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
	QAction* mTrashAction;
	QAction* mDelAction;
	QAction* mShowPropertiesAction;

	void delOrTrash(Operation operation) {
		QList<KFileItem> list = mContextManagerItem->contextManager()->selection();
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
			kWarning() << "Unknown operation " << operation << endl;
			break;
		}
	}
};


FileOpsContextManagerItem::FileOpsContextManagerItem(ContextManager* manager)
: AbstractContextManagerItem(manager)
, d(new FileOpsContextManagerItemPrivate) {
	d->mContextManagerItem = this;

	connect(contextManager(), SIGNAL(selectionChanged()),
		SLOT(updateSideBarContent()) );
	connect(contextManager(), SIGNAL(currentDirUrlChanged()),
		SLOT(updateSideBarContent()) );

	d->mTrashAction = new QAction(this);
	d->mTrashAction->setText(i18nc("Verb", "Trash"));
	d->mTrashAction->setIcon(KIcon("edit-trash"));
	d->mTrashAction->setShortcut(Qt::Key_Delete);
	connect(d->mTrashAction, SIGNAL(triggered()),
		SLOT(trash()) );

	d->mDelAction = new QAction(this);
	d->mDelAction->setText(i18n("Delete"));
	d->mDelAction->setIcon(KIcon("edit-delete"));
	d->mDelAction->setShortcut(Qt::Key_Shift + Qt::Key_Delete);
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


QAction* FileOpsContextManagerItem::showPropertiesAction() const {
	return d->mShowPropertiesAction;
}


void FileOpsContextManagerItem::updateSideBarContent() {
	if (!d->mSideBar->isVisible()) {
		return;
	}

	QList<KFileItem> list = contextManager()->selection();
	d->mGroup->clear();
	bool selectionNotEmpty = list.count() > 0;
	d->mTrashAction->setEnabled(selectionNotEmpty);
	d->mDelAction->setEnabled(selectionNotEmpty);

	if (selectionNotEmpty) {
		d->mGroup->addAction(d->mTrashAction);
		d->mGroup->addAction(d->mDelAction);
	} else {
		// TODO: Insert current dir actions
	}

	d->mGroup->addAction(d->mShowPropertiesAction);
}


void FileOpsContextManagerItem::showProperties() {
	QList<KFileItem> list = contextManager()->selection();
	if (list.count() > 0) {
		KFileItemList itemList;
		Q_FOREACH(KFileItem item, list) {
			itemList << &item;
		}
		KPropertiesDialog::showDialog(itemList, d->mSideBar);
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


} // namespace
