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
#include "thumbnailslider.moc"

// Qt
#include <QHBoxLayout>
#include <QSlider>
#include <QToolButton>
#include <QToolTip>

// KDE
#include <kicon.h>

// Local
#include <lib/thumbnailview/thumbnailview.h>

namespace Gwenview {


struct ThumbnailSliderPrivate {
	QToolButton* mZoomOutButton;
	QToolButton* mZoomInButton;
	QSlider* mSlider;

	void updateButtons() {
		mZoomOutButton->setEnabled(mSlider->value() > mSlider->minimum());
		mZoomInButton->setEnabled(mSlider->value() < mSlider->maximum());
	}
};

static QToolButton* createZoomButton(const char* iconName) {
	QToolButton* button = new QToolButton;
	button->setIcon(KIcon(iconName));
	button->setAutoRaise(true);
	button->setAutoRepeat(true);
	return button;
}

ThumbnailSlider::ThumbnailSlider(QWidget* parent)
: QWidget(parent)
, d(new ThumbnailSliderPrivate) {
	d->mZoomInButton = createZoomButton("zoom-in");
	d->mZoomOutButton = createZoomButton("zoom-out");

	d->mSlider = new QSlider;
	d->mSlider->setOrientation(Qt::Horizontal);
	d->mSlider->setRange(48, 256);

	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addWidget(d->mZoomOutButton);
	layout->addWidget(d->mSlider);
	layout->addWidget(d->mZoomInButton);

	connect(d->mSlider, SIGNAL(actionTriggered(int)),
		SLOT(slotActionTriggered(int)) );
	connect(d->mSlider, SIGNAL(valueChanged(int)),
		SIGNAL(valueChanged(int)));

	connect(d->mZoomOutButton, SIGNAL(clicked()),
		SLOT(zoomOut()));
	connect(d->mZoomInButton, SIGNAL(clicked()),
		SLOT(zoomIn()));
}


ThumbnailSlider::~ThumbnailSlider() {
	delete d;
}


int ThumbnailSlider::value() const {
	return d->mSlider->value();
}


void ThumbnailSlider::setValue(int value) {
	d->mSlider->setValue(value);
	d->updateButtons();
}


void ThumbnailSlider::updateToolTip() {
	// FIXME: i18n?
	const int size = d->mSlider->sliderPosition();
	const QString text = QString("%1 x %2").arg(size).arg(size);
	d->mSlider->setToolTip(text);
}

void ThumbnailSlider::slotActionTriggered(int actionTriggered) {
	updateToolTip();

	if (actionTriggered != QAbstractSlider::SliderNoAction) {
		// If we are updating because of a direct action on the slider, show
		// the tooltip immediatly.
		const QPoint pos = d->mSlider->mapToGlobal(QPoint(0, d->mSlider->height() / 2));
		QToolTip::showText(pos, d->mSlider->toolTip(), d->mSlider);
	}

	d->updateButtons();
}


void ThumbnailSlider::zoomOut() {
	d->mSlider->triggerAction(QAbstractSlider::SliderPageStepSub);
}


void ThumbnailSlider::zoomIn() {
	d->mSlider->triggerAction(QAbstractSlider::SliderPageStepAdd);
}


} // namespace
