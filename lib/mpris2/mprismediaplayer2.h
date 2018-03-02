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

#ifndef MPRISMEDIAPLAYER2_H
#define MPRISMEDIAPLAYER2_H

#include "dbusabstractadaptor.h"
// Qt
#include <QStringList>

class QAction;

// Used QGuiApplication::desktopFileName() only available with Qt 5.7
// DesktopEntry is an optional property for MPRIS MediaPlayer2,
// so simply only supported for Qt 5.7 & later
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
#define GV_SUPPORT_MPRIS_DESKTOPENTRY 1
#endif

namespace Gwenview
{

// https://specifications.freedesktop.org/mpris-spec/latest/Media_Player.html
class MprisMediaPlayer2 : public DBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")

    Q_PROPERTY(bool CanQuit READ canQuit CONSTANT)
    Q_PROPERTY(bool CanRaise READ canRaise CONSTANT)
    Q_PROPERTY(bool CanSetFullscreen READ canSetFullscreen CONSTANT)

    Q_PROPERTY(QString Identity READ identity CONSTANT)
#ifdef GV_SUPPORT_MPRIS_DESKTOPENTRY
    Q_PROPERTY(QString DesktopEntry READ desktopEntry CONSTANT)
#endif

    Q_PROPERTY(bool HasTrackList READ hasTrackList CONSTANT)
    Q_PROPERTY(bool Fullscreen READ isFullscreen WRITE setFullscreen)

    Q_PROPERTY(QStringList SupportedUriSchemes READ supportedUriSchemes CONSTANT)
    Q_PROPERTY(QStringList SupportedMimeTypes READ supportedMimeTypes CONSTANT)

public:
    MprisMediaPlayer2(const QString &objectDBusPath, QAction* fullScreenAction, QObject* parent);
    ~MprisMediaPlayer2() override;

public Q_SLOTS: // D-Bus API
    void Quit();
    void Raise();

private:
    bool canQuit() const;
    bool canRaise() const;
    bool canSetFullscreen() const;
    bool hasTrackList() const;

    QString identity() const;
#ifdef GV_SUPPORT_MPRIS_DESKTOPENTRY
    QString desktopEntry() const;
#endif
    bool isFullscreen() const;
    void setFullscreen(bool isFullscreen);

    QStringList supportedUriSchemes() const;
    QStringList supportedMimeTypes() const;

    void onFullScreenActionToggled(bool checked);

private:
    QAction* mFullScreenAction;
};

}

#endif
