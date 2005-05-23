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
#ifndef GVPRINTDIALOG_H
#define GVPRINTDIALOG_H

//Qt
#include <qfontmetrics.h>
#include <qstring.h>

// KDE
#include <kdockwidget.h>
#include <kdeprint/kprintdialogpage.h>

#include "libgwenview_export.h"
class GVDocument;
class GVPrintDialogPageBase;

enum GVUnits {
	GV_MILLIMETERS = 1,
	GV_CENTIMETERS,
	GV_INCHES
};

class LIBGWENVIEW_EXPORT GVPrintDialogPage : public KPrintDialogPage {
	Q_OBJECT

public:
	GVPrintDialogPage( GVDocument* document, QWidget *parent = 0L, const char *name = 0 );
	~GVPrintDialogPage();

	virtual void getOptions(QMap<QString,QString>& opts, bool incldef = false);
	virtual void setOptions(const QMap<QString,QString>& opts);

private slots:
	void toggleRatio(bool enable);
	void setNewUnit(const QString& string);
	void setHValue (int value);
	void setWValue (int value);

private:
	int scaleWidth() const;
	int scaleHeight() const;
	void setScaleWidth(int pixels);
	void setScaleHeight(int pixels);
	int getPosition(const QString& align);
	QString setPosition(int align);
	int getUnit(const QString& unit);
	QString setUnit(int unit);

	GVDocument *mDocument;
	GVPrintDialogPageBase* mContent;
};

#endif
