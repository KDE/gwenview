/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
// Qt
#include <QDir>
#include <QString>

// KDE
#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <klocale.h>
#include <kmessagebox.h>

// Local
#include "mainwindow.h"

static KCmdLineOptions options[] = {
	{ "+[file or folder]", I18N_NOOP("A starting file or folder"), 0 },
	KCmdLineLastOption
};

int main(int argc, char *argv[]) {
	KAboutData aboutData( "gwenview", "Gwenview",
		"2.0", "An Image Viewer",
		KAboutData::License_GPL, "(c) 2007" );
	KCmdLineArgs::init( argc, argv, &aboutData );
	KCmdLineArgs::addCmdLineOptions( options );

	KApplication app;

	Gwenview::MainWindow* window = new Gwenview::MainWindow();
	KUrl url;
	KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
	if (args->count()>0) {
		url=args->url(0);
	} else {
		url.setPath( QDir::currentPath() );
	}
	
	window->show();
	window->openUrl(url);

	return app.exec();
}
