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

#ifndef MPRIS2SERVICE_H
#define MPRIS2SERVICE_H

#include <gwenviewlib_export.h>
// Qt
#include <QObject>
#include <QString>

class QAction;

namespace Gwenview
{
class SlideShow;
class ContextManager;

class GWENVIEWLIB_EXPORT Mpris2Service : public QObject
{
    Q_OBJECT

public:
    Mpris2Service(SlideShow* slideShow, ContextManager* contextManager,
                  QAction* toggleSlideShowAction, QAction* fullScreenAction,
                  QAction* previousAction, QAction* nextAction, QObject* parent);
    ~Mpris2Service() override;

private:
    void registerOnDBus();
    void unregisterOnDBus();

    void onLockScreenLockedChanged(bool isLocked);

private:
    QString mMpris2ServiceName;
};

}

#endif
