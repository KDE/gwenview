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
#include <QAudioOutput>
#include <QElapsedTimer>
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsVideoItem>
#include <QIcon>
#include <QLabel>
#include <QMediaPlayer>
#include <QMouseEvent>

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
    QMediaPlayer *mMediaPlayer = nullptr;
    QGraphicsVideoItem *mVideoItem = nullptr;
    QAudioOutput *mAudioOutput = nullptr;
    HudWidget *mHud = nullptr;
    GraphicsWidgetFloater *mFloater = nullptr;

    HudSlider *mSeekSlider = nullptr;
    QElapsedTimer mLastSeekSliderActionTime;

    QLabel *mCurrentTime = nullptr;
    QLabel *mRemainingTime = nullptr;

    QAction *mPlayPauseAction = nullptr;
    QAction *mMuteAction = nullptr;
    QGraphicsWidget *mWidget = nullptr;

    HudSlider *mVolumeSlider = nullptr;
    QElapsedTimer mLastVolumeSliderChangeTime;

    Document::Ptr mDocument;

    void setupActions()
    {
        mPlayPauseAction = new QAction(q);
        mPlayPauseAction->setShortcut(Qt::Key_P);
        QObject::connect(mPlayPauseAction, &QAction::triggered, q, &VideoViewAdapter::slotPlayPauseClicked);
        QObject::connect(mMediaPlayer, &QMediaPlayer::playbackStateChanged, q, &VideoViewAdapter::updatePlayUi);

        mMuteAction = new QAction(q);
        mMuteAction->setShortcut(Qt::Key_M);
        QObject::connect(mMuteAction, &QAction::triggered, q, &VideoViewAdapter::slotMuteClicked);
        QObject::connect(mAudioOutput, &QAudioOutput::mutedChanged, q, &VideoViewAdapter::updateMuteAction);
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
        QObject::connect(mMediaPlayer, &QMediaPlayer::positionChanged, q, &VideoViewAdapter::slotTicked);
        QObject::connect(mMediaPlayer, &QMediaPlayer::durationChanged, q, &VideoViewAdapter::updatePlayUi);
        QObject::connect(mMediaPlayer, &QMediaPlayer::seekableChanged, q, &VideoViewAdapter::updatePlayUi);

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
        QObject::connect(mAudioOutput, &QAudioOutput::volumeChanged, q, &VideoViewAdapter::slotOutputVolumeChanged);

        // Timestamps
        mCurrentTime = new QLabel(QStringLiteral("--:--"));
        mCurrentTime->setAttribute(Qt::WA_TranslucentBackground);
        mCurrentTime->setStyleSheet(QStringLiteral("QLabel { color : white; }"));
        mCurrentTime->setAlignment(Qt::AlignCenter);
        mRemainingTime = new QLabel(QStringLiteral("--:--"));
        mRemainingTime->setAttribute(Qt::WA_TranslucentBackground);
        mRemainingTime->setStyleSheet(QStringLiteral("QLabel { color : white; }"));
        mRemainingTime->setAlignment(Qt::AlignCenter);
        QObject::connect(mMediaPlayer, &QMediaPlayer::playbackStateChanged, q, &VideoViewAdapter::updateTimestamps);
        QObject::connect(mMediaPlayer, &QMediaPlayer::positionChanged, q, &VideoViewAdapter::updateTimestamps);

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
        return mMediaPlayer->playbackState() == QMediaPlayer::PlayingState;
    }

    void updateHudVisibility(int yPos)
    {
        const int floaterY = mVideoItem->size().height() - mFloater->verticalMargin() - mHud->effectiveSizeHint(Qt::MinimumSize).height() * 3 / 2;
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

VideoViewAdapter::VideoViewAdapter()
    : d(new VideoViewAdapterPrivate)
{
    d->q = this;
    d->mMediaPlayer = new QMediaPlayer(this);
    connect(d->mMediaPlayer, &QMediaPlayer::mediaStatusChanged, this, [=](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            Q_EMIT videoFinished();
        }
    });

    d->mWidget = new QGraphicsWidget;
    d->mWidget->setFlag(QGraphicsItem::ItemIsSelectable); // Needed for doubleclick to work
    d->mWidget->setAcceptHoverEvents(GwenviewConfig::autoplayVideos()); // Makes hud visible when autoplay is disabled
    d->mWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    d->mWidget->setAttribute(Qt::WA_Hover);
    d->mWidget->installEventFilter(this);

    d->mVideoItem = new QGraphicsVideoItem(d->mWidget);
    d->mMediaPlayer->setVideoOutput(d->mVideoItem);

    connect(d->mWidget, &QGraphicsWidget::geometryChanged, this, [=]() {
        d->mVideoItem->setSize(d->mVideoItem->parentWidget()->size());
    });

    d->mAudioOutput = new QAudioOutput(this);
    d->mMediaPlayer->setAudioOutput(d->mAudioOutput);

    setWidget(d->mWidget);

    d->setupActions();
    d->setupHud(d->mWidget);

    updatePlayUi();
    updateMuteAction();
    // Enforce a redraw to make the hud visible if needed
    d->mWidget->adjustSize();
}

VideoViewAdapter::~VideoViewAdapter()
{
    // This prevents a memory leak that can occur after switching
    // to the next/previous video. For details see:
    // https://git.reviewboard.kde.org/r/108070/
    d->mMediaPlayer->stop();

    delete d;
}

void VideoViewAdapter::setDocument(const Document::Ptr &doc)
{
    d->mHud->show();
    d->mDocument = doc;
    d->mMediaPlayer->setSource(d->mDocument->url());
    if (GwenviewConfig::autoplayVideos()) {
        d->mMediaPlayer->play();
        Q_EMIT slotOutputVolumeChanged(d->mAudioOutput->volume());
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
        d->mMediaPlayer->pause();
        d->mHud->fadeIn();
        d->mWidget->setAcceptHoverEvents(false);
    } else {
        d->mMediaPlayer->play();
        d->mWidget->setAcceptHoverEvents(true);
    }
}

void VideoViewAdapter::slotMuteClicked()
{
    d->mAudioOutput->setMuted(!d->mAudioOutput->isMuted());
}

bool VideoViewAdapter::eventFilter(QObject *, QEvent *event)
{
    if (event->type() == QEvent::GraphicsSceneHoverMove) {
        d->updateHudVisibility(static_cast<QGraphicsSceneHoverEvent *>(event)->scenePos().y());
    } else if (event->type() == QEvent::KeyPress) {
        d->keyPressEvent(static_cast<QKeyEvent *>(event));
    } else if (event->type() == QEvent::GraphicsSceneMouseDoubleClick) {
        if (static_cast<QGraphicsSceneMouseEvent *>(event)->modifiers() == Qt::NoModifier) {
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

    if (d->mMediaPlayer->duration() > 0) {
        d->mSeekSlider->setRange(0, d->mMediaPlayer->duration());
    }

    if (d->mMediaPlayer->playbackState() == QMediaPlayer::StoppedState) {
        d->mSeekSlider->setEnabled(false);
        d->mSeekSlider->setValue(0);
    } else {
        d->mSeekSlider->setEnabled(true);
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
    d->mMediaPlayer->setPosition(d->mSeekSlider->sliderPosition());
}

void VideoViewAdapter::updateTimestamps()
{
    QString currentTime(QStringLiteral("--:--"));
    QString remainingTime(QStringLiteral("--:--"));

    if (d->mMediaPlayer->playbackState() != QMediaPlayer::StoppedState) {
        qint64 current = d->mMediaPlayer->position();
        currentTime = QDateTime::fromSecsSinceEpoch(current / 1000).toUTC().toString(QStringLiteral("h:mm:ss"));
        if (currentTime.startsWith(QStringLiteral("0:"))) {
            currentTime.remove(0, 2);
        }

        qint64 remaining = d->mMediaPlayer->duration() - d->mMediaPlayer->position();
        remainingTime = QDateTime::fromSecsSinceEpoch(remaining / 1000).toUTC().toString(QStringLiteral("h:mm:ss"));
        if (remainingTime.startsWith(QStringLiteral("0:"))) {
            remainingTime.remove(0, 2);
        }
        remainingTime = QStringLiteral("-") + remainingTime;
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

#include "moc_videoviewadapter.cpp"
