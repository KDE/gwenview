// vim: set tabstop=4 shiftwidth=4 expandtab:
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
#include <Phonon/VideoWidget>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QTime>

// KDE
#include <KDebug>
#include <KIcon>
#include <KUrl>

// Local
#include <document/documentfactory.h>
#include <graphicshudbutton.h>
#include <graphicshudslider.h>
#include <graphicshudwidget.h>
#include <graphicswidgetfloater.h>

namespace Gwenview
{

struct VideoViewAdapterPrivate
{
    VideoViewAdapter* q;
    Phonon::MediaObject* mMediaObject;
    Phonon::VideoWidget* mVideoWidget;
    Phonon::AudioOutput* mAudioOutput;
    GraphicsHudWidget* mHud;
    GraphicsWidgetFloater* mFloater;

    GraphicsHudSlider* mSeekSlider;
    QTime mLastSeekSliderActionTime;

    GraphicsHudButton* mPlayPauseButton;
    GraphicsHudButton* mMuteButton;

    GraphicsHudSlider* mVolumeSlider;
    QTime mLastVolumeSliderChangeTime;

    Document::Ptr mDocument;

    void setupHud(QGraphicsWidget* parent)
    {
        // Create hud content
        mPlayPauseButton = new GraphicsHudButton;
        QObject::connect(mPlayPauseButton, SIGNAL(clicked()),
                         q, SLOT(slotPlayPauseClicked()));
        QObject::connect(mMediaObject, SIGNAL(stateChanged(Phonon::State, Phonon::State)),
                         q, SLOT(updatePlayUi()));

        mSeekSlider = new GraphicsHudSlider;
        mSeekSlider->setPageStep(5000);
        mSeekSlider->setSingleStep(200);
        QObject::connect(mSeekSlider, SIGNAL(actionTriggered(int)),
            q, SLOT(slotSeekSliderActionTriggered(int)));
        QObject::connect(mMediaObject, SIGNAL(tick(qint64)),
            q, SLOT(slotTicked(qint64)));
        QObject::connect(mMediaObject, SIGNAL(totalTimeChanged(qint64)),
            q, SLOT(updatePlayUi()));
        QObject::connect(mMediaObject, SIGNAL(seekableChanged(bool)),
            q, SLOT(updatePlayUi()));

        mMuteButton = new GraphicsHudButton;
        q->updateMuteButton();
        QObject::connect(mMuteButton, SIGNAL(clicked()),
            q, SLOT(slotMuteClicked()));
        QObject::connect(mAudioOutput, SIGNAL(mutedChanged(bool)),
            q, SLOT(updateMuteButton()));

        mVolumeSlider = new GraphicsHudSlider;
        mVolumeSlider->setMinimumWidth(100);
        mVolumeSlider->setRange(0, 100);
        mVolumeSlider->setPageStep(5);
        mVolumeSlider->setSingleStep(1);
        QObject::connect(mVolumeSlider, SIGNAL(valueChanged(int)),
            q, SLOT(slotVolumeSliderChanged(int)));
        QObject::connect(mAudioOutput, SIGNAL(volumeChanged(qreal)),
            q, SLOT(slotOutputVolumeChanged(qreal)));

        QGraphicsWidget* hudContent = new QGraphicsWidget;
        QGraphicsLinearLayout* layout = new QGraphicsLinearLayout(hudContent);
        layout->addItem(mPlayPauseButton);
        layout->addItem(mSeekSlider);
        layout->setStretchFactor(mSeekSlider, 5);
        layout->addItem(mMuteButton);
        layout->addItem(mVolumeSlider);
        layout->setStretchFactor(mVolumeSlider, 1);

        q->updatePlayUi();

        // Create hud
        mHud = new GraphicsHudWidget(parent);
        mHud->init(hudContent, GraphicsHudWidget::OptionNone);
        mHud->setZValue(1);

        // Init floater
        mFloater = new GraphicsWidgetFloater(parent);
        mFloater->setChildWidget(mHud);
        mFloater->setAlignment(Qt::AlignJustify | Qt::AlignBottom);

        mVideoWidget->installEventFilter(q);
    }

    bool isPlaying() const
    {
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
        const int floaterY = mVideoWidget->height() - mFloater->verticalMargin() - mHud->effectiveSizeHint(Qt::MinimumSize).height() * 3 / 2;
        if (yPos < floaterY) {
            mHud->fadeOut();
        } else {
            mHud->fadeIn();
        }
    }
};

VideoViewAdapter::VideoViewAdapter()
: d(new VideoViewAdapterPrivate)
{
    d->q = this;
    d->mMediaObject = new Phonon::MediaObject(this);
    d->mMediaObject->setTickInterval(350);
    connect(d->mMediaObject, SIGNAL(finished()), SIGNAL(videoFinished()));

    d->mVideoWidget = new Phonon::VideoWidget;
    d->mVideoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    d->mVideoWidget->setAttribute(Qt::WA_Hover);

    Phonon::createPath(d->mMediaObject, d->mVideoWidget);

    d->mAudioOutput = new Phonon::AudioOutput(Phonon::VideoCategory, this);
    Phonon::createPath(d->mMediaObject, d->mAudioOutput);

    QGraphicsProxyWidget* proxy = new QGraphicsProxyWidget;
    proxy->setWidget(d->mVideoWidget);
    setWidget(proxy);

    d->setupHud(proxy);
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
    // If we do not use a queued connection, the signal arrives too early.
    // This prevents loading of the dir content when Gwenview is started with a
    // video as an argument.
    QMetaObject::invokeMethod(this, "completed", Qt::QueuedConnection);
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

void VideoViewAdapter::slotMuteClicked()
{
    d->mAudioOutput->setMuted(!d->mAudioOutput->isMuted());
}

bool VideoViewAdapter::eventFilter(QObject*, QEvent* event)
{
    if (event->type() == QEvent::MouseMove) {
        d->updateHudVisibility(static_cast<QMouseEvent*>(event)->y());
    }
    return false;
}

void VideoViewAdapter::updatePlayUi()
{
    if (d->isPlaying()) {
        d->mPlayPauseButton->setIcon(KIcon("media-playback-pause"));
    } else {
        d->mPlayPauseButton->setIcon(KIcon("media-playback-start"));
    }

    d->mLastSeekSliderActionTime.restart();
    d->mSeekSlider->setRange(0, d->mMediaObject->totalTime());

    switch (d->mMediaObject->state()) {
    case Phonon::PlayingState:
    case Phonon::BufferingState:
    case Phonon::PausedState:
        d->mSeekSlider->setEnabled(true);
        break;
    case Phonon::StoppedState:
    case Phonon::LoadingState:
    case Phonon::ErrorState:
        d->mSeekSlider->setEnabled(false);
        d->mSeekSlider->setValue(0);
        break;
    }
}

void VideoViewAdapter::updateMuteButton()
{
    d->mMuteButton->setIcon(
        KIcon(d->mAudioOutput->isMuted() ? "player-volume-muted" : "player-volume")
        );
}

void VideoViewAdapter::slotVolumeSliderChanged(int value)
{
    d->mLastVolumeSliderChangeTime.restart();
    d->mAudioOutput->setVolume(value / 100.);
}

void VideoViewAdapter::slotOutputVolumeChanged(qreal value)
{
    if (d->mLastVolumeSliderChangeTime.isValid() && d->mLastVolumeSliderChangeTime.elapsed() < 2000) {
        return;
    }
    d->mVolumeSlider->setValue(qRound(value * 100));
}

void VideoViewAdapter::slotSeekSliderActionTriggered(int /*action*/)
{
    d->mLastSeekSliderActionTime.restart();
    d->mMediaObject->seek(d->mSeekSlider->sliderPosition());
}

void VideoViewAdapter::slotTicked(qint64 value)
{
    if (d->mLastSeekSliderActionTime.isValid() && d->mLastSeekSliderActionTime.elapsed() < 2000) {
        return;
    }
    if (!d->mSeekSlider->isSliderDown()) {
        d->mSeekSlider->setValue(value);
    }
}

} // namespace
