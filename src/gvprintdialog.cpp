// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - printing support
Copyright (c) 2003 Angelo Naselli
 
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
#include <gvmainwindow.h>
#include <gvpixmap.h>
#include <gvprintdialogpagebase.h>
#include <gvprintdialog.moc>


const char* STR_TRUE="true";
const char* STR_FALSE="false";


GVPrintDialogPage::GVPrintDialogPage( QWidget *parent, const char *name )
: KPrintDialogPage( parent, name ) {
    mGVPixmap = ((GVMainWindow*)parent)->gvPixmap();
	mContent = new GVPrintDialogPageBase(this);
	setTitle( mContent->caption() );

	QVBoxLayout *layout = new QVBoxLayout( this );
	layout->addWidget( mContent );

    connect(mContent->mWidth, SIGNAL( valueChanged( int )), SLOT( setWValue( int )));
    connect(mContent->mHeight, SIGNAL( valueChanged( int )), SLOT( setHValue( int )));
    connect(mContent->mKeepRatio, SIGNAL( toggled( bool )), SLOT( toggleRatio( bool )));
    connect(mContent->mUnits, SIGNAL(activated(const QString &)), SLOT(setNewUnit(const QString& )));

    toggleRatio(mContent->mScale->isChecked() );
}

GVPrintDialogPage::~GVPrintDialogPage() {}

void GVPrintDialogPage::getOptions( QMap<QString,QString>& opts,
                                    bool /*incldef*/ ) {
    opts["app-gwenview-position"] = mContent->mPosition->currentText();
	opts["app-gwenview-printFilename"] = mContent->mAddFileName->isChecked() ? STR_TRUE : STR_FALSE;
    opts["app-gwenview-printComment"] = mContent->mAddComment->isChecked() ? STR_TRUE : STR_FALSE;
	opts["app-gwenview-shrinkToFit"] = mContent->mShrinkToFit->isChecked() ? STR_TRUE : STR_FALSE;

    opts["app-gwenview-scale"] = mContent->mScale->isChecked() ? STR_TRUE : STR_FALSE;
    opts["app-gwenview-scaleKeepRatio"] = mContent->mKeepRatio->isChecked() ? STR_TRUE : STR_FALSE;
    opts["app-gwenview-scaleUnit"] = mContent->mUnits->currentText();
    opts["app-gwenview-scaleWidth"] = QString::number( scaleWidth() );
    opts["app-gwenview-scaleHeight"] = QString::number( scaleHeight() );

}

void GVPrintDialogPage::setOptions( const QMap<QString,QString>& opts ) {
    mContent->mPosition->setCurrentItem( opts["app-gwenview-position"] );
	mContent->mAddFileName->setChecked( opts["app-gwenview-printFilename"] == STR_TRUE );
	mContent->mAddComment->setChecked( opts["app-gwenview-printComment"] == STR_TRUE );
	mContent->mShrinkToFit->setChecked( opts["app-gwenview-shrinkToFit"] == STR_TRUE );
    
    mContent->mScale->setChecked( opts["app-gwenview-scale"] == STR_TRUE );
    mContent->mUnits->setCurrentItem( opts["app-gwenview-scaleUnit"] ); 
	mContent->mKeepRatio->setChecked( opts["app-gwenview-scaleKeepRatio"] == STR_TRUE );
    
    bool ok;
    int val = opts["app-gwenview-scaleWidth"].toInt( &ok );
    if ( ok )
        setScaleWidth( val );
    val = opts["app-gwenview-scaleHeight"].toInt( &ok );
    if ( ok )
        setScaleHeight( val );

    toggleRatio(mContent->mScale->isChecked() );
}

int GVPrintDialogPage::scaleWidth() const {
    return mContent->mWidth->value();
}

int GVPrintDialogPage::scaleHeight() const {
    return mContent->mHeight->value();
}

void GVPrintDialogPage::setScaleWidth( int value ) {
    mContent->mWidth->setValue(value);
}

void GVPrintDialogPage::setScaleHeight( int value ) {
    mContent->mHeight->setValue(value);
}

// SLOTS
void GVPrintDialogPage::setHValue (int value){
    mContent->mWidth->blockSignals(true);
    mContent->mHeight->blockSignals(true);

    if (mContent->mKeepRatio->isChecked()) {
            int w = (mGVPixmap->width() * value) / mGVPixmap->height()  ;
            mContent->mWidth->setValue( w ? w : 1);                            
    }
    mContent->mHeight->setValue(value);     

    mContent->mWidth->blockSignals(false);
    mContent->mHeight->blockSignals(false);
    
}

void GVPrintDialogPage::setWValue (int value){    
    mContent->mWidth->blockSignals(true);
    mContent->mHeight->blockSignals(true);
    if (mContent->mKeepRatio->isChecked()) {
            int h = (mGVPixmap->height() * value) / mGVPixmap->width();            
            mContent->mHeight->setValue( h ? h : 1);
    }
    mContent->mWidth->setValue(value);    
    mContent->mWidth->blockSignals(false);
    mContent->mHeight->blockSignals(false);
}
    
void GVPrintDialogPage::toggleRatio(bool enable) {
    if (enable) {
        float cm = 1;
        if (mContent->mUnits->currentText() == "Millimeters")
            cm = 10;
        else if (mContent->mUnits->currentText() == "Inches")
            cm = 1/(2.54);
        // 15x10 cm 
        float hValue, wValue;
        if (mGVPixmap->height() > mGVPixmap->width()) {
            hValue = cm*15;
            wValue = (mGVPixmap->width() * (hValue))/ mGVPixmap->height();
        } else {
            wValue = cm*15;
            hValue = (mGVPixmap->height() * wValue)/ mGVPixmap->width();            
        }        
        mContent->mWidth->setValue((int)wValue);
        mContent->mHeight->setValue((int)hValue);
    }
}


void GVPrintDialogPage::setNewUnit(const QString& string) {
    mContent->mUnits->setCurrentItem(string);
    toggleRatio(true); // to do better
}



