// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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
#ifndef HUDTHEME_H
#define HUDTHEME_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QBrush>
#include <QPen>

// KF

// Local

namespace Gwenview
{

namespace HudTheme
{

struct RenderInfo
{
    RenderInfo()
    : borderRadius(0)
    , padding(0)
    {}
    qreal borderRadius;
    QPen borderPen;
    QBrush bgBrush;
    qreal padding;
    QPen textPen;
};

enum State
{
    NormalState,
    MouseOverState,
    DownState
};

enum WidgetType
{
    ButtonWidget,
    FrameWidget,
    CountDown,
    SliderWidgetHandle,
    SliderWidgetGroove
};

GWENVIEWLIB_EXPORT RenderInfo renderInfo(WidgetType, State = NormalState);

} // HudTheme namespace

} // Gwenview namespace

#endif /* HUDTHEME_H */
