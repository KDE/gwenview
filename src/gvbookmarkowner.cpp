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

// KDE includes
#include <kbookmarkmenu.h>
#include <kpopupmenu.h>
#include <kstandarddirs.h>

// Our includes
#include "gvbookmarkowner.moc"


GVBookmarkOwner::GVBookmarkOwner(QWidget* parent,KActionCollection* actionCollection)
: QObject(parent)
{
	mMenu=new KPopupMenu(parent);

	QString file = locate( "data", "gwenview/bookmarks.xml" );
	if (file.isEmpty()) {
		file = locateLocal( "data", "gwenview/bookmarks.xml" );
	}

	KBookmarkManager* manager=KBookmarkManager::managerForFile(file,false);
	manager->setUpdate(true);
	manager->setShowNSBookmarks(false);

	new KBookmarkMenu(manager, this, mMenu, actionCollection, true);
}


void GVBookmarkOwner::openBookmarkURL(const QString& strURL)
{
	KURL url(strURL);
	emit openURL(url);
}


QString GVBookmarkOwner::currentURL() const
{
	return mURL.path();
}


void GVBookmarkOwner::setURL(const KURL& url)
{
	mURL=url;
}
