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
#include <qobject.h>

// KDE includes
#include <kconfig.h>
#include <krun.h>

// Our includes
#include "fileopobject.h"
#include "fileoperation.h"


//-Configuration keys----------------------------------------------
static const char* CONFIG_CONFIRM_DELETE="confirm file delete";
static const char* CONFIG_CONFIRM_MOVE="confirm file move";
static const char* CONFIG_CONFIRM_COPY="confirm file copy";
static const char* CONFIG_DEST_DIR="destination dir";
static const char* CONFIG_EDITOR="editor";


//-Static configuration data---------------------------------------
static bool sConfirmCopy,sConfirmMove,sConfirmDelete;
static QString sDestDir,sEditor;


//-FileOperations--------------------------------------------------
void FileOperation::copyTo(const KURL::List& srcURL,QWidget* parent) {
	FileOpCopyToObject* op=new FileOpCopyToObject(srcURL,parent);
	(*op)();
}


void FileOperation::moveTo(const KURL::List& srcURL,QWidget* parent,QObject* receiver,const char* slot) {
	FileOpMoveToObject* op=new FileOpMoveToObject(srcURL,parent);
	if (receiver && slot) QObject::connect(op,SIGNAL(success()),receiver,slot);
	(*op)();
}


void FileOperation::del(const KURL::List& url,QWidget* parent,QObject* receiver,const char* slot) {
	FileOpDelObject* op=new FileOpDelObject(url,parent);
	if (receiver && slot) QObject::connect(op,SIGNAL(success()),receiver,slot);
	(*op)();
}


void FileOperation::rename(const KURL& url,QWidget* parent,QObject* receiver,const char* slot) {
	FileOpRenameObject* op=new FileOpRenameObject(url,parent);
	if (receiver && slot) QObject::connect(op,SIGNAL(renamed(const QString&)),receiver,slot);
	(*op)();
}


void FileOperation::openWithEditor(const KURL& url) {
	QString path=url.path();
	KRun::shellQuote(path);
	QString cmdLine=sEditor;
	cmdLine.append(' ').append(path);
	KRun::runCommand(cmdLine);
}


//-Configuration---------------------------------------------------
void FileOperation::readConfig(KConfig* config,const QString& group) {
	config->setGroup(group);

	sConfirmDelete=config->readBoolEntry(CONFIG_CONFIRM_DELETE,true);
	sConfirmMove=config->readBoolEntry(CONFIG_CONFIRM_MOVE,true);
	sConfirmCopy=config->readBoolEntry(CONFIG_CONFIRM_COPY,true);

	sDestDir=config->readEntry(CONFIG_DEST_DIR);
	sEditor=config->readEntry(CONFIG_EDITOR,"gimp-remote -n");
}


void FileOperation::writeConfig(KConfig* config,const QString& group) {
	config->setGroup(group);

	config->writeEntry(CONFIG_CONFIRM_DELETE,sConfirmDelete);
	config->writeEntry(CONFIG_CONFIRM_MOVE,sConfirmMove);
	config->writeEntry(CONFIG_CONFIRM_COPY,sConfirmCopy);

	config->writeEntry(CONFIG_DEST_DIR,sDestDir);
	config->writeEntry(CONFIG_EDITOR,sEditor);
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


QString FileOperation::editor() {
	return sEditor;
}

void FileOperation::setEditor(const QString& value) {
	sEditor=value;
}



