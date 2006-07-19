// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aur�ien G�eau

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// Qt
#include <qfile.h>
#include <qstylesheet.h>
#include <qwidget.h>

// KDE
#include <kdeversion.h>
#include <kfiledialog.h>
#include <kfilefiltercombo.h>
#include <kglobalsettings.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kurlcombobox.h>

// Local
#include "deletedialog.h"
#include "fileoperation.h"
#include "fileopobject.moc"
#include "fileoperationconfig.h"
#include "inputdialog.h"
namespace Gwenview {


/**
 * A tweaked KFileDialog used to select an existing directory. More efficient
 * than KDirSelectDialog, since it provides access to bookmarks and let you
 * create a dir.
 */
class DirSelectDialog : public KFileDialog {
public:
	DirSelectDialog(const QString& startDir, QWidget* parent)
	: KFileDialog(startDir, QString::null, parent, "dirselectdialog", true) {
		locationEdit->setEnabled(false);
		filterWidget->setEnabled(false);
		setMode(KFile::Directory | KFile::ExistingOnly);

		// Cast to avoid gcc being confused
		setPreviewWidget(static_cast<KPreviewWidgetBase*>(0));
	}
};


//-FileOpObject--------------------------------------------------------------------
FileOpObject::FileOpObject(const KURL& url,QWidget* parent)
: mParent(parent)
{
	mURLList.append(url);
}


FileOpObject::FileOpObject(const KURL::List& list,QWidget* parent)
: mParent(parent), mURLList(list)
{}


void FileOpObject::slotResult(KIO::Job* job) {
	if (job->error()) {
		job->showErrorDialog(mParent);
	}

	emit success();

// Let's shoot ourself in the foot...
	delete this;
}


void FileOpObject::polishJob(KIO::Job* job) {
	job->setWindow(mParent->topLevelWidget());
	connect( job, SIGNAL( result(KIO::Job*) ),
		this, SLOT( slotResult(KIO::Job*) ) );
}


//-FileOpCopyToObject--------------------------------------------------------------


void FileOpCopyToObject::operator()() {
	KURL destURL;

	if (FileOperationConfig::confirmCopy()) {
		QString destDir = FileOperationConfig::destDir();
		if( !destDir.isEmpty()) {
			destDir += "/";
		}
		if (mURLList.size()==1) {
			destURL=KFileDialog::getSaveURL(destDir + mURLList.first().fileName(),
					QString::null, mParent, i18n("Copy File"));
		} else {
			DirSelectDialog dialog(destDir, mParent);
			dialog.setCaption(i18n("Select Folder Where Files Will be Copied"));
			dialog.exec();
			destURL=dialog.selectedURL();
		}
	} else {
		destURL.setPath(FileOperationConfig::destDir());
	}
	if (destURL.isEmpty()) return;

// Copy the file
	KIO::Job* job=KIO::copy(mURLList,destURL,true);
	polishJob(job);

}


//-FileOpCopyToObject--------------------------------------------------------------


void FileOpLinkToObject::operator()() {
	KURL destURL;

	if (FileOperationConfig::confirmCopy()) {
		QString destDir = FileOperationConfig::destDir();
		if( !destDir.isEmpty()) {
			destDir += "/";
		}
		if (mURLList.size()==1) {
			destURL=KFileDialog::getSaveURL(destDir + mURLList.first().fileName(),
					QString::null, mParent, i18n("Link File"));
		} else {
			DirSelectDialog dialog(destDir, mParent);
			dialog.setCaption(i18n("Select Folder Where the Files Will be Linked"));
			dialog.exec();
			destURL=dialog.selectedURL();
		}
	} else {
		destURL.setPath(FileOperationConfig::destDir());
	}
	if (destURL.isEmpty()) return;

// Copy the file
	KIO::Job* job=KIO::link(mURLList,destURL,true);
	polishJob(job);
}


//-FileOpMoveToObject--------------------------------------------------------------
void FileOpMoveToObject::operator()() {
	KURL destURL;

	if (FileOperationConfig::confirmMove()) {
		QString destDir = FileOperationConfig::destDir();
		if( !destDir.isEmpty()) {
			destDir += "/";
		}
		if (mURLList.size()==1) {
			destURL=KFileDialog::getSaveURL(destDir + mURLList.first().fileName(),
					QString::null, mParent, i18n("Move File"));
		} else {
			DirSelectDialog dialog(destDir, mParent);
			dialog.setCaption(i18n("Select Folder Where Files Will be Moved"));
			dialog.exec();
			destURL=dialog.selectedURL();
		}
	} else {
		destURL.setPath(FileOperationConfig::destDir());
	}
	if (destURL.isEmpty()) return;

// Move the file
	KIO::Job* job=KIO::move(mURLList,destURL,true);
	polishJob(job);
}


//-FileOpMakeDirObject-------------------------------------------------------------
void FileOpMakeDirObject::operator()() {
	InputDialog dlg(mParent);
	dlg.setCaption( i18n("Creating Folder") );
	dlg.setLabel( i18n("Enter the name of the new folder:") );
	dlg.setButtonOK( KGuiItem(i18n("Create Folder"), "folder_new") );
	if (!dlg.exec()) return;
	
	QString newDir = dlg.lineEdit()->text();

	KURL newURL(mURLList.first());
	newURL.addPath(newDir);
    KIO::Job* job=KIO::mkdir(newURL);
	polishJob(job);
}


static KIO::Job* createTrashJob(KURL::List lst) {
	KURL trashURL("trash:/");
	// Go do it
	if (lst.count()==1) {
		// If there's only one file, KIO::move will think we want to overwrite
		// the trash dir with the file to trash, so we add the file name
		trashURL.addPath(lst.first().fileName());
	}
	return KIO::move(lst, trashURL);
}

static KIO::Job* createDeleteJob(KURL::List lst) {
	return KIO::del(lst, false, true);
}


//-FileOpDelObject-----------------------------------------------------------------
void FileOpDelObject::operator()() {
	bool shouldDelete;
	if (FileOperationConfig::confirmDelete()) {
		DeleteDialog dlg(mParent);
		dlg.setURLList(mURLList);
		if (!dlg.exec()) return;
		shouldDelete = dlg.shouldDelete();
	} else {
		shouldDelete = not FileOperationConfig::deleteToTrash();
	}
		

	KIO::Job* job;
	if (shouldDelete) {
		job = createDeleteJob(mURLList);
	} else {
		job = createTrashJob(mURLList);
	}
	polishJob(job);
}


//-FileOpTrashObject---------------------------------------------------------------
void FileOpTrashObject::operator()() {
	// Confirm operation
	if (FileOperationConfig::confirmDelete()) {
		int response;
		if (mURLList.count()>1) {
			QStringList fileList;
			KURL::List::ConstIterator it=mURLList.begin();
			for (; it!=mURLList.end(); ++it) {
				fileList.append((*it).filename());
			}
			response=KMessageBox::warningContinueCancelList(mParent,
				i18n("Do you really want to trash these files?"),fileList,i18n("Trash used as a verb", "Trash Files"),KGuiItem(i18n("Trash used as a verb", "&Trash"),"edittrash"));
		} else {
			QString filename=QStyleSheet::escape(mURLList.first().filename());
			response=KMessageBox::warningContinueCancel(mParent,
				i18n("<p>Do you really want to move <b>%1</b> to the trash?</p>").arg(filename),i18n("Trash used as a verb", "Trash File"),KGuiItem(i18n("Trash used as a verb", "&Trash"),"edittrash"));
		}
		if (response!=KMessageBox::Continue) return;
	}

	KIO::Job* job = createTrashJob(mURLList);
	polishJob(job);
}

//-FileOpRealDeleteObject----------------------------------------------------------
void FileOpRealDeleteObject::operator()() {
	// Confirm operation
	if (FileOperationConfig::confirmDelete()) {
		int response;
		if (mURLList.count()>1) {
			QStringList fileList;
			KURL::List::ConstIterator it=mURLList.begin();
			for (; it!=mURLList.end(); ++it) {
				fileList.append((*it).filename());
			}
			response=KMessageBox::warningContinueCancelList(mParent,
				i18n("Do you really want to delete these files?"),fileList,
				i18n("Delete Files"),
				KStdGuiItem::del()
				);
		} else {
			QString filename=QStyleSheet::escape(mURLList.first().filename());
			response=KMessageBox::warningContinueCancel(mParent,
				i18n("<p>Do you really want to delete <b>%1</b>?</p>").arg(filename),
				i18n("Delete File"),
				KStdGuiItem::del()
				);
		}
		if (response!=KMessageBox::Continue) return;
	}

	// Delete the file
	KIO::Job* job = createDeleteJob(mURLList);
	polishJob(job);
}


//-FileOpRenameObject--------------------------------------------------------------
void FileOpRenameObject::operator()() {
	KURL srcURL=mURLList.first();

	// Prompt for the new filename
	QString filename = srcURL.filename();
	InputDialog dlg(mParent);
	dlg.setCaption(i18n("Renaming File"));
	dlg.setLabel(i18n("<p>Rename file <b>%1</b> to:</p>").arg(QStyleSheet::escape(filename)));
	dlg.setButtonOK( KGuiItem(i18n("&Rename"), "edit") );

	dlg.lineEdit()->setText(filename);
	int extPos = filename.findRev('.');
	if (extPos != -1) {
		if (filename.mid(extPos - 4, 4) == ".tar") {
			// Special case: *.tar.*
			extPos -= 4;
		}
		dlg.lineEdit()->setSelection(0, extPos);
	}
	if (!dlg.exec()) return;
	mNewFilename = dlg.lineEdit()->text();

	// Rename the file
	KURL destURL=srcURL;
	destURL.setFileName(mNewFilename);
	KIO::Job* job=KIO::move(srcURL,destURL);
	polishJob(job);
}


void FileOpRenameObject::slotResult(KIO::Job* job) {
	if (job->error()) {
		job->showErrorDialog(mParent);
	}

	emit success();
	emit renamed(mNewFilename);

// Let's shoot ourself in the foot...
	delete this;
}

} // namespace
