// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright (c) 2000-2003 Aurélien Gâteau

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

class QCheckBox;
class QRadioButton;
class KComboBox;
class KPrinter;
class KIntNumInput;

class GVPrintDialogPage : public KPrintDialogPage
{
    Q_OBJECT

public:
    GVPrintDialogPage( QWidget *parent = 0L, const char *name = 0 );
    ~GVPrintDialogPage();

    virtual void getOptions(QMap<QString,QString>& opts, bool incldef = false);
    virtual void setOptions(const QMap<QString,QString>& opts);

private:
    QCheckBox *mAddFileName;
    QCheckBox *mShrinkToFit;
    QCheckBox *mBlackWhite;

};

#endif
