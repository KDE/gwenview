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


inline bool mimeTypeMatches(const QString& candidate, const QString& reference) {
	if (reference=="*") return true;

	if (reference.right(1)=="*") {
		return candidate.startsWith(reference.left(reference.length()-1));
	} else {
		return candidate==reference;
	}
}

inline bool isSubSetOf(const QStringList& subSet, const QStringList& set) {
	// Simple implementation, might need some optimization
	QStringList::ConstIterator itSubSet=subSet.begin();
	QStringList::ConstIterator itSetBegin=set.begin();
	QStringList::ConstIterator itSetEnd=set.end();

	for (; itSubSet!=subSet.end(); ++itSubSet) {
		bool matchFound=false;
		QStringList::ConstIterator itSet=itSetBegin;
		for (; itSet!=itSetEnd; ++itSet) {
			if (mimeTypeMatches(*itSubSet, *itSet)) {
				matchFound=true;
				break;
			}
		}
		if (!matchFound) {
			return false;
		}
	}
	return true;
}


GVExternalToolContext* GVExternalToolManager::createContext(
	QObject* parent, const KFileItemList* items)
{
	KURL::List urls;
	QStringList mimeTypes;

	// Create our URL list and a list of the different mime types present in
	// the selection
	QPtrListIterator<KFileItem> it(*items);
	for (; it.current(); ++it) {
		urls.append(it.current()->url());
		QString mimeType=it.current()->mimetype();
		if (!mimeTypes.contains(mimeType)) {
			mimeTypes.append(mimeType);
		}
	}

	return createContextInternal(parent, urls, mimeTypes);
}


GVExternalToolContext* GVExternalToolManager::createContext(
	QObject* parent, const KURL& url)
{
	KURL::List urls;
	QStringList mimeTypes;
	
	urls.append(url);
	QString mimeType=KMimeType::findByURL(url, 0, url.isLocalFile(), true)->name();
	mimeTypes.append(mimeType);
	
	return createContextInternal(parent, urls, mimeTypes);
}

	
GVExternalToolContext* GVExternalToolManager::createContextInternal(
	QObject* parent, const KURL::List& urls, const QStringList& mimeTypes)
{
	bool onlyOneURL=urls.size()==1;
	
	// Only add to selectionServices the services which can handle all the
	// different mime types present in the selection
	QPtrList<KService> selectionServices;
	QPtrListIterator<KService> it(mServices);
	for (; it.current(); ++it) {
		KService* service=it.current();
		if (!onlyOneURL && !service->allowMultipleFiles()) {
			continue;
		}
		
		QStringList serviceTypes=service->serviceTypes();
		if (isSubSetOf(mimeTypes, serviceTypes)) {
			selectionServices.append(service);
		}
	}
	
	return new GVExternalToolContext(parent, selectionServices, urls);
}

