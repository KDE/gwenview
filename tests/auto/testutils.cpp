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
#include <QTimer>

// KDE
#include <KDebug>
#include <KIO/NetAccess>
#include <KStandardDirs>
#include <QStandardPaths>

QUrl setUpRemoteTestDir(const QString& testFile)
{
    QWidget* authWindow = 0;
    bool ok;
    if (qgetenv("GV_REMOTE_TESTS_BASE_URL").isEmpty()) {
        kWarning() << "Environment variable GV_REMOTE_TESTS_BASE_URL not set: remote tests disabled";
        return QUrl();
    }

    QUrl baseUrl(QString::fromLocal8Bit(qgetenv("GV_REMOTE_TESTS_BASE_URL")));
    baseUrl = baseUrl.adjusted(QUrl::StripTrailingSlash);
    baseUrl.setPath(baseUrl.path() + "/gwenview-remote-tests");

    if (KIO::NetAccess::exists(baseUrl, KIO::NetAccess::DestinationSide, authWindow)) {
        KIO::NetAccess::del(baseUrl, authWindow);
    }
    ok = KIO::NetAccess::mkdir(baseUrl, authWindow);
    if (!ok) {
        kFatal() << "Could not create dir" << baseUrl << ":" << KIO::NetAccess::lastErrorString();
        return QUrl();
    }

    if (!testFile.isEmpty()) {
        QUrl dstUrl = baseUrl;
        dstUrl = dstUrl.adjusted(QUrl::StripTrailingSlash);
        dstUrl.setPath(dstUrl.path() + '/' + testFile);
        ok = KIO::NetAccess::file_copy(urlForTestFile(testFile), dstUrl, authWindow);
        if (!ok) {
            kFatal() << "Could not copy" << testFile << "to" << dstUrl << ":" << KIO::NetAccess::lastErrorString();
            return QUrl();
        }
    }

    return baseUrl;
}

void createEmptyFile(const QString& path)
{
    QVERIFY(!QFile::exists(path));
    QFile file(path);
    bool ok = file.open(QIODevice::WriteOnly);
    QVERIFY(ok);
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
    QString confDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QVERIFY(confDir.endsWith(".qttest/share")); // Better safe than sorry
    if (QFileInfo(confDir).isDir()) {
        bool ok = KIO::NetAccess::del(QUrl::fromLocalFile(confDir), 0);
        QVERIFY(ok);
    }

    QFile kdebugrc(KStandardDirs::locateLocal("config", "kdebugrc"));
    QVERIFY(kdebugrc.open(QIODevice::WriteOnly));
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

static QImage simplifyFormats(const QImage& img)
{
    switch (img.format()) {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32_Premultiplied:
        return img.convertToFormat(QImage::Format_ARGB32);
    default:
        return img;
    }
}

inline bool fuzzyColorComponentCompare(int c1, int c2, int delta)
{
    return qAbs(c1 - c2) < delta;
}

bool fuzzyImageCompare(const QImage& img1_, const QImage& img2_, int delta)
{
    if (img1_.size() != img2_.size()) {
        kWarning() << "Different sizes" << img1_.size() << "!=" << img2_.size();
        return false;
    }
    QImage img1 = simplifyFormats(img1_);
    QImage img2 = simplifyFormats(img2_);
    if (img1.format() != img2.format()) {
        kWarning() << "Different formats" << img1.format() << "!=" << img2.format();
        return false;
    }

    for (int posY = 0; posY < img1.height(); ++posY) {
        for (int posX = 0; posX < img2.width(); ++posX) {
            QColor col1 = img1.pixel(posX, posY);
            QColor col2 = img2.pixel(posX, posY);
            bool ok =
                fuzzyColorComponentCompare(col1.red(), col2.red(), delta)
                && fuzzyColorComponentCompare(col1.green(), col2.green(), delta)
                && fuzzyColorComponentCompare(col1.blue(), col2.blue(), delta)
                && fuzzyColorComponentCompare(col1.alpha(), col2.alpha(), delta);
            if (!ok) {
                kWarning() << "Different at" << QPoint(posX, posY) << col1.name() << "!=" << col2.name();
                return false;
            }
        }
    }
    return true;
}

bool imageCompare(const QImage& img1, const QImage& img2)
{
    return fuzzyImageCompare(img1, img2, 1);
}

SandBoxDir::SandBoxDir()
: mTempDir(QDir::currentPath() + "/sandbox-")
{
    setPath(mTempDir.path());
}

void SandBoxDir::fill(const QStringList& filePaths)
{
    Q_FOREACH(const QString& filePath, filePaths) {
        QFileInfo info(*this, filePath);
        mkpath(info.absolutePath());
        createEmptyFile(info.absoluteFilePath());
    }
}

TimedEventLoop::TimedEventLoop(int maxDuration)
: mTimer(new QTimer(this))
{
    mTimer->setSingleShot(true);
    mTimer->setInterval(maxDuration * 1000);
    connect(mTimer, SIGNAL(timeout()), SLOT(fail()));
}

int TimedEventLoop::exec(ProcessEventsFlags flags)
{
    mTimer->start();
    return QEventLoop::exec(flags);
}

void TimedEventLoop::fail()
{
    if (isRunning()) {
        qFatal("TimedEventLoop has been running for %d seconds. Aborting.", mTimer->interval() / 1000);
        exit(1);
    }
}

} // namespace TestUtils
