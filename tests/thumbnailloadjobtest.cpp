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
#include <kdebug.h>
#include <kio/deletejob.h>

// Local
#include "../lib/thumbnailloadjob.h"
#include "testutils.h"


using namespace Gwenview;


QTEST_KDEMAIN(ThumbnailLoadJobTest, GUI)


SandBox::SandBox()
: mPath(QDir::currentPath() + "/sandbox")
{}


void SandBox::create() {
	QDir dir(mPath);
	if (dir.exists()) {
		KUrl sandBoxUrl("file://" + mPath);
		KIO::Job* job = KIO::del(sandBoxUrl);
		QVERIFY2(job->exec(), "Couldn't delete sandbox");
	}
	dir.mkpath(".");
	createTestImage("red.png", 300, 200, Qt::red);
	createTestImage("blue.png", 200, 300, Qt::blue);
	createTestImage("small.png", 50, 50, Qt::green);
}


void SandBox::createTestImage(const QString& name, int width, int height, const QColor& color) {
	QImage image(width, height, QImage::Format_RGB32);
	QPainter painter(&image);
	painter.fillRect(image.rect(), color);
	painter.end();
	image.save(mPath + '/' + name, "png");
	mSizeHash.insert(name, QSize(width, height));
}


void ThumbnailLoadJobTest::init() {
	ThumbnailLoadJob::setThumbnailBaseDir(mSandBox.mPath + "/thumbnails/");
	mSandBox.create();
}


void ThumbnailLoadJobTest::testLoadLocal() {
	QDir dir(mSandBox.mPath);
	qRegisterMetaType<KFileItem>("KFileItem");

	KFileItemList list;
	Q_FOREACH(const QFileInfo& info, dir.entryInfoList(QDir::Files)) {
		KUrl url("file://" + info.absoluteFilePath());
		KFileItem item(KFileItem::Unknown, KFileItem::Unknown, url);
		list << item;
	}
	QPointer<ThumbnailLoadJob> job = new ThumbnailLoadJob(list, ThumbnailGroup::Normal);
	QSignalSpy spy(job, SIGNAL(thumbnailLoaded(const KFileItem&, const QPixmap&, const QSize&)));
	// FIXME: job->exec() causes a double free(), so wait for the job to be
	// deleted instead
	//job->exec();
	job->start();
	while (job) {
		QTest::qWait(100);
	}

	QDir thumbnailDir = ThumbnailLoadJob::thumbnailBaseDir(ThumbnailGroup::Normal);
	// There should be one file less because small.png is a png and is too
	// small to have a thumbnail
	QStringList entryList = thumbnailDir.entryList(QStringList("*.png"));
	QCOMPARE(entryList.count(), mSandBox.mSizeHash.size() - 1);

	// Check what was in the thumbnailLoaded() signals
	QCOMPARE(spy.count(), mSandBox.mSizeHash.size());
	QSignalSpy::ConstIterator it = spy.begin(),
		end = spy.end();
	for (;it != end; ++it) {
		QVariantList args = *it;
		KFileItem item = qvariant_cast<KFileItem>(args.at(0));
		QSize size = args.at(2).toSize();
		QSize expectedSize = mSandBox.mSizeHash.value(item.url().fileName());
		QCOMPARE(size, expectedSize);
	}
}


void ThumbnailLoadJobTest::testLoadRemote() {
	QString testTarGzPath = pathForTestFile("test.tar.gz");
	KUrl url;
	url.setProtocol("tar");
	url.setPath(testTarGzPath + "/test.png");

	KFileItemList list;
	KFileItem item(KFileItem::Unknown, KFileItem::Unknown, url);
	list << item;

	QPointer<ThumbnailLoadJob> job = new ThumbnailLoadJob(list, ThumbnailGroup::Normal);
	// FIXME: job->exec() causes a double free(), so wait for the job to be
	// deleted instead
	//job->exec();
	job->start();
	while (job) {
		QTest::qWait(100);
	}

	QDir thumbnailDir = ThumbnailLoadJob::thumbnailBaseDir(ThumbnailGroup::Normal);
	QStringList entryList = thumbnailDir.entryList(QStringList("*.png"));
	QCOMPARE(entryList.count(), 1);
}
