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
#include <config.h>
#ifdef GV_HAVE_KIPI

// Qt
#include <qdir.h>

// KDE
#include <kdebug.h>
#include <kfileitem.h>
#include <klocale.h>
#include <kurl.h>

// KIPI
#include <libkipi/imagecollectionshared.h>
#include <libkipi/imageinfoshared.h>

// Local
#include "gvfileviewbase.h"
#include "gvfileviewstack.h"
#include "gvkipiinterface.moc"

//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

class GVImageCollection : public KIPI::ImageCollectionShared {
public:
	GVImageCollection(const QString& name, const KURL::List& images)
	: KIPI::ImageCollectionShared(), mName(name), mImages(images) {}

	QString name() { return mName; }
	QString comment() { return QString::null; }

	KURL::List images() { return mImages; }
	// FIXME: Return current URL instead
	KURL uploadPath() {
		KURL url;
		url.setPath(QDir::homeDirPath());
		return url;
	}

private:
	QString mName;
	KURL::List mImages;
};



class GVImageInfo : public KIPI::ImageInfoShared {
public:
	GVImageInfo(KIPI::Interface* interface, const KURL& url) : KIPI::ImageInfoShared(interface, url) {}

	QString title() {
		return _url.fileName();
	}

	QString description() {
		return QString::null;
	}

	void setDescription(const QString&) {}

	QMap<QString,QVariant> attributes() {
		return QMap<QString,QVariant>();
	}

	void clearAttributes() {}

	void addAttributes(const QMap<QString, QVariant>&) {}
};



struct GVKIPIInterfacePrivate {
	GVFileViewStack* mFileView;
};


GVKIPIInterface::GVKIPIInterface( QWidget* parent, GVFileViewStack* fileView)
:KIPI::Interface(parent, "Gwenview kipi interface") {
	d=new GVKIPIInterfacePrivate;
	d->mFileView=fileView;

	connect(d->mFileView, SIGNAL(selectionChanged()),
		this, SLOT(slotSelectionChanged()) );

	connect(d->mFileView, SIGNAL(completedURLListing(const KURL&)),
		this, SLOT(slotDirectoryChanged()) );
}


GVKIPIInterface::~GVKIPIInterface() {
	delete d;
}


KIPI::ImageCollection GVKIPIInterface::currentAlbum() {
	LOG("");
	KURL::List list;
	KFileItemListIterator it( *d->mFileView->currentFileView()->items() );
	for ( ; it.current(); ++it ) {
		list.append(it.current()->url());
	}
	return KIPI::ImageCollection(new GVImageCollection(i18n("Folder Content"), list));
}


KIPI::ImageCollection GVKIPIInterface::currentSelection() {
	LOG("");
	KURL::List list=d->mFileView->selectedURLs();
	return KIPI::ImageCollection(new GVImageCollection(i18n("Selected Images"), list));
}


QValueList<KIPI::ImageCollection> GVKIPIInterface::allAlbums() {
	LOG("");
	QValueList<KIPI::ImageCollection> list;
	list << currentAlbum() << currentSelection();
	return list;
}


KIPI::ImageInfo GVKIPIInterface::info(const KURL& url) {
	LOG("");
	return KIPI::ImageInfo( new GVImageInfo(this, url) );
}

int GVKIPIInterface::features() const {
	return KIPI::AcceptNewImages;
}

/**
 * We don't need to do anything here, the KDirLister will pick up the image if
 * necessary
 */
bool GVKIPIInterface::addImage(const KURL&, QString&) {
	return true;
}

// TODO currently KDirWatch doesn't have watching of files in a directory
// implemented, so KDirLister will not inform when a file changes
void GVKIPIInterface::refreshImages( const KURL::List& urls ) {
	d->mFileView->refreshItems( urls );
}


void GVKIPIInterface::slotSelectionChanged() {
	emit selectionChanged(d->mFileView->selectionSize() > 0);
}


void GVKIPIInterface::slotDirectoryChanged() {
	emit currentAlbumChanged(d->mFileView->fileCount() > 0);
}


#endif /* GV_HAVE_KIPI */
