/*
Gwenview: an image viewer
Copyright 2018 Friedrich W. H. Kossebau <kossebau@kde.org>

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

#include "mprismediaplayer2player.h"

#include <config-gwenview.h>

// lib
#include <slideshow.h>
#include <mimetypeutils.h>
#include <contextmanager.h>
#include <imagemetainfomodel.h>
#include <document/documentfactory.h>
#include <semanticinfo/semanticinfodirmodel.h>
#include <semanticinfo/sorteddirmodel.h>
// KF
#include <KLocalizedString>
#include <KFileItem>
// Qt
#include <QDBusObjectPath>
#include <QAction>
#include <QDebug>

namespace Gwenview
{

static const double MAX_RATE = 1.0;
static const double MIN_RATE = 1.0;


MprisMediaPlayer2Player::MprisMediaPlayer2Player(const QString &objectDBusPath,
                                                 SlideShow* slideShow,
                                                 ContextManager* contextManager,
                                                 QAction* toggleSlideShowAction, QAction* fullScreenAction,
                                                 QAction* previousAction, QAction* nextAction,
                                                 QObject* parent)
    : DBusAbstractAdaptor(objectDBusPath, parent)
    , mSlideShow(slideShow)
    , mContextManager(contextManager)
    , mToggleSlideShowAction(toggleSlideShowAction)
    , mFullScreenAction(fullScreenAction)
    , mPreviousAction(previousAction)
    , mNextAction(nextAction)
    , mSlideShowEnabled(mToggleSlideShowAction->isEnabled())
    , mPreviousEnabled(mPreviousAction->isEnabled())
    , mNextEnabled(mNextAction->isEnabled())
{
    updatePlaybackStatus();

    connect(mSlideShow, &SlideShow::stateChanged,
            this, &MprisMediaPlayer2Player::onSlideShowStateChanged);
    connect(mSlideShow, &SlideShow::intervalChanged,
            this, &MprisMediaPlayer2Player::onMetaInfoUpdated);
    connect(mContextManager, &ContextManager::currentUrlChanged,
            this, &MprisMediaPlayer2Player::onCurrentUrlChanged);
    connect(mSlideShow->randomAction(), &QAction::toggled,
            this, &MprisMediaPlayer2Player::onRandomActionToggled);
    connect(mToggleSlideShowAction, &QAction::changed,
            this, &MprisMediaPlayer2Player::onToggleSlideShowActionChanged);
    connect(mFullScreenAction, &QAction::toggled,
            this, &MprisMediaPlayer2Player::onFullScreenActionToggled);
    connect(mNextAction, &QAction::changed,
            this, &MprisMediaPlayer2Player::onNextActionChanged);
    connect(mPreviousAction, &QAction::changed,
            this, &MprisMediaPlayer2Player::onPreviousActionChanged);
}

MprisMediaPlayer2Player::~MprisMediaPlayer2Player()
{
}

bool MprisMediaPlayer2Player::updatePlaybackStatus()
{
    const QString newStatus =
        (!mSlideShowEnabled || !mFullScreenAction->isChecked()) ?
            QStringLiteral("Stopped") :
        mSlideShow->isRunning() ?
            QStringLiteral("Playing") :
        /* else */
            QStringLiteral("Paused");

    const bool changed = (newStatus != mPlaybackStatus);
    if (changed) {
        mPlaybackStatus = newStatus;
    }

    return changed;
}

QString MprisMediaPlayer2Player::playbackStatus() const
{
    return mPlaybackStatus;
}

bool MprisMediaPlayer2Player::canGoNext() const
{
    return mNextEnabled;
}

void MprisMediaPlayer2Player::Next()
{
    mNextAction->trigger();
}


bool MprisMediaPlayer2Player::canGoPrevious() const
{
    return mPreviousEnabled;
}

void MprisMediaPlayer2Player::Previous()
{
    mPreviousAction->trigger();
}

bool MprisMediaPlayer2Player::canPause() const
{
    return mSlideShowEnabled;
}

void MprisMediaPlayer2Player::Pause()
{
    mSlideShow->pause();
}

void MprisMediaPlayer2Player::PlayPause()
{
    mToggleSlideShowAction->trigger();
}

void MprisMediaPlayer2Player::Stop()
{
    if (mFullScreenAction->isChecked()) {
        mFullScreenAction->trigger();
    }
}

bool MprisMediaPlayer2Player::canPlay() const
{
    return mSlideShowEnabled;
}

void MprisMediaPlayer2Player::Play()
{
    if (mSlideShow->isRunning()) {
        return;
    }
    mToggleSlideShowAction->trigger();
}

double MprisMediaPlayer2Player::volume() const
{
    return 0;
}

void MprisMediaPlayer2Player::setVolume(double volume)
{
    Q_UNUSED(volume);
}

void MprisMediaPlayer2Player::setShuffle(bool isShuffle)
{
    mSlideShow->randomAction()->setChecked(isShuffle);
}

QVariantMap MprisMediaPlayer2Player::metadata() const
{
    return mMetaData;
}

qlonglong MprisMediaPlayer2Player::position() const
{
    // milliseconds -> microseconds
    return mSlideShow->position() * 1000;
}

double MprisMediaPlayer2Player::rate() const
{
    return 1.0;
}

void MprisMediaPlayer2Player::setRate(double newRate)
{
    Q_UNUSED(newRate);
}

double MprisMediaPlayer2Player::minimumRate() const
{
    return MIN_RATE;
}

double MprisMediaPlayer2Player::maximumRate() const
{
    return MAX_RATE;
}

bool MprisMediaPlayer2Player::isShuffle() const
{
    return mSlideShow->randomAction()->isChecked();
}


bool MprisMediaPlayer2Player::canSeek() const
{
    return false;
}

bool MprisMediaPlayer2Player::canControl() const
{
    return true;
}

void MprisMediaPlayer2Player::Seek(qlonglong offset)
{
    Q_UNUSED(offset);
}

void MprisMediaPlayer2Player::SetPosition(const QDBusObjectPath& trackId, qlonglong pos)
{
    Q_UNUSED(trackId);
    Q_UNUSED(pos);
}

void MprisMediaPlayer2Player::OpenUri(const QString& uri)
{
    Q_UNUSED(uri);
}

void MprisMediaPlayer2Player::onSlideShowStateChanged()
{
    if (!updatePlaybackStatus()) {
        return;
    }

    signalPropertyChange("Position", position());
    signalPropertyChange("PlaybackStatus", mPlaybackStatus);
}

void MprisMediaPlayer2Player::onCurrentUrlChanged(const QUrl& url)
{
    if (url.isEmpty()) {
        mCurrentDocument = Document::Ptr();
    } else {
        mCurrentDocument = DocumentFactory::instance()->load(url);
        connect(mCurrentDocument.data(), &Document::metaInfoUpdated,
                this, &MprisMediaPlayer2Player::onMetaInfoUpdated);
    }

    onMetaInfoUpdated();
    signalPropertyChange("Position", position());
}

void MprisMediaPlayer2Player::onMetaInfoUpdated()
{
    QVariantMap updatedMetaData;

    if (mCurrentDocument) {
        const QUrl url = mCurrentDocument->url();
        ImageMetaInfoModel* metaInfoModel = mCurrentDocument->metaInfo();

        // We need some unique id mapping to urls. The index in the list is not reliable,
        // as images can be added/removed during a running slideshow
        // To allow some bidrectional mapping, convert the url to base64 to encode it for
        // matching the D-Bus object path spec
        const QString slideId = QString::fromLatin1(url.toString().toUtf8().toBase64(QByteArray::OmitTrailingEquals));
        const QDBusObjectPath trackId(QLatin1String("/org/kde/gwenview/imagelist/") + slideId);
        updatedMetaData.insert(QStringLiteral("mpris:trackid"),
                        QVariant::fromValue<QDBusObjectPath>(trackId));

        // TODO: for videos also get and report the length
        if (MimeTypeUtils::urlKind(url) != MimeTypeUtils::KIND_VIDEO) {
            // convert seconds in microseconds
            const qlonglong duration = qlonglong(mSlideShow->interval() * 1000000);
            updatedMetaData.insert(QStringLiteral("mpris:length"), duration);
        }

        // TODO: update on metadata changes, given user can edit most of this data

        const QString name = metaInfoModel->getValueForKey(QStringLiteral("General.Name"));
        updatedMetaData.insert(QStringLiteral("xesam:title"), name);
        const QString comment = metaInfoModel->getValueForKey(QStringLiteral("General.Comment"));
        if (!comment.isEmpty()) {
            updatedMetaData.insert(QStringLiteral("xesam:comment"), comment);
        }
        updatedMetaData.insert(QStringLiteral("xesam:url"), url.toString());
        // slight bending of semantics :)
        const KFileItem folderItem(mContextManager->currentDirUrl());
        updatedMetaData.insert(QStringLiteral("xesam:album"), folderItem.text());
        // TODO: hook up with thumbnail cache and pass that as arturl
        // updatedMetaData.insert(QStringLiteral("mpris:artUrl"), url.toString());

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
        const QModelIndex index = mContextManager->dirModel()->indexForUrl(url);
        if (index.isValid()) {
            const double rating = index.data(SemanticInfoDirModel::RatingRole).toInt() / 10.0;
            updatedMetaData.insert(QStringLiteral("xesam:userRating"), rating);
        }
#endif
        // consider export of other metadata where mapping works
    }

    if (updatedMetaData != mMetaData) {
        mMetaData = updatedMetaData;

        signalPropertyChange("Metadata", mMetaData);
    }
}

void MprisMediaPlayer2Player::onRandomActionToggled(bool checked)
{
    signalPropertyChange("Shuffle", checked);
}

void MprisMediaPlayer2Player::onFullScreenActionToggled()
{
    if (!updatePlaybackStatus()) {
        return;
    }

    signalPropertyChange("Position", position());
    signalPropertyChange("PlaybackStatus", mPlaybackStatus);
}

void MprisMediaPlayer2Player::onToggleSlideShowActionChanged()
{
    const bool isEnabled = mToggleSlideShowAction->isEnabled();
    if (mSlideShowEnabled == isEnabled) {
        return;
    }

    mSlideShowEnabled = isEnabled;

    const bool playbackStatusChanged = updatePlaybackStatus();

    signalPropertyChange("CanPlay", mSlideShowEnabled);
    signalPropertyChange("CanPause", mSlideShowEnabled);
    if (playbackStatusChanged) {
        signalPropertyChange("Position", position());
        signalPropertyChange("PlaybackStatus", mPlaybackStatus);
    }
}

void MprisMediaPlayer2Player::onNextActionChanged()
{
    const bool isEnabled = mNextAction->isEnabled();
    if (mNextEnabled == isEnabled) {
        return;
    }

    mNextEnabled = isEnabled;

    signalPropertyChange("CanGoNext", mNextEnabled);
}


void MprisMediaPlayer2Player::onPreviousActionChanged()
{
    const bool isEnabled = mPreviousAction->isEnabled();
    if (mPreviousEnabled == isEnabled) {
        return;
    }

    mPreviousEnabled = isEnabled;

    signalPropertyChange("CanGoPrevious", mPreviousEnabled);
}

}
