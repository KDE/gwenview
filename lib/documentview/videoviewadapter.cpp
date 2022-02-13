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
#include <QAction>
#include <QElapsedTimer>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QIcon>
#include <QLabel>
#include <QMouseEvent>

// Phonon
#include <Phonon/AudioOutput>
#include <Phonon/MediaObject>
#include <Phonon/Path>
#include <Phonon/VideoWidget>

// KF

// Local
#include "gwenview_lib_debug.h"
#include <document/documentfactory.h>
#include <graphicswidgetfloater.h>
#include <hud/hudbutton.h>
#include <hud/hudslider.h>
#include <hud/hudwidget.h>
#include <lib/gwenviewconfig.h>

namespace Gwenview
{
struct VideoViewAdapterPrivate {
    VideoViewAdapter *q = nullptr;
    Phonon::MediaObject *mMediaObject = nullptr;
    Phonon::VideoWidget *mVideoWidget = nullptr;
    Phonon::AudioOutput *mAudioOutput = nullptr;
    HudWidget *mHud = nullptr;
    GraphicsWidgetFloater *mFloater = nullptr;

    HudSlider *mSeekSlider = nullptr;
    QElapsedTimer mLastSeekSliderActionTime;

    QLabel *mCurrentTime = nullptr;
    QLabel *mRemainingTime = nullptr;

    QAction *mPlayPauseAction = nullptr;
    QAction *mMuteAction = nullptr;
    QGraphicsProxyWidget *mProxy = nullptr;

    HudSlider *mVolumeSlider = nullptr;
    QElapsedTimer mLastVolumeSliderChangeTime;

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

    void setupHud(QGraphicsWidget *parent)
    {
        // Play/Pause
        auto playPauseButton = new HudButton;
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
        auto muteButton = new HudButton;
        muteButton->setDefaultAction(mMuteAction);

        // Volume
        mVolumeSlider = new HudSlider;
        mVolumeSlider->setMinimumWidth(100);
        mVolumeSlider->setRange(0, 100);
        mVolumeSlider->setPageStep(5);
        mVolumeSlider->setSingleStep(1);
        QObject::connect(mVolumeSlider, &HudSlider::valueChanged, q, &VideoViewAdapter::slotVolumeSliderChanged);
        QObject::connect(mAudioOutput, &Phonon::AudioOutput::volumeChanged, q, &VideoViewAdapter::slotOutputVolumeChanged);

        // Timestamps
        mCurrentTime = new QLabel(QStringLiteral("--:--"));
        mCurrentTime->setAttribute(Qt::WA_TranslucentBackground);
        mCurrentTime->setStyleSheet(QStringLiteral("QLabel { color : white; }"));
        mCurrentTime->setAlignment(Qt::AlignCenter);
        mRemainingTime = new QLabel(QStringLiteral("--:--"));
        mRemainingTime->setAttribute(Qt::WA_TranslucentBackground);
        mRemainingTime->setStyleSheet(QStringLiteral("QLabel { color : white; }"));
        mRemainingTime->setAlignment(Qt::AlignCenter);
        QObject::connect(mMediaObject, &Phonon::MediaObject::stateChanged, q, &VideoViewAdapter::updateTimestamps);
        QObject::connect(mMediaObject, &Phonon::MediaObject::tick, q, &VideoViewAdapter::updateTimestamps);

        // Layout
        auto hudContent = new QGraphicsWidget;
        auto layout = new QGraphicsLinearLayout(hudContent);

        auto currentTimeProxy = new QGraphicsProxyWidget(hudContent);
        currentTimeProxy->setWidget(mCurrentTime);

        auto remainingTimeProxy = new QGraphicsProxyWidget(hudContent);
        remainingTimeProxy->setWidget(mRemainingTime);

        layout->addItem(playPauseButton);
        layout->addItem(currentTimeProxy);
        layout->setStretchFactor(currentTimeProxy, 1);
        layout->addItem(mSeekSlider);
        layout->setStretchFactor(mSeekSlider, 6);
        layout->addItem(remainingTimeProxy);
        layout->setStretchFactor(remainingTimeProxy, 1);
        layout->addItem(muteButton);
        layout->addItem(mVolumeSlider);
        layout->setStretchFactor(mVolumeSlider, 2);

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

    void keyPressEvent(QKeyEvent *event)
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
            Q_EMIT q->previousImageRequested();
            break;
        case Qt::Key_Down:
            Q_EMIT q->nextImageRequested();
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
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override
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

    d->mProxy = new DoubleClickableProxyWidget;
    d->mProxy->setFlag(QGraphicsItem::ItemIsSelectable); // Needed for doubleclick to work
    d->mProxy->setWidget(d->mVideoWidget);
    d->mProxy->setAcceptHoverEvents(GwenviewConfig::autoplayVideos()); // Makes hud visible when autoplay is disabled
    setWidget(d->mProxy);

    d->setupActions();
    d->setupHud(d->mProxy);

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
    if (GwenviewConfig::autoplayVideos()) {
        d->mMediaObject->play();
    }
    // If we do not use a queued connection, the signal arrives too early,
    // preventing the listing of the dir content when Gwenview is started with
    // a video as an argument.
    QMetaObject::invokeMethod(this, &VideoViewAdapter::completed, Qt::QueuedConnection);
}

Document::Ptr VideoViewAdapter::document() const
{
    return d->mDocument;
}

void VideoViewAdapter::slotPlayPauseClicked()
{
    if (d->isPlaying()) {
        d->mMediaObject->pause();
        d->mHud->fadeIn();
        d->mProxy->setAcceptHoverEvents(false);
    } else {
        d->mMediaObject->play();
        d->mProxy->setAcceptHoverEvents(true);
    }
}

void VideoViewAdapter::slotMuteClicked()
{
    d->mAudioOutput->setMuted(!d->mAudioOutput->isMuted());
}

bool VideoViewAdapter::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::MouseMove) {
        d->updateHudVisibility(static_cast<QMouseEvent *>(event)->y());
    } else if (event->type() == QEvent::KeyPress) {
        d->keyPressEvent(static_cast<QKeyEvent *>(event));
    } else if (event->type() == QEvent::MouseButtonDblClick) {
        if (static_cast<QMouseEvent *>(event)->modifiers() == Qt::NoModifier) {
            Q_EMIT toggleFullScreenRequested();
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
    d->mMuteAction->setIcon(QIcon::fromTheme(d->mAudioOutput->isMuted() ? QStringLiteral("player-volume-muted") : QStringLiteral("player-volume")));
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

void VideoViewAdapter::updateTimestamps()
{
    QString currentTime(QStringLiteral("--:--"));
    QString remainingTime(QStringLiteral("--:--"));

    switch (d->mMediaObject->state()) {
    case Phonon::PlayingState:
    case Phonon::BufferingState:
    case Phonon::PausedState: {
        qint64 current = d->mMediaObject->currentTime();
        currentTime = QDateTime::fromSecsSinceEpoch(current / 1000).toUTC().toString(QStringLiteral("h:mm:ss"));
        if (currentTime.startsWith(QStringLiteral("0:"))) {
            currentTime.remove(0, 2);
        }

        qint64 remaining = d->mMediaObject->remainingTime();
        remainingTime = QDateTime::fromSecsSinceEpoch(remaining / 1000).toUTC().toString(QStringLiteral("h:mm:ss"));
        if (remainingTime.startsWith(QStringLiteral("0:"))) {
            remainingTime.remove(0, 2);
        }
        remainingTime = QStringLiteral("-") + remainingTime;
        break;
    }

    default:
        break;
    }

    d->mCurrentTime->setText(currentTime);
    d->mRemainingTime->setText(remainingTime);
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
