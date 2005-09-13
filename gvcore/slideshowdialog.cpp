// vim: set tabstop=4 shiftwidth=4 noexpandtab
// kate: indent-mode csands; indent-width 4; replace-tabs-save off; replace-tabs off; replace-trailing-space-save off; space-indent off; tabs-indents on; tab-width 4;
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
#include <qspinbox.h>
#include <qcheckbox.h>

// Local
#include "slideshowdialog.moc"

#include "slideshow.h"
#include "slideshowdialogbase.h"
namespace Gwenview {


SlideShowDialog::SlideShowDialog(QWidget* parent,SlideShow* slideShow)
: KDialogBase(
	parent,0,true,QString::null,KDialogBase::Ok|KDialogBase::Cancel,
	KDialogBase::Ok,true)
, mSlideShow(slideShow)
{
	mContent=new SlideShowDialogBase(this);
	setMainWidget(mContent);
	setCaption(mContent->caption());
	
	mContent->mDelay->setValue(mSlideShow->delay());
	mContent->mLoop->setChecked(mSlideShow->loop());
	mContent->mRandomOrder->setChecked(mSlideShow->random());
}


void SlideShowDialog::slotOk() {
	mSlideShow->setDelay(mContent->mDelay->value());
	mSlideShow->setLoop(mContent->mLoop->isChecked());
	mSlideShow->setRandom(mContent->mRandomOrder->isChecked());
	accept();
}

} // namespace
