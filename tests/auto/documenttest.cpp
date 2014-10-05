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
// Qt
#include <QConicalGradient>
#include <QImage>
#include <QPainter>

// KDE
#include <QDebug>
#include <KJobUiDelegate>
#include <KIO/NetAccess>
#include <qtest.h>
#include <libkdcraw/kdcraw.h>

// Local
#include "../lib/abstractimageoperation.h"
#include "../lib/document/abstractdocumenteditor.h"
#include "../lib/document/documentjob.h"
#include "../lib/document/documentfactory.h"
#include "../lib/imagemetainfomodel.h"
#include "../lib/imageutils.h"
#include "../lib/transformimageoperation.h"
#include "testutils.h"

#include <exiv2/exif.hpp>

#include "documenttest.h"

QTEST_MAIN(DocumentTest)

using namespace Gwenview;

static void waitUntilMetaInfoLoaded(Document::Ptr doc)
{
    while (doc->loadingState() < Document::MetaInfoLoaded) {
        QTest::qWait(100);
    }
}

static bool waitUntilJobIsDone(DocumentJob* job)
{
    JobWatcher watcher(job);
    watcher.wait();
    return watcher.error() == KJob::NoError;
}

void DocumentTest::initTestCase()
{
    qRegisterMetaType<QUrl>("QUrl");
}

void DocumentTest::init()
{
    DocumentFactory::instance()->clearCache();
}

void DocumentTest::testLoad()
{
    QFETCH(QString, fileName);
    QFETCH(QByteArray, expectedFormat);
    QFETCH(int, expectedKindInt);
    QFETCH(bool, expectedIsAnimated);
    QFETCH(QImage, expectedImage);
    QFETCH(int, maxHeight); // number of lines to test. -1 to test all lines

    MimeTypeUtils::Kind expectedKind = MimeTypeUtils::Kind(expectedKindInt);

    QUrl url = urlForTestFile(fileName);

    // testing RAW loading. For raw, QImage directly won't work -> load it using KDCRaw
    QByteArray mFormatHint = url.fileName().section('.', -1).toAscii().toLower();
    if (KDcrawIface::KDcraw::rawFilesList().contains(QString(mFormatHint))) {
        if (!KDcrawIface::KDcraw::loadEmbeddedPreview(expectedImage, url.toLocalFile())) {
            QSKIP("Not running this test: failed to get expectedImage. Try running ./fetch_testing_raw.sh\
 in the tests/data directory and then rerun the tests.", SkipSingle);
        }
    }

    if (expectedKind != MimeTypeUtils::KIND_SVG_IMAGE) {
        if (expectedImage.isNull()) {
            QSKIP("Not running this test: QImage failed to load the test image", SkipSingle);
        }
    }

    Document::Ptr doc = DocumentFactory::instance()->load(url);
    QSignalSpy spy(doc.data(), SIGNAL(isAnimatedUpdated()));
    doc->startLoadingFullImage();
    doc->waitUntilLoaded();
    QCOMPARE(doc->loadingState(), Document::Loaded);

    QCOMPARE(doc->kind(), expectedKind);
    QCOMPARE(doc->isAnimated(), expectedIsAnimated);
    QCOMPARE(spy.count(), doc->isAnimated() ? 1 : 0);
    if (doc->kind() == MimeTypeUtils::KIND_RASTER_IMAGE) {
        QImage image = doc->image();
        if (maxHeight > -1) {
            QRect poiRect(0, 0, image.width(), maxHeight);
            image = image.copy(poiRect);
            expectedImage = expectedImage.copy(poiRect);
        }
        QCOMPARE(image, expectedImage);
        QCOMPARE(QString(doc->format()), QString(expectedFormat));
    }
}

static void testLoad_newRow(
    const char* fileName,
    const QByteArray& format,
    MimeTypeUtils::Kind kind = MimeTypeUtils::KIND_RASTER_IMAGE,
    bool isAnimated = false,
    int maxHeight = -1
    )
{
    QTest::newRow(fileName)
        << fileName
        << QByteArray(format)
        << int(kind)
        << isAnimated
        << QImage(pathForTestFile(fileName), format)
        << maxHeight;
}

void DocumentTest::testLoad_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QByteArray>("expectedFormat");
    QTest::addColumn<int>("expectedKindInt");
    QTest::addColumn<bool>("expectedIsAnimated");
    QTest::addColumn<QImage>("expectedImage");
    QTest::addColumn<int>("maxHeight");

    testLoad_newRow("test.png", "png");
    testLoad_newRow("160216_no_size_before_decoding.eps", "eps");
    testLoad_newRow("160382_corrupted.jpeg", "jpeg", MimeTypeUtils::KIND_RASTER_IMAGE, false, 55);
    testLoad_newRow("1x10k.png", "png");
    testLoad_newRow("1x10k.jpg", "jpeg");
    testLoad_newRow("test.xcf", "xcf");
    testLoad_newRow("188191_does_not_load.tga", "tga");
    testLoad_newRow("289819_does_not_load.png", "png");
    testLoad_newRow("png-with-jpeg-extension.jpg", "png");
    testLoad_newRow("jpg-with-gif-extension.gif", "jpeg");

    // RAW preview
    testLoad_newRow("CANON-EOS350D-02.CR2", "cr2", MimeTypeUtils::KIND_RASTER_IMAGE, false);
    testLoad_newRow("dsc_0093.nef", "nef", MimeTypeUtils::KIND_RASTER_IMAGE, false);

    // SVG
    testLoad_newRow("test.svg", "", MimeTypeUtils::KIND_SVG_IMAGE);
    // FIXME: Test svgz

    // Animated
    testLoad_newRow("4frames.gif", "gif", MimeTypeUtils::KIND_RASTER_IMAGE, true);
    testLoad_newRow("1frame.gif", "gif", MimeTypeUtils::KIND_RASTER_IMAGE, false);
    testLoad_newRow("185523_1frame_with_graphic_control_extension.gif",
                    "gif", MimeTypeUtils::KIND_RASTER_IMAGE, false);
}

void DocumentTest::testLoadTwoPasses()
{
    QUrl url = urlForTestFile("test.png");
    QImage image;
    bool ok = image.load(url.toLocalFile());
    QVERIFY2(ok, "Could not load 'test.png'");
    Document::Ptr doc = DocumentFactory::instance()->load(url);
    waitUntilMetaInfoLoaded(doc);
    QVERIFY2(doc->image().isNull(), "Image shouldn't have been loaded at this time");
    QCOMPARE(doc->format().data(), "png");
    doc->startLoadingFullImage();
    doc->waitUntilLoaded();
    QCOMPARE(image, doc->image());
}

void DocumentTest::testLoadEmpty()
{
    QUrl url = urlForTestFile("empty.png");
    Document::Ptr doc = DocumentFactory::instance()->load(url);
    while (doc->loadingState() <= Document::KindDetermined) {
        QTest::qWait(100);
    }
    QCOMPARE(doc->loadingState(), Document::LoadingFailed);
}

#define NEW_ROW(fileName) QTest::newRow(fileName) << fileName
void DocumentTest::testLoadDownSampled_data()
{
    QTest::addColumn<QString>("fileName");

    NEW_ROW("orient6.jpg");
    NEW_ROW("1x10k.jpg");
}
#undef NEW_ROW

void DocumentTest::testLoadDownSampled()
{
    // Note: for now we only support down sampling on jpeg, do not use test.png
    // here
    QFETCH(QString, fileName);
    QUrl url = urlForTestFile(fileName);
    QImage image;
    bool ok = image.load(url.toLocalFile());
    QVERIFY2(ok, "Could not load test image");
    Document::Ptr doc = DocumentFactory::instance()->load(url);

    QSignalSpy downSampledImageReadySpy(doc.data(), SIGNAL(downSampledImageReady()));
    QSignalSpy loadingFailedSpy(doc.data(), SIGNAL(loadingFailed(QUrl)));
    QSignalSpy loadedSpy(doc.data(), SIGNAL(loaded(QUrl)));
    bool ready = doc->prepareDownSampledImageForZoom(0.2);
    QVERIFY2(!ready, "There should not be a down sampled image at this point");

    while (downSampledImageReadySpy.count() == 0 && loadingFailedSpy.count() == 0 && loadedSpy.count() == 0) {
        QTest::qWait(100);
    }
    QImage downSampledImage = doc->downSampledImageForZoom(0.2);
    QVERIFY2(!downSampledImage.isNull(), "Down sampled image should not be null");

    QSize expectedSize = doc->size() / 2;
    if (expectedSize.isEmpty()) {
        expectedSize = image.size();
    }
    QCOMPARE(downSampledImage.size(), expectedSize);
}

/**
 * Down sampling is not supported on png. We should get a complete image
 * instead.
 */
void DocumentTest::testLoadDownSampledPng()
{
    QUrl url = urlForTestFile("test.png");
    QImage image;
    bool ok = image.load(url.toLocalFile());
    QVERIFY2(ok, "Could not load test image");
    Document::Ptr doc = DocumentFactory::instance()->load(url);

    LoadingStateSpy stateSpy(doc);
    connect(doc.data(), SIGNAL(loaded(QUrl)), &stateSpy, SLOT(readState()));

    bool ready = doc->prepareDownSampledImageForZoom(0.2);
    QVERIFY2(!ready, "There should not be a down sampled image at this point");

    doc->waitUntilLoaded();

    QCOMPARE(stateSpy.mCallCount, 1);
    QCOMPARE(stateSpy.mState, Document::Loaded);
}

void DocumentTest::testLoadRemote()
{
    QUrl url = setUpRemoteTestDir("test.png");
    if (!url.isValid()) {
        return;
    }
    url = url.adjusted(QUrl::StripTrailingSlash);
    url.setPath(url.path() + '/' + "test.png");

    QVERIFY2(KIO::NetAccess::exists(url, KIO::NetAccess::SourceSide, 0), "test url not found");

    Document::Ptr doc = DocumentFactory::instance()->load(url);
    doc->startLoadingFullImage();
    doc->waitUntilLoaded();
    QImage image = doc->image();
    QCOMPARE(image.width(), 150);
    QCOMPARE(image.height(), 100);
}

void DocumentTest::testLoadAnimated()
{
    QUrl srcUrl = urlForTestFile("40frames.gif");
    Document::Ptr doc = DocumentFactory::instance()->load(srcUrl);
    QSignalSpy spy(doc.data(), SIGNAL(imageRectUpdated(QRect)));
    doc->startLoadingFullImage();
    doc->waitUntilLoaded();
    QVERIFY(doc->isAnimated());

    // Test we receive only one imageRectUpdated() until animation is started
    // (the imageRectUpdated() is triggered by the loading of the first image)
    QTest::qWait(1000);
    QCOMPARE(spy.count(), 1);

    // Test we now receive some imageRectUpdated()
    doc->startAnimation();
    QTest::qWait(1000);
    int count = spy.count();
    doc->stopAnimation();
    QVERIFY2(count > 0, "No imageRectUpdated() signal received");

    // Test we do not receive imageRectUpdated() anymore
    QTest::qWait(1000);
    QCOMPARE(count, spy.count());

    // Start again, we should receive imageRectUpdated() again
    doc->startAnimation();
    QTest::qWait(1000);
    QVERIFY2(spy.count() > count, "No imageRectUpdated() signal received after restarting");
}

void DocumentTest::testPrepareDownSampledAfterFailure()
{
    QUrl url = urlForTestFile("empty.png");
    Document::Ptr doc = DocumentFactory::instance()->load(url);

    doc->startLoadingFullImage();
    doc->waitUntilLoaded();
    QCOMPARE(doc->loadingState(), Document::LoadingFailed);

    bool ready = doc->prepareDownSampledImageForZoom(0.25);
    QVERIFY2(!ready, "Down sampled image should not be ready");
}

void DocumentTest::testSaveRemote()
{
    QUrl dstUrl = setUpRemoteTestDir();
    if (!dstUrl.isValid()) {
        return;
    }

    QUrl srcUrl = urlForTestFile("test.png");
    Document::Ptr doc = DocumentFactory::instance()->load(srcUrl);
    doc->startLoadingFullImage();
    doc->waitUntilLoaded();

    dstUrl = dstUrl.adjusted(QUrl::StripTrailingSlash);
    dstUrl.setPath(dstUrl.path() + '/' + "testSaveRemote.png");
    QVERIFY(waitUntilJobIsDone(doc->save(dstUrl, "png")));
}

/**
 * Check that deleting a document while it is loading does not crash
 */
void DocumentTest::testDeleteWhileLoading()
{
    {
        QUrl url = urlForTestFile("test.png");
        QImage image;
        bool ok = image.load(url.toLocalFile());
        QVERIFY2(ok, "Could not load 'test.png'");
        Document::Ptr doc = DocumentFactory::instance()->load(url);
    }
    DocumentFactory::instance()->clearCache();
    // Wait two seconds. If the test fails we will get a segfault while waiting
    QTest::qWait(2000);
}

void DocumentTest::testLoadRotated()
{
    QUrl url = urlForTestFile("orient6.jpg");
    QImage image;
    bool ok = image.load(url.toLocalFile());
    QVERIFY2(ok, "Could not load 'orient6.jpg'");
    QMatrix matrix = ImageUtils::transformMatrix(ROT_90);
    image = image.transformed(matrix);

    Document::Ptr doc = DocumentFactory::instance()->load(url);
    doc->startLoadingFullImage();
    doc->waitUntilLoaded();
    QCOMPARE(image, doc->image());

    // RAW preview on rotated image
    url = urlForTestFile("dsd_1838.nef");
    if (!KDcrawIface::KDcraw::loadEmbeddedPreview(image, url.toLocalFile())) {
        QSKIP("Not running this test: failed to get image. Try running ./fetch_testing_raw.sh\
 in the tests/data directory and then rerun the tests.", SkipSingle);
    }
    matrix = ImageUtils::transformMatrix(ROT_270);
    image = image.transformed(matrix);

    doc = DocumentFactory::instance()->load(url);
    doc->startLoadingFullImage();
    doc->waitUntilLoaded();
    QCOMPARE(image, doc->image());
}

/**
 * Checks that asking the DocumentFactory the same document twice in a row does
 * not load it twice
 */
void DocumentTest::testMultipleLoads()
{
    QUrl url = urlForTestFile("orient6.jpg");
    Document::Ptr doc1 = DocumentFactory::instance()->load(url);
    Document::Ptr doc2 = DocumentFactory::instance()->load(url);

    QCOMPARE(doc1.data(), doc2.data());
}

void DocumentTest::testSaveAs()
{
    QUrl url = urlForTestFile("orient6.jpg");
    DocumentFactory* factory = DocumentFactory::instance();
    Document::Ptr doc = factory->load(url);
    QSignalSpy savedSpy(doc.data(), SIGNAL(saved(QUrl,QUrl)));
    QSignalSpy modifiedDocumentListChangedSpy(factory, SIGNAL(modifiedDocumentListChanged()));
    QSignalSpy documentChangedSpy(factory, SIGNAL(documentChanged(QUrl)));
    doc->startLoadingFullImage();

    QUrl destUrl = urlForTestOutputFile("result.png");
    QVERIFY(waitUntilJobIsDone(doc->save(destUrl, "png")));
    QCOMPARE(doc->format().data(), "png");
    QCOMPARE(doc->url(), destUrl);
    QCOMPARE(doc->metaInfo()->getValueForKey("General.Name"), destUrl.fileName());

    QVERIFY2(doc->loadingState() == Document::Loaded,
             "Document is supposed to finish loading before saving"
            );

    QTest::qWait(100); // saved() is emitted asynchronously
    QCOMPARE(savedSpy.count(), 1);
    QVariantList args = savedSpy.takeFirst();
    QCOMPARE(args.at(0).value<QUrl>(), url);
    QCOMPARE(args.at(1).value<QUrl>(), destUrl);

    QImage image("result.png", "png");
    QCOMPARE(doc->image(), image);

    QVERIFY(!DocumentFactory::instance()->hasUrl(url));
    QVERIFY(DocumentFactory::instance()->hasUrl(destUrl));

    QCOMPARE(modifiedDocumentListChangedSpy.count(), 0); // No changes were made

    QCOMPARE(documentChangedSpy.count(), 1);
    args = documentChangedSpy.takeFirst();
    QCOMPARE(args.at(0).value<QUrl>(), destUrl);
}

void DocumentTest::testLosslessSave()
{
    QUrl url1 = urlForTestFile("orient6.jpg");
    Document::Ptr doc = DocumentFactory::instance()->load(url1);
    doc->startLoadingFullImage();

    QUrl url2 = urlForTestOutputFile("orient1.jpg");
    QVERIFY(waitUntilJobIsDone(doc->save(url2, "jpeg")));

    QImage image1;
    QVERIFY(image1.load(url1.toLocalFile()));

    QImage image2;
    QVERIFY(image2.load(url2.toLocalFile()));

    QCOMPARE(image1, image2);
}

void DocumentTest::testLosslessRotate()
{
    // Generate test image
    QImage image1(200, 96, QImage::Format_RGB32);
    {
        QPainter painter(&image1);
        QConicalGradient gradient(QPointF(100, 48), 100);
        gradient.setColorAt(0, Qt::white);
        gradient.setColorAt(1, Qt::blue);
        painter.fillRect(image1.rect(), gradient);
    }

    QUrl url1 = urlForTestOutputFile("lossless1.jpg");
    QVERIFY(image1.save(url1.toLocalFile(), "jpeg"));

    // Load it as a Gwenview document
    Document::Ptr doc = DocumentFactory::instance()->load(url1);
    doc->startLoadingFullImage();
    doc->waitUntilLoaded();

    // Rotate one time
    QVERIFY(doc->editor());
    doc->editor()->applyTransformation(ROT_90);

    // Save it
    QUrl url2 = urlForTestOutputFile("lossless2.jpg");
    waitUntilJobIsDone(doc->save(url2, "jpeg"));

    // Load the saved image
    doc = DocumentFactory::instance()->load(url2);
    doc->startLoadingFullImage();
    doc->waitUntilLoaded();

    // Rotate the other way
    QVERIFY(doc->editor());
    doc->editor()->applyTransformation(ROT_270);
    waitUntilJobIsDone(doc->save(url2, "jpeg"));

    // Compare the saved images
    QVERIFY(image1.load(url1.toLocalFile()));
    QImage image2;
    QVERIFY(image2.load(url2.toLocalFile()));

    QCOMPARE(image1, image2);
}

void DocumentTest::testModifyAndSaveAs()
{
    QVariantList args;
    class TestOperation : public AbstractImageOperation
    {
    public:
        void redo()
    {
            QImage image(10, 10, QImage::Format_ARGB32);
            image.fill(QColor(Qt::white).rgb());
            document()->editor()->setImage(image);
            finish(true);
        }
    };
    QUrl url = urlForTestFile("orient6.jpg");
    DocumentFactory* factory = DocumentFactory::instance();
    Document::Ptr doc = factory->load(url);

    QSignalSpy savedSpy(doc.data(), SIGNAL(saved(QUrl,QUrl)));
    QSignalSpy modifiedDocumentListChangedSpy(factory, SIGNAL(modifiedDocumentListChanged()));
    QSignalSpy documentChangedSpy(factory, SIGNAL(documentChanged(QUrl)));

    doc->startLoadingFullImage();
    doc->waitUntilLoaded();
    QVERIFY(!doc->isModified());
    QCOMPARE(modifiedDocumentListChangedSpy.count(), 0);

    // Modify image
    QVERIFY(doc->editor());
    TestOperation* op = new TestOperation;
    op->applyToDocument(doc);
    QVERIFY(doc->isModified());
    QCOMPARE(modifiedDocumentListChangedSpy.count(), 1);
    modifiedDocumentListChangedSpy.clear();
    QList<QUrl> lst = factory->modifiedDocumentList();
    QCOMPARE(lst.count(), 1);
    QCOMPARE(lst.first(), url);
    QCOMPARE(documentChangedSpy.count(), 1);
    args = documentChangedSpy.takeFirst();
    QCOMPARE(args.at(0).value<QUrl>(), url);

    // Save it under a new name
    QUrl destUrl = urlForTestOutputFile("modify.png");
    QVERIFY(waitUntilJobIsDone(doc->save(destUrl, "png")));

    // Wait a bit because save() will clear the undo stack when back to the
    // event loop
    QTest::qWait(100);
    QVERIFY(!doc->isModified());

    QVERIFY(!factory->hasUrl(url));
    QVERIFY(factory->hasUrl(destUrl));
    QCOMPARE(modifiedDocumentListChangedSpy.count(), 1);
    QVERIFY(DocumentFactory::instance()->modifiedDocumentList().isEmpty());

    QCOMPARE(documentChangedSpy.count(), 2);
    QUrl::List modifiedUrls = QUrl::List() << url << destUrl;
    QVERIFY(modifiedUrls.contains(url));
    QVERIFY(modifiedUrls.contains(destUrl));
}

void DocumentTest::testMetaInfoJpeg()
{
    QUrl url = urlForTestFile("orient6.jpg");
    Document::Ptr doc = DocumentFactory::instance()->load(url);

    // We cleared the cache, so the document should not be loaded
    Q_ASSERT(doc->loadingState() <= Document::KindDetermined);

    // Wait until we receive the metaInfoUpdated() signal
    QSignalSpy metaInfoUpdatedSpy(doc.data(), SIGNAL(metaInfoUpdated()));
    while (metaInfoUpdatedSpy.count() == 0) {
        QTest::qWait(100);
    }

    // Extract an exif key
    QString value = doc->metaInfo()->getValueForKey("Exif.Image.Make");
    QCOMPARE(value, QString::fromUtf8("Canon"));
}

void DocumentTest::testMetaInfoBmp()
{
    QUrl url = urlForTestOutputFile("metadata.bmp");
    const int width = 200;
    const int height = 100;
    QImage image(width, height, QImage::Format_ARGB32);
    image.fill(Qt::black);
    image.save(url.toLocalFile(), "BMP");

    Document::Ptr doc = DocumentFactory::instance()->load(url);
    QSignalSpy metaInfoUpdatedSpy(doc.data(), SIGNAL(metaInfoUpdated()));
    waitUntilMetaInfoLoaded(doc);

    Q_ASSERT(metaInfoUpdatedSpy.count() >= 1);

    QString value = doc->metaInfo()->getValueForKey("General.ImageSize");
    QString expectedValue = QString("%1x%2").arg(width).arg(height);
    QCOMPARE(value, expectedValue);
}

void DocumentTest::testForgetModifiedDocument()
{
    QSignalSpy spy(DocumentFactory::instance(), SIGNAL(modifiedDocumentListChanged()));
    DocumentFactory::instance()->forget(QUrl("file://does/not/exist.png"));
    QCOMPARE(spy.count(), 0);

    // Generate test image
    QImage image1(200, 96, QImage::Format_RGB32);
    {
        QPainter painter(&image1);
        QConicalGradient gradient(QPointF(100, 48), 100);
        gradient.setColorAt(0, Qt::white);
        gradient.setColorAt(1, Qt::blue);
        painter.fillRect(image1.rect(), gradient);
    }

    QUrl url = urlForTestOutputFile("testForgetModifiedDocument.png");
    QVERIFY(image1.save(url.toLocalFile(), "png"));

    // Load it as a Gwenview document
    Document::Ptr doc = DocumentFactory::instance()->load(url);
    doc->startLoadingFullImage();
    doc->waitUntilLoaded();

    // Modify it
    TransformImageOperation* op = new TransformImageOperation(ROT_90);
    op->applyToDocument(doc);
    QTest::qWait(100);

    QCOMPARE(spy.count(), 1);

    QList<QUrl> lst = DocumentFactory::instance()->modifiedDocumentList();
    QCOMPARE(lst.length(), 1);
    QCOMPARE(lst.first(), url);

    // Forget it
    DocumentFactory::instance()->forget(url);

    QCOMPARE(spy.count(), 2);
    lst = DocumentFactory::instance()->modifiedDocumentList();
    QVERIFY(lst.isEmpty());
}

void DocumentTest::testModifiedAndSavedSignals()
{
    TransformImageOperation* op;

    QUrl url = urlForTestFile("orient6.jpg");
    Document::Ptr doc = DocumentFactory::instance()->load(url);
    QSignalSpy modifiedSpy(doc.data(), SIGNAL(modified(QUrl)));
    QSignalSpy savedSpy(doc.data(), SIGNAL(saved(QUrl,QUrl)));
    doc->startLoadingFullImage();
    doc->waitUntilLoaded();

    QCOMPARE(modifiedSpy.count(), 0);
    QCOMPARE(savedSpy.count(), 0);

    op = new TransformImageOperation(ROT_90);
    op->applyToDocument(doc);
    QTest::qWait(100);
    QCOMPARE(modifiedSpy.count(), 1);

    op = new TransformImageOperation(ROT_90);
    op->applyToDocument(doc);
    QTest::qWait(100);
    QCOMPARE(modifiedSpy.count(), 2);

    doc->undoStack()->undo();
    QCOMPARE(modifiedSpy.count(), 3);

    doc->undoStack()->undo();
    QCOMPARE(savedSpy.count(), 1);
}

class TestJob : public DocumentJob
{
public:
    TestJob(QString* str, char ch)
        : mStr(str)
        , mCh(ch)
    {}

protected:
    virtual void doStart()
    {
        *mStr += mCh;
        emitResult();
    }

private:
    QString* mStr;
    char mCh;
};

void DocumentTest::testJobQueue()
{
    QUrl url = urlForTestFile("orient6.jpg");
    Document::Ptr doc = DocumentFactory::instance()->load(url);
    QSignalSpy spy(doc.data(), SIGNAL(busyChanged(QUrl,bool)));

    QString str;
    doc->enqueueJob(new TestJob(&str, 'a'));
    doc->enqueueJob(new TestJob(&str, 'b'));
    doc->enqueueJob(new TestJob(&str, 'c'));
    QVERIFY(doc->isBusy());
    QEventLoop loop;
    connect(doc.data(), SIGNAL(allTasksDone()),
            &loop, SLOT(quit()));
    loop.exec();
    QVERIFY(!doc->isBusy());
    QCOMPARE(spy.count(), 2);
    QVariantList row = spy.takeFirst();
    QCOMPARE(row.at(0).value<QUrl>(), url);
    QVERIFY(row.at(1).toBool());
    row = spy.takeFirst();
    QCOMPARE(row.at(0).value<QUrl>(), url);
    QVERIFY(!row.at(1).toBool());
    QCOMPARE(str, QString("abc"));
}

class TestCheckDocumentEditorJob : public DocumentJob
{
public:
    TestCheckDocumentEditorJob(int* hasEditor)
        : mHasEditor(hasEditor)
        {
        *mHasEditor = -1;
    }

protected:
    virtual void doStart()
    {
        document()->waitUntilLoaded();
        *mHasEditor = checkDocumentEditor() ? 1 : 0;
        emitResult();
    }

private:
    int* mHasEditor;
};

class TestUiDelegate : public KJobUiDelegate
{
public:
    TestUiDelegate(bool* showErrorMessageCalled)
        : mShowErrorMessageCalled(showErrorMessageCalled)
        {
        setAutoErrorHandlingEnabled(true);
        *mShowErrorMessageCalled = false;
    }

    virtual void showErrorMessage()
    {
        //qDebug();
        *mShowErrorMessageCalled = true;
    }

private:
    bool* mShowErrorMessageCalled;
};

/**
 * Test that an error is reported when a DocumentJob fails because there is no
 * document editor available
 */
void DocumentTest::testCheckDocumentEditor()
{
    int hasEditor;
    bool showErrorMessageCalled;
    QEventLoop loop;
    Document::Ptr doc;
    TestCheckDocumentEditorJob* job;

    doc = DocumentFactory::instance()->load(urlForTestFile("orient6.jpg"));

    job = new TestCheckDocumentEditorJob(&hasEditor);
    job->setUiDelegate(new TestUiDelegate(&showErrorMessageCalled));
    doc->enqueueJob(job);
    connect(doc.data(), SIGNAL(allTasksDone()), &loop, SLOT(quit()));
    loop.exec();
    QVERIFY(!showErrorMessageCalled);
    QCOMPARE(hasEditor, 1);

    doc = DocumentFactory::instance()->load(urlForTestFile("test.svg"));

    job = new TestCheckDocumentEditorJob(&hasEditor);
    job->setUiDelegate(new TestUiDelegate(&showErrorMessageCalled));
    doc->enqueueJob(job);
    connect(doc.data(), SIGNAL(allTasksDone()), &loop, SLOT(quit()));
    loop.exec();
    QVERIFY(showErrorMessageCalled);
    QCOMPARE(hasEditor, 0);
}

/**
 * An operation should only pushed to the document undo stack if it succeed
 */
void DocumentTest::testUndoStackPush()
{
    class SuccessOperation : public AbstractImageOperation
    {
    protected:
        virtual void redo()
    {
            QMetaObject::invokeMethod(this, "finish", Qt::QueuedConnection, Q_ARG(bool, true));
        }
    };

    class FailureOperation : public AbstractImageOperation
    {
    protected:
        virtual void redo()
    {
            QMetaObject::invokeMethod(this, "finish", Qt::QueuedConnection, Q_ARG(bool, false));
        }
    };

    AbstractImageOperation* op;
    Document::Ptr doc = DocumentFactory::instance()->load(urlForTestFile("orient6.jpg"));

    // A successful operation should be added to the undo stack
    op = new SuccessOperation;
    op->applyToDocument(doc);
    QTest::qWait(100);
    QVERIFY(!doc->undoStack()->isClean());

    // Reset
    doc->undoStack()->undo();
    QVERIFY(doc->undoStack()->isClean());

    // A failed operation should not be added to the undo stack
    op = new FailureOperation;
    op->applyToDocument(doc);
    QTest::qWait(100);
    QVERIFY(doc->undoStack()->isClean());
}
