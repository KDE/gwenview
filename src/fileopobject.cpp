/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2003 Aurélien Gâteau

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

// Qt includes
#include <qstylesheet.h>
#include <qwidget.h>

// KDE includes
#include <kfiledialog.h>
#include <kglobalsettings.h>
#include <klineeditdlg.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>

// Our includes
#include "fileoperation.h"
#include "fileopobject.moc"


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
	QString destDir;
	KURL destURL;

	if (FileOperation::confirmCopy()) {
		destDir=KFileDialog::getExistingDirectory(FileOperation::destDir(), mParent);
	} else {
		destDir=FileOperation::destDir();
	}
	if (destDir.isEmpty()) return;
	destURL.setPath(destDir);

// Copy the file
	KIO::Job* copyJob=KIO::copy(mURLList,destURL,true);
	connect( copyJob, SIGNAL( result(KIO::Job*) ),
		this, SLOT( slotResult(KIO::Job*) ) );

}


//-FileOpMoveToObject--------------------------------------------------------------
void FileOpMoveToObject::operator()() {
	QString destDir;
	KURL destURL;

	if (FileOperation::confirmMove()) {
		destDir=KFileDialog::getExistingDirectory(FileOperation::destDir(), mParent);
	} else {
		destDir=FileOperation::destDir();
	}
	if (destDir.isEmpty()) return;

// Copy the file
	destURL.setPath(destDir);

	KIO::Job* moveJob=KIO::move(mURLList,destURL,true);
	connect( moveJob, SIGNAL( result(KIO::Job*) ),
		this, SLOT( slotResult(KIO::Job*) ) );

}


//-FileOpTrashObject---------------------------------------------------------------
void FileOpTrashObject::operator()() {
	// Get the trash path (and make sure it exists)
	QString trashPath=KGlobalSettings::trashPath();
	if ( !QFile::exists(trashPath) ) {
		KStandardDirs::makeDir( QFile::encodeName(trashPath) );
	}
	KURL trashURL;
	trashURL.setPath(trashPath);

	// Check we don't want to trash the trash
	KURL::List::ConstIterator it=mURLList.begin();
	for (; it!=mURLList.end(); ++it) {
		if ( (*it).isLocalFile() && (*it).path(1)==trashPath ) {
			KMessageBox::sorry(0, i18n("You can't trash the trash bin."));
			return;
		}
	}

	// Confirm operation
	if (FileOperation::confirmDelete()) {
		int response;
		if (mURLList.count()>1) {
			QStringList fileList;
			KURL::List::ConstIterator it=mURLList.begin();
			for (; it!=mURLList.end(); ++it) {
				fileList.append((*it).filename());
			}
			response=KMessageBox::questionYesNoList(mParent,
				i18n("Do you really want to trash these files?"),fileList);
		} else {
			QString filename=QStyleSheet::escape(mURLList.first().filename());
			response=KMessageBox::questionYesNo(mParent,
				i18n("<p>Do you really want to move <b>%1</b> to the trash?</p>").arg(filename));
		}
		if (response==KMessageBox::No) return;
	}

	// Go do it
	KIO::Job* job=KIO::move(mURLList,trashURL);
	connect( job, SIGNAL( result(KIO::Job*) ),
		this, SLOT( slotResult(KIO::Job*) ) );
}

//-FileOpRealDeleteObject----------------------------------------------------------
void FileOpRealDeleteObject::operator()() {
	// Confirm operation
	if (FileOperation::confirmDelete()) {
		int response;
		if (mURLList.count()>1) {
			QStringList fileList;
			KURL::List::ConstIterator it=mURLList.begin();
			for (; it!=mURLList.end(); ++it) {
				fileList.append((*it).filename());
			}
			response=KMessageBox::questionYesNoList(mParent,
				i18n("Do you really want to delete these files?"),fileList);
		} else {
			QString filename=QStyleSheet::escape(mURLList.first().filename());
			response=KMessageBox::questionYesNo(mParent,
				i18n("<p>Do you really want to delete <b>%1</b>?</p>").arg(filename));
		}
		if (response==KMessageBox::No) return;
	}
	
	// Delete the file
	KIO::Job* removeJob=KIO::del(mURLList,false,true);
	connect( removeJob, SIGNAL( result(KIO::Job*) ),
		this, SLOT( slotResult(KIO::Job*) ) );
}


//-FileOpRenameObject--------------------------------------------------------------
void FileOpRenameObject::operator()() {
	bool ok;
	KURL srcURL=mURLList.first();

// Prompt for the new filename
	QString filename=QStyleSheet::escape(srcURL.filename());
	mNewFilename=KLineEditDlg::getText(
		i18n("<p>Rename file <b>%1</b> to:</p>").arg(filename),
		srcURL.filename(),
		&ok,mParent);

	if (!ok) return;

// Rename the file
	KURL destURL=srcURL;
	destURL.setFileName(mNewFilename);
	KIO::Job* job=KIO::move(srcURL,destURL);
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
