// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "fullscreentheme.h"

// Qt
#include <QDir>
#include <QFile>
#include <QString>

// KDE
#include <kcomponentdata.h>
#include <kmacroexpander.h>
#include <kstandarddirs.h>
#include <kstringhandler.h>

// Local
#include <lib/gwenviewconfig.h>

namespace Gwenview {


static const char* THEME_BASE_DIR = "fullscreenthemes/";


struct FullScreenThemePrivate {
	QString mThemeDir;
	QString mStyleSheet;
};


FullScreenTheme::FullScreenTheme(const QString& themeName)
: d(new FullScreenThemePrivate) {
	// Get theme dir
	d->mThemeDir = KStandardDirs::locate("appdata", THEME_BASE_DIR + themeName + "/");
	if (d->mThemeDir.isEmpty()) {
		kWarning() << "Couldn't find fullscreen theme" << themeName;
		return;
	}

	// Read css file
	QString styleSheetPath = d->mThemeDir + "/style.css";
	QFile file(styleSheetPath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
		kWarning() << "Couldn't open" << styleSheetPath;
		return;
	}
	QString styleSheet = QString::fromUtf8(file.readAll());

	d->mStyleSheet = replaceThemeVars(styleSheet);
}


FullScreenTheme::~FullScreenTheme() {
	delete d;
}


QString FullScreenTheme::styleSheet() const {
	return d->mStyleSheet;
}


QString FullScreenTheme::replaceThemeVars(const QString& styleSheet) {
	QHash<QString, QString> macros;
	macros["themeDir"] = d->mThemeDir;
	return KMacroExpander::expandMacros(styleSheet, macros, QLatin1Char('$'));
}


QString FullScreenTheme::currentThemeName() {
	return GwenviewConfig::fullScreenTheme();
}


void FullScreenTheme::setCurrentThemeName(const QString& name) {
	GwenviewConfig::setFullScreenTheme(name);
}


static bool themeNameLessThan(const QString& s1, const QString& s2) {
	return KStringHandler::naturalCompare(s1.toLower(), s2.toLower()) < 0;
}


QStringList FullScreenTheme::themeNameList() {
	QStringList list;
	QStringList themeBaseDirs =
		KGlobal::mainComponent().dirs()
		->findDirs("appdata", THEME_BASE_DIR);
	Q_FOREACH(const QString& themeBaseDir, themeBaseDirs) {
		QDir dir(themeBaseDir);
		list += dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	}
	qSort(list.begin(), list.end(), themeNameLessThan);

	return list;
}


} // namespace
