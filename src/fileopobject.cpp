/*
Gwenview - A simple image viewer for KDE
Copyright (C) 2000-2002 Aurélien Gâteau

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
#include <klineeditdlg.h>
#include <klocale.h>
#include <kmessagebox.h>

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
	KURL srcURL=mURLList.first(); // FIXME : Correct this for multi file operations

	if (FileOperation::confirmCopy()) {
		destDir=KFileDialog::getExistingDirectory(FileOperation::destDir(), mParent);
	} else {
		destDir=FileOperation::destDir();
	}
	if (destDir.isEmpty()) return;

// Copy the file
	destURL.setPath(destDir);

	KIO::Job* copyJob=KIO::copy(srcURL,destURL,true);
	connect( copyJob, SIGNAL( result(KIO::Job*) ),
		this, SLOT( slotResult(KIO::Job*) ) );

}


//-FileOpMoveToObject--------------------------------------------------------------
void FileOpMoveToObject::operator()() {
	QString destDir;
	KURL destURL;
	KURL srcURL=mURLList.first(); // FIXME : Correct this for multi file operations

	if (FileOperation::confirmMove()) {
		destDir=KFileDialog::getExistingDirectory(FileOperation::destDir(), mParent);
	} else {
		destDir=FileOperation::destDir();
	}
	if (destDir.isEmpty()) return;

// Copy the file
	destURL.setPath(destDir);

	KIO::Job* moveJob=KIO::move(srcURL,destURL,true);
	connect( moveJob, SIGNAL( result(KIO::Job*) ),
		this, SLOT( slotResult(KIO::Job*) ) );

}


//-FileOpDelObject-----------------------------------------------------------------
void FileOpDelObject::operator()() {
	KURL url=mURLList.first(); // FIXME : Correct this for multi file operations

	if (FileOperation::confirmDelete()) {
		QString filename=QStyleSheet::escape(url.filename());
		int response=KMessageBox::questionYesNo(mParent,
			"<qt>"+i18n("Are you sure you want to delete file <b>%1</b> ?").arg(filename)+"</qt>");
		if (response==KMessageBox::No) return;
	}

	KIO::Job* removeJob=KIO::del(url,false,true);
	connect( removeJob, SIGNAL( result(KIO::Job*) ),
		this, SLOT( slotResult(KIO::Job*) ) );
}


//-FileOpRenameObject--------------------------------------------------------------
void FileOpRenameObject::operator()() {
	bool ok;
	KURL srcURL=mURLList.first(); // FIXME : Correct this for multi file operations

// Prompt for the new filename
	QString filename=QStyleSheet::escape(srcURL.filename());
	mNewFilename=KLineEditDlg::getText(
		"<qt>" + i18n("Rename file <b>%1</b> to :").arg(filename) + "</qt>",
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
