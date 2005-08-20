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
#include "externaltoolcontext.moc"

// KDE
#include <kaction.h>
#include <kapplication.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kpopupmenu.h>
#include <krun.h>
#include <kservice.h>

// Local
#include "externaltoolaction.h"
#include "externaltooldialog.h"

namespace Gwenview {

ExternalToolContext::ExternalToolContext(
	QObject* parent,
	std::list<KService*> services,
	KURL::List urls)
: QObject(parent)
, mServices(services)
, mURLs(urls)
{}


void ExternalToolContext::showExternalToolDialog() {
	ExternalToolDialog* dialog=new ExternalToolDialog(kapp->mainWidget());
	dialog->show();
}


void ExternalToolContext::showOpenWithDialog() {
	KRun::displayOpenWithDialog(mURLs, false /*tempFiles*/);
}


QPopupMenu* ExternalToolContext::popupMenu() {
	QPopupMenu* menu=new QPopupMenu();
	std::list<KService*>::const_iterator it=mServices.begin();
	std::list<KService*>::const_iterator itEnd=mServices.end();
	for (;it!=itEnd; ++it) {
		ExternalToolAction* action=
			new ExternalToolAction(this, *it, mURLs);
		action->plug(menu);
	}

	menu->insertSeparator();
	menu->insertItem(i18n("Other..."),
		this, SLOT(showOpenWithDialog()) );
	menu->insertItem(
		SmallIcon("configure"),
		i18n("Configure External Tools..."), 
		this, SLOT(showExternalToolDialog()) );
	
	return menu;
}

} // namespace
