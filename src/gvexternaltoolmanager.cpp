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
// Qt
#include <qdir.h>

// KDE
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kglobal.h>
#include <kservice.h>
#include <kstandarddirs.h>
#include <kurl.h>

// Local
#include "gvexternaltoolcontext.h"
#include "gvexternaltoolmanager.h"


GVExternalToolManager::GVExternalToolManager() {
	mServices.setAutoDelete(true);
	
	QStringList dirs=KGlobal::dirs()->findDirs("appdata", "tools");
	QStringList::Iterator it;
	for (it=dirs.begin(); it!=dirs.end(); ++it) {
		loadDesktopFiles(*it);
	}
}


void GVExternalToolManager::loadDesktopFiles(const QString& dirString) {
	//TODO: Let the user hide some desktop files
	QDir dir(dirString);
	QStringList list=dir.entryList("*.desktop");
	QStringList::Iterator it=list.begin();
	for (; it!=list.end(); ++it) {
		KDesktopFile df( dir.filePath(*it) );
		KService* service=new KService(&df);
		mServices.append(service);
	}
}


GVExternalToolManager* GVExternalToolManager::instance() {
	static GVExternalToolManager manager;
	return &manager;
}


GVExternalToolContext* GVExternalToolManager::createContext(
	QObject* parent, const KFileItemList* items)
{
	KURL::List urls;
	QPtrListIterator<KFileItem> it(*items);
	for (; it.current(); ++it) {
		urls.append(it.current()->url());
	}

	//TODO: Add code to filter services
	QPtrList<KService> services(mServices);
	return new GVExternalToolContext(parent, services, urls);
}

