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

#include "documentanimatedloadedimpl.moc"

// Qt
#include <qstring.h>
#include <qtimer.h>

// KDE
#include <kdebug.h>
#include <klocale.h>
namespace Gwenview {

// Local

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

class DocumentAnimatedLoadedImplPrivate {
public:
	ImageFrames mFrames;
	int mCurrentFrame;
	QTimer mFrameTimer;
};


DocumentAnimatedLoadedImpl::DocumentAnimatedLoadedImpl(Document* document, const ImageFrames& frames)
: DocumentLoadedImpl(document) {
	LOG("" << mDocument->url().prettyURL() << ", frames: " << frames.count() );
	d=new DocumentAnimatedLoadedImplPrivate;
	d->mFrames = frames;
	d->mCurrentFrame = -1;
	connect( &d->mFrameTimer, SIGNAL( timeout()), SLOT( nextFrame()));
}

void DocumentAnimatedLoadedImpl::init() {
	DocumentLoadedImpl::init();
	nextFrame();
}

void DocumentAnimatedLoadedImpl::nextFrame() {
	++d->mCurrentFrame;
	if( d->mCurrentFrame == int( d->mFrames.count())) d->mCurrentFrame = 0;
	d->mFrameTimer.start( QMAX( 10, d->mFrames[ d->mCurrentFrame ].delay ));
// NOTE! If this ever gets changed to already animate the picture while it's still
// loading, with MNG the frame delay gets announced only after the frame is ready.
// See ImageLoader::frameDone() .
	LOG("" << d->mCurrentFrame );
	setImage( d->mFrames[ d->mCurrentFrame ].image, true );
}

DocumentAnimatedLoadedImpl::~DocumentAnimatedLoadedImpl() {
	delete d;
}


void DocumentAnimatedLoadedImpl::transform(ImageUtils::Orientation orientation) {
	for( ImageFrames::Iterator it = d->mFrames.begin(); it != d->mFrames.end(); ++it ) {
	        (*it).image = ImageUtils::transform( (*it).image, orientation );
	}
	setImage( d->mFrames[ d->mCurrentFrame ].image, true );
}


QString DocumentAnimatedLoadedImpl::localSave(QFile* /*file*/, const QCString& /*format*/) const {
	return i18n("Sorry, cannot save animated images.");
}

} // namespace
