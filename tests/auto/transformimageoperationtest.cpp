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
// Qt
#include <QEventLoop>
#include <QImage>

// KDE
#include <QDebug>
#include <QTest>

// Local
#include "../lib/document/documentfactory.h"
#include "../lib/imageutils.h"
#include "../lib/transformimageoperation.h"
#include "testutils.h"

#include "transformimageoperationtest.h"

QTEST_MAIN(TransformImageOperationTest)

using namespace Gwenview;

void TransformImageOperationTest::initTestCase()
{
    qRegisterMetaType<QUrl>("QUrl");
}

void TransformImageOperationTest::init()
{
    DocumentFactory::instance()->clearCache();
}

void TransformImageOperationTest::testRotate90()
{
    QUrl url = urlForTestFile("test.png");
    QImage image;

    bool ok = image.load(url.toLocalFile());
    QVERIFY2(ok, "Could not load 'test.png'");
    QTransform matrix = ImageUtils::transformMatrix(ROT_90);
    image = image.transformed(matrix);

    Document::Ptr doc = DocumentFactory::instance()->load(url);

    TransformImageOperation* op = new TransformImageOperation(ROT_90);
    QEventLoop loop;
    connect(doc.data(), &Document::allTasksDone, &loop, &QEventLoop::quit);
    op->applyToDocument(doc);
    loop.exec();

    QCOMPARE(image, doc->image());
}
