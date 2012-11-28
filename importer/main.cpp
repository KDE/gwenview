/*
Gwenview: an image viewer
Copyright 2000-2009 Aurélien Gâteau <agateau@kde.org>

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

// KDE
#include <KAboutData>
#include <KApplication>
#include <KCmdLineArgs>
#include <KDebug>
#include <KLocale>
#include <KUrl>

// Local
#include <lib/imageformats/imageformats.h>
#include <lib/version.h>
#include "importdialog.h"

int main(int argc, char *argv[])
{
    KAboutData aboutData(
        "gwenview_importer",        /* appname */
        "gwenview",                          /* catalogName */
        ki18n("Gwenview Importer"), /* programName */
        GWENVIEW_VERSION);          /* version */
    aboutData.setShortDescription(ki18n("Photo Importer"));
    aboutData.setLicense(KAboutData::License_GPL);
    aboutData.setCopyrightStatement(ki18n("Copyright 2009-2010 Aurélien Gâteau"));
    aboutData.addAuthor(
        ki18n("Aurélien Gâteau"),
        ki18n("Main developer"),
        "agateau@kde.org");

    KCmdLineArgs::init(argc, argv, &aboutData);

    KCmdLineOptions options;
    options.add("+folder", ki18n("Source folder"));
    options.add("udi <device-udi>", ki18n("Device UDI"));
    KCmdLineArgs::addCmdLineOptions(options);

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    if (args->count() != 1) {
        kError() << "Wrong arg count. FIXME";
        return 1;
    }
    KUrl url = args->url(0);
    if (!url.isValid()) {
        kError() << "Invalid url. FIXME";
        return 1;
    }
    QString deviceUdi = args->isSet("udi") ? args->getOption("udi") : QString();
    args->clear();

    KApplication app;

    Gwenview::ImageFormats::registerPlugins();

    Gwenview::ImportDialog* dialog = new Gwenview::ImportDialog();
    dialog->show();
    QMetaObject::invokeMethod(dialog, "setSourceUrl", Qt::QueuedConnection, Q_ARG(KUrl, url), Q_ARG(QString, deviceUdi));
    return app.exec();
}
