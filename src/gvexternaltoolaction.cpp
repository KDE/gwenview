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
#include <krun.h>
#include <kservice.h>

// Local
#include "gvexternaltoolaction.moc"

GVExternalToolAction::GVExternalToolAction(
	QObject* parent, const KService* service,
	const KURL::List& urls)
: KAction(parent)
, mService(service)
, mURLs(urls)
{
	setText(service->name());
	setIcon(service->icon());
	connect(this, SIGNAL(activated()),
		this, SLOT(openExternalTool()) );
			
}


void GVExternalToolAction::openExternalTool() {
	KRun::run(*mService, mURLs, false);
}
