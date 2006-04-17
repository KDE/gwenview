// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2006 Aurélien Gâteau

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
// Self
#include "bcgdialog.moc"

// Qt
#include <qslider.h>
#include <qvbox.h>

// KDE
#include <klocale.h>

// Local
#include "imageview.h"
#include "bcgdialogbase.h"

namespace Gwenview {


struct BCGDialog::Private {
	ImageView* mView;
	BCGDialogBase* mContent;
};


BCGDialog::BCGDialog(ImageView* view) 
: KDialogBase(view, "bcg_dialog", false /* modal */, 
	i18n("Adjust Brightness/Contrast/Gamma"), KDialogBase::Close | KDialogBase::Default)
{
	d=new Private;
	d->mView=view;
	d->mContent=new BCGDialogBase(this);
	setMainWidget(d->mContent);
	connect(d->mContent->mBSlider, SIGNAL(valueChanged(int)),
		view, SLOT(setBrightness(int)) );
	connect(d->mContent->mCSlider, SIGNAL(valueChanged(int)),
		view, SLOT(setContrast(int)) );
	connect(d->mContent->mGSlider, SIGNAL(valueChanged(int)),
		view, SLOT(setGamma(int)) );
	
	connect(view, SIGNAL(bcgChanged()),
		this, SLOT(updateFromImageView()) );
}


BCGDialog::~BCGDialog() {
	delete d;
}


void BCGDialog::slotDefault() {
	d->mView->setBrightness(0);	
	d->mView->setContrast(0);	
	d->mView->setGamma(0);	
	updateFromImageView();
}


void BCGDialog::updateFromImageView() {
	d->mContent->mBSlider->setValue(d->mView->brightness());
	d->mContent->mCSlider->setValue(d->mView->contrast());
	d->mContent->mGSlider->setValue(d->mView->gamma());
}


} // namespace
