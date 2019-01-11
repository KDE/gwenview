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
#include "videoviewadapter.h"

// Qt
#include <Phonon/AudioOutput>
#include <Phonon/MediaObject>
#include <Phonon/Path>
#include <Phonon/VideoWidget>
#include <QAction>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QMouseEvent>
#include <QTime>
#include <QDebug>
#include <QIcon>

// KDE

// Local
#include <document/documentfactory.h>
#include <hud/hudbutton.h>
#include <hud/hudslider.h>
#include <hud/hudwidget.h>
#include <graphicswidgetfloater.h>

namespace Gwenview
{

struct VideoViewAdapterPrivate
{
    VideoViewAdapter* q;
    Phonon::MediaObject* mMediaObject;
    Phonon::VideoWidget* mVideoWidget;
    Phonon::AudioOutput* mAudioOutput;
    HudWidget* mHud;
    GraphicsWidgetFloater* mFloater;

    HudSlider* mSeekSlider;
    QTime mLastSeekSliderActionTime;

    QAction* mPlayPauseAction;
    QAction* mMuteAction;

    HudSlider* mVolumeSlider;
    QTime mLastVolumeSliderChangeTime;

    Document::Ptr mDocument;

    void setupActions()
    {
        mPlayPauseAction = new QAction(q);
        mPlayPauseAction->setShortcut(Qt::Key_P);
        QObject::connect(mPlayPauseAction, &QAction::triggered, q, &VideoViewAdapter::slotPlayPauseClicked);
        QObject::connect(mMediaObject, &Phonon::MediaObject::stateChanged, q, &VideoViewAdapter::updatePlayUi);

        mMuteAction = new QAction(q);
        mMuteAction->setShortcut(Qt::Key_M);
        QObject::connect(mMuteAction, &QAction::triggered, q, &VideoViewAdapter::slotMuteClicked);
        QObject::connect(mAudioOutput, &Phonon::AudioOutput::mutedChanged, q, &VideoViewAdapter::updateMuteAction);
    }

    void setupHud(QGraphicsWidget* parent)
    {
        // Play/Pause
        HudButton* playPauseButton = new HudButton;
        playPauseButton->setDefaultAction(mPlayPauseAction);

        // Seek
        mSeekSlider = new HudSlider;
        mSeekSlider->setPageStep(5000);
        mSeekSlider->setSingleStep(200);
        QObject::connect(mSeekSlider, &HudSlider::actionTriggered, q, &VideoViewAdapter::slotSeekSliderActionTriggered);
        QObject::connect(mMediaObject, &Phonon::MediaObject::tick, q, &VideoViewAdapter::slotTicked);
        QObject::connect(mMediaObject, &Phonon::MediaObject::totalTimeChanged, q, &VideoViewAdapter::updatePlayUi);
        QObject::connect(mMediaObject, &Phonon::MediaObject::seekableChanged, q, &VideoViewAdapter::updatePlayUi);

        // Mute
        HudButton* muteButton = new HudButton;
        muteButton->setDefaultAction(mMuteAction);

        // Volume
        mVolumeSlider = new HudSlider;
        mVolumeSlider->setMinimumWidth(100);
        mVolumeSlider->setRange(0, 100);
        mVolumeSlider->setPageStep(5);
        mVolumeSlider->setSingleStep(1);
        QObject::connect(mVolumeSlider, &HudSlider::valueChanged, q, &VideoViewAdapter::slotVolumeSliderChanged);
        QObject::connect(mAudioOutput, &Phonon::AudioOutput::volumeChanged, q, &VideoViewAdapter::slotOutputVolumeChanged);

        // Layout
        QGraphicsWidget* hudContent = new QGraphicsWidget;
        QGraphicsLinearLayout* layout = new QGraphicsLinearLayout(hudContent);
        layout->addItem(playPauseButton);
        layout->addItem(mSeekSlider);
        layout->setStretchFactor(mSeekSlider, 5);
        layout->addItem(muteButton);
        layout->addItem(mVolumeSlider);
        layout->setStretchFactor(mVolumeSlider, 1);

        // Create hud
        mHud = new HudWidget(parent);
        mHud->init(hudContent, HudWidget::OptionNone);
        mHud->setZValue(1);

        // Init floater
        mFloater = new GraphicsWidgetFloater(parent);
        mFloater->setChildWidget(mHud);
        mFloater->setAlignment(Qt::AlignJustify | Qt::AlignBottom);
    }

    bool isPlaying() const
    {
        switch (mMediaObject->state()) {
        case Phonon::PlayingState:
        case Phonon::BufferingState:
            return true;
        default:
            return false;
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

    void keyPressEvent(QKeyEvent* event)
    {
        if (event->modifiers() != Qt::NoModifier) {
            return;
        }

        switch (event->key()) {
        case Qt::Key_Left:
            mSeekSlider->triggerAction(QAbstractSlider::SliderSingleStepSub);
            break;
        case Qt::Key_Right:
            mSeekSlider->triggerAction(QAbstractSlider::SliderSingleStepAdd);
            break;
        case Qt::Key_Up:
            emit q->previousImageRequested();
            break;
        case Qt::Key_Down:
            emit q->nextImageRequested();
            break;
        default:
            break;
        }
    }
};

/**
 * This is a workaround for a bug in QGraphicsProxyWidget: it does not forward
 * double-click events to the proxy-fied widget.
 *
 * QGraphicsProxyWidget::mouseDoubleClickEvent() correctly forwards the event
 * to its QWidget, but it is never called. This is because for it to be called,
 * the implementation of mousePressEvent() must call
 * QGraphicsItem::mousePressEvent() but it does not.
 */
class DoubleClickableProxyWidget : public QGraphicsProxyWidget
{
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override
    {
        QGraphicsWidget::mousePressEvent(event);
    }
};

VideoViewAdapter::VideoViewAdapter()
: d(new VideoViewAdapterPrivate)
{
    d->q = this;
    d->mMediaObject = new Phonon::MediaObject(this);
    d->mMediaObject->setTickInterval(350);
    connect(d->mMediaObject, &Phonon::MediaObject::finished, this, &VideoViewAdapter::videoFinished);

    d->mVideoWidget = new Phonon::VideoWidget;
    d->mVideoWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    d->mVideoWidget->setAttribute(Qt::WA_Hover);
    d->mVideoWidget->installEventFilter(this);

    Phonon::createPath(d->mMediaObject, d->mVideoWidget);

    d->mAudioOutput = new Phonon::AudioOutput(Phonon::VideoCategory, this);
    Phonon::createPath(d->mMediaObject, d->mAudioOutput);

    QGraphicsProxyWidget* proxy = new DoubleClickableProxyWidget;
    proxy->setFlag(QGraphicsItem::ItemIsSelectable); // Needed for doubleclick to work
    proxy->setWidget(d->mVideoWidget);
    proxy->setAcceptHoverEvents(true);
    setWidget(proxy);

    d->setupActions();
    d->setupHud(proxy);

    updatePlayUi();
    updateMuteAction();
}

VideoViewAdapter::~VideoViewAdapter()
{
    // This prevents a memory leak that can occur after switching
    // to the next/previous video. For details see:
    // https://git.reviewboard.kde.org/r/108070/
    d->mMediaObject->stop();

    delete d;
}

void VideoViewAdapter::setDocument(const Document::Ptr &doc)
{
    d->mHud->show();
    d->mDocument = doc;
    d->mMediaObject->setCurrentSource(d->mDocument->url());
    d->mMediaObject->play();
    // If we do not use a queued connection, the signal arrives too early,
    // preventing the listing of the dir content when Gwenview is started with
    // a video as an argument.
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
    } else if (event->type() == QEvent::KeyPress) {
        d->keyPressEvent(static_cast<QKeyEvent*>(event));
    } else if (event->type() == QEvent::MouseButtonDblClick) {
        if (static_cast<QMouseEvent*>(event)->modifiers() == Qt::NoModifier) {
            emit toggleFullScreenRequested();
        }
    }
    return false;
}

void VideoViewAdapter::updatePlayUi()
{
    if (d->isPlaying()) {
        d->mPlayPauseAction->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-pause")));
    } else {
        d->mPlayPauseAction->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
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

void VideoViewAdapter::updateMuteAction()
{
    d->mMuteAction->setIcon(
        QIcon::fromTheme(d->mAudioOutput->isMuted() ? QStringLiteral("player-volume-muted") : QStringLiteral("player-volume"))
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
