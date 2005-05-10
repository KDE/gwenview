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

// Qt
#include <qtimer.h>

// KDE
#include <kconfig.h>
#include <kdebug.h>

// Local
#include "gvslideshow.moc"
#include "gvdocument.h"
#include "gvimageloader.h"


static const char* CONFIG_DELAY="delay";
static const char* CONFIG_LOOP="loop";


GVSlideShow::GVSlideShow(GVDocument* document)
: mDelay(10), mLoop(false), mDocument(document), mStarted(false), mPrefetch( NULL ), mPrefetchAdvance( 1 ) {
	mTimer=new QTimer(this);
	connect(mTimer, SIGNAL(timeout()),
			this, SLOT(slotTimeout()) );
	connect(mDocument, SIGNAL(loaded(const KURL&)),
			this, SLOT(slotLoaded()) );
}

GVSlideShow::~GVSlideShow() {
	prefetchDone( false );
}

void GVSlideShow::setLoop(bool value) {
	mLoop=value;
}

void GVSlideShow::setDelay(int delay) {
	mDelay=delay;
	if (mTimer->isActive()) {
		mTimer->changeInterval(delay*1000);
	}
}


void GVSlideShow::start(const KURL::List& urls) {
	mURLs=urls;
	mStartIt=qFind(mURLs.begin(), mURLs.end(), mDocument->url());
	if (mStartIt==mURLs.end()) {
		kdWarning() << k_funcinfo << "Current URL not found in list, aborting.\n";
		return;
	}
	
	mTimer->start(mDelay*1000, true);
	mStarted=true;
	mPrefetchAdvance = 1;
	prefetch();
}


void GVSlideShow::stop() {
	mTimer->stop();
	mStarted=false;
}


void GVSlideShow::slotTimeout() {
	KURL::List::ConstIterator it=qFind(mURLs.begin(), mURLs.end(), mDocument->url());
	if (it==mURLs.end()) {
		kdWarning() << k_funcinfo << "Current URL not found in list, aborting.\n";
		stop();
		emit finished();
		return;
	}

	++it;
	if (it==mURLs.end()) {
		it=mURLs.begin();
	}

	if (it==mStartIt && !mLoop) {
		stop();
		emit finished();
		return;
	}

	emit nextURL(*it);
	if( mPrefetchAdvance > 1 ) --mPrefetchAdvance;
}


void GVSlideShow::slotLoaded() {
	if (mStarted) {
		mTimer->start(mDelay*1000, true);
		prefetch();
	}
}


void GVSlideShow::prefetch() {
	KURL::List::ConstIterator it=qFind(mURLs.begin(), mURLs.end(), mDocument->url());
	if (it==mURLs.end()) {
		return;
	}

	for( int i = 0;
	     i < mPrefetchAdvance;
	     ++i ) {
		++it;
		if (it==mURLs.end()) {
			it=mURLs.begin();
		}
		if (it==mStartIt && !mLoop) {
			return;
		}
	}

	prefetchDone( false );
	mPrefetch = GVImageLoader::loader( *it );
	connect( mPrefetch, SIGNAL( imageLoaded( bool )), SLOT( prefetchDone( bool )));
}

void GVSlideShow::prefetchDone( bool ok ) {
	if( mPrefetch != NULL ) { 
		mPrefetch->disconnect( this );
		mPrefetch->release();
	}
	mPrefetch = NULL;
	if( !ok ) return;
// TODO this doesn't work for two reasons:
// - more distant images could put more needed images from the cache (needs cache priority?)
// - when doing manual image changes during slideshow, mPrefetchAdvance can get out of sync
//	if( ++mPrefetchAdvance <= 3 ) prefetch();
}

//-Configuration--------------------------------------------
void GVSlideShow::readConfig(KConfig* config,const QString& group) {
	config->setGroup(group);
	mDelay=config->readNumEntry(CONFIG_DELAY,10);
	mLoop=config->readBoolEntry(CONFIG_LOOP,false);
}


void GVSlideShow::writeConfig(KConfig* config,const QString& group) const {
	config->setGroup(group);
	config->writeEntry(CONFIG_DELAY,mDelay);
	config->writeEntry(CONFIG_LOOP,mLoop);
}
