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
// Self
#include "cmsprofiletest.h"

// Local
#include <lib/cms/cmsprofile.h>
#include <lib/exiv2imageloader.h>
#include <testutils.h>

// KDE
#include <qtest.h>

// Qt

QTEST_MAIN(CmsProfileTest)

using namespace Gwenview;

void CmsProfileTest::testLoadFromImageData()
{
    QFETCH(QString, fileName);
    QFETCH(QByteArray, format);
    QByteArray data;
    {
        QString path = pathForTestFile(fileName);
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        data = file.readAll();
    }
    Cms::Profile::Ptr ptr = Cms::Profile::loadFromImageData(data, format);
    QVERIFY(!ptr.isNull());
}

#define NEW_ROW(fileName, format) QTest::newRow(fileName) << fileName << QByteArray(format)
void CmsProfileTest::testLoadFromImageData_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QByteArray>("format");
    NEW_ROW("cms/colourTestFakeBRG.png", "png");
    NEW_ROW("cms/colourTestsRGB.png", "png");
    NEW_ROW("cms/Upper_Left.jpg", "jpeg");
    NEW_ROW("cms/Upper_Right.jpg", "jpeg");
    NEW_ROW("cms/Lower_Left.jpg", "jpeg");
    NEW_ROW("cms/Lower_Right.jpg", "jpeg");
}
#undef NEW_ROW

#if 0

void CmsProfileTest::testLoadFromExiv2Image()
{
    QFETCH(QString, fileName);
    Exiv2::Image::AutoPtr image;
    {
        QByteArray data;
        QString path = pathForTestFile(fileName);
        kWarning() << path;
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        data = file.readAll();

        Exiv2ImageLoader loader;
        QVERIFY(loader.load(data));
        image = loader.popImage();
    }
    Cms::Profile::Ptr ptr = Cms::Profile::loadFromExiv2Image(image.get());
    QVERIFY(!ptr.isNull());
}

#define NEW_ROW(fileName) QTest::newRow(fileName) << fileName
void CmsProfileTest::testLoadFromExiv2Image_data()
{
    QTest::addColumn<QString>("fileName");
}
#undef NEW_ROW

#endif
