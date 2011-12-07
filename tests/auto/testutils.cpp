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

// KDE
#include <KDebug>
#include <KIO/DeleteJob>
#include <KIO/NetAccess>
#include <KIO/Job>

static void rm_rf(const QString& pathOrUrl)
{
    KIO::Job* job = KIO::del(pathOrUrl, KIO::HideProgressInfo);
    job->setUiDelegate(0);
    KIO::NetAccess::synchronousRun(job, 0);
}

static bool mkdir(const QString& pathOrUrl)
{
    KIO::Job* job = KIO::mkdir(pathOrUrl, -1);
    job->setUiDelegate(0);
    return KIO::NetAccess::synchronousRun(job, 0);
}

static bool cp(const QString& src, const QString& _dst)
{
    KIO::Job* job = KIO::file_copy(src, _dst, -1, KIO::Overwrite | KIO::HideProgressInfo);
    job->setUiDelegate(0);
    return KIO::NetAccess::synchronousRun(job, 0);
}

KUrl setUpRemoteTestDir(const QString& testFile)
{
    if (!qgetenv("NO_REMOTE_TESTS").isEmpty()) {
        kWarning() << "Remote tests disabled";
        return KUrl();
    }

    QString testDir("/tmp/gwenview-remote-tests/");
    rm_rf(testDir);

    if (!mkdir(testDir)) {
        kFatal() << "Could not create dir" << testDir;
        return KUrl();
    }

    if (!testFile.isEmpty()) {
        if (!cp(pathForTestFile(testFile), testDir + testFile)) {
            kFatal() << "Could not copy" << testFile << "to" << testDir;
            return KUrl();
        }
    }

    return KUrl("sftp://localhost" + testDir);
}

void createEmptyFile(const QString& path)
{
    Q_ASSERT(!QFile::exists(path));
    QFile file(path);
    bool ok = file.open(QIODevice::WriteOnly);
    Q_ASSERT(ok);
}