// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#include "videoviewadapter.moc"

// Qt
#include <Phonon/AudioOutput>
#include <Phonon/MediaObject>
#include <Phonon/Path>
#include <Phonon/VideoWidget>

// KDE
#include <kurl.h>

// Local
#include "document/documentfactory.h"

namespace Gwenview {


struct VideoViewAdapterPrivate {
	Document::Ptr mDocument;
	Phonon::MediaObject* mMediaObject;
	Phonon::VideoWidget* mVideoWidget;
};


VideoViewAdapter::VideoViewAdapter(QWidget* parent)
: AbstractDocumentViewAdapter(parent)
, d(new VideoViewAdapterPrivate) {
	d->mMediaObject = new Phonon::MediaObject(this);

	d->mVideoWidget = new Phonon::VideoWidget(parent);
	Phonon::createPath(d->mMediaObject, d->mVideoWidget);

	Phonon::AudioOutput* audioOutput =
		new Phonon::AudioOutput(Phonon::VideoCategory, this);
	Phonon::createPath(d->mMediaObject, audioOutput);

	setWidget(d->mVideoWidget);
}


void VideoViewAdapter::installEventFilterOnViewWidgets(QObject* object) {
	d->mVideoWidget->installEventFilter(object);
}


VideoViewAdapter::~VideoViewAdapter() {
	delete d;
}


void VideoViewAdapter::setDocument(Document::Ptr doc) {
	d->mDocument = doc;
	d->mMediaObject->setCurrentSource(d->mDocument->url());
	d->mMediaObject->play();
}


Document::Ptr VideoViewAdapter::document() const {
	return d->mDocument;
}


} // namespace
