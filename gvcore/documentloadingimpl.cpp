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

#include "documentloadingimpl.moc"

// Qt

// KDE
#include <kmimetype.h>

// Local
#include "imageloader.h"
#include "documentotherloadedimpl.h"
#include "documentanimatedloadedimpl.h"
#include "documentloadedimpl.h"
#include "documentjpegloadedimpl.h"
#include "mimetypeutils.h"

namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

//---------------------------------------------------------------------
//
// DocumentLoadingImplPrivate
//
//---------------------------------------------------------------------

class DocumentLoadingImplPrivate {
public:
	DocumentLoadingImplPrivate()
	: mLoader( NULL )
	{}

	ImageLoader* mLoader;
};

//---------------------------------------------------------------------
//
// DocumentLoadingImpl
//
//---------------------------------------------------------------------
DocumentLoadingImpl::DocumentLoadingImpl(Document* document) 
: DocumentImpl(document) {
	LOG("");
	d=new DocumentLoadingImplPrivate;
}


DocumentLoadingImpl::~DocumentLoadingImpl() {
	LOG("");
	delete d;
}


void DocumentLoadingImpl::init() {
	MimeTypeUtils::Kind kind=MimeTypeUtils::urlKind(mDocument->url());
	if (kind==MimeTypeUtils::KIND_FILE) {
		switchToImpl(new DocumentOtherLoadedImpl(mDocument));
		return;
	}
	d->mLoader = ImageLoader::loader( mDocument->url(), this, BUSY_LOADING );
	connect( d->mLoader, SIGNAL( sizeLoaded( int, int )), SLOT( sizeLoaded( int, int )));
	connect( d->mLoader, SIGNAL( imageChanged( const QRect& )), SLOT( imageChanged( const QRect& )));
	connect( d->mLoader, SIGNAL( frameLoaded()), SLOT( frameLoaded()));
	connect( d->mLoader, SIGNAL( imageLoaded( bool )), SLOT( imageLoaded( bool )));

	// it's possible the loader already has the whole or at least part of the image loaded
	QSize s = d->mLoader->knownSize();
	if( !s.isEmpty()) {
		if( d->mLoader->frames().count() > 0 ) {
			setImage( d->mLoader->frames().first().image, false );
			emit sizeUpdated( s.width(), s.height());
			emit rectUpdated( QRect( QPoint( 0, 0 ), s ));
		} else {
			setImage( d->mLoader->processedImage(), false );
			emit sizeUpdated( s.width(), s.height());
			QMemArray< QRect > rects = d->mLoader->loadedRegion().rects();
			for( unsigned int i = 0; i < rects.count(); ++i ) {
				emit rectUpdated( rects[ i ] );
			}
		}
	}
	if( d->mLoader->completed()) emit imageLoaded( d->mLoader->frames().count() != 0 );
	// this may be deleted here
}

void DocumentLoadingImpl::imageLoaded( bool ok ) {
	LOG("");

	QCString format = d->mLoader->imageFormat();
	if ( !ok || format.isEmpty()) {
		// Unknown format, no need to go further
		emit finished(false);
		switchToImpl(new DocumentEmptyImpl(mDocument));
		return;
	}
	setImageFormat( format );

	// Update file info
	setFileSize(d->mLoader->rawData().size());

	// Now we switch to a loaded implementation
	if ( d->mLoader->frames().count() > 1 ) {
		switchToImpl( new DocumentAnimatedLoadedImpl(mDocument, d->mLoader->frames()));
	} else if ( format == "JPEG" ) {
		switchToImpl( new DocumentJPEGLoadedImpl(mDocument, d->mLoader->rawData()) );
	} else {
		switchToImpl(new DocumentLoadedImpl(mDocument));
	}
}


void DocumentLoadingImpl::imageChanged(const QRect& rect) {
	if( d->mLoader->frames().count() > 0 ) return;
	setImage(d->mLoader->processedImage(), false);
	emit rectUpdated( rect );
}

void DocumentLoadingImpl::frameLoaded() {
	if( d->mLoader->frames().count() == 1 ) {
		// explicit sharing - don't modify the image in document anymore
		setImage( d->mLoader->frames().first().image.copy(), false ); 
	}
}

void DocumentLoadingImpl::sizeLoaded(int width, int height) {
	LOG(width << "x" << height);
	setImage( d->mLoader->processedImage(), false);
	emit sizeUpdated(width, height);
}

} // namespace
