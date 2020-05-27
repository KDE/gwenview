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

#include "mprismediaplayer2.h"

// Qt
#include <QGuiApplication>
#include <QAction>

namespace Gwenview
{

MprisMediaPlayer2::MprisMediaPlayer2(const QString &objectDBusPath,
                                     QAction* fullScreenAction,
                                     QObject* parent)
    : DBusAbstractAdaptor(objectDBusPath, parent)
    , mFullScreenAction(fullScreenAction)
{
    connect(mFullScreenAction, &QAction::toggled,
            this, &MprisMediaPlayer2::onFullScreenActionToggled);
}

MprisMediaPlayer2::~MprisMediaPlayer2()
{
}

bool MprisMediaPlayer2::canQuit() const
{
    return true;
}

bool MprisMediaPlayer2::canRaise() const
{
    // spec: "If true, calling Raise will cause the media application to attempt to bring its user interface to the front,
    // Which seems to be about the app specific control UI, less about the rendered media (display).
    // Could perhaps be supported by pulling in the toppanel when invoked?
    return false;
}

bool MprisMediaPlayer2::canSetFullscreen() const
{
    return true;
}

bool MprisMediaPlayer2::hasTrackList() const
{
    return false;
}

void MprisMediaPlayer2::Quit()
{
    QGuiApplication::quit();
}

void MprisMediaPlayer2::Raise()
{
}

QString MprisMediaPlayer2::identity() const
{
    return QGuiApplication::applicationDisplayName();
}

QString MprisMediaPlayer2::desktopEntry() const
{
    return QGuiApplication::desktopFileName();
}

QStringList MprisMediaPlayer2::supportedUriSchemes() const
{
    return QStringList();
}

QStringList MprisMediaPlayer2::supportedMimeTypes() const
{
    return QStringList();
}

bool MprisMediaPlayer2::isFullscreen() const
{
    return mFullScreenAction->isChecked();
}

void MprisMediaPlayer2::setFullscreen(bool isFullscreen)
{
    if (isFullscreen == mFullScreenAction->isChecked()) {
        return;
    }

    mFullScreenAction->trigger();
}

void MprisMediaPlayer2::onFullScreenActionToggled(bool checked)
{
    signalPropertyChange(QStringLiteral("Fullscreen"), checked);
}

}
