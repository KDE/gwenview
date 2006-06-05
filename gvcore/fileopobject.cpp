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
#include <kinputdialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kurlcombobox.h>

// Local
#include "fileoperation.h"
#include "fileopobject.moc"
#include "fileoperationconfig.h"
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
	KIO::Job* copyJob=KIO::copy(mURLList,destURL,true);
	copyJob->setWindow(mParent->topLevelWidget());
	connect( copyJob, SIGNAL( result(KIO::Job*) ),
		this, SLOT( slotResult(KIO::Job*) ) );

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
	KIO::Job* copyJob=KIO::link(mURLList,destURL,true);
	copyJob->setWindow(mParent->topLevelWidget());
	connect( copyJob, SIGNAL( result(KIO::Job*) ),
		this, SLOT( slotResult(KIO::Job*) ) );
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
	KIO::Job* moveJob=KIO::move(mURLList,destURL,true);
	moveJob->setWindow(mParent->topLevelWidget());
	connect( moveJob, SIGNAL( result(KIO::Job*) ),
		this, SLOT( slotResult(KIO::Job*) ) );

}


//-FileOpMakeDirObject-------------------------------------------------------------
void FileOpMakeDirObject::operator()() {
	bool ok;
	QString newDir=KInputDialog::getText(
			i18n("Creating Folder"),
			i18n("Enter the name of the new folder:"),
			QString::null, &ok, mParent);
	if (!ok) return;

	KURL newURL(mURLList.first());
	newURL.addPath(newDir);
    KIO::Job* job=KIO::mkdir(newURL);
	job->setWindow(mParent->topLevelWidget());

	connect( job, SIGNAL(result(KIO::Job*)),
        this, SLOT( slotResult(KIO::Job*)) );
}


//-FileOpTrashObject---------------------------------------------------------------
void FileOpTrashObject::operator()() {
	KURL trashURL("trash:/");

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

	// Go do it
	if (mURLList.count()==1) {
		// If there's only one file, KIO::move will think we want to overwrite
		// the trash dir with the file to trash, so we add the file name
		trashURL.addPath(mURLList.first().fileName());
	}
	KIO::Job* job=KIO::move(mURLList,trashURL);
	job->setWindow(mParent->topLevelWidget());
	connect( job, SIGNAL( result(KIO::Job*) ),
		this, SLOT( slotResult(KIO::Job*) ) );
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
	KIO::Job* removeJob=KIO::del(mURLList,false,true);
	removeJob->setWindow(mParent->topLevelWidget());
	connect( removeJob, SIGNAL( result(KIO::Job*) ),
		this, SLOT( slotResult(KIO::Job*) ) );
}


//-FileOpRenameObject--------------------------------------------------------------
void FileOpRenameObject::operator()() {
	bool ok;
	KURL srcURL=mURLList.first();

// Prompt for the new filename
	QString filename=QStyleSheet::escape(srcURL.filename());
	mNewFilename=KInputDialog::getText(i18n("Renaming File"),
		i18n("<p>Rename file <b>%1</b> to:</p>").arg(filename),
		srcURL.filename(),
		&ok,mParent);

	if (!ok) return;

// Rename the file
	KURL destURL=srcURL;
	destURL.setFileName(mNewFilename);
	KIO::Job* job=KIO::move(srcURL,destURL);
	job->setWindow(mParent->topLevelWidget());
	connect( job, SIGNAL( result(KIO::Job*) ),
		this, SLOT( slotResult(KIO::Job*) ) );
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
