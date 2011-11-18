// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
#include <QGraphicsProxyWidget>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QToolButton>

// KDE
#include <kicon.h>
#include <kurl.h>

// Local
#include "document/documentfactory.h"
#include "hudwidget.h"
#include "widgetfloater.h"

namespace Gwenview
{

struct VideoViewAdapterPrivate {
    VideoViewAdapter* q;
    Phonon::MediaObject* mMediaObject;
    Phonon::VideoWidget* mVideoWidget;
    Phonon::AudioOutput* mAudioOutput;
    HudWidget* mHud;
    WidgetFloater* mFloater;
    QToolButton* mPlayPauseButton;

    Document::Ptr mDocument;

    void setupHud(QWidget* parent)
    {
        // Create hud content
        QWidget* widget = new QWidget;
        QHBoxLayout* layout = new QHBoxLayout(widget);

        mPlayPauseButton = new QToolButton;
        mPlayPauseButton->setAutoRaise(true);
        q->updatePlayPauseButton();
        QObject::connect(mPlayPauseButton, SIGNAL(clicked()),
                         q, SLOT(slotPlayPauseClicked()));
        QObject::connect(mMediaObject, SIGNAL(stateChanged(Phonon::State, Phonon::State)),
                         q, SLOT(updatePlayPauseButton()));

        Phonon::SeekSlider* seekSlider = new Phonon::SeekSlider;
        seekSlider->setTracking(false);
        seekSlider->setIconVisible(false);
        seekSlider->setMediaObject(mMediaObject);

        Phonon::VolumeSlider* volumeSlider = new Phonon::VolumeSlider;
        volumeSlider->setAudioOutput(mAudioOutput);
        volumeSlider->setMinimumWidth(100);

        layout->addWidget(mPlayPauseButton);
        layout->addWidget(seekSlider, 5 /* stretch */);
        layout->addWidget(volumeSlider, 1 /* stretch */);
        widget->adjustSize();

        // Create hud
        mHud = new HudWidget;
        mHud->setAutoFillBackground(true);
        mHud->init(widget, HudWidget::OptionDoNotFollowChildSize | HudWidget::OptionOpaque);

        // Init floater
        mFloater = new WidgetFloater(parent);
        mFloater->setChildWidget(mHud);
        mFloater->setAlignment(Qt::AlignJustify | Qt::AlignBottom);

        mVideoWidget->installEventFilter(q);
    }

    bool isPlaying() const {
        switch (mMediaObject->state()) {
        case Phonon::StoppedState:
        case Phonon::PausedState:
            return false;
        default:
            return true;
        }
    }

    void updateHudVisibility(int yPos)
    {
        const int floaterY = mVideoWidget->height() - mFloater->verticalMargin() - mHud->sizeHint().height() * 3 / 2;
        if (mHud->isVisible() && yPos < floaterY) {
            mHud->hide();
        } else if (!mHud->isVisible() && yPos >= floaterY) {
            mHud->show();
        }
    }
};

VideoViewAdapter::VideoViewAdapter()
: d(new VideoViewAdapterPrivate)
{
    d->q = this;
    d->mMediaObject = new Phonon::MediaObject(this);
    connect(d->mMediaObject, SIGNAL(finished()), SIGNAL(videoFinished()));

    d->mVideoWidget = new Phonon::VideoWidget;
    d->mVideoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    d->mVideoWidget->setAttribute(Qt::WA_Hover);

    Phonon::createPath(d->mMediaObject, d->mVideoWidget);

    d->mAudioOutput = new Phonon::AudioOutput(Phonon::VideoCategory, this);
    Phonon::createPath(d->mMediaObject, d->mAudioOutput);

    d->setupHud(d->mVideoWidget);

    QGraphicsProxyWidget* proxy = new QGraphicsProxyWidget;
    proxy->setWidget(d->mVideoWidget);
    setWidget(proxy);
}

VideoViewAdapter::~VideoViewAdapter()
{
    delete d;
}

void VideoViewAdapter::setDocument(Document::Ptr doc)
{
    d->mHud->show();
    d->mDocument = doc;
    d->mMediaObject->setCurrentSource(d->mDocument->url());
    d->mMediaObject->play();
}

Document::Ptr VideoViewAdapter::document() const
{
    return d->mDocument;
}

void VideoViewAdapter::slotPlayPauseClicked()
{
    if (d->isPlaying()) {
        d->mMediaObject->pause();
    } else {
        d->mMediaObject->play();
    }
}

bool VideoViewAdapter::eventFilter(QObject*, QEvent* event)
{
    if (event->type() == QEvent::MouseMove) {
        d->updateHudVisibility(static_cast<QMouseEvent*>(event)->y());
    }
    return false;
}

void VideoViewAdapter::updatePlayPauseButton()
{
    if (d->isPlaying()) {
        d->mPlayPauseButton->setIcon(KIcon("media-playback-pause"));
    } else {
        d->mPlayPauseButton->setIcon(KIcon("media-playback-start"));
    }
}

} // namespace
