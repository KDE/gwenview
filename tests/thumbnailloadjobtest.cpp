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


using namespace Gwenview;


QTEST_KDEMAIN(ThumbnailLoadJobTest, GUI)


QString sandBoxPath() {
	return QDir::currentPath() + "/sandbox";
}


void createTestImage(const QString& name, int width, int height, QColor color) {
	QImage image(width, height, QImage::Format_RGB32);
	QPainter painter(&image);
	painter.fillRect(image.rect(), color);
	painter.end();
	image.save(sandBoxPath() + '/' + name, "png");
}


void ThumbnailLoadJobTest::initTestCase() {
	ThumbnailLoadJob::setThumbnailBaseDir(sandBoxPath() + "/thumbnails/");
}


static void resetSandBox() {
	QDir dir(sandBoxPath());
	if (dir.exists()) {
		KUrl sandBoxUrl("file://" + sandBoxPath());
		KIO::Job* job = KIO::del(sandBoxUrl);
		QVERIFY2(job->exec(), "Couldn't delete sandbox");
	}
	dir.mkpath(".");
	createTestImage("red.png", 300, 200, Qt::red);
	createTestImage("blue.png", 200, 300, Qt::blue);
	createTestImage("small.png", 50, 50, Qt::green);
}


void ThumbnailLoadJobTest::testLoadLocal() {
	resetSandBox();
	QDir dir(sandBoxPath());

	KFileItemList list;
	Q_FOREACH(QFileInfo info, dir.entryInfoList()) {
		KUrl url("file://" + info.absoluteFilePath());
		KFileItem item(KFileItem::Unknown, KFileItem::Unknown, url);
		list << item;
	}
	QPointer<ThumbnailLoadJob> job = new ThumbnailLoadJob(list, 128);
	// FIXME: job->exec() causes a double free(), so wait for the job to be
	// deleted instead
	//job->exec();
	job->start();
	while (job) {
		QTest::qWait(100);
	}

	QDir thumbnailDir = ThumbnailLoadJob::thumbnailBaseDir(128);
	// There should be 2 files, because small.png is too small to have a
	// thumbnail
	QStringList entryList = thumbnailDir.entryList(QStringList("*.png"));
	QCOMPARE(entryList.count(), 2);
}


void ThumbnailLoadJobTest::testLoadRemote() {
	resetSandBox();
	QString urlString = QString("tar://%1/test.tar.gz/test.png").arg(QDir::currentPath());
	KUrl url(urlString);
	KFileItemList list;
	KFileItem item(KFileItem::Unknown, KFileItem::Unknown, url);
	list << item;

	QPointer<ThumbnailLoadJob> job = new ThumbnailLoadJob(list, 128);
	// FIXME: job->exec() causes a double free(), so wait for the job to be
	// deleted instead
	//job->exec();
	job->start();
	while (job) {
		QTest::qWait(100);
	}

	QDir thumbnailDir = ThumbnailLoadJob::thumbnailBaseDir(128);
	QStringList entryList = thumbnailDir.entryList(QStringList("*.png"));
	QCOMPARE(entryList.count(), 1);
}
