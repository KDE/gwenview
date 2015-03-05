/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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
#ifndef TESTUTILS_H
#define TESTUTILS_H

#include <math.h>

// Qt
#include <QDir>
#include <QImage>
#include <QSignalSpy>
#include <QTest>

// KDE
#include <QDebug>
#include <QTemporaryDir>
#include <QUrl>

#include "config-gwenview.h"

/*
 * This file contains simple helpers to access test files
 */

inline QString pathForTestFile(const QString& name)
{
    return QDir::cleanPath(QString("%1/%2").arg(GV_TEST_DATA_DIR).arg(name));
}

inline QUrl urlForTestFile(const QString& name)
{
    return QUrl::fromLocalFile(pathForTestFile(name));
}

inline QString pathForTestOutputFile(const QString& name)
{
    return QString("%1/%2").arg(QDir::currentPath()).arg(name);
}

inline QUrl urlForTestOutputFile(const QString& name)
{
    return QUrl::fromLocalFile(pathForTestOutputFile(name));
}

inline bool waitForSignal(const QSignalSpy& spy, int timeout = 5)
{
    for (int x = 0; x < timeout; ++x) {
        if (spy.count() > 0) {
            return true;
        }
        QTest::qWait(1000);
    }
    return false;
}

void createEmptyFile(const QString& path);

/**
 * Returns the url of the remote url dir if remote test dir was successfully
 * set up.
 * If testFile is valid, it is copied into the test dir.
 */
QUrl setUpRemoteTestDir(const QString& testFile = QString());

/**
 * Make sure all objects on which deleteLater() have been called have been
 * destroyed.
 */
void waitForDeferredDeletes();

// FIXME: Top-level functions should move to the TestUtils namespace
namespace TestUtils
{

bool fuzzyImageCompare(const QImage& img1, const QImage& img2, int delta = 2);

bool imageCompare(const QImage& img1, const QImage& img2);

void purgeUserConfiguration();

class SandBoxDir : public QDir
{
public:
    SandBoxDir();
    void fill(const QStringList& files);

private:
    QTemporaryDir mTempDir;
};

/**
 * An event loop which stops itself after a predefined duration
 */
class TimedEventLoop : public QEventLoop
{
    Q_OBJECT
public:
    TimedEventLoop(int maxDurationInSeconds = 60);

    int exec(ProcessEventsFlags flags = AllEvents);

private Q_SLOTS:
    void fail();

private:
    QTimer *mTimer;
};

} // namespace

#endif /* TESTUTILS_H */
