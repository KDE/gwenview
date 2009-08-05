/*
Gwenview: an image viewer
Copyright 2007-2009 Aurélien Gâteau <agateau@kde.org>

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
#include <config-gwenview.h>
// Qt

// KDE
#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kurl.h>

// Local
#include <lib/imageformats/imageformats.h>
#include "mainwindow.h"

int main(int argc, char *argv[]) {
	KAboutData aboutData(
		"gwenview",        /* appname */
		0,                 /* catalogName */
		ki18n("Gwenview"), /* programName */
		"2.3.1");          /* version */
	aboutData.setShortDescription(ki18n("An Image Viewer"));
	aboutData.setLicense(KAboutData::License_GPL);
	aboutData.setCopyrightStatement(ki18n("Copyright 2000-2009 Aurélien Gâteau"));
	aboutData.addAuthor(
		ki18n("Aurélien Gâteau"),
		ki18n("Main developer"),
		"agateau@kde.org");

	KCmdLineArgs::init( argc, argv, &aboutData );

	KCmdLineOptions options;
	options.add("f", ki18n("Start in fullscreen mode"));
	options.add("s", ki18n("Start in slideshow mode"));
	options.add("+[file or folder]", ki18n("A starting file or folder"));
	KCmdLineArgs::addCmdLineOptions( options );

	KUrl url;
	bool startInFullScreen = false;
	bool startSlideShow = false;
	KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
	if (args->count() > 0) {
		url = args->url(0);
		if (url.isValid() && (args->isSet("f") || args->isSet("s"))) {
			startInFullScreen = true;
			if (args->isSet("s")) {
				startSlideShow = true;
			}
		}
	}
	args->clear();

	KApplication app;

	Gwenview::ImageFormats::registerPlugins();

	Gwenview::MainWindow* window = new Gwenview::MainWindow();
	if (url.isValid()) {
		window->setInitialUrl(url);
	} else {
		window->showStartPage();
	}

	window->show();
	if (startInFullScreen) {
		window->actionCollection()->action("fullscreen")->trigger();
	} else {
		window->show();
	}

	if (startSlideShow) {
		window->startSlideShow();
	}
	return app.exec();
}
