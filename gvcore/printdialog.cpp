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
#include "document.h"
#include "printdialogpagebase.h"
#include "printdialog.moc"
namespace Gwenview {


const char* STR_TRUE="true";
const char* STR_FALSE="false";

static inline Unit stringToUnit(const QString& unit) {
	if (unit == i18n("Millimeters")) {
		return GV_MILLIMETERS;
	} else if (unit == i18n("Centimeters")) {
		return GV_CENTIMETERS;
	} else {//Inches
		return GV_INCHES;
	}
}

static inline QString unitToString(Unit unit) {
	if (unit == GV_MILLIMETERS) {
		return i18n("Millimeters");
	} else if (unit == GV_CENTIMETERS) {
		return i18n("Centimeters");
	} else { //GV_INCHES
		return i18n("Inches");
	}
}


static inline double unitToMM(Unit unit) {
	if (unit == GV_MILLIMETERS) {
		return 1.;
	} else if (unit == GV_CENTIMETERS) {
		return 10.;
	} else { //GV_INCHES
		return 25.4;
	}
}


PrintDialogPage::PrintDialogPage( Document* document, QWidget *parent, const char *name )
		: KPrintDialogPage( parent, name ) {
	mDocument = document;
	mContent = new PrintDialogPageBase(this);
	setTitle( mContent->caption() );

	QVBoxLayout *layout = new QVBoxLayout( this );
	layout->addWidget( mContent );

	connect(mContent->mWidth, SIGNAL( valueChanged( double )), SLOT( slotWidthChanged( double )));
	connect(mContent->mHeight, SIGNAL( valueChanged( double )), SLOT( slotHeightChanged( double )));
	connect(mContent->mKeepRatio, SIGNAL( toggled( bool )), SLOT( toggleRatio( bool )));
	connect(mContent->mUnit, SIGNAL(activated(const QString &)), SLOT(slotUnitChanged(const QString& )));

	mPreviousUnit = GV_MILLIMETERS;
}

PrintDialogPage::~PrintDialogPage() {}

void PrintDialogPage::getOptions( QMap<QString,QString>& opts, bool /*incldef*/ ) {
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
	opts["app-gwenview-scaleUnit"] = QString::number(stringToUnit(mContent->mUnit->currentText()));
	opts["app-gwenview-scaleWidth"] = QString::number( scaleWidth() );
	opts["app-gwenview-scaleHeight"] = QString::number( scaleHeight() );

}

void PrintDialogPage::setOptions( const QMap<QString,QString>& opts ) {
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

	Unit unit = static_cast<Unit>( opts["app-gwenview-scaleUnit"].toInt( &ok ) );
	if (ok) {
		stVal = unitToString(unit);
		mContent->mUnit->setCurrentItem(stVal);
		mPreviousUnit = unit;
	}

	mContent->mKeepRatio->setChecked( opts["app-gwenview-scaleKeepRatio"] == STR_TRUE );

	double dbl;
	dbl = opts["app-gwenview-scaleWidth"].toDouble( &ok );
	if ( ok ) setScaleWidth( dbl );
	dbl = opts["app-gwenview-scaleHeight"].toDouble( &ok );
	if ( ok ) setScaleHeight( dbl );
}

double PrintDialogPage::scaleWidth() const {
	return mContent->mWidth->value();
}

double PrintDialogPage::scaleHeight() const {
	return mContent->mHeight->value();
}

void PrintDialogPage::setScaleWidth( double value ) {
	mContent->mWidth->setValue(value);
}

void PrintDialogPage::setScaleHeight( double value ) {
	mContent->mHeight->setValue(value);
}

int PrintDialogPage::getPosition(const QString& align) {
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

QString PrintDialogPage::setPosition(int align) {
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

// SLOTS
void PrintDialogPage::slotHeightChanged (double value) {
	mContent->mWidth->blockSignals(true);
	mContent->mHeight->blockSignals(true);

	if (mContent->mKeepRatio->isChecked()) {
		double width = (mDocument->width() * value) / mDocument->height();
		mContent->mWidth->setValue( width ? width : 1.);
	}
	mContent->mHeight->setValue(value);

	mContent->mWidth->blockSignals(false);
	mContent->mHeight->blockSignals(false);

}

void PrintDialogPage::slotWidthChanged (double value) {
	mContent->mWidth->blockSignals(true);
	mContent->mHeight->blockSignals(true);
	if (mContent->mKeepRatio->isChecked()) {
		double height = (mDocument->height() * value) / mDocument->width();
		mContent->mHeight->setValue( height ? height : 1);
	}
	mContent->mWidth->setValue(value);
	mContent->mWidth->blockSignals(false);
	mContent->mHeight->blockSignals(false);
}

void PrintDialogPage::toggleRatio(bool enable) {
	if (!enable) return;
	double hValue, wValue;
	if (mDocument->height() > mDocument->width()) {
		hValue = mContent->mHeight->value();
		wValue = (mDocument->width() * hValue)/ mDocument->height();
	} else {
		wValue = mContent->mWidth->value();
		hValue = (mDocument->height() * wValue)/ mDocument->width();
	}
	
	mContent->mWidth->blockSignals(true);
	mContent->mHeight->blockSignals(true);
	mContent->mWidth->setValue(wValue);
	mContent->mHeight->setValue(hValue);
	mContent->mWidth->blockSignals(false);
	mContent->mHeight->blockSignals(false);
}


void PrintDialogPage::slotUnitChanged(const QString& string) {
	Unit newUnit = stringToUnit(string);
	double ratio = unitToMM(mPreviousUnit) / unitToMM(newUnit);

	mContent->mWidth->blockSignals(true);
	mContent->mHeight->blockSignals(true);

	mContent->mWidth->setValue( mContent->mWidth->value() * ratio);
	mContent->mHeight->setValue( mContent->mHeight->value() * ratio);

	mContent->mWidth->blockSignals(false);
	mContent->mHeight->blockSignals(false);

	mPreviousUnit = newUnit;
}




} // namespace
