// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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
#include "imagesequencecontroller.h"

// Qt
#include <QPixmap>
#include <QPointer>
#include <QTimer>

// KDE
#include <kdebug.h>

// Local
#include <lib/kpixmapsequence.h>

namespace Gwenview {


struct ImageSequenceControllerPrivate {
	ImageSequenceController* that;
	KPixmapSequence mSequence;
	int mIndex;
	QTimer mTimer;

	void slotTimerTimeout() {
		mIndex = (mIndex + 1) % mSequence.frameCount();
		emitFrameChanged();
	}

	void emitFrameChanged() {
		that->frameChanged(mSequence.frameAt(mIndex));
	}
};


ImageSequenceController::ImageSequenceController(QObject* parent)
: QObject(parent)
, d(new ImageSequenceControllerPrivate) {
	d->that = this;
	d->mIndex = 0;
	d->mTimer.setInterval(200);
	d->mTimer.setSingleShot(false);
	connect(&d->mTimer, SIGNAL(timeout()), SLOT(slotTimerTimeout()));
}


ImageSequenceController::~ImageSequenceController() {
	delete d;
}


void ImageSequenceController::setPixmapSequence(const KPixmapSequence& sequence) {
	d->mSequence = sequence;
}


void ImageSequenceController::setInterval(int interval) {
	d->mTimer.setInterval(interval);
}


void ImageSequenceController::start() {
	if (d->mSequence.frameCount() == 0) {
		kWarning() << "Empty KPixmapSequence!";
		return;
	}
	d->mIndex = 0;
	d->mTimer.start();
	d->emitFrameChanged();
}


void ImageSequenceController::stop() {
	d->mTimer.stop();
}


} // namespace

#include "imagesequencecontroller.moc"
