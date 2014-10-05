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
#include "jpegcontenttest.h"
#include <iostream>

// Qt
#include <QDir>
#include <QFile>
#include <QImage>
#include <QString>

// KDE
#include <qtest.h>
#include <QDebug>
#include <KFileMetaInfo>

// Local
#include "../lib/orientation.h"
#include "../lib/jpegcontent.h"
#include "testutils.h"

using namespace std;

const char* ORIENT6_FILE = "orient6.jpg";
const char* ORIENT1_VFLIP_FILE = "orient1_vflip.jpg";
const char* CUT_FILE = "cut.jpg";
const char* TMP_FILE = "tmp.jpg";
const char* THUMBNAIL_FILE = "test_thumbnail.jpg";

const int ORIENT6_WIDTH = 128; // This size is the size *after* orientation
const int ORIENT6_HEIGHT = 256; // has been applied
const QString ORIENT6_COMMENT = "a comment";

QTEST_MAIN(JpegContentTest)

void JpegContentTest::initTestCase()
{
    bool result;
    QFile in(pathForTestFile(ORIENT6_FILE));
    result = in.open(QIODevice::ReadOnly);
    QVERIFY(result);

    QFileInfo info(in);
    int size = info.size() / 2;

    char* data = new char[size];
    int readSize = in.read(data, size);
    QCOMPARE(size, readSize);

    QFile out(CUT_FILE);
    result = out.open(QIODevice::WriteOnly);
    QVERIFY(result);

    int wroteSize = out.write(data, size);
    QCOMPARE(size, wroteSize);
    delete []data;
}

void JpegContentTest::cleanupTestCase()
{
    QDir::current().remove(CUT_FILE);
}

typedef QMap<QString, QString> MetaInfoMap;

MetaInfoMap getMetaInfo(const QString& path)
{
    KFileMetaInfo fmi(path);
    QStringList list = fmi.supportedKeys();
    QStringList::ConstIterator it = list.constBegin();
    MetaInfoMap map;

    for (; it != list.constEnd(); ++it) {
        KFileMetaInfoItem item = fmi.item(*it);
        map[*it] = item.value().toString();
    }

    return map;
}

void compareMetaInfo(const QString& path1, const QString& path2, const QStringList& ignoredKeys)
{
    MetaInfoMap mim1 = getMetaInfo(path1);
    MetaInfoMap mim2 = getMetaInfo(path2);

    QCOMPARE(mim1.keys(), mim2.keys());
    QList<QString> keys = mim1.keys();
    QList<QString>::ConstIterator it = keys.constBegin();
    for (; it != keys.constEnd(); ++it) {
        QString key = *it;
        if (ignoredKeys.contains(key)) continue;

        QString msg =
            QString("Meta info differs for key '%1': v1=%2 v2=%3")
            .arg(key)
            .arg(mim1[key])
            .arg(mim2[key]);

        QVERIFY2(mim1[key] == mim2[key], msg.toUtf8());
    }
}

void JpegContentTest::testResetOrientation()
{
    Gwenview::JpegContent content;
    bool result;

    // Test resetOrientation without transform
    result = content.load(pathForTestFile(ORIENT6_FILE));
    QVERIFY(result);

    content.resetOrientation();

    result = content.save(TMP_FILE);
    QVERIFY(result);

    result = content.load(TMP_FILE);
    QVERIFY(result);
    QCOMPARE(content.orientation(), Gwenview::NORMAL);

    // Test resetOrientation with transform
    result = content.load(pathForTestFile(ORIENT6_FILE));
    QVERIFY(result);

    content.resetOrientation();
    content.transform(Gwenview::ROT_90);

    result = content.save(TMP_FILE);
    QVERIFY(result);

    result = content.load(TMP_FILE);
    QVERIFY(result);
    QCOMPARE(content.orientation(), Gwenview::NORMAL);
}

/**
 * This function tests JpegContent::transform() by applying a ROT_90
 * transformation, saving, reloading and applying a ROT_270 to undo the ROT_90.
 * Saving and reloading are necessary because lossless transformation only
 * happens in JpegContent::save()
 */
void JpegContentTest::testTransform()
{
    bool result;
    QImage finalImage, expectedImage;

    Gwenview::JpegContent content;
    result = content.load(pathForTestFile(ORIENT6_FILE));
    QVERIFY(result);

    content.transform(Gwenview::ROT_90);
    result = content.save(TMP_FILE);
    QVERIFY(result);

    result = content.load(TMP_FILE);
    QVERIFY(result);
    content.transform(Gwenview::ROT_270);
    result = content.save(TMP_FILE);
    QVERIFY(result);

    result = finalImage.load(TMP_FILE);
    QVERIFY(result);

    result = expectedImage.load(pathForTestFile(ORIENT6_FILE));
    QVERIFY(result);

    QCOMPARE(finalImage , expectedImage);
}

void JpegContentTest::testSetComment()
{
    QString comment = "test comment";
    Gwenview::JpegContent content;
    bool result;
    result = content.load(pathForTestFile(ORIENT6_FILE));
    QVERIFY(result);

    content.setComment(comment);
    QCOMPARE(content.comment() , comment);
    result = content.save(TMP_FILE);
    QVERIFY(result);

    result = content.load(TMP_FILE);
    QVERIFY(result);
    QCOMPARE(content.comment() , comment);
}

void JpegContentTest::testReadInfo()
{
    Gwenview::JpegContent content;
    bool result = content.load(pathForTestFile(ORIENT6_FILE));
    QVERIFY(result);
    QCOMPARE(int(content.orientation()), 6);
    QCOMPARE(content.comment() , ORIENT6_COMMENT);
    QCOMPARE(content.size() , QSize(ORIENT6_WIDTH, ORIENT6_HEIGHT));
}

void JpegContentTest::testThumbnail()
{
    Gwenview::JpegContent content;
    bool result = content.load(pathForTestFile(ORIENT6_FILE));
    QVERIFY(result);
    QImage thumbnail = content.thumbnail();
    result = thumbnail.save(THUMBNAIL_FILE, "JPEG");
    QVERIFY(result);
}

void JpegContentTest::testMultipleRotations()
{
    // Test that rotating a file a lot of times does not cause findJxform() to fail
    Gwenview::JpegContent content;
    bool result = content.load(pathForTestFile(ORIENT6_FILE));
    QVERIFY(result);
    result = content.load(pathForTestFile(ORIENT6_FILE));
    QVERIFY(result);

    // 12*4 + 1 is the same as 1, since rotating four times brings you back
    for (int loop = 0; loop < 12 * 4 + 1; ++loop) {
        content.transform(Gwenview::ROT_90);
    }
    result = content.save(TMP_FILE);
    QVERIFY(result);

    result = content.load(TMP_FILE);
    QVERIFY(result);

    QCOMPARE(content.size() , QSize(ORIENT6_HEIGHT, ORIENT6_WIDTH));

    // Check the other meta info are still here
    QStringList ignoredKeys;
    ignoredKeys << "Orientation" << "Comment";
    compareMetaInfo(pathForTestFile(ORIENT6_FILE), pathForTestFile(ORIENT1_VFLIP_FILE), ignoredKeys);
}

void JpegContentTest::testLoadTruncated()
{
    // Test that loading and manipulating a truncated file does not crash
    Gwenview::JpegContent content;
    bool result = content.load(CUT_FILE);
    QVERIFY(result);
    QCOMPARE(int(content.orientation()), 6);
    QCOMPARE(content.comment() , ORIENT6_COMMENT);
    content.transform(Gwenview::VFLIP);
    qWarning() << "# Next function should output errors about incomplete image" ;
    content.save(TMP_FILE);
    qWarning() << "#" ;
}

void JpegContentTest::testRawData()
{
    Gwenview::JpegContent content;
    bool result = content.load(pathForTestFile(ORIENT6_FILE));
    QVERIFY(result);

    QByteArray fileData;
    QFile file(pathForTestFile(ORIENT6_FILE));
    result = file.open(QIODevice::ReadOnly);
    QVERIFY(result);
    fileData = file.readAll();

    QCOMPARE(content.rawData(), fileData);
}

void JpegContentTest::testSetImage()
{
    Gwenview::JpegContent content;
    bool result = content.load(pathForTestFile(ORIENT6_FILE));
    QVERIFY(result);

    QImage image = QImage(400, 300, QImage::Format_RGB32);
    image.fill(Qt::red);

    content.setImage(image);

    result = content.save(TMP_FILE);
    QVERIFY(result);

    result = content.load(TMP_FILE);
    QVERIFY(result);

    QCOMPARE(content.size(), image.size());

    QStringList ignoredKeys;
    ignoredKeys << "Orientation";
    compareMetaInfo(pathForTestFile(ORIENT6_FILE), pathForTestFile(TMP_FILE), ignoredKeys);
}
