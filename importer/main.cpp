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
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QCommandLineParser>
#include <QScopedPointer>
#include <QUrl>

// KDE
#include <KAboutData>
#include <KCmdLineArgs>
#include <KLocale>

// Local
#include <lib/about.h>
#include <lib/imageformats/imageformats.h>
#include "importdialog.h"

int main(int argc, char *argv[])
{
    QScopedPointer<KAboutData> aboutData(
        Gwenview::createAboutData(
            QStringLiteral("org.kde.gwenview"), /* component name */
            i18n("Gwenview Importer")  /* programName */
        ));
    aboutData->setShortDescription(i18n("Photo Importer"));

    QApplication app(argc, argv);
    KAboutData::setApplicationData(*aboutData);

    QCommandLineParser parser;
    aboutData.data()->setupCommandLine(&parser);
    parser.addOption(QCommandLineOption({QStringLiteral("udi")}, i18n("Device UDI")));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("folder", i18n("Source folder"));
    parser.process(app);
    aboutData.data()->processCommandLine(&parser);

    if (parser.positionalArguments().count() == 0) {
        KCmdLineArgs::usageError(i18n("Missing required source folder argument."));
        return 1;
    }
    if (parser.positionalArguments().count() > 1) {
        KCmdLineArgs::usageError(i18n("Too many arguments."));
        return 1;
    }
    QString urlString = parser.positionalArguments().first();
    QUrl url = QUrl::fromUserInput(urlString, QDir::currentPath(), QUrl::AssumeLocalFile);
    if (!url.isValid()) {
        qCritical() << i18n("Invalid source folder.");
        return 1;
    }
    QString deviceUdi = parser.isSet("udi") ? parser.value("udi") : QString();

    Gwenview::ImageFormats::registerPlugins();

    Gwenview::ImportDialog* dialog = new Gwenview::ImportDialog();
    dialog->show();
    QMetaObject::invokeMethod(dialog, "setSourceUrl", Qt::QueuedConnection, Q_ARG(QUrl, url), Q_ARG(QString, deviceUdi));
    return app.exec();
}
