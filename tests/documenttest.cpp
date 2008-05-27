/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#include <QConicalGradient>
#include <QDir>
#include <QImage>
#include <QPainter>

// KDE
#include <kdebug.h>
#include <kio/netaccess.h>
#include <qtest_kde.h>

// Local
#include "../lib/abstractimageoperation.h"
#include "../lib/document/documentfactory.h"
#include "../lib/imagemetainfomodel.h"
#include "../lib/imageutils.h"
#include "testutils.h"

#include <exiv2/exif.hpp>

#include "documenttest.moc"

QTEST_KDEMAIN( DocumentTest, GUI )

using namespace Gwenview;


void DocumentTest::initTestCase() {
	qRegisterMetaType<KUrl>("KUrl");
}


void DocumentTest::init() {
	DocumentFactory::instance()->clearCache();
}


void DocumentTest::testLoad() {
	QFETCH(QString, fileName);
	QFETCH(QByteArray, expectedFormat);
	QFETCH(QImage, expectedImage);
	KUrl url = urlForTestFile(fileName);
	QVERIFY2(!expectedImage.isNull(), "Could not load test image");
	Document::Ptr doc = DocumentFactory::instance()->load(url);
	doc->loadFullImage();
	doc->waitUntilLoaded();
	QCOMPARE(expectedImage, doc->image());
	QCOMPARE(expectedFormat, doc->format());
}

#define NEW_ROW(fileName, format) QTest::newRow(fileName) << fileName << QByteArray(format) << QImage(pathForTestFile(fileName))
void DocumentTest::testLoad_data() {
	QTest::addColumn<QString>("fileName");
	QTest::addColumn<QByteArray>("expectedFormat");
	QTest::addColumn<QImage>("expectedImage");

	NEW_ROW("test.png", "png");
	NEW_ROW("160216_no_size_before_decoding.eps", "eps");
	NEW_ROW("160382_corrupted.jpeg", "jpeg");
}

void DocumentTest::testLoadTwoPasses() {
	KUrl url = urlForTestFile("test.png");
	QImage image;
	bool ok = image.load(url.path());
	QVERIFY2(ok, "Could not load 'test.png'");
	Document::Ptr doc = DocumentFactory::instance()->load(url);
	doc->waitUntilMetaDataLoaded();
	QVERIFY2(doc->image().isNull(), "Image shouldn't have been loaded at this time");
	QCOMPARE(doc->format().data(), "png");
	doc->loadFullImage();
	doc->waitUntilLoaded();
	QCOMPARE(image, doc->image());
}

void DocumentTest::testLoadEmpty() {
	KUrl url = urlForTestFile("empty.png");
	Document::Ptr doc = DocumentFactory::instance()->load(url);
	QSignalSpy loadingFailedSpy(doc.data(), SIGNAL(loadingFailed(const KUrl&)));
	while (doc->loadingState() == Document::Loading) {
		QTest::qWait(100);
	}
	QCOMPARE(doc->loadingState(), Document::LoadingFailed);
	QCOMPARE(loadingFailedSpy.count(), 1);
}

void DocumentTest::testLoadDownSampled() {
	// Note: for now we only support down sampling on jpeg, do not use test.png
	// here
	KUrl url = urlForTestFile("orient6.jpg");
	QImage image;
	bool ok = image.load(url.path());
	QVERIFY2(ok, "Could not load 'test.png'");
	Document::Ptr doc = DocumentFactory::instance()->load(url);

	QSignalSpy downSampledImageReadySpy(doc.data(), SIGNAL(downSampledImageReady()));
	bool ready = doc->prepareDownSampledImageForZoom(0.4);
	QVERIFY2(!ready, "There should not be a down sampled image at this point");

	while (downSampledImageReadySpy.count() == 0) {
		QTest::qWait(100);
	}
	QImage downSampledImage = doc->downSampledImageForZoom(0.4);
	QVERIFY2(!downSampledImage.isNull(), "Down sampled image should not be null");

	QCOMPARE(downSampledImage.size(), doc->size() / 2);
}

void DocumentTest::testLoadRemote() {
	QString testTarGzPath = pathForTestFile("test.tar.gz");
	KUrl url;
	url.setProtocol("tar");
	url.setPath(testTarGzPath + "/test.png");

	QVERIFY2(KIO::NetAccess::exists(url, KIO::NetAccess::SourceSide, 0), "test archive not found");

	Document::Ptr doc = DocumentFactory::instance()->load(url);
	doc->loadFullImage();
	doc->waitUntilLoaded();
	QImage image = doc->image();
	QCOMPARE(image.width(), 300);
	QCOMPARE(image.height(), 200);
}

void DocumentTest::testSaveRemote() {
	KUrl srcUrl = urlForTestFile("test.png");
	Document::Ptr doc = DocumentFactory::instance()->load(srcUrl);
	doc->loadFullImage();
	doc->waitUntilLoaded();

	KUrl dstUrl;
	dstUrl.setProtocol("trash");
	dstUrl.setPath("/test.png");
	Document::SaveResult result = doc->save(dstUrl, "png");
	QCOMPARE(result, Document::SR_UploadFailed);

	if (qgetenv("REMOTE_SFTP_TEST").isEmpty()) {
		kWarning() << "*** Define the environment variable REMOTE_SFTP_TEST to try saving an image to sftp://localhost/tmp/test.png";
	} else {
		dstUrl.setProtocol("sftp");
		dstUrl.setHost("localhost");
		dstUrl.setPath("/tmp/test.png");
		result = doc->save(dstUrl, "png");
		QCOMPARE(result, Document::SR_OK);
	}
}

/**
 * Check that deleting a document while it is loading does not crash
 */
void DocumentTest::testDeleteWhileLoading() {
	{
		KUrl url = urlForTestFile("test.png");
		QImage image;
		bool ok = image.load(url.path());
		QVERIFY2(ok, "Could not load 'test.png'");
		Document::Ptr doc = DocumentFactory::instance()->load(url);
	}
	DocumentFactory::instance()->clearCache();
	// Wait two seconds. If the test fails we will get a segfault while waiting
	QTest::qWait(2000);
}

void DocumentTest::testLoadRotated() {
	KUrl url = urlForTestFile("orient6.jpg");
	QImage image;
	bool ok = image.load(url.path());
	QVERIFY2(ok, "Could not load 'orient6.jpg'");
	QMatrix matrix = ImageUtils::transformMatrix(ROT_90);
	image = image.transformed(matrix);

	Document::Ptr doc = DocumentFactory::instance()->load(url);
	doc->loadFullImage();
	doc->waitUntilLoaded();
	QCOMPARE(image, doc->image());
}

/**
 * Checks that asking the DocumentFactory the same document twice in a row does
 * not load it twice
 */
void DocumentTest::testMultipleLoads() {
	KUrl url = urlForTestFile("orient6.jpg");
	Document::Ptr doc1 = DocumentFactory::instance()->load(url);
	Document::Ptr doc2 = DocumentFactory::instance()->load(url);

	QCOMPARE(doc1.data(), doc2.data());
}

void DocumentTest::testSave() {
	KUrl url = urlForTestFile("orient6.jpg");
	Document::Ptr doc = DocumentFactory::instance()->load(url);
	KUrl destUrl = urlForTestOutputFile("result.png");
	Document::SaveResult result = doc->save(destUrl, "png");
	QCOMPARE(result, Document::SR_OK);
	QCOMPARE(doc->format().data(), "png");

	QVERIFY2(doc->loadingState() == Document::Loaded,
		"Document is supposed to finish loading before saving"
		);
	
	QImage image("result.png", "png");
	QCOMPARE(doc->image(), image);
}

void DocumentTest::testLosslessSave() {
	KUrl url1 = urlForTestFile("orient6.jpg");
	Document::Ptr doc = DocumentFactory::instance()->load(url1);
	KUrl url2 = urlForTestOutputFile("orient1.jpg");
	Document::SaveResult result = doc->save(url2, "jpeg");
	QCOMPARE(result, Document::SR_OK);

	QImage image1;
	QVERIFY(image1.load(url1.path()));

	QImage image2;
	QVERIFY(image2.load(url2.path()));

	QCOMPARE(image1, image2);
}

void DocumentTest::testLosslessRotate() {
	// Generate test image
	QImage image1(200, 96, QImage::Format_RGB32);
	{
		QPainter painter(&image1);
		QConicalGradient gradient(QPointF(100, 48), 100);
		gradient.setColorAt(0, Qt::white);
		gradient.setColorAt(1, Qt::blue);
		painter.fillRect(image1.rect(), gradient);
	}

	KUrl url1 = urlForTestOutputFile("lossless1.jpg");
	QVERIFY(image1.save(url1.path(), "jpeg"));

	// Load it as a Gwenview document
	Document::Ptr doc = DocumentFactory::instance()->load(url1);
	doc->loadFullImage();
	doc->waitUntilLoaded();

	// Rotate one time
	doc->applyTransformation(ROT_90);

	// Save it
	KUrl url2 = urlForTestOutputFile("lossless2.jpg");
	doc->save(url2, "jpeg");

	// Load the saved image
	doc = DocumentFactory::instance()->load(url2);
	doc->loadFullImage();
	doc->waitUntilLoaded();

	// Rotate the other way
	doc->applyTransformation(ROT_270);
	doc->save(url2, "jpeg");

	// Compare the saved images
	QVERIFY(image1.load(url1.path()));
	QImage image2;
	QVERIFY(image2.load(url2.path()));

	QCOMPARE(image1, image2);
}

void DocumentTest::testModify() {
	class TestOperation : public AbstractImageOperation {
	public:
		void redo() {
			QImage image(10, 10, QImage::Format_ARGB32);
			image.fill(QColor(Qt::white).rgb());
			document()->setImage(image);
		}
	};

	KUrl url = urlForTestFile("orient6.jpg");
	Document::Ptr doc = DocumentFactory::instance()->load(url);
	doc->loadFullImage();
	doc->waitUntilLoaded();
	QVERIFY(!doc->isModified());

	TestOperation* op = new TestOperation;
	op->setDocument(doc);
	doc->undoStack()->push(op);
	QVERIFY(doc->isModified());

	KUrl destUrl = urlForTestOutputFile("modify.png");
	QCOMPARE(doc->save(destUrl, "png"), Document::SR_OK);

	QVERIFY(!doc->isModified());
}

void DocumentTest::testMetaDataJpeg() {
	KUrl url = urlForTestFile("orient6.jpg");
	Document::Ptr doc = DocumentFactory::instance()->load(url);

	// We cleared the cache, so the document should not be loaded
	Q_ASSERT(doc->loadingState() == Document::Loading);

	// Wait until we receive the metaDataUpdated() signal
	QSignalSpy metaDataUpdatedSpy(doc.data(), SIGNAL(metaDataUpdated()));
	while (metaDataUpdatedSpy.count() == 0) {
		QTest::qWait(100);
	}

	// Extract an exif key
	QString value = doc->metaInfo()->getValueForKey("Exif.Image.Make");
	QCOMPARE(value, QString::fromUtf8("Canon"));
}


void DocumentTest::testMetaDataBmp() {
	KUrl url = urlForTestOutputFile("metadata.bmp");
	const int width = 200;
	const int height = 100;
	QImage image(width, height, QImage::Format_ARGB32);
	image.fill(Qt::black);
	image.save(url.path(), "BMP");

	Document::Ptr doc = DocumentFactory::instance()->load(url);
	QSignalSpy metaDataUpdatedSpy(doc.data(), SIGNAL(metaDataUpdated()));
	doc->waitUntilMetaDataLoaded();

	Q_ASSERT(metaDataUpdatedSpy.count() >= 1);

	QString value = doc->metaInfo()->getValueForKey("General.ImageSize");
	QString expectedValue = QString("%1x%2").arg(width).arg(height);
	QCOMPARE(value, expectedValue);
}
