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

// Local
#include "gvmainwindow.h"
#include "gvdocument.h"
#include "gvprintdialogpagebase.h"
#include "gvprintdialog.moc"


const char* STR_TRUE="true";
const char* STR_FALSE="false";


GVPrintDialogPage::GVPrintDialogPage( GVDocument* document, QWidget *parent, const char *name )
		: KPrintDialogPage( parent, name ) {
	mDocument = document;
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

void GVPrintDialogPage::getOptions( QMap<QString,QString>& opts, bool /*incldef*/ ) {
	opts["app-gwenview-position"] = QString::number(getPosition(mContent->mPosition->currentText()));
	opts["app-gwenview-printFilename"] = mContent->mAddFileName->isChecked() ? STR_TRUE : STR_FALSE;
	opts["app-gwenview-printComment"] = mContent->mAddComment->isChecked() ? STR_TRUE : STR_FALSE;
	opts["app-gwenview-scale"] = QString::number(
		mContent->mNoScale->isChecked() ? 0
		: mContent->mFitToPage->isChecked() ? 1
		: 2);
	opts["app-gwenview-fitToPage"] = mContent->mFitToPage->isChecked() ? STR_TRUE : STR_FALSE;
	opts["app-gwenview-enlargeToFit"] = mContent->mEnlargeToFit->isChecked() ? STR_TRUE : STR_FALSE;

	opts["app-gwenview-scaleKeepRatio"] = mContent->mKeepRatio->isChecked() ? STR_TRUE : STR_FALSE;
	opts["app-gwenview-scaleUnit"] = QString::number(getUnit(mContent->mUnits->currentText()));
	opts["app-gwenview-scaleWidth"] = QString::number( scaleWidth() );
	opts["app-gwenview-scaleHeight"] = QString::number( scaleHeight() );

}

void GVPrintDialogPage::setOptions( const QMap<QString,QString>& opts ) {
	int val;
	bool ok;
	QString stVal;

	val = opts["app-gwenview-position"].toInt( &ok );
	if (ok) {
		stVal = setPosition(val);
		mContent->mPosition->setCurrentItem(stVal);
	}

	mContent->mAddFileName->setChecked( opts["app-gwenview-printFilename"] == STR_TRUE );
	mContent->mAddComment->setChecked( opts["app-gwenview-printComment"] == STR_TRUE );
	
	val = opts["app-gwenview-scale"].toInt( &ok );
	if (ok) {
		mContent->mScaleGroup->setButton( val );
	} else {
		mContent->mScaleGroup->setButton( 0 );
	}
	mContent->mEnlargeToFit->setChecked( opts["app-gwenview-enlargeToFit"] == STR_TRUE );

	val = opts["app-gwenview-scaleUnit"].toInt( &ok );
	if (ok) {
		stVal = setUnit(val);
		mContent->mUnits->setCurrentItem(stVal);
	}

	mContent->mKeepRatio->setChecked( opts["app-gwenview-scaleKeepRatio"] == STR_TRUE );

	val = opts["app-gwenview-scaleWidth"].toInt( &ok );
	if ( ok ) setScaleWidth( val );
	val = opts["app-gwenview-scaleHeight"].toInt( &ok );
	if ( ok ) setScaleHeight( val );

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

int GVPrintDialogPage::getPosition(const QString& align) {
	int alignment;

	if (align == i18n("Central-Left")) {
		alignment = Qt::AlignLeft | Qt::AlignVCenter;
	} else if (align == i18n("Central-Right")) {
		alignment = Qt::AlignRight | Qt::AlignVCenter;
	} else if (align == i18n("Top-Left")) {
		alignment = Qt::AlignTop | Qt::AlignLeft;
	} else if (align == i18n("Top-Right")) {
		alignment = Qt::AlignTop | Qt::AlignRight;
	} else if (align == i18n("Bottom-Left")) {
		alignment = Qt::AlignBottom | Qt::AlignLeft;
	} else if (align == i18n("Bottom-Right")) {
		alignment = Qt::AlignBottom | Qt::AlignRight;
	} else if (align == i18n("Top-Central")) {
		alignment = Qt::AlignTop | Qt::AlignHCenter;
	} else if (align == i18n("Bottom-Central")) {
		alignment = Qt::AlignBottom | Qt::AlignHCenter;
	} else	{
		// Central
		alignment = Qt::AlignCenter; // Qt::AlignHCenter || Qt::AlignVCenter
	}

	return alignment;
}

QString GVPrintDialogPage::setPosition(int align) {
	QString alignment;

	if (align == (Qt::AlignLeft | Qt::AlignVCenter)) {
		alignment = i18n("Central-Left");
	} else if (align == (Qt::AlignRight | Qt::AlignVCenter)) {
		alignment = i18n("Central-Right");
	} else if (align == (Qt::AlignTop | Qt::AlignLeft)) {
		alignment = i18n("Top-Left");
	} else if (align == (Qt::AlignTop | Qt::AlignRight)) {
		alignment = i18n("Top-Right");
	} else if (align == (Qt::AlignBottom | Qt::AlignLeft)) {
		alignment = i18n("Bottom-Left");
	} else if (align == (Qt::AlignBottom | Qt::AlignRight)) {
		alignment = i18n("Bottom-Right");
	} else if (align == (Qt::AlignTop | Qt::AlignHCenter)) {
		alignment = i18n("Top-Central");
	} else if (align == (Qt::AlignBottom | Qt::AlignHCenter)) {
		alignment = i18n("Bottom-Central");
	} else	{
		// Central: Qt::AlignCenter or (Qt::AlignHCenter || Qt::AlignVCenter)
		alignment = i18n("Central");
	}

	return alignment;
}

int GVPrintDialogPage::getUnit(const QString& unit) {
	if (unit == i18n("Millimeters")) {
		return	GV_MILLIMETERS;
	} else if (unit == i18n("Centimeters")) {
		return GV_CENTIMETERS;
	} else {//Inches
		return GV_INCHES;
	}
}

QString GVPrintDialogPage::setUnit(int unit) {
	if (unit == GV_MILLIMETERS) {
		return i18n("Millimeters");
	} else if (unit == GV_CENTIMETERS) {
		return i18n("Centimeters");
	} else { //GV_INCHES
		return i18n("Inches");
	}
}

// SLOTS
void GVPrintDialogPage::setHValue (int value) {
	mContent->mWidth->blockSignals(true);
	mContent->mHeight->blockSignals(true);

	if (mContent->mKeepRatio->isChecked()) {
		int w = (mDocument->width() * value) / mDocument->height();
		mContent->mWidth->setValue( w ? w : 1);
	}
	mContent->mHeight->setValue(value);

	mContent->mWidth->blockSignals(false);
	mContent->mHeight->blockSignals(false);

}

void GVPrintDialogPage::setWValue (int value) {
	mContent->mWidth->blockSignals(true);
	mContent->mHeight->blockSignals(true);
	if (mContent->mKeepRatio->isChecked()) {
		int h = (mDocument->height() * value) / mDocument->width();
		mContent->mHeight->setValue( h ? h : 1);
	}
	mContent->mWidth->setValue(value);
	mContent->mWidth->blockSignals(false);
	mContent->mHeight->blockSignals(false);
}

void GVPrintDialogPage::toggleRatio(bool enable) {
	if (enable) {
		float cm = 1;
		if (getUnit(mContent->mUnits->currentText()) == GV_MILLIMETERS) cm = 10;
		else if (getUnit(mContent->mUnits->currentText()) == GV_INCHES) cm = 1/(2.54);
		// 15x10 cm
		float hValue, wValue;
		if (mDocument->height() > mDocument->width()) {
			hValue = cm*15;
			wValue = (mDocument->width() * (hValue))/ mDocument->height();
		} else {
			wValue = cm*15;
			hValue = (mDocument->height() * wValue)/ mDocument->width();
		}
		mContent->mWidth->setValue((int)wValue);
		mContent->mHeight->setValue((int)hValue);
	}
}


void GVPrintDialogPage::setNewUnit(const QString& string) {
	mContent->mUnits->setCurrentItem(string);
	toggleRatio(true); // to do better
}



