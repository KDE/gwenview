// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2003 Aurélien Gâteau
 
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
#include <qcheckbox.h>
#include <qlayout.h>
#include <qpainter.h>
#include <qradiobutton.h>
#include <qvbuttongroup.h>

// KDE
#include <kcombobox.h>
#include <kdebug.h>
#include <kdialog.h>
#include <klocale.h>
#include <knuminput.h>
#include <kprinter.h>

// Our
#include <gvprintdialog.moc>


const char* STR_TRUE="true";
const char* STR_FALSE="false";


GVPrintDialogPage::GVPrintDialogPage( QWidget *parent, const char *name )
: KPrintDialogPage( parent, name ) {
	setTitle( i18n("Image Settings") );

	QVBoxLayout *layout = new QVBoxLayout( this );
	layout->setMargin( KDialog::marginHint() );
	layout->setSpacing( KDialog::spacingHint() );

	mAddFileName = new QCheckBox( i18n("Print fi&lename below image"), this);
	mAddFileName->setChecked( true );
	layout->addWidget( mAddFileName );

	mBlackWhite = new QCheckBox ( i18n("Print image in &black and white"), this);
	mBlackWhite->setChecked( false );
	layout->addWidget (mBlackWhite );

	mShrinkToFit = new QCheckBox( i18n("Shrink image to &fit, if necessary"), this);
	mShrinkToFit->setChecked( true );
	layout->addWidget( mShrinkToFit );

}

GVPrintDialogPage::~GVPrintDialogPage() {}

void GVPrintDialogPage::getOptions( QMap<QString,QString>& opts,
                                    bool /*incldef*/ ) {
	opts["app-gwenview-printFilename"] = mAddFileName->isChecked() ? STR_TRUE : STR_FALSE;
	opts["app-gwenview-blackWhite"] = mBlackWhite->isChecked() ? STR_TRUE : STR_FALSE;
	opts["app-gwenview-shrinkToFit"] = mShrinkToFit->isChecked() ? STR_TRUE : STR_FALSE;

}

void GVPrintDialogPage::setOptions( const QMap<QString,QString>& opts ) {
	mAddFileName->setChecked( opts["app-gwenview-printFilename"] == STR_TRUE );
	mBlackWhite->setChecked( opts["app-gwenview-blackWhite"] == STR_TRUE );
	mShrinkToFit->setChecked( opts["app-gwenview-shrinkToFit"] == STR_TRUE );

}

