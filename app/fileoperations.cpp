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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "fileoperations.h"

// KDE
#include <kdebug.h>
#include <kfiledialog.h>
#include <kfileitem.h>
#include <kguiitem.h>
#include <kinputdialog.h>
#include <kio/copyjob.h>
#include <kio/deletejob.h>
#include <kio/job.h>
#include <klocale.h>
#include <kmessagebox.h>

namespace Gwenview {

namespace FileOperations {


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


static void copyMoveOrLink(Operation operation, const KUrl::List& urlList, QWidget* parent) {
	Q_ASSERT(urlList.count() > 0);

	KFileDialog dialog(
		KUrl("kfiledialog:///<copyMoveOrLink>"),
		QString() /* filter */,
		parent);
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


static void delOrTrash(Operation operation, const KUrl::List& urlList, QWidget* parent) {
	Q_ASSERT(urlList.count() > 0);

	if (!askDeleteConfirmation(urlList, operation, DEFAULT_CONFIRMATION, parent)) {
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

void copyTo(const KUrl::List& urlList, QWidget* parent) {
	copyMoveOrLink(COPY, urlList, parent);
}


void moveTo(const KUrl::List& urlList, QWidget* parent) {
	copyMoveOrLink(MOVE, urlList, parent);
}


void linkTo(const KUrl::List& urlList, QWidget* parent) {
	copyMoveOrLink(LINK, urlList, parent);
}


void trash(const KUrl::List& urlList, QWidget* parent) {
	delOrTrash(TRASH, urlList, parent);
}


void del(const KUrl::List& urlList, QWidget* parent) {
	delOrTrash(DEL, urlList, parent);
}


void createFolder(const KUrl& parentUrl, QWidget* parent) {
	bool ok;
	QString name = KInputDialog::getText(
		i18n("Create Folder"),
		i18n("Enter the name of the folder to create:"),
		QString(),
		&ok,
		parent);

	if (!ok) {
		return;
	}

	KUrl url = parentUrl;
	url.addPath(name);

	KIO::mkdir(url);
}


} // namespace

} // namespace
