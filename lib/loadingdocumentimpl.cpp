// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "loadingdocumentimpl.moc"

// Qt
#include <QImage>
#include <QThread>

// KDE
#include <KUrl>

// Local
#include "document.h"
#include "documentloadedimpl.h"

namespace Gwenview {


class LoadingThread : public QThread {
public:
	virtual void run() {
		mImage.load(mUrl.path());
	}

	KUrl mUrl;
	QImage mImage;
};


struct LoadingDocumentImplPrivate {
	LoadingThread mThread;
};


LoadingDocumentImpl::LoadingDocumentImpl(Document* document)
: AbstractDocumentImpl(document)
, d(new LoadingDocumentImplPrivate) {
}


LoadingDocumentImpl::~LoadingDocumentImpl() {
	if (d->mThread.isRunning()) {
		d->mThread.terminate();
		d->mThread.wait();
	}
	delete d;
}

void LoadingDocumentImpl::init() {
	d->mThread.mUrl = document()->url();
	connect(&d->mThread, SIGNAL(finished()), SLOT(slotImageLoaded()) );
	d->mThread.start();
}


bool LoadingDocumentImpl::isLoaded() const {
	return false;
}


void LoadingDocumentImpl::slotImageLoaded() {
	Q_ASSERT(d->mThread.isFinished());
	setDocumentImage(d->mThread.mImage);
	loaded();
	switchToImpl(new DocumentLoadedImpl(document(), d->mThread.mImage));
}

} // namespace
