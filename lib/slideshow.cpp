/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "slideshow.moc"

// STL
//#include <algorithm>

// Qt
#include <QTimer>

// KDE
#include <kconfig.h>
#include <kdebug.h>

namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << k_funcinfo << x
#else
#define LOG(x) ;
#endif

struct SlideShowPrivate {
	QTimer* mTimer;
	bool mStarted;
	QVector<KUrl> mUrls;
	QVector<KUrl>::ConstIterator mStartIt;
	KUrl mCurrentUrl;
};

SlideShow::SlideShow(QObject* parent)
: QObject(parent)
, d(new SlideShowPrivate) {
	d->mStarted = false;
	d->mTimer = new QTimer(this);
	connect(d->mTimer, SIGNAL(timeout()),
			this, SLOT(slotTimeout()) );
}

SlideShow::~SlideShow() {
	delete d;
}


int SlideShow::timerInterval() {
	/*
	int documentDuration = d->mDocument->duration();
	if (documentDuration != 0) {
		return documentDuration * 1000;
	} else {
		return int(SlideShowConfig::delay()*1000);
	}
	*/
	// FIXME interval is hardcoded
	return 1000;
}


void SlideShow::start(const QList<KUrl>& urls) {
	d->mUrls.resize(urls.size());
	qCopy(urls.begin(), urls.end(), d->mUrls.begin());
	// FIXME: random
	/*
	if (SlideShowConfig::random()) {
		std::random_shuffle(d->mUrls.begin(), d->mUrls.end());
	}
	*/

	d->mStartIt=qFind(d->mUrls.begin(), d->mUrls.end(), d->mCurrentUrl);
	if (d->mStartIt==d->mUrls.end()) {
		kWarning() << k_funcinfo << "Current url not found in list, aborting.\n";
		return;
	}
	
	d->mTimer->setInterval(timerInterval());
	d->mTimer->setSingleShot(false);
	d->mTimer->start();
	d->mStarted=true;
	stateChanged(true);
}


void SlideShow::stop() {
	d->mTimer->stop();
	d->mStarted=false;
	stateChanged(false);
}


QVector<KUrl>::ConstIterator SlideShow::findNextUrl() const {
	QVector<KUrl>::ConstIterator it=qFind(d->mUrls.begin(), d->mUrls.end(), d->mCurrentUrl);
	if (it==d->mUrls.end()) {
		kWarning() << k_funcinfo << "Current url not found in list. This should not happen.\n";
		return it;
	}

	++it;
	// FIXME: loop
	if (/*SlideShowConfig::loop()*/ false) {
		// Looping, if we reach the end, start again
		if (it==d->mUrls.end()) {
			it = d->mUrls.begin();
		}
	} else {
		// Not looping, have we reached the end?
		// FIXME: stopAtEnd
		if ((it==d->mUrls.end() /*&& SlideShowConfig::stopAtEnd()*/) || it==d->mStartIt) {
			it = d->mUrls.end();
		}
	}

	return it;
}


void SlideShow::slotTimeout() {
	LOG("");
	QVector<KUrl>::ConstIterator it=findNextUrl();
	if (it==d->mUrls.end()) {
		stop();
		return;
	}
	goToUrl(*it);
}


void SlideShow::setCurrentUrl(const KUrl& url) {
	d->mCurrentUrl = url;
}


bool SlideShow::isRunning() const {
	return d->mStarted;
}


} // namespace
