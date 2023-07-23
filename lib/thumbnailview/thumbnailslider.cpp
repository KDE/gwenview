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
#include "thumbnailslider.h"

// Local
#include <thumbnailview/thumbnailview.h>

// Qt
#include <QToolTip>

// KF

namespace Gwenview
{
struct ThumbnailSliderPrivate {
};

ThumbnailSlider::ThumbnailSlider(QWidget *parent)
    : ZoomSlider(parent)
    , d(new ThumbnailSliderPrivate)
{
    connect(slider(), SIGNAL(actionTriggered(int)), SLOT(slotActionTriggered(int)));
    slider()->setRange(ThumbnailView::MinThumbnailSize, ThumbnailView::MaxThumbnailSize);
}

ThumbnailSlider::~ThumbnailSlider()
{
    delete d;
}

void ThumbnailSlider::slotActionTriggered(int actionTriggered)
{
    updateToolTip();

    if (actionTriggered != QAbstractSlider::SliderNoAction) {
        // If we are updating because of a direct action on the slider, show
        // the tooltip immediately.
        const QPoint pos = slider()->mapToGlobal(QPoint(0, slider()->height() / 2));
        QToolTip::showText(pos, slider()->toolTip(), slider());
    }
}

void ThumbnailSlider::updateToolTip()
{
    // FIXME: i18n?
    const int size = qRound(slider()->sliderPosition() * devicePixelRatioF());
    const QString text = QStringLiteral("%1 x %2").arg(size).arg(size);
    slider()->setToolTip(text);
}

} // namespace

#include "moc_thumbnailslider.cpp"
