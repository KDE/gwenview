// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aurélien Gâteau

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
#include <qcursor.h>
#include <qpopupmenu.h>
#include <qobject.h>

// KDE
#include <kconfig.h>
#include <kiconloader.h>
#include <klocale.h>

// Local
#include "fileopobject.h"
#include "fileoperation.h"


//-Configuration keys----------------------------------------------
static const char CONFIG_DELETE_TO_TRASH[] = "delete to trash";
static const char CONFIG_CONFIRM_DELETE[]  = "confirm file delete";
static const char CONFIG_CONFIRM_MOVE[]    = "confirm file move";
static const char CONFIG_CONFIRM_COPY[]    = "confirm file copy";
static const char CONFIG_DEST_DIR[]        = "destination dir";


//-Static configuration data---------------------------------------
static bool sDeleteToTrash,sConfirmCopy,sConfirmMove,sConfirmDelete;
static QString sDestDir,sEditor;


//-FileOperations--------------------------------------------------
void FileOperation::copyTo(const KURL::List& srcURL,QWidget* parent) {
	FileOpObject* op=new FileOpCopyToObject(srcURL,parent);
	(*op)();
}


void FileOperation::moveTo(const KURL::List& srcURL,QWidget* parent,QObject* receiver,const char* slot) {
	FileOpObject* op=new FileOpMoveToObject(srcURL,parent);
	if (receiver && slot) QObject::connect(op,SIGNAL(success()),receiver,slot);
	(*op)();
}


void FileOperation::del(const KURL::List& url,QWidget* parent,QObject* receiver,const char* slot) {
	FileOpObject* op;
	if (sDeleteToTrash) {
		op=new FileOpTrashObject(url,parent);
	} else {
		op=new FileOpRealDeleteObject(url,parent);
	}
	if (receiver && slot) QObject::connect(op,SIGNAL(success()),receiver,slot);
	(*op)();
}


void FileOperation::rename(const KURL& url,QWidget* parent,QObject* receiver,const char* slot) {
	FileOpObject* op=new FileOpRenameObject(url,parent);
	if (receiver && slot) QObject::connect(op,SIGNAL(renamed(const QString&)),receiver,slot);
	(*op)();
}


void FileOperation::openDropURLMenu(QWidget* parent, const KURL::List& urls, const KURL& target, bool* wasMoved) {
	QPopupMenu menu(parent);
	if (wasMoved) *wasMoved=false;

	int copyItemID = menu.insertItem( SmallIcon("editcopy"), i18n("&Copy Here") );
	int moveItemID = menu.insertItem( i18n("&Move Here") );
	menu.insertSeparator();
	menu.insertItem( SmallIcon("cancel"), i18n("Cancel") );

	menu.setMouseTracking(true);
	int id = menu.exec(QCursor::pos());

	// Handle menu choice
	if (id==copyItemID) {
		KIO::copy(urls, target, true);
	} else if (id==moveItemID) {
		KIO::move(urls, target, true);
		if (wasMoved) *wasMoved=true;
	}
}


//-Configuration---------------------------------------------------
void FileOperation::readConfig(KConfig* config,const QString& group) {
	config->setGroup(group);

	sDeleteToTrash=config->readBoolEntry(CONFIG_DELETE_TO_TRASH,true);
	sConfirmDelete=config->readBoolEntry(CONFIG_CONFIRM_DELETE,true);
	sConfirmMove=config->readBoolEntry(CONFIG_CONFIRM_MOVE,true);
	sConfirmCopy=config->readBoolEntry(CONFIG_CONFIRM_COPY,true);

	sDestDir=config->readPathEntry(CONFIG_DEST_DIR);
}


void FileOperation::kpartConfig() {
	sDeleteToTrash=true;
	sConfirmDelete=true;
	sConfirmMove=true;
	sConfirmCopy=true;
}


void FileOperation::writeConfig(KConfig* config,const QString& group) {
	config->setGroup(group);

	config->writeEntry(CONFIG_DELETE_TO_TRASH,sDeleteToTrash);
	config->writeEntry(CONFIG_CONFIRM_DELETE,sConfirmDelete);
	config->writeEntry(CONFIG_CONFIRM_MOVE,sConfirmMove);
	config->writeEntry(CONFIG_CONFIRM_COPY,sConfirmCopy);

	config->writePathEntry(CONFIG_DEST_DIR,sDestDir);
}



bool FileOperation::deleteToTrash() {
	return sDeleteToTrash;
}

void FileOperation::setDeleteToTrash(bool value) {
	sDeleteToTrash=value;
}


bool FileOperation::confirmDelete() {
	return sConfirmDelete;
}

void FileOperation::setConfirmDelete(bool value) {
	sConfirmDelete=value;
}


bool FileOperation::confirmCopy() {
	return sConfirmCopy;
}

void FileOperation::setConfirmCopy(bool value) {
	sConfirmCopy=value;
}


bool FileOperation::confirmMove() {
	return sConfirmMove;
}

void FileOperation::setConfirmMove(bool value) {
	sConfirmMove=value;
}


QString FileOperation::destDir() {
	return sDestDir;
}

void FileOperation::setDestDir(const QString& value) {
	sDestDir=value;
}


