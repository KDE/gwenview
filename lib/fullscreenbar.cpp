// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "fullscreenbar.moc"

// Qt
#include <QAction>
#include <QApplication>
#include <QDesktopWidget>
#include <QBitmap>
#include <QEvent>
#include <QTimeLine>
#include <QTimer>
#include <QToolButton>

// KDE
#include <kdebug.h>
#include <klocale.h>

// Local

namespace Gwenview {


static const int SLIDE_DURATION = 150;
static const int AUTO_HIDE_CURSOR_TIMEOUT = 1000;

// How long before the bar slide out after switching to fullscreen
static const int INITIAL_HIDE_TIMEOUT = 2000;


struct FullScreenBarPrivate {
	FullScreenBar* that;
	QTimeLine* mTimeLine;
	QTimer* mAutoHideCursorTimer;
	bool mAutoHidingEnabled;

	void startTimeLine() {
		if (mTimeLine->state() != QTimeLine::Running) {
			mTimeLine->start();
		}
	}

	void hideCursor() {
		QBitmap empty(32, 32);
		empty.clear();
		QCursor blankCursor(empty, empty);
		QApplication::setOverrideCursor(blankCursor);
	}

	/**
	 * Returns the rectangle the bar should occupy when its fully visible
	 * The rectangle is in global coords
	 */
	QRect barRect() const {
		const QRect screen = QApplication::desktop()->screenGeometry();
		return QRect(screen.topLeft(), that->size());
	}

	bool shouldHide() const {
		Q_ASSERT(that->parentWidget());

		if (!mAutoHidingEnabled) {
			return false;
		}
		if (barRect().contains(QCursor::pos())) {
			return false;
		}
		if (qApp->activePopupWidget()) {
			return false;
		}
		return true;
	}
};


FullScreenBar::FullScreenBar(QWidget* parent)
: QFrame(parent)
, d(new FullScreenBarPrivate) {
	d->that = this;
	d->mAutoHidingEnabled = true;
	setObjectName("fullScreenBar");

	d->mTimeLine = new QTimeLine(SLIDE_DURATION, this);
	connect(d->mTimeLine, SIGNAL(valueChanged(qreal)), SLOT(moveBar(qreal)) );

	d->mAutoHideCursorTimer = new QTimer(this);
	d->mAutoHideCursorTimer->setInterval(AUTO_HIDE_CURSOR_TIMEOUT);
	d->mAutoHideCursorTimer->setSingleShot(true);
	connect(d->mAutoHideCursorTimer, SIGNAL(timeout()), SLOT(slotAutoHideCursorTimeout()) );

	hide();
}


FullScreenBar::~FullScreenBar() {
	delete d;
}

QSize FullScreenBar::sizeHint() const {
	int width = QApplication::desktop()->screenGeometry(this).width();
	return QSize(width, QFrame::sizeHint().height());
}

void FullScreenBar::moveBar(qreal value) {
	move(0, -height() + int(value * height()) );

	// For some reason, if Gwenview is started with command line options to
	// start a slideshow, the bar might end up below the view. Calling raise()
	// here fixes it.
	raise();
}


void FullScreenBar::setActivated(bool activated) {
	if (activated) {
		// Delay installation of event filter because switching to fullscreen
		// cause a few window adjustments, which seems to generate unwanted
		// mouse events, which cause the bar to slide in.
		QTimer::singleShot(500, this, SLOT(delayedInstallEventFilter()));

		// Make sure the widget is visible on start
		move(0, 0);
		raise();
		show();
	} else {
		qApp->removeEventFilter(this);
		hide();
		d->mAutoHideCursorTimer->stop();
		QApplication::restoreOverrideCursor();
	}
}


void FullScreenBar::delayedInstallEventFilter() {
	qApp->installEventFilter(this);
	if (d->shouldHide()) {
		QTimer::singleShot(INITIAL_HIDE_TIMEOUT, this, SLOT(slideOut()));
		d->hideCursor();
	}
}


void FullScreenBar::slotAutoHideCursorTimeout() {
	if (d->shouldHide()) {
		d->hideCursor();
	} else {
		d->mAutoHideCursorTimer->start();
	}
}


void FullScreenBar::slideOut() {
	d->mTimeLine->setDirection(QTimeLine::Backward);
	d->startTimeLine();
}


void FullScreenBar::slideIn() {
	d->mTimeLine->setDirection(QTimeLine::Forward);
	d->startTimeLine();
}


bool FullScreenBar::eventFilter(QObject* object, QEvent* event) {
	if (event->type() == QEvent::MouseMove) {
		QApplication::restoreOverrideCursor();
		d->mAutoHideCursorTimer->start();
		QPoint pos = parentWidget()->mapFromGlobal(QCursor::pos());
		if (y() == 0) {
			if (d->shouldHide()) {
				slideOut();
			}
		} else {
			if (pos.y() < height()) {
				slideIn();
			}
		}
		return false;
	}

	// Filtering message on tooltip text for CJK to remove accelerators.
	// Quoting ktoolbar.cpp:
	// """
	// CJK languages use more verbose accelerator marker: they add a Latin
	// letter in parenthesis, and put accelerator on that. Hence, the default
	// removal of ampersand only may not be enough there, instead the whole
	// parenthesis construct should be removed. Provide these filtering i18n
	// messages so that translators can use Transcript for custom removal.
	// """
	if (event->type() == QEvent::Show || event->type() == QEvent::Paint) {
		QToolButton* button = qobject_cast<QToolButton*>(object);
		if (button && !button->actions().isEmpty()) {
			QAction* action = button->actions().first();
			button->setToolTip(i18nc("@info:tooltip of custom toolbar button", "%1", action->toolTip()));
		}
	}

	return false;
}


void FullScreenBar::setAutoHidingEnabled(bool value) {
	d->mAutoHidingEnabled = value;
}


} // namespace
