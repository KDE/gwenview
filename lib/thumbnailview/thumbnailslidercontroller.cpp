// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
#include "thumbnailslidercontroller.h"

// Qt
#include <QSlider>
#include <QToolTip>

// KDE

// Local
#include <lib/thumbnailview/thumbnailview.h>

namespace Gwenview {


struct ThumbnailSliderControllerPrivate {
	QSlider* mSlider;
};


ThumbnailSliderController::ThumbnailSliderController(ThumbnailView* view, QSlider* slider)
: QObject(view)
, d(new ThumbnailSliderControllerPrivate) {
	d->mSlider = slider;
	connect(slider, SIGNAL(valueChanged(int)),
		view, SLOT(setThumbnailSize(int)) );
	connect(slider, SIGNAL(actionTriggered(int)),
		SLOT(slotActionTriggered(int)) );
}


ThumbnailSliderController::~ThumbnailSliderController() {
	delete d;
}


void ThumbnailSliderController::updateToolTip() {
	// FIXME: i18n?
	const int size = d->mSlider->sliderPosition();
	const QString text = QString("%1 x %2").arg(size).arg(size);
	d->mSlider->setToolTip(text);
}

void ThumbnailSliderController::slotActionTriggered(int actionTriggered) {
	updateToolTip();

	if (actionTriggered != QAbstractSlider::SliderNoAction) {
		// If we are updating because of a direct action on the slider, show
		// the tooltip immediatly.
		const QPoint pos = d->mSlider->mapToGlobal(QPoint(0, d->mSlider->height() / 2));
		QToolTip::showText(pos, d->mSlider->toolTip(), d->mSlider);
	}
}


} // namespace
