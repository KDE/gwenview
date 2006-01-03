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
// Self
#include "fileoperation.moc"

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
#include "fileoperationconfig.h"

namespace Gwenview {


namespace FileOperation {

void copyTo(const KURL::List& srcURL,QWidget* parent) {
	FileOpObject* op=new FileOpCopyToObject(srcURL,parent);
	(*op)();
}

void linkTo(const KURL::List& srcURL,QWidget* parent) {
	FileOpObject* op=new FileOpLinkToObject(srcURL,parent);
	(*op)();
}

void moveTo(const KURL::List& srcURL,QWidget* parent,QObject* receiver,const char* slot) {
	FileOpObject* op=new FileOpMoveToObject(srcURL,parent);
	if (receiver && slot) QObject::connect(op,SIGNAL(success()),receiver,slot);
	(*op)();
}


void del(const KURL::List& url,QWidget* parent,QObject* receiver,const char* slot) {
	FileOpObject* op;
	if (FileOperationConfig::deleteToTrash()) {
		op=new FileOpTrashObject(url,parent);
	} else {
		op=new FileOpRealDeleteObject(url,parent);
	}
	if (receiver && slot) QObject::connect(op,SIGNAL(success()),receiver,slot);
	(*op)();
}


void rename(const KURL& url,QWidget* parent,QObject* receiver,const char* slot) {
	FileOpObject* op=new FileOpRenameObject(url,parent);
	if (receiver && slot) QObject::connect(op,SIGNAL(renamed(const QString&)),receiver,slot);
	(*op)();
}


void fillDropURLMenu(QPopupMenu* menu, const KURL::List& urls, const KURL& target, bool* wasMoved) {
	DropMenuContext* context=new DropMenuContext(menu, urls, target, wasMoved);
	menu->insertItem( SmallIcon("goto"), i18n("&Move Here"),
		context, SLOT(move()) );
	menu->insertItem( SmallIcon("editcopy"), i18n("&Copy Here"),
		context, SLOT(copy()) );
	menu->insertItem( SmallIcon("www"), i18n("&Link Here"),
		context, SLOT(link()) );
}


void openDropURLMenu(QWidget* parent, const KURL::List& urls, const KURL& target, bool* wasMoved) {
	QPopupMenu menu(parent);
	if (wasMoved) *wasMoved=false;

	fillDropURLMenu(&menu, urls, target, wasMoved);
	menu.insertSeparator();
	menu.insertItem( SmallIcon("cancel"), i18n("Cancel") );

	menu.setMouseTracking(true);
	menu.exec(QCursor::pos());
}


} // namespace FileOperation

} // namespace
