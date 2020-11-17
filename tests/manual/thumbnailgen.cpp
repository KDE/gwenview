// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2012 Aurélien Gâteau <agateau@kde.org>

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
// Local
#include <lib/thumbnailprovider/thumbnailprovider.h>
#include <../auto/testutils.h>
#include <lib/about.h>

// KF
#include <KAboutData>
#include <KLocalizedString>

// Qt
#include <QDir>
#include <QTime>
#include <QtDebug>
#include <QCommandLineParser>

using namespace Gwenview;

int main(int argc, char** argv)
{
    KLocalizedString::setApplicationDomain("thumbnailgen");
    QScopedPointer<KAboutData> aboutData(
        Gwenview::createAboutData(
            QStringLiteral("thumbnailgen"), /* component name */
            i18n("thumbnailgen")                    /* display name */
        ));

    QApplication app(argc, argv);

    QCommandLineParser parser;
    aboutData->setupCommandLine(&parser);
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("image-dir", i18n("Image dir to open"));
    parser.addPositionalArgument("size", i18n("What size of thumbnails to generate. Can be either 'normal' or 'large'"));
    parser.addOption(QCommandLineOption(QStringList() << QStringLiteral("t") << QStringLiteral("thumbnail-dir"),
                                        i18n("Use <dir> instead of ~/.thumbnails to store thumbnails"), "thumbnail-dir"));
    parser.process(app);
    aboutData->processCommandLine(&parser);




    // Read cmdline options
    QStringList args = parser.positionalArguments();
    if (args.count() != 2) {
        qFatal("Wrong number of arguments");
        return 1;
    }
    QString imageDirName = args.first();
    ThumbnailGroup::Enum group = ThumbnailGroup::Normal;
    if (args.last() == "large") {
        group = ThumbnailGroup::Large;
    } else if (args.last() == "normal") {
        // group is already set to the right value
    } else {
        qFatal("Invalid thumbnail size: %s", qPrintable(args.last()));
    }
    QString thumbnailBaseDirName = parser.value("thumbnail-dir");

    // Set up thumbnail base dir
    if (!thumbnailBaseDirName.isEmpty()) {
        QDir dir = QDir(thumbnailBaseDirName);
        thumbnailBaseDirName = dir.absolutePath();
        if (!dir.exists()) {
            bool ok = QDir::root().mkpath(thumbnailBaseDirName);
            if (!ok) {
                qFatal("Could not create %s", qPrintable(thumbnailBaseDirName));
                return 1;
            }
        }
        if (!thumbnailBaseDirName.endsWith('/')) {
            thumbnailBaseDirName += '/';
        }
        ThumbnailProvider::setThumbnailBaseDir(thumbnailBaseDirName);
    }

    // List dir
    QDir dir(imageDirName);
    KFileItemList list;
    const auto entryList = dir.entryList();
    for (const QString &name : entryList) {
        QUrl url = QUrl::fromLocalFile(dir.absoluteFilePath(name));
        KFileItem item(url);
        list << item;
    }
    qWarning() << "Generating thumbnails for" << list.count() << "files";

    // Start the job
    QElapsedTimer chrono;
    ThumbnailProvider job;
    job.setThumbnailGroup(group);
    job.appendItems(list);

    chrono.start();

    QEventLoop loop;
    QObject::connect(&job, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();

    qWarning() << "Time to generate thumbnails:" << chrono.restart();

    waitForDeferredDeletes();
    while (!ThumbnailProvider::isThumbnailWriterEmpty()) {
        QCoreApplication::processEvents();
    }
    qWarning() << "Time to save pending thumbnails:" << chrono.restart();

    return 0;
}
