// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2006 Aurelien Gateau

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
#include <qdir.h>

#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kdebug.h>
#include <klocale.h>

#include "gvcore/cache.h"
#include "gvcore/fileviewcontroller.h"
#include <../gvcore/fileviewconfig.h>
#include <../gvcore/miscconfig.h>
#include "mainwindow.h"
namespace Gwenview {

static KCmdLineOptions options[] = {
	{ "f", I18N_NOOP("Start in fullscreen mode"), 0 },
	{ "filter-type <all,images,videos>", I18N_NOOP("Filter by file type"), 0 },
	{ "filter-name <pattern>", I18N_NOOP("Filter by file pattern (*.jpg, 01*...)"), 0 },
	{ "filter-from <date>", I18N_NOOP("Only show files newer or equal to <date>"), 0 },
	{ "filter-to <date>", I18N_NOOP("Only show files older or equal to <date>"), 0 },
	{ "+[file or folder]", I18N_NOOP("A starting file or folder"), 0 },
	KCmdLineLastOption
};

static const char version[] = "1.4.0";


void applyFilterArgs(KCmdLineArgs* args, FileViewController* controller) {
	QString filterType = args->getOption("filter-type");
	QString filterName = args->getOption("filter-name");
	QString filterFrom = args->getOption("filter-from");
	QString filterTo = args->getOption("filter-to");
	// Do nothing if there is no filter
	if (filterType.isEmpty() && filterName.isEmpty() 
		&& filterFrom.isEmpty() && filterTo.isEmpty())
	{
		return;
	}

	QStringList typeList;
	typeList << "all" << "images" << "videos";
	int mode = typeList.findIndex(filterType);
	if (mode == -1) {
		// Default to "all"
		controller->setFilterMode(0);
	} else {
		controller->setFilterMode(mode);
	}

	controller->setShowFilterBar(
		!filterName.isEmpty()
		|| !filterFrom.isEmpty()
		|| !filterTo.isEmpty() );

	controller->setFilterName(filterName);

	bool ok = false;
	QDate date;
	if (!filterFrom.isEmpty()) {
		date = KGlobal::locale()->readDate(filterFrom, &ok);
		if (!ok) {
			kdWarning() << "Invalid value for filter-from option\n";
		}
	}
	controller->setFilterFromDate(date);

	date=QDate();
	if (!filterTo.isEmpty()) {
		date = KGlobal::locale()->readDate(filterTo, &ok);
		if (!ok) {
			kdWarning() << "Invalid value for filter-to option\n";
		}
	}
	controller->setFilterToDate(date);

	controller->applyFilter();
}


#ifndef __KDE_HAVE_GCC_VISIBILITY
#undef KDE_EXPORT
#define KDE_EXPORT
#endif

extern "C"
KDE_EXPORT int kdemain (int argc, char *argv[]) {
	KAboutData aboutData("gwenview", I18N_NOOP("Gwenview"),
		version, I18N_NOOP("An image viewer for KDE"), KAboutData::License_GPL,
		"Copyright 2000-2006, The Gwenview developers",0,"http://gwenview.sourceforge.net");
	aboutData.addAuthor("Aurélien Gâteau", I18N_NOOP("Main developer"), "aurelien.gateau@free.fr");
	aboutData.addAuthor("Luboš Luňák", I18N_NOOP("Developer"), "l.lunak@suse.cz");

	aboutData.addCredit("Frank Becker", I18N_NOOP("Fast JPEG thumbnail generation (v0.13.0)"), "ff@telus.net");
	aboutData.addCredit("Tudor Calin", I18N_NOOP("Address bar (v0.16.0)\nHistory support (v1.0.0)"), "tudor_calin@mymail.ro");
	aboutData.addCredit("Avinash Chopde", I18N_NOOP("File operation patch (v0.9.2)"), "avinash@acm.org");
	aboutData.addCredit("Marco Gazzetta", I18N_NOOP("Fixed crash when trying to generate a thumbnail for a broken JPEG file (v0.16.0)"), "mililani@pobox.com");
	aboutData.addCredit("GeniusR13", I18N_NOOP("Fixed compilation on KDE 3.0 (v0.16.1)"), "geniusr13@gmx.net");
	aboutData.addCredit("Ian Koenig", I18N_NOOP("First RPM spec file"), "iguy@ionsphere.org");
	aboutData.addCredit("Meni Livne", I18N_NOOP("Toolbar layout patch for RTL languages (v0.16.0)"), "livne@kde.org");
	aboutData.addCredit("Angelo Naselli", I18N_NOOP("Printing support (v1.0.0)"), "anaselli@linux.it");
	aboutData.addCredit("Jos van den Oever", I18N_NOOP("File info view (v1.0.0)\nPatch to toggle auto-zoom on click (v1.0.0)"), "jos@vandenoever.info");
	aboutData.addCredit("Jeroen Peters", I18N_NOOP("Configurable mouse wheel behavior (v1.1.1)"), "jpeters@coldmail.nl");
	aboutData.addCredit("Andreas Pfaller", I18N_NOOP("Option to prevent Gwenview from automatically loading the first image of a folder (v0.15.0)"), "apfaller@yahoo.com.au");
	aboutData.addCredit("Renchi Raju", I18N_NOOP("Fixed thumbnail generation to share the thumbnail folder of Konqueror v3 (v0.15.0)"), "renchi@green.tam.uiuc.edu");
	aboutData.addCredit("Michael Spanier", I18N_NOOP("Patch for mouse navigation (v0.7.0)"), "mail@michael-spanier.de");
	aboutData.addCredit("Christian A Strømmen", I18N_NOOP("Integration in Konqueror folder context menu"), "number1@realityx.net");

	KCmdLineArgs::init( argc, argv, &aboutData );
	KCmdLineArgs::addCmdLineOptions( options );

	KApplication kapplication;

	if (kapplication.isRestored()) {
		RESTORE(MainWindow)
	} else {
		KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

		MainWindow *mainWindow = new MainWindow;
		applyFilterArgs(args, mainWindow->fileViewController());

		bool fullscreen=args->isSet("f");
		if (fullscreen) mainWindow->setFullScreen(true);

		KURL url;
		if (args->count()>0) {
			url=args->url(0);
		} else {
			if (MiscConfig::rememberURL() && MiscConfig::history().count() > 0) {
				url = KURL(MiscConfig::history()[0]);
			} else {
				url.setPath( QDir::currentDirPath() );
			}
		}
		mainWindow->openURL(url);
		
		mainWindow->show();
	}

	Cache::instance()->ref();
	return kapplication.exec();
}

} // namespace
