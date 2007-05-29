// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
#include "fullscreenbar.moc"

// Qt
#include <QApplication>
#include <QEvent>
#include <QTimeLine>
#include <QTimer>

// KDE

// Local

namespace Gwenview {


static const int SLIDE_DURATION = 300;
static const int AUTO_HIDE_TIMEOUT = 3000;


struct FullScreenBarPrivate {
	QTimeLine* mTimeLine;
	QTimer* mAutoHideTimer;

	void startTimeLine() {
		if (mTimeLine->state() != QTimeLine::Running) {
			mTimeLine->start();
		}
	}
};


FullScreenBar::FullScreenBar(QWidget* parent)
: KToolBar(parent)
, d(new FullScreenBarPrivate) {
	setToolButtonStyle(Qt::ToolButtonIconOnly);
	setAutoFillBackground(true);

	d->mTimeLine = new QTimeLine(SLIDE_DURATION, this);
	connect(d->mTimeLine, SIGNAL(valueChanged(qreal)), SLOT(moveBar(qreal)) );
	connect(d->mTimeLine, SIGNAL(finished()), SLOT(slotTimeLineFinished()) );

	d->mAutoHideTimer = new QTimer(this);
	d->mAutoHideTimer->setInterval(AUTO_HIDE_TIMEOUT);
	d->mAutoHideTimer->setSingleShot(true);
	connect(d->mAutoHideTimer, SIGNAL(timeout()), SLOT(slideOut()) );
}


FullScreenBar::~FullScreenBar() {
	delete d;
}


void FullScreenBar::moveBar(qreal value) {
	move(0, -height() + int(value * height()) );
}


void FullScreenBar::setActivated(bool activated) {
	if (activated) {
		qApp->installEventFilter(this);
		slideIn();
	} else {
		qApp->removeEventFilter(this);
		hide();
	}
}


void FullScreenBar::slideOut() {
	d->mTimeLine->setDirection(QTimeLine::Backward);
	d->startTimeLine();
}


void FullScreenBar::slideIn() {
	if (!isVisible()) {
		move(0, -150);
		show();
	}
	d->mTimeLine->setDirection(QTimeLine::Forward);
	d->startTimeLine();
}


bool FullScreenBar::eventFilter(QObject*, QEvent* event) {
	if (event->type() == QEvent::MouseMove) {
		if (y() == 0) {
			d->mAutoHideTimer->start();
		} else {
			slideIn();
		}
		return false;
	}

	return false;
}


void FullScreenBar::slotTimeLineFinished() {
	if (d->mTimeLine->direction() == QTimeLine::Forward) {
		d->mAutoHideTimer->start();
	}
}


} // namespace
