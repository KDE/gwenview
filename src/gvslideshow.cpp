// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aurélien Gâteau

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

// Qt includes
#include <qtimer.h>

// KDE includes
#include <kaction.h>
#include <kconfig.h>

// Our includes
#include "gvslideshow.moc"


static const char* CONFIG_DELAY="delay";
static const char* CONFIG_LOOP="loop";


GVSlideShow::GVSlideShow(KAction* first,KAction* next)
: mFirst(first),mNext(next),mDelay(10),mLoop(false)
{
	mTimer=new QTimer(this);
	connect(mTimer,SIGNAL(timeout()),
			this,SLOT(slotTimeout()) );
}


void GVSlideShow::setLoop(bool value)
{
	mLoop=value;
}

void GVSlideShow::setDelay(int delay)
{
	mDelay=delay;
	if (mTimer->isActive()) {
		mTimer->changeInterval(delay*1000);
	}
}


void GVSlideShow::start()
{
	if (!mFirst->isEnabled()
		&& !mNext->isEnabled()) {
		emit finished();
		return;
	}
	if (mFirst->isEnabled()) {
		mFirst->activate();
	}
	mTimer->start(mDelay*1000);
}


void GVSlideShow::stop()
{
	mTimer->stop();
}


void GVSlideShow::slotTimeout()
{
	if (!mNext->isEnabled()) {
		if (mLoop) {
			mFirst->activate();
			return;
		}
		mTimer->stop();
		emit finished();
		return;
	}

	mNext->activate();
}


//-Configuration--------------------------------------------	
void GVSlideShow::readConfig(KConfig* config,const QString& group) {
	config->setGroup(group);
	mDelay=config->readNumEntry(CONFIG_DELAY,10);
	mLoop=config->readBoolEntry(CONFIG_LOOP,false);
}


void GVSlideShow::writeConfig(KConfig* config,const QString& group) const {
	config->setGroup(group);
	config->writeEntry(CONFIG_DELAY,mDelay);
	config->writeEntry(CONFIG_LOOP,mLoop);
}
