/*
Gwenview: an image viewer
Copyright 2010 Aurélien Gâteau <agateau@kde.org>

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
#include "testutils.h"

// Qt
#include <QDir>

// KDE
#include <KDebug>
#include <KIO/NetAccess>
#include <KStandardDirs>

KUrl setUpRemoteTestDir(const QString& testFile)
{
    QWidget* authWindow = 0;
    bool ok;
    if (qgetenv("GV_REMOTE_TESTS_BASE_URL").isEmpty()) {
        kWarning() << "Environment variable GV_REMOTE_TESTS_BASE_URL not set: remote tests disabled";
        return KUrl();
    }

    KUrl baseUrl = QString::fromLocal8Bit(qgetenv("GV_REMOTE_TESTS_BASE_URL"));
    baseUrl.addPath("gwenview-remote-tests");

    if (KIO::NetAccess::exists(baseUrl, KIO::NetAccess::DestinationSide, authWindow)) {
        KIO::NetAccess::del(baseUrl, authWindow);
    }
    ok = KIO::NetAccess::mkdir(baseUrl, authWindow);
    if (!ok) {
        kFatal() << "Could not create dir" << baseUrl << ":" << KIO::NetAccess::lastErrorString();
        return KUrl();
    }

    if (!testFile.isEmpty()) {
        KUrl dstUrl = baseUrl;
        dstUrl.addPath(testFile);
        ok = KIO::NetAccess::file_copy(urlForTestFile(testFile), dstUrl, authWindow);
        if (!ok) {
            kFatal() << "Could not copy" << testFile << "to" << dstUrl << ":" << KIO::NetAccess::lastErrorString();
            return KUrl();
        }
    }

    return baseUrl;
}

void createEmptyFile(const QString& path)
{
    Q_ASSERT(!QFile::exists(path));
    QFile file(path);
    bool ok = file.open(QIODevice::WriteOnly);
    Q_ASSERT(ok);
}

void waitForDeferredDeletes()
{
    while (QCoreApplication::hasPendingEvents()) {
        QCoreApplication::sendPostedEvents();
        QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
        QCoreApplication::processEvents();
    }
}

namespace TestUtils
{

void purgeUserConfiguration()
{
    QString confDir = qgetenv("KDEHOME");
    Q_ASSERT(confDir.endsWith(".kde-unit-test")); // Better safe than sorry
    if (QFileInfo(confDir).isDir()) {
        bool ok = KIO::NetAccess::del(KUrl::fromPath(confDir), 0);
        Q_ASSERT(ok);
    }

    QFile kdebugrc(KStandardDirs::locateLocal("config", "kdebugrc"));
    Q_ASSERT(kdebugrc.open(QIODevice::WriteOnly));
    kdebugrc.write(
        "DisableAll=true\n"
        "InfoOutput=4\n"
        "[gwenview]\n"
        "InfoOutput=2\n"
        "[gwenview_importer]\n"
        "InfoOutput=2\n"
        );
    kClearDebugConfig();
}

}
