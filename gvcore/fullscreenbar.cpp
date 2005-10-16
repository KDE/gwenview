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
#include <qpopupmenu.h>
#include <qtimer.h>
#include <qtoolbutton.h>

// KDE
#include <kdebug.h>
#include <kiconloader.h>
#include <kicontheme.h>

// Local
#include "fullscreenbar.moc"
namespace Gwenview {


const int FULLSCREEN_LABEL_RADIUS = 6;
// Intervals are in milliseconds
const int SLIDE_IN_INTERVAL = 4;
const int SLIDE_OUT_INTERVAL = 12;
// Step is in pixels
const int SLIDE_STEP = 4;


/**
 * A KAction aware QToolButton
 */
class ActionButton : public QToolButton {
	
	/*
	 * This is hackish: We want our ActionButton to get updated whenever the
	 * KAction gets updated (for example when the icon changes). Unfortunately,
	 * containers handling in KAction is hardcoded to KToolBar, QPopupMenu and
	 * QMenuBar.
	 * To work around this, we plug the KAction to a DummyPopupMenu which
	 * inherits from QPopupMenu and notifies our ActionButton whenever it is
	 * aware of a change in the KAction.
	 */
	class DummyPopupMenu : public QPopupMenu {
	public:
		DummyPopupMenu(ActionButton* btn)
		: mActionButton(btn) {}

		void updateItem(int /*id*/) {
			mActionButton->updateFromAction();
		}
		
		void menuContentsChanged() {
			mActionButton->updateFromAction();
		}

	private:
		ActionButton* mActionButton;
	};
	
public:
	ActionButton(QWidget* parent, KAction* action)
	: QToolButton(parent), mDummyPopupMenu(this), mAction(action)
	{
		action->plug(&mDummyPopupMenu);
		setAutoRaise(true);
		updateFromAction();

		connect(this, SIGNAL(clicked()), action, SLOT(activate()) );
		connect(action, SIGNAL(enabled(bool)), this, SLOT(setEnabled(bool)) );
	}

private:
	void updateFromAction() {
		setIconSet(MainBarIconSet(mAction->icon()));
		setTextLabel(mAction->plainText(), true);
		setEnabled(mAction->isEnabled());
	}
	DummyPopupMenu mDummyPopupMenu;
	KAction* mAction;
};


static void fillMask(QPainter& painter, const QRect& rect) {
	painter.fillRect(
		rect.left(),
		rect.top(),
		rect.width() - FULLSCREEN_LABEL_RADIUS,
		rect.bottom(),
		painter.brush());

	painter.fillRect(
		rect.right() - FULLSCREEN_LABEL_RADIUS,
		rect.top(),
		FULLSCREEN_LABEL_RADIUS,
		rect.height() - FULLSCREEN_LABEL_RADIUS,
		painter.brush());

	painter.drawPie(
		rect.right() - 2*FULLSCREEN_LABEL_RADIUS,
		rect.bottom() - 2*FULLSCREEN_LABEL_RADIUS,
		FULLSCREEN_LABEL_RADIUS*2, FULLSCREEN_LABEL_RADIUS*2,
		0, -16*90);
}


static void drawBorder(QPainter& painter, const QRect& rect) {
	painter.drawLine(
		rect.right() - 1,
		rect.top(),
		rect.right() - 1,
		rect.bottom() - FULLSCREEN_LABEL_RADIUS);

	painter.drawLine(
		rect.right() - FULLSCREEN_LABEL_RADIUS,
		rect.bottom() - 1,
		rect.left(),
		rect.bottom() - 1);

	painter.drawArc(
		rect.right() - 2*FULLSCREEN_LABEL_RADIUS,
		rect.bottom() - 2*FULLSCREEN_LABEL_RADIUS,
		FULLSCREEN_LABEL_RADIUS*2, FULLSCREEN_LABEL_RADIUS*2,
		0, -16*90);
}


enum BarState { OUT, SLIDING_OUT, SLIDING_IN, IN };


struct FullScreenBar::Private {
	QTimer mTimer;
	BarState mState;
	QHBoxLayout* mLayout;
};


FullScreenBar::FullScreenBar(QWidget* parent)
: QLabel(parent) {
	d=new Private;
	d->mState=IN;

	QColor bg=colorGroup().highlight();
	QColor fg=colorGroup().highlightedText();
	QPalette pal(palette());
	pal.setColor(QColorGroup::Background, bg);
	pal.setColor(QColorGroup::Foreground, fg);
	pal.setColor(QColorGroup::Button, bg);
	pal.setColor(QColorGroup::ButtonText, fg);
	setPalette(pal);

	QGridLayout* outerLayout=new QGridLayout(this,2,2);
	d->mLayout=new QHBoxLayout;
	outerLayout->addLayout(d->mLayout, 0, 0);
	outerLayout->addItem(
		new QSpacerItem(FULLSCREEN_LABEL_RADIUS, FULLSCREEN_LABEL_RADIUS),
		1, 0);
	outerLayout->addItem(
		new QSpacerItem(FULLSCREEN_LABEL_RADIUS, FULLSCREEN_LABEL_RADIUS),
		0, 1);

	// Make sure the widget is resized when d->mLayout grows
	outerLayout->setResizeMode(QLayout::Fixed);

	// Timer
	connect(&d->mTimer, SIGNAL(timeout()), this, SLOT(slotUpdateSlide()) );
}


void FullScreenBar::plugActions(const KActionPtrList& actions) {
	KActionPtrList::ConstIterator
		it=actions.begin(),
		end=actions.end();
	for (; it!=end; ++it) {
		ActionButton* btn=new ActionButton(this, *it);
		d->mLayout->addWidget(btn);
	}
}


void FullScreenBar::plugWidget(QWidget* widget) {
	d->mLayout->addWidget(widget);
}


FullScreenBar::~FullScreenBar() {
	delete d;
}


void FullScreenBar::resizeEvent(QResizeEvent* event) {
	QSize size=event->size();
	QRect rect=QRect(QPoint(0, 0), size);

	QPainter painter;
	// Create a mask for the text
	QBitmap mask(size,true);
	painter.begin(&mask);
	painter.setBrush(Qt::white);
	fillMask(painter, rect);
	painter.end();

	// Draw the background on a pixmap
	QPixmap pixmap(size);
	painter.begin(&pixmap, this);
	painter.eraseRect(rect);
	drawBorder(painter, rect);
	painter.end();

	// Update the label
	setPixmap(pixmap);
	setMask(mask);
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


void FullScreenBar::toggle() {
	if (d->mState==SLIDING_OUT || d->mState==OUT) {
		slideIn();
	} else {
		slideOut();
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
