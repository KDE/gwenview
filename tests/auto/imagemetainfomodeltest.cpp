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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
// Qt

// KDE
#include <KDebug>
#include <qtest_kde.h>

// Local
#include "../lib/exiv2imageloader.h"
#include "../lib/imagemetainfomodel.h"
#include "testutils.h"

#include <exiv2/exif.hpp>

#include "imagemetainfomodeltest.moc"

QTEST_KDEMAIN(ImageMetaInfoModelTest, GUI)

using namespace Gwenview;

void ImageMetaInfoModelTest::testCatchExiv2Errors()
{
    QByteArray data;
    {
        QString path = pathForTestFile("302350_exiv_0.23_exception.jpg");
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        data = file.readAll();
    }

    Exiv2::Image::AutoPtr image;
    {
        Exiv2ImageLoader loader;
        QVERIFY(loader.load(data));
        image = loader.popImage();
    }

    ImageMetaInfoModel model;
    model.setExiv2Image(image.get());
}
