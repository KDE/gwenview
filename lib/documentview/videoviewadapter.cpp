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
#include <Phonon/SeekSlider>
#include <Phonon/VideoWidget>
#include <Phonon/VolumeSlider>
#include <QHBoxLayout>
#include <QToolButton>

// KDE
#include <kicon.h>
#include <kurl.h>

// Local
#include "document/documentfactory.h"
#include "widgetfloater.h"
#include "hudwidget.h"

namespace Gwenview {


struct VideoViewAdapterPrivate {
	VideoViewAdapter* q;
	Document::Ptr mDocument;
	Phonon::MediaObject* mMediaObject;
	Phonon::VideoWidget* mVideoWidget;
	Phonon::AudioOutput* mAudioOutput;
	QFrame* mHud;
	QToolButton* mPlayPauseButton;

	void setupHud(QWidget* parent) {
		WidgetFloater* floater = new WidgetFloater(parent);

		mHud = new QFrame(parent);
		mHud->setAutoFillBackground(true);
		QHBoxLayout* layout = new QHBoxLayout(mHud);

		mPlayPauseButton = new QToolButton;
		mPlayPauseButton->setAutoRaise(true);
		mPlayPauseButton->setIcon(KIcon("media-playback-start"));
		QObject::connect(mPlayPauseButton, SIGNAL(clicked()),
			q, SLOT(slotPlayPauseClicked()));

		Phonon::SeekSlider* seekSlider = new Phonon::SeekSlider;
		seekSlider->setMediaObject(mMediaObject);

		Phonon::VolumeSlider* volumeSlider = new Phonon::VolumeSlider;
		volumeSlider->setAudioOutput(mAudioOutput);
		volumeSlider->setMinimumWidth(100);

		layout->addWidget(mPlayPauseButton);
		layout->addWidget(seekSlider, 5 /* stretch */);
		layout->addWidget(volumeSlider, 1 /* stretch */);
		mHud->adjustSize();

		floater->setChildWidget(mHud);
		floater->setAlignment(Qt::AlignJustify | Qt::AlignBottom);
	}
};


VideoViewAdapter::VideoViewAdapter(QWidget* parent)
: AbstractDocumentViewAdapter(parent)
, d(new VideoViewAdapterPrivate) {
	d->q = this;
	d->mMediaObject = new Phonon::MediaObject(this);

	d->mVideoWidget = new Phonon::VideoWidget(parent);
	d->mVideoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	Phonon::createPath(d->mMediaObject, d->mVideoWidget);

	d->mAudioOutput = new Phonon::AudioOutput(Phonon::VideoCategory, this);
	Phonon::createPath(d->mMediaObject, d->mAudioOutput);

	d->setupHud(d->mVideoWidget);

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
}


Document::Ptr VideoViewAdapter::document() const {
	return d->mDocument;
}


void VideoViewAdapter::slotPlayPauseClicked() {
	switch (d->mMediaObject->state()) {
	case Phonon::StoppedState:
	case Phonon::PausedState:
		d->mMediaObject->play();
		d->mPlayPauseButton->setIcon(KIcon("media-playback-pause"));
		break;
	default:
		d->mMediaObject->pause();
		d->mPlayPauseButton->setIcon(KIcon("media-playback-start"));
	}
}


} // namespace
