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

// KDE
#include <K4AboutData>
#include <KApplication>
#include <KCmdLineArgs>

// Qt
#include <QDir>
#include <QTime>
#include <QtDebug>

using namespace Gwenview;

int main(int argc, char** argv)
{
    K4AboutData aboutData(
        "thumbnailgen", // appName
        0,     // catalogName
        ki18n("thumbnailgen"), // programName
        "0.0.0");

    // Parser init
    KCmdLineArgs::init(argc, argv, &aboutData);

    KCmdLineOptions options;
    options.add("+image-dir", ki18n("Image dir to open"));
    options.add("+size", ki18n("What size of thumbnails to generate. Can be either 'normal' or 'large'"));
    options.add("t").add("thumbnail-dir <dir>", ki18n("Use <dir> instead of ~/.thumbnails to store thumbnails"));
    KCmdLineArgs::addCmdLineOptions(options);

    KApplication app;

    // Read cmdline options
    KCmdLineArgs* args = KCmdLineArgs::parsedArgs();
    if (args->count() != 2) {
        qFatal("Wrong number of arguments");
        return 1;
    }
    QString imageDirName = args->arg(0);
    ThumbnailGroup::Enum group = ThumbnailGroup::Normal;
    if (args->arg(1) == "large") {
        group = ThumbnailGroup::Large;
    } else if (args->arg(1) == "normal") {
        // group is already set to the right value
    } else {
        qFatal("Invalid thumbnail size: %s", qPrintable(args->arg(1)));
    }
    QString thumbnailBaseDirName = args->isSet("thumbnail-dir") ? args->getOption("thumbnail-dir") : QString();

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
        if (!thumbnailBaseDirName.endsWith("/")) {
            thumbnailBaseDirName += "/";
        }
        ThumbnailProvider::setThumbnailBaseDir(thumbnailBaseDirName);
    }

    // List dir
    QDir dir(imageDirName);
    KFileItemList list;
    Q_FOREACH(const QString &name, dir.entryList()) {
        QUrl url = QUrl::fromLocalFile(dir.absoluteFilePath(name));
        KFileItem item(url);
        list << item;
    }
    qWarning() << "Generating thumbnails for" << list.count() << "files";

    // Start the job
    QTime chrono;
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
