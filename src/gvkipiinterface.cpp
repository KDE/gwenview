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

// Local
#include "gvfileviewbase.h"
#include "gvfileviewstack.h"
#include "gvkipiinterface.moc"

class GVImageCollection : public KIPI::ImageCollection {
public:
	GVImageCollection(const QString& name, const QValueList<KURL>& images)
	: mName(name), mImages(images) {}
	
	QString name() const { return mName; }
	QValueList<KURL> images() const { return mImages; }
	bool valid() const { return true; }

private:
	QString mName;
	QValueList<KURL> mImages;
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


KIPI::ImageCollection* GVKIPIInterface::currentAlbum() {
	kdDebug() << "GVKIPIInterface::currentAlbum\n";
	QValueList<KURL> list;
	KFileItemListIterator it( *d->mFileView->currentFileView()->items() );
	for ( ; it.current(); ++it ) {
		list.append(it.current()->url());
	}
	return new GVImageCollection(i18n("Folder content"), list); 
}


KIPI::ImageCollection* GVKIPIInterface::currentSelection() {
	kdDebug() << "GVKIPIInterface::currentSelection\n";
	KURL::List list=d->mFileView->selectedURLs();
	return new GVImageCollection(i18n("Selected images"), list); 
}


KIPI::ImageInfo GVKIPIInterface::info( const KURL& ) {
	kdDebug() << "GVKIPIInterface::info\n";
	return KIPI::ImageInfo();
}

#endif /* HAVE_KIPI */
