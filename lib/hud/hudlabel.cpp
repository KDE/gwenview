// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2011 Aurélien Gâteau <agateau@kde.org>

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
#include "hud/hudlabel.h"

// Local
#include <hud/hudtheme.h>

// KF

// Qt
#include "gwenview_lib_debug.h"
#include <QFontDatabase>
#include <QFontMetrics>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionGraphicsItem>

namespace Gwenview
{
struct HudLabelPrivate {
    QString mText;
};

HudLabel::HudLabel(QGraphicsItem *parent)
    : QGraphicsWidget(parent)
    , d(new HudLabelPrivate)
{
    setCursor(Qt::ArrowCursor);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
}

HudLabel::~HudLabel()
{
    delete d;
}

void HudLabel::setText(const QString &text)
{
    d->mText = text;
    QFont font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    QFontMetrics fm(font);
    QSize minSize = fm.size(0, d->mText);
    setMinimumSize(minSize);
    setPreferredSize(minSize);
}

void HudLabel::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    HudTheme::RenderInfo info = HudTheme::renderInfo(HudTheme::FrameWidget);
    painter->setPen(info.textPen);
    painter->drawText(boundingRect(), Qt::AlignCenter, d->mText);
}

} // namespace
