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
#ifdef HAVE_KIPI

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

class GVImageCollection : public KIPI::ImageCollectionShared {
public:
	GVImageCollection(const QString& name, const KURL::List& images)
	: KIPI::ImageCollectionShared(), mName(name), mImages(images) {}

	QString name() { return mName; }
	QString comment() { return QString::null; }
	
	KURL::List images() { return mImages; }

private:
	QString mName;
	KURL::List mImages;
};



class GVImageInfo : public KIPI::ImageInfoShared {
public:
	GVImageInfo( const KURL& url ) : KIPI::ImageInfoShared(url) {}
	QString name() {
		return _url.fileName();
	}
	
	void setName(const QString&) {}
	
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
}


GVKIPIInterface::~GVKIPIInterface() {
	delete d;
}


KIPI::ImageCollection GVKIPIInterface::currentAlbum() {
	kdDebug() << "GVKIPIInterface::currentAlbum\n";
	KURL::List list;
	KFileItemListIterator it( *d->mFileView->currentFileView()->items() );
	for ( ; it.current(); ++it ) {
		list.append(it.current()->url());
	}
	return KIPI::ImageCollection(new GVImageCollection(i18n("Folder content"), list)); 
}


KIPI::ImageCollection GVKIPIInterface::currentSelection() {
	kdDebug() << "GVKIPIInterface::currentSelection\n";
	KURL::List list=d->mFileView->selectedURLs();
	return KIPI::ImageCollection(new GVImageCollection(i18n("Selected images"), list)); 
}


QValueList<KIPI::ImageCollection> GVKIPIInterface::allAlbums() {
	kdDebug() << "GVKIPIInterface::allAlbums\n";
	QValueList<KIPI::ImageCollection> list;
	list << currentAlbum() << currentSelection();
	return list;
}


KIPI::ImageInfo GVKIPIInterface::info(const KURL& url) {
	kdDebug() << "GVKIPIInterface::info\n";
	return KIPI::ImageInfo( new GVImageInfo(url) );
}

int GVKIPIInterface::features() const {
	return KIPI::AlbumEQDir | KIPI::AcceptNewImages; 
}

#endif /* HAVE_KIPI */
