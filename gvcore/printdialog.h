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
#ifndef PRINTDIALOG_H
#define PRINTDIALOG_H

//Qt
#include <qfontmetrics.h>
#include <qstring.h>

// KDE
#include <kdockwidget.h>
#include <kdeprint/kprintdialogpage.h>

#include "libgwenview_export.h"
class PrintDialogPageBase;
namespace Gwenview {
class Document;

enum Unit {
	GV_MILLIMETERS = 1,
	GV_CENTIMETERS,
	GV_INCHES
};

enum ScaleId {
	GV_NOSCALE=1,
	GV_FITTOPAGE,
	GV_SCALE
};

class LIBGWENVIEW_EXPORT PrintDialogPage : public KPrintDialogPage {
	Q_OBJECT

public:
	PrintDialogPage( Document* document, QWidget *parent = 0L, const char *name = 0 );
	~PrintDialogPage();

	virtual void getOptions(QMap<QString,QString>& opts, bool incldef = false);
	virtual void setOptions(const QMap<QString,QString>& opts);

private slots:
	void toggleRatio(bool enable);
	void slotUnitChanged(const QString& string);
	void slotHeightChanged(double value);
	void slotWidthChanged(double value);

private:
	double scaleWidth() const;
	double scaleHeight() const;
	void setScaleWidth(double pixels);
	void setScaleHeight(double pixels);
	int getPosition(const QString& align);
	QString setPosition(int align);

	Document *mDocument;
	PrintDialogPageBase* mContent;
	Unit mPreviousUnit;
};

} // namespace
#endif

