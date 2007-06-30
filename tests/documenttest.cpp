/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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
#include <QImage>

// KDE
#include <qtest_kde.h>

// Local
#include "../lib/documentfactory.h"
#include "../lib/imageutils.h"

#include "documenttest.moc"

QTEST_KDEMAIN( DocumentTest, GUI )

using namespace Gwenview;

void DocumentTest::testLoad() {
	KUrl url("test.png");
	QImage image;
	bool ok = image.load(url.path());
	QVERIFY2(ok, "Could not load 'test.png'");
	Document::Ptr doc = DocumentFactory::instance()->load(url);
	while (!doc->isLoaded()) {
		QTest::qWait(30);
	}
	QCOMPARE(image, doc->image());
	QCOMPARE(doc->format().data(), "png");
}

/**
 * Check that deleting a document while it is loading does not crash
 */
void DocumentTest::testDeleteWhileLoading() {
	{
		KUrl url("test.png");
		QImage image;
		bool ok = image.load(url.path());
		QVERIFY2(ok, "Could not load 'test.png'");
		Document::Ptr doc = DocumentFactory::instance()->load(url);
	}
	// Wait two seconds. If the test fails we will get a segfault while waiting
	QTest::qWait(2000);
}

void DocumentTest::testLoadRotated() {
	KUrl url("orient6.jpg");
	QImage image;
	bool ok = image.load(url.path());
	QVERIFY2(ok, "Could not load 'orient6.jpg'");
	QMatrix matrix = ImageUtils::transformMatrix(ROT_90);
	image = image.transformed(matrix);
	image.save("expected.png", "PNG");

	Document::Ptr doc = DocumentFactory::instance()->load(url);
	while (!doc->isLoaded()) {
		QTest::qWait(30);
	}
	doc->image().save("result.png", "PNG");
	QCOMPARE(image, doc->image());
}

/**
 * Checks that asking the DocumentFactory the same document twice in a row does
 * not load it twice
 */
void DocumentTest::testMultipleLoads() {
	KUrl url("orient6.jpg");
	Document::Ptr doc1 = DocumentFactory::instance()->load(url);
	Document::Ptr doc2 = DocumentFactory::instance()->load(url);

	QCOMPARE(doc1.data(), doc2.data());
}

void DocumentTest::testSave() {
	KUrl url("orient6.jpg");
	Document::Ptr doc = DocumentFactory::instance()->load(url);
	KUrl destUrl(QDir::currentPath() + "/result.png");
	Document::SaveResult result = doc->save(destUrl, "png");
	QCOMPARE(result, Document::SR_OK);

	QVERIFY2(doc->isLoaded(),
		"Document is supposed to finish loading before saving"
		);
	
	QImage image("result.png", "png");
	QCOMPARE(doc->image(), image);
}

void DocumentTest::testLosslessSave() {
	KUrl url("orient6.jpg");
	Document::Ptr doc = DocumentFactory::instance()->load(url);
	KUrl destUrl(QDir::currentPath() + "/result.jpg");
	Document::SaveResult result = doc->save(destUrl, "jpeg");
	QCOMPARE(result, Document::SR_OK);

	QFile originalFile(url.path());
	originalFile.open(QIODevice::ReadOnly);
	QByteArray originalData = originalFile.readAll();

	QFile resultFile(destUrl.path());
	resultFile.open(QIODevice::ReadOnly);
	QByteArray resultData = resultFile.readAll();
	QCOMPARE(originalData, resultData);
}
