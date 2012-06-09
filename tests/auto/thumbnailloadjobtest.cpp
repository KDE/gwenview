/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
#include "thumbnailloadjobtest.moc"

// Qt
#include <QDir>
#include <QImage>
#include <QPainter>

// KDE
#include <qtest_kde.h>
#include <KDebug>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>

// Local
#include "../lib/imageformats/imageformats.h"
#include "../lib/thumbnailloadjob.h"
#include "testutils.h"

using namespace Gwenview;

QTEST_KDEMAIN(ThumbnailLoadJobTest, GUI)

SandBox::SandBox()
: mPath(QDir::currentPath() + "/sandbox")
{}

void SandBox::initDir()
{
    KIO::Job* job;
    QDir dir(mPath);
    if (dir.exists()) {
        KUrl sandBoxUrl("file://" + mPath);
        job = KIO::del(sandBoxUrl);
        QVERIFY2(job->exec(), "Couldn't delete sandbox");
    }
    dir.mkpath(".");
}

void SandBox::fill()
{
    initDir();
    createTestImage("red.png", 300, 200, Qt::red);
    createTestImage("blue.png", 200, 300, Qt::blue);
    createTestImage("small.png", 50, 50, Qt::green);

    copyTestImage("orient6.jpg", 128, 256);
    copyTestImage("orient6-small.jpg", 32, 64);
}

void SandBox::copyTestImage(const QString& testFileName, int width, int height)
{
    QString testPath = pathForTestFile(testFileName);
    KIO::Job* job = KIO::copy(testPath, KUrl(mPath + '/' + testFileName));
    QVERIFY2(job->exec(), "Couldn't copy test image");
    mSizeHash.insert(testFileName, QSize(width, height));
}

static QImage createColoredImage(int width, int height, const QColor& color)
{
    QImage image(width, height, QImage::Format_RGB32);
    QPainter painter(&image);
    painter.fillRect(image.rect(), color);
    return image;
}

void SandBox::createTestImage(const QString& name, int width, int height, const QColor& color)
{
    QImage image = createColoredImage(width, height, color);
    image.save(mPath + '/' + name, "png");
    mSizeHash.insert(name, QSize(width, height));
}

void ThumbnailLoadJobTest::initTestCase()
{
    qRegisterMetaType<KFileItem>("KFileItem");
    Gwenview::ImageFormats::registerPlugins();
}

void ThumbnailLoadJobTest::init()
{
    ThumbnailLoadJob::setThumbnailBaseDir(mSandBox.mPath + "/thumbnails/");
    mSandBox.fill();
}

void ThumbnailLoadJobTest::testLoadLocal()
{
    QDir dir(mSandBox.mPath);

    // Create a list of items which will be thumbnailed
    KFileItemList list;
    Q_FOREACH(const QFileInfo & info, dir.entryInfoList(QDir::Files)) {
        KUrl url("file://" + info.absoluteFilePath());
        KFileItem item(KFileItem::Unknown, KFileItem::Unknown, url);
        list << item;
    }

    // Generate the thumbnails
    ThumbnailLoadJob* job = new ThumbnailLoadJob(list, ThumbnailGroup::Normal);
    QSignalSpy spy(job, SIGNAL(thumbnailLoaded(KFileItem,QPixmap,QSize)));
    job->exec();
    waitForDeferredDeletes();
    while (!ThumbnailLoadJob::isPendingThumbnailCacheEmpty()) {
        QTest::qWait(100);
    }

    // Check we generated the correct number of thumbnails
    QDir thumbnailDir = ThumbnailLoadJob::thumbnailBaseDir(ThumbnailGroup::Normal);
    // There should be one file less because small.png is a png and is too
    // small to have a thumbnail
    QStringList entryList = thumbnailDir.entryList(QStringList("*.png"));
    QCOMPARE(entryList.count(), mSandBox.mSizeHash.size() - 1);

    // Check what was in the thumbnailLoaded() signals
    QCOMPARE(spy.count(), mSandBox.mSizeHash.size());
    QSignalSpy::ConstIterator it = spy.constBegin(),
                              end = spy.constEnd();
    for (; it != end; ++it) {
        const QVariantList args = *it;
        const KFileItem item = qvariant_cast<KFileItem>(args.at(0));
        const QSize size = args.at(2).toSize();
        const QSize expectedSize = mSandBox.mSizeHash.value(item.url().fileName());
        QCOMPARE(size, expectedSize);
    }
}

void ThumbnailLoadJobTest::testUseEmbeddedOrNot()
{
    QImage expectedThumbnail;
    ThumbnailLoadJob* job;
    QPixmap thumbnailPix;
    SandBox sandBox;
    sandBox.initDir();
    // This image is red and 256x128 but contains a white 128x64 thumbnail
    sandBox.copyTestImage("embedded-thumbnail.jpg", 256, 128);

    KFileItemList list;
    KUrl url("file://" + QDir(sandBox.mPath).absoluteFilePath("embedded-thumbnail.jpg"));
    list << KFileItem(KFileItem::Unknown, KFileItem::Unknown, url);

    // Loading a normal thumbnail should bring the white one
    job = new ThumbnailLoadJob(list, ThumbnailGroup::Normal);
    QSignalSpy spy1(job, SIGNAL(thumbnailLoaded(KFileItem,QPixmap,QSize)));
    job->exec();
    waitForDeferredDeletes();

    QCOMPARE(spy1.count(), 1);
    expectedThumbnail = createColoredImage(128, 64, Qt::white);
    thumbnailPix = qvariant_cast<QPixmap>(spy1.at(0).at(1));
    QVERIFY(fuzzyImageCompare(expectedThumbnail, thumbnailPix.toImage()));

    // Loading a large thumbnail should bring the red one
    job = new ThumbnailLoadJob(list, ThumbnailGroup::Large);
    QSignalSpy spy2(job, SIGNAL(thumbnailLoaded(KFileItem,QPixmap,QSize)));
    job->exec();
    waitForDeferredDeletes();

    QCOMPARE(spy2.count(), 1);
    expectedThumbnail = createColoredImage(256, 128, Qt::red);
    thumbnailPix = qvariant_cast<QPixmap>(spy2.at(0).at(1));
    QVERIFY(fuzzyImageCompare(expectedThumbnail, thumbnailPix.toImage()));
}

void ThumbnailLoadJobTest::testLoadRemote()
{
    KUrl url = setUpRemoteTestDir("test.png");
    if (!url.isValid()) {
        return;
    }
    url.addPath("test.png");

    KFileItemList list;
    KFileItem item(KFileItem::Unknown, KFileItem::Unknown, url);
    list << item;

    ThumbnailLoadJob* job = new ThumbnailLoadJob(list, ThumbnailGroup::Normal);
    job->exec();
    waitForDeferredDeletes();
    while (!ThumbnailLoadJob::isPendingThumbnailCacheEmpty()) {
        QTest::qWait(100);
    }

    QDir thumbnailDir = ThumbnailLoadJob::thumbnailBaseDir(ThumbnailGroup::Normal);
    QStringList entryList = thumbnailDir.entryList(QStringList("*.png"));
    QCOMPARE(entryList.count(), 1);
}
