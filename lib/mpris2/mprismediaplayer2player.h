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

#ifndef MPRISMEDIAPLAYER2PLAYER_H
#define MPRISMEDIAPLAYER2PLAYER_H

#include "dbusabstractadaptor.h"
// lib
#include <document/document.h>

class QDBusObjectPath;
class QAction;

namespace Gwenview
{
class SlideShow;
class ContextManager;

// https://specifications.freedesktop.org/mpris-spec/latest/Player_Interface.html
class MprisMediaPlayer2Player : public DBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")

    Q_PROPERTY(QString PlaybackStatus READ playbackStatus)
    Q_PROPERTY(double Rate READ rate WRITE setRate)
    Q_PROPERTY(QVariantMap Metadata READ metadata)
    Q_PROPERTY(double Volume READ volume WRITE setVolume)
    Q_PROPERTY(qlonglong Position READ position)
    Q_PROPERTY(double MinimumRate READ minimumRate CONSTANT)
    Q_PROPERTY(double MaximumRate READ maximumRate CONSTANT)
    Q_PROPERTY(bool Shuffle READ isShuffle WRITE setShuffle)

    Q_PROPERTY(bool CanControl READ canControl CONSTANT)
    Q_PROPERTY(bool CanPlay READ canPlay)
    Q_PROPERTY(bool CanPause READ canPause)
    Q_PROPERTY(bool CanGoNext READ canGoNext)
    Q_PROPERTY(bool CanGoPrevious READ canGoPrevious)
    Q_PROPERTY(bool CanSeek READ canSeek CONSTANT)

public:
    MprisMediaPlayer2Player(const QString &objectDBusPath,
                            SlideShow* slideShow, ContextManager* contextManager,
                            QAction* toggleSlideShowAction, QAction* fullScreenAction,
                            QAction* previousAction, QAction* nextAction, QObject* parent);
    ~MprisMediaPlayer2Player() override;

public Q_SLOTS: // D-Bus API
    void Next();
    void Previous();
    void Pause();
    void PlayPause();
    void Stop();
    void Play();
    void Seek(qlonglong Offset);
    void SetPosition(const QDBusObjectPath& trackId, qlonglong pos);
    void OpenUri(const QString& uri);

Q_SIGNALS: // D-Bus API
    void Seeked(qlonglong Position) const;

private:
    QString playbackStatus() const;
    double rate() const;
    QVariantMap metadata() const;
    double volume() const;
    qlonglong position() const;
    double minimumRate() const;
    double maximumRate() const;
    bool isShuffle() const;

    bool canGoNext() const;
    bool canGoPrevious() const;
    bool canPlay() const;
    bool canPause() const;
    bool canSeek() const;
    bool canControl() const;

    void setRate(double newRate);
    void setVolume(double volume);
    void setShuffle(bool isShuffle);

    bool updatePlaybackStatus();

    void onSlideShowStateChanged();
    void onCurrentUrlChanged(const QUrl& url);
    void onRandomActionToggled(bool checked);
    void onFullScreenActionToggled();
    void onToggleSlideShowActionChanged();
    void onPreviousActionChanged();
    void onNextActionChanged();
    void onMetaInfoUpdated();

private:
    SlideShow* mSlideShow;
    ContextManager* mContextManager;
    QAction* mToggleSlideShowAction;
    QAction* mFullScreenAction;
    QAction* mPreviousAction;
    QAction* mNextAction;

    bool mSlideShowEnabled;
    bool mPreviousEnabled;
    bool mNextEnabled;
    QString mPlaybackStatus;
    QVariantMap mMetaData;
    Document::Ptr mCurrentDocument;
};

}

#endif
