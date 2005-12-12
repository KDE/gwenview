// vim: set tabstop=4 shiftwidth=4 noexpandtab:
// kate: indent-mode csands; indent-width 4; replace-tabs-save off; replace-tabs off; replace-trailing-space-save off; space-indent off; tabs-indents on; tab-width 4;
/*
Gwenview - A simple image viewer for KDE
Copyright 2005 Aurelien Gateau

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
#include <qlabel.h>

// KDE
#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <kiconloader.h>

// Local
#include <vtabwidget.h>

int main(int argc, char* argv[]) {
	KAboutData aboutData("testvtabwidget", "testvtabwidget", "0");
	KCmdLineArgs::init( argc, argv, &aboutData );
	KApplication app;

	Gwenview::VTabWidget tabWidget(0);
	QLabel* lbl=new QLabel("label 1", &tabWidget);
	tabWidget.addTab(lbl, SmallIcon("text"), "tab1");
	lbl=new QLabel("label 2", &tabWidget);
	tabWidget.addTab(lbl, SmallIcon("image"), "tab2");
	
	app.setMainWidget(&tabWidget);
	tabWidget.show();
	app.exec();
}
