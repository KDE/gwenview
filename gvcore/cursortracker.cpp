// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aur�ien G�eau

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "cursortracker.h"

// Qt
#include <qevent.h>
#include <qtooltip.h>

namespace Gwenview {

CursorTracker::CursorTracker(const QString& txt, QWidget* reference)
: QLabel(txt, 0, "", WX11BypassWM) {
	reference->setMouseTracking(true);
	reference->installEventFilter(this);
}


/**
 * Overload to make sure the widget size is correct
 */
void CursorTracker::setText(const QString& txt) {
	QLabel::setText(txt);
	adjustSize();
}


bool CursorTracker::eventFilter(QObject* object, QEvent* _event) {
	QWidget* widget=static_cast<QWidget*>(object);
	
	switch (_event->type()) {
	case QEvent::MouseMove: {
		QMouseEvent* event=static_cast<QMouseEvent*>(_event);
		if (widget->rect().contains(event->pos()) || (event->stateAfter() & LeftButton)) {
			show();
			move(event->globalPos().x() + 15, event->globalPos().y() + 15);
		} else {
			hide();
		}
		break;
	}

	case QEvent::MouseButtonRelease: {
		QMouseEvent* event=static_cast<QMouseEvent*>(_event);
		if ( !widget->rect().contains(event->pos()) ) {
			hide();
		}
		break;
	}
	
	default:
		break;
	}

	return false;
}


TipTracker::TipTracker(const QString& txt, QWidget* reference)
: CursorTracker(txt, reference) {
	setPalette(QToolTip::palette());
	setFrameStyle(QFrame::Plain | QFrame::Box);
	setLineWidth(1);
	setAlignment(AlignAuto | AlignTop);
}
	

} // namespace
