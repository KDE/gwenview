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
#include <QCommandLineParser>
#include <QDir>
#include <QLocale>
#include <QScopedPointer>
#include <QUrl>

// KF
#include <KAboutData>
#include <KLocalizedString>

// Local
#include "gwenview_importer_debug.h"
#include "importdialog.h"
#include <lib/about.h>

int main(int argc, char *argv[])
{
    KLocalizedString::setApplicationDomain("gwenview");
    QApplication app(argc, argv);
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    QScopedPointer<KAboutData> aboutData(Gwenview::createAboutData(QStringLiteral("gwenview_importer"), /* component name */
                                                                   i18n("Gwenview Importer") /* programName */
                                                                   ));
    aboutData->setShortDescription(i18n("Photo Importer"));

    KAboutData::setApplicationData(*aboutData);

    QCommandLineParser parser;
    aboutData.data()->setupCommandLine(&parser);
    parser.addOption(
        QCommandLineOption(QStringLiteral("udi"), i18n("The device UDI, used to retrieve information about the device (name, icon...)"), i18n("Device UDI")));
    parser.addPositionalArgument(QStringLiteral("folder"), i18n("Source folder"));
    parser.process(app);
    aboutData.data()->processCommandLine(&parser);

    if (parser.positionalArguments().isEmpty()) {
        qCWarning(GWENVIEW_IMPORTER_LOG) << i18n("Missing required source folder argument.");
        parser.showHelp();
    }
    if (parser.positionalArguments().count() > 1) {
        qCWarning(GWENVIEW_IMPORTER_LOG) << i18n("Too many arguments.");
        parser.showHelp();
    }
    QString urlString = parser.positionalArguments().constFirst();
    QUrl url = QUrl::fromUserInput(urlString, QDir::currentPath(), QUrl::AssumeLocalFile);
    if (!url.isValid()) {
        qCCritical(GWENVIEW_IMPORTER_LOG) << i18n("Invalid source folder.");
        return 1;
    }
    QString deviceUdi = parser.isSet(QStringLiteral("udi")) ? parser.value(QStringLiteral("udi")) : QString();

    auto *dialog = new Gwenview::ImportDialog();
    dialog->show();
    QMetaObject::invokeMethod(
        dialog,
        [dialog, url, deviceUdi]() {
            dialog->setSourceUrl(url, deviceUdi);
        },
        Qt::QueuedConnection);
    return app.exec();
}
