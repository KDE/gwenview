// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright (c) 2000-2003 Aurélien Gâteau
 
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
// KDE
#include <kaction.h>
#include <kpopupmenu.h>
#include <kservice.h>

// Local
#include "gvexternaltoolaction.h"

#include "gvexternaltoolcontext.moc"

GVExternalToolContext::GVExternalToolContext(
	QObject* parent,
	QPtrList<KService> services,
	KURL::List urls)
: QObject(parent)
, mServices(services)
, mURLs(urls)
{}


QPopupMenu* GVExternalToolContext::popupMenu() {
	KActionMenu* actionMenu=new KActionMenu(this);
	QPtrListIterator<KService> it(mServices);
	for (;it.current(); ++it) {
		GVExternalToolAction* action=
			new GVExternalToolAction(this, it.current(), mURLs);
		actionMenu->insert(action);
	}
	
	return actionMenu->popupMenu();
}
