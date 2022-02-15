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
// Self
#include <hud/hudtheme.h>

// Qt
#include <QMap>

// KF

// Local

namespace Gwenview
{
namespace HudTheme
{
struct RenderInfoSet {
    QMap<HudTheme::State, HudTheme::RenderInfo> infos;
};

static QLinearGradient createGradient()
{
    QLinearGradient gradient(0, 0, 0, 1.);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    return gradient;
}

RenderInfo renderInfo(WidgetType widget, State state)
{
    static QMap<WidgetType, RenderInfoSet> renderInfoMap;
    if (renderInfoMap.isEmpty()) {
        QLinearGradient gradient;

        // Button, normal
        RenderInfo button;
        button.borderRadius = 5;
        button.borderPen = QPen(QColor::fromHsvF(0, 0, .25, .8));
        gradient = createGradient();
        gradient.setColorAt(0, QColor::fromHsvF(0, 0, .34, .66));
        gradient.setColorAt(0.5, QColor::fromHsvF(0, 0, 0, .66));
        button.bgBrush = gradient;
        button.padding = 6;
        button.textPen = QPen(QColor(0xcc, 0xcc, 0xcc));
        renderInfoMap[ButtonWidget].infos[NormalState] = button;

        // Button, over
        RenderInfo overButton = button;
        overButton.bgBrush = gradient;
        overButton.borderPen = QPen(QColor(0xcc, 0xcc, 0xcc));
        renderInfoMap[ButtonWidget].infos[MouseOverState] = overButton;

        // Button, down
        RenderInfo downButton = button;
        gradient = createGradient();
        gradient.setColorAt(0, QColor::fromHsvF(0, 0, .12));
        gradient.setColorAt(0.6, Qt::black);
        downButton.bgBrush = gradient;
        downButton.borderPen = QPen(QColor(0x44, 0x44, 0x44));
        renderInfoMap[ButtonWidget].infos[DownState] = downButton;

        // Frame
        RenderInfo frame;
        gradient = createGradient();
        gradient.setColorAt(0, QColor::fromHsvF(0, 0, 0.1, .6));
        gradient.setColorAt(.6, QColor::fromHsvF(0, 0, 0, .6));
        frame.bgBrush = gradient;
        frame.borderPen = QPen(QColor::fromHsvF(0, 0, .4, .6));
        frame.borderRadius = 8;
        frame.textPen = QPen(QColor(0xcc, 0xcc, 0xcc));
        renderInfoMap[FrameWidget].infos[NormalState] = frame;

        // CountDown
        RenderInfo countDownWidget;
        countDownWidget.bgBrush = QColor::fromHsvF(0, 0, .5);
        countDownWidget.borderPen = QPen(QColor::fromHsvF(0, 0, .8));
        renderInfoMap[CountDown].infos[NormalState] = countDownWidget;

        // SliderWidgetHandle
        RenderInfo sliderWidgetHandle = button;
        sliderWidgetHandle.borderPen = QPen(QColor(0x66, 0x66, 0x66));
        sliderWidgetHandle.borderRadius = 7;
        renderInfoMap[SliderWidgetHandle].infos[NormalState] = sliderWidgetHandle;

        // SliderWidgetHandle, over
        sliderWidgetHandle = overButton;
        sliderWidgetHandle.borderPen = QPen(QColor(0xcc, 0xcc, 0xcc));
        sliderWidgetHandle.borderRadius = 7;
        renderInfoMap[SliderWidgetHandle].infos[MouseOverState] = sliderWidgetHandle;

        // SliderWidgetHandle, down
        sliderWidgetHandle = downButton;
        sliderWidgetHandle.borderRadius = 7;
        renderInfoMap[SliderWidgetHandle].infos[DownState] = sliderWidgetHandle;

        // SliderWidgetGroove
        RenderInfo sliderWidgetGroove = button;
        sliderWidgetGroove.borderPen = QPen(QColor(0x66, 0x66, 0x66));
        gradient = createGradient();
        gradient.setColorAt(0, Qt::black);
        gradient.setColorAt(1, QColor(0x44, 0x44, 0x44));
        sliderWidgetGroove.bgBrush = gradient;
        sliderWidgetGroove.borderRadius = 3;
        renderInfoMap[SliderWidgetGroove].infos[NormalState] = sliderWidgetGroove;

        // SliderWidgetGroove, over
        RenderInfo overSliderWidgetGroove = sliderWidgetGroove;
        overSliderWidgetGroove.borderPen = QPen(QColor(0xcc, 0xcc, 0xcc));
        renderInfoMap[SliderWidgetGroove].infos[MouseOverState] = overSliderWidgetGroove;
    }
    const RenderInfo normalInfo = renderInfoMap[widget].infos.value(NormalState);
    if (state == NormalState) {
        return normalInfo;
    } else {
        return renderInfoMap[widget].infos.value(state, normalInfo);
    }
}

} // HudTheme namespace

} // Gwenview namespace
