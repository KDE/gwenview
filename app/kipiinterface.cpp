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
#include "gvcore/archive.h"
#include "gvcore/fileviewbase.h"
#include "gvcore/fileviewcontroller.h"
#include "imageutils/jpegcontent.h"
#include "kipiinterface.moc"
namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

class ImageCollection : public KIPI::ImageCollectionShared {
public:
	ImageCollection(KURL dirURL, const QString& name, const KURL::List& images)
	: KIPI::ImageCollectionShared()
    , mDirURL(dirURL)
    , mName(name)
    , mImages(images) {}

	QString name()           { return mName; }
	QString comment()        { return QString::null; }
	KURL::List images()      { return mImages; }
	KURL uploadRoot()        { return KURL("/"); }
	KURL uploadPath()        { return mDirURL; }
	QString uploadRootName() { return "/"; }

private:
    KURL mDirURL;
	QString mName;
	KURL::List mImages;
};



class ImageInfo : public KIPI::ImageInfoShared {
public:
	ImageInfo(KIPI::Interface* interface, const KURL& url) : KIPI::ImageInfoShared(interface, url) {}

	QString title() {
		return _url.fileName();
	}

	QString description() {
		if (!_url.isLocalFile()) return QString::null;

		ImageUtils::JPEGContent content;
		bool ok=content.load(_url.path());
		if (!ok) return QString::null;

		return content.comment();
	}

	void setDescription(const QString&) {}

	QMap<QString,QVariant> attributes() {
		return QMap<QString,QVariant>();
	}

	void clearAttributes() {}

	void addAttributes(const QMap<QString, QVariant>&) {}
};



struct KIPIInterfacePrivate {
	FileViewController* mFileView;
};


KIPIInterface::KIPIInterface( QWidget* parent, FileViewController* fileView)
:KIPI::Interface(parent, "Gwenview kipi interface") {
	d=new KIPIInterfacePrivate;
	d->mFileView=fileView;

	connect(d->mFileView, SIGNAL(selectionChanged()),
		this, SLOT(slotSelectionChanged()) );

	connect(d->mFileView, SIGNAL(completed()),
		this, SLOT(slotDirectoryChanged()) );
}


KIPIInterface::~KIPIInterface() {
	delete d;
}


KIPI::ImageCollection KIPIInterface::currentAlbum() {
	LOG("");
	KURL::List list;
	KFileItemListIterator it( *d->mFileView->currentFileView()->items() );
	for ( ; it.current(); ++it ) {
		KFileItem* item=it.current();
		if (!Archive::fileItemIsDirOrArchive(item)) {
			list.append(it.current()->url());
		}
	}
	KURL url=d->mFileView->dirURL();
	return KIPI::ImageCollection(new ImageCollection(url, url.fileName(), list));
}


KIPI::ImageCollection KIPIInterface::currentSelection() {
	LOG("");
	KURL::List list=d->mFileView->selectedImageURLs();
	KURL url=d->mFileView->dirURL();
	return KIPI::ImageCollection(new ImageCollection(url, i18n("%1 (Selected Images)").arg(url.fileName()), list));
}


QValueList<KIPI::ImageCollection> KIPIInterface::allAlbums() {
	LOG("");
	QValueList<KIPI::ImageCollection> list;
	list << currentAlbum() << currentSelection();
	return list;
}


KIPI::ImageInfo KIPIInterface::info(const KURL& url) {
	LOG("");
	return KIPI::ImageInfo( new ImageInfo(this, url) );
}

int KIPIInterface::features() const {
	return KIPI::AcceptNewImages;
}

/**
 * We don't need to do anything here, the KDirLister will pick up the image if
 * necessary
 */
bool KIPIInterface::addImage(const KURL&, QString&) {
	return true;
}

// TODO currently KDirWatch doesn't have watching of files in a directory
// implemented, so KDirLister will not inform when a file changes
void KIPIInterface::refreshImages( const KURL::List& urls ) {
	d->mFileView->refreshItems( urls );
}


void KIPIInterface::slotSelectionChanged() {
	emit selectionChanged(d->mFileView->selectionSize() > 0);
}


void KIPIInterface::slotDirectoryChanged() {
	emit currentAlbumChanged(d->mFileView->fileCount() > 0);
}


} // namespace

#endif /* GV_HAVE_KIPI */
