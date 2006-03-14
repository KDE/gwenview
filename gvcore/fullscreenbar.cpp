// vim:set tabstop=4 shiftwidth=4 noexpandtab:
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
// Qt
#include <qbitmap.h>
#include <qevent.h>
#include <qlayout.h>
#include <qpainter.h>
#include <qtimer.h>

// KDE
#include <kdebug.h>

// Local
#include "fullscreenbar.moc"
namespace Gwenview {


const int FULLSCREEN_ICON_SIZE = 32;
const int FULLSCREEN_LABEL_RADIUS = 6;
// Intervals are in milliseconds
const int SLIDE_IN_INTERVAL = 4;
const int SLIDE_OUT_INTERVAL = 12;
// Step is in pixels
const int SLIDE_STEP = 4;


static void fillMask(QPainter& painter, const QRect& rect) {
	painter.fillRect(
		rect.left(),
		rect.top(),
		rect.width() - FULLSCREEN_LABEL_RADIUS,
		rect.height(),
		painter.brush());

	painter.fillRect(
		rect.right() - FULLSCREEN_LABEL_RADIUS + 1,
		rect.top(),
		FULLSCREEN_LABEL_RADIUS,
		rect.height() - FULLSCREEN_LABEL_RADIUS,
		painter.brush());

	painter.drawPie(
		rect.right() - 2*FULLSCREEN_LABEL_RADIUS + 1,
		rect.bottom() - 2*FULLSCREEN_LABEL_RADIUS + 1,
		FULLSCREEN_LABEL_RADIUS*2, FULLSCREEN_LABEL_RADIUS*2,
		0, -16*90);
}


enum BarState { OUT, SLIDING_OUT, SLIDING_IN, IN };


struct FullScreenBar::Private {
	QTimer mTimer;
	BarState mState;
	bool mFirstShow;
};


FullScreenBar::FullScreenBar(QWidget* parent)
: KToolBar(parent, "FullScreenBar") {
	d=new Private;
	d->mState=OUT;
	d->mFirstShow=true;
	setIconSize(FULLSCREEN_ICON_SIZE);
	setMovingEnabled(false);

	QColor bg=colorGroup().highlight();
	QColor fg=colorGroup().highlightedText();
	QPalette pal(palette());
	pal.setColor(QColorGroup::Background, bg);
	pal.setColor(QColorGroup::Foreground, fg);
	pal.setColor(QColorGroup::Button, bg);
	pal.setColor(QColorGroup::ButtonText, fg);
	setPalette(pal);
	
	// Timer
	connect(&d->mTimer, SIGNAL(timeout()), this, SLOT(slotUpdateSlide()) );
}


FullScreenBar::~FullScreenBar() {
	delete d;
}


void FullScreenBar::resizeEvent(QResizeEvent* event) {
	KToolBar::resizeEvent(event);

	// Create a mask
	QPainter painter;
	QBitmap mask(size(), true);
	painter.begin(&mask);
	painter.setBrush(Qt::white);
	fillMask(painter, rect());
	painter.end();
	
	setMask(mask);
}


void FullScreenBar::showEvent(QShowEvent* event) {
	KToolBar::showEvent(event);
	// Make sure the bar position corresponds to the OUT state
	if (!d->mFirstShow) return;
	d->mFirstShow=false;
	move(0, -height());
	layout()->setResizeMode(QLayout::Fixed);
}


void FullScreenBar::slideIn() {
	if (d->mState!=IN) {
		d->mState=SLIDING_IN;
		d->mTimer.start(SLIDE_IN_INTERVAL);
	}
}


void FullScreenBar::slideOut() {
	if (d->mState!=OUT) {
		d->mState=SLIDING_OUT;
		d->mTimer.start(SLIDE_OUT_INTERVAL);
	}
}


void FullScreenBar::slotUpdateSlide() {
	int pos=y();

	switch (d->mState) {
	case SLIDING_OUT:
		pos-=SLIDE_STEP;
		if (pos<=-height()) {
			d->mState=OUT;
			d->mTimer.stop();
		}
		break;
	case SLIDING_IN:
		pos+=SLIDE_STEP;
		if (pos>=0) {
			pos=0;
			d->mState=IN;
			d->mTimer.stop();
		}
		break;
	default:
		kdWarning() << k_funcinfo << "We should not get there\n";
	}
	move(0, pos);
}

} // namespace
