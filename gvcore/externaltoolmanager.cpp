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
// STL
#include <list>

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
#include "externaltoolcontext.h"
#include "externaltoolmanager.h"
namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

// Helper functions for createContextInternal
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


struct ExternalToolManagerPrivate {
	QDict<KDesktopFile> mDesktopFiles;
	QPtrList<KService> mServices;
	QString mUserToolDir;

	/**
	 * Helper function for createContextInternal
	 */
	static bool compareKServicePtrByName(const KService* s1, const KService* s2) {
		Q_ASSERT(s1);
		Q_ASSERT(s2);
		return s1->name() < s2->name();
	}

	ExternalToolContext* createContextInternal(
		QObject* parent, const KURL::List& urls, const QStringList& mimeTypes)
	{
		bool onlyOneURL=urls.size()==1;
		
		// Only add to selectionServices the services which can handle all the
		// different mime types present in the selection
		//
		// We use std::list instead of QValueList because it's not possible to
		// pass a sort functor to qHeapSort
		std::list<KService*> selectionServices;
		QPtrListIterator<KService> it(mServices);
		for (; it.current(); ++it) {
			KService* service=it.current();
			if (!onlyOneURL && !service->allowMultipleFiles()) {
				continue;
			}
			
			QStringList serviceTypes=service->serviceTypes();
			if (isSubSetOf(mimeTypes, serviceTypes)) {
				selectionServices.push_back(service);
			}
		}
		selectionServices.sort(compareKServicePtrByName);
		
		return new ExternalToolContext(parent, selectionServices, urls);
	}

};


// Helper function for ctor
void loadDesktopFiles(QDict<KDesktopFile>& dict, const QString& dirString) {
	QDir dir(dirString);
	QStringList list=dir.entryList("*.desktop");
	QStringList::ConstIterator it=list.begin();
	for (; it!=list.end(); ++it) {
		KDesktopFile* df=new KDesktopFile( dir.filePath(*it) );
		dict.insert(*it, df);
	}
}

inline QString addSlash(const QString& _str) {
	QString str(_str);
	if (str.right(1)!="/") str.append('/');
	return str;
}

ExternalToolManager::ExternalToolManager() {
	d=new ExternalToolManagerPrivate;

	// Getting dirs
	d->mUserToolDir=KGlobal::dirs()->saveLocation("appdata", "tools");
	d->mUserToolDir=addSlash(d->mUserToolDir);
	Q_ASSERT(!d->mUserToolDir.isEmpty());
	LOG("d->mUserToolDir:" << d->mUserToolDir);
	
	QStringList dirs=KGlobal::dirs()->findDirs("appdata", "tools");
	LOG("dirs:" << dirs.join(","));

	// Loading desktop files
	QDict<KDesktopFile> systemDesktopFiles;
	QStringList::ConstIterator it;
	for (it=dirs.begin(); it!=dirs.end(); ++it) {
		if (addSlash(*it)==d->mUserToolDir) {
			LOG("skipping " << *it);
			continue;
		}
		LOG("loading system desktop files from " << *it);
		loadDesktopFiles(systemDesktopFiles, *it);
	}
	QDict<KDesktopFile> userDesktopFiles;
	loadDesktopFiles(userDesktopFiles, d->mUserToolDir);

	// Merge system and user desktop files into our KDesktopFile dictionary
	d->mDesktopFiles=systemDesktopFiles;
	d->mDesktopFiles.setAutoDelete(true);
	QDictIterator<KDesktopFile> itDict(userDesktopFiles);
	
	for (; itDict.current(); ++itDict) {
		QString name=itDict.currentKey();
		KDesktopFile* df=itDict.current();
		if (d->mDesktopFiles.find(name)) {
			d->mDesktopFiles.remove(name);
		}
		if (df->readBoolEntry("Hidden")) {
			delete df;
		} else {
			d->mDesktopFiles.insert(name, df);
		}
	}

	d->mServices.setAutoDelete(true);
	updateServices();
}


ExternalToolManager::~ExternalToolManager() {
	delete d;
}

	
ExternalToolManager* ExternalToolManager::instance() {
	static ExternalToolManager manager;
	return &manager;
}


void ExternalToolManager::updateServices() {
	d->mServices.clear();
	QDictIterator<KDesktopFile> it(d->mDesktopFiles);
	for (; it.current(); ++it) {
		KDesktopFile* desktopFile=it.current();
		// If sync() is not called, KService does not read up to date content
		desktopFile->sync();
		KService* service=new KService(desktopFile);
		d->mServices.append(service);
	}
}


QDict<KDesktopFile>& ExternalToolManager::desktopFiles() const {
	return d->mDesktopFiles;
}


void ExternalToolManager::hideDesktopFile(KDesktopFile* desktopFile) {
	QFileInfo fi(desktopFile->fileName());
	QString name=QString("%1.desktop").arg( fi.baseName(true) );
	d->mDesktopFiles.take(name);
	
	if (desktopFile->isReadOnly()) {
		delete desktopFile;
		desktopFile=new KDesktopFile(d->mUserToolDir + "/" + name, false);
	}
	desktopFile->writeEntry("Hidden", true);
	desktopFile->sync();
	delete desktopFile;
}


KDesktopFile* ExternalToolManager::editSystemDesktopFile(const KDesktopFile* desktopFile) {
	Q_ASSERT(desktopFile);
	QFileInfo fi(desktopFile->fileName());

	QString name=fi.baseName(true);
	d->mDesktopFiles.remove(QString("%1.desktop").arg(name));
	
	return createUserDesktopFile(name);
}


KDesktopFile* ExternalToolManager::createUserDesktopFile(const QString& name) {
	Q_ASSERT(!name.isEmpty());
	KDesktopFile* desktopFile=new KDesktopFile(
		d->mUserToolDir + "/" + name + ".desktop", false);
	d->mDesktopFiles.insert(QString("%1.desktop").arg(name), desktopFile);	

	return desktopFile;
}


ExternalToolContext* ExternalToolManager::createContext(
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

	return d->createContextInternal(parent, urls, mimeTypes);
}


ExternalToolContext* ExternalToolManager::createContext(
	QObject* parent, const KURL& url)
{
	KURL::List urls;
	QStringList mimeTypes;
	
	urls.append(url);
	QString mimeType=KMimeType::findByURL(url, 0, url.isLocalFile(), true)->name();
	mimeTypes.append(mimeType);
	
	return d->createContextInternal(parent, urls, mimeTypes);
}


} // namespace
