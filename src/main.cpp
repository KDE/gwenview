/*
Gwenview - A simple image viewer for KDE
Copyright (C) 2000-2002 Aurélien Gâteau

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
#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kimageio.h>
#include <klocale.h>

#include "mainwindow.h"
#include "qxcfi.h"

static KCmdLineOptions options[] = {
	{ "f", I18N_NOOP("Start in fullscreen mode"), 0 },
	{ "+[file or folder]", I18N_NOOP("A starting file or folder"), 0 },
	{ 0, 0, 0 }
};

static const char* version="CVS";


int main (int argc, char *argv[]) {
    KAboutData aboutData("gwenview", "Gwenview",
                        version, I18N_NOOP("An image viewer for KDE"), KAboutData::License_GPL,
                        "(c) 2000-2002 Aurélien Gâteau",0,"http://gwenview.sourceforge.net");

	KCmdLineArgs::init( argc, argv, &aboutData );
	KCmdLineArgs::addCmdLineOptions( options );

	KApplication kapplication;

	KImageIO::registerFormats();
	XCFImageFormat::registerFormat();

	if (kapplication.isRestored()) {
		RESTORE(MainWindow());
	} else {
		MainWindow *mainWindow = new MainWindow;
		mainWindow->show();
	}

	return kapplication.exec();
}
