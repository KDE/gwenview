/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
#include "importertest.moc"

// stdlib
#include <sys/stat.h>

// Qt
#include <QSignalSpy>

// KDE
#include <kdebug.h>
#include <qtest_kde.h>

// Local
#include "../importer/fileutils.h"
#include "../importer/importer.h"
#include "testutils.h"

QTEST_KDEMAIN(ImporterTest, GUI)

using namespace Gwenview;

class VoidRenamer : public AbstractRenamer {
public:
	QString operator()(const KUrl& src) {
		return src.fileName();
	}
};

void ImporterTest::init() {
	mDocumentList = KUrl::List()
		<< urlForTestFile("import/pict0001.jpg")
		<< urlForTestFile("import/pict0002.jpg")
		<< urlForTestFile("import/pict0003.jpg")
		;

	mTempDir.reset(new KTempDir());
}

void ImporterTest::testContentsAreIdentical() {
	QVERIFY(!FileUtils::contentsAreIdentical(mDocumentList[0], mDocumentList[1]));
	QVERIFY(FileUtils::contentsAreIdentical(mDocumentList[0], mDocumentList[0]));

	KUrl url1 = mDocumentList[0];
	KUrl url2 = urlForTestFile("foo");

	// Test on a copy of a file
	QFile::copy(url1.toLocalFile(), url2.toLocalFile());

	QVERIFY(FileUtils::contentsAreIdentical(url1, url2));

	// Alter one byte of the copy and test again
	QFile file(url2.toLocalFile());
	QVERIFY(file.open(QIODevice::ReadOnly));
	QByteArray data = file.readAll();
	file.close();
	data[data.size() / 2] = 255 - data[data.size() / 2];

	file.open(QIODevice::WriteOnly);
	file.write(data);
	file.close();

	QVERIFY(!FileUtils::contentsAreIdentical(url1, url2));
}

void ImporterTest::testSuccessfulImport() {
	KUrl destUrl = KUrl::fromPath(mTempDir->name() + "/foo");

	Importer importer(0);
	importer.setRenamer(new VoidRenamer);
	QSignalSpy maximumChangedSpy(&importer, SIGNAL(maximumChanged(int)));
	QSignalSpy errorSpy(&importer, SIGNAL(error(const QString&)));

	KUrl::List list = mDocumentList;

	QEventLoop loop;
	connect(&importer, SIGNAL(importFinished()), &loop, SLOT(quit()));
	importer.start(list, destUrl);
	loop.exec();

	QCOMPARE(maximumChangedSpy.count(), 1);
	QCOMPARE(maximumChangedSpy.takeFirst().at(0).toInt(), list.count() * 100);
	QCOMPARE(errorSpy.count(), 0);

	QCOMPARE(importer.importedUrlList().count(), list.count());
	QCOMPARE(importer.importedUrlList(), list);

	Q_FOREACH(const KUrl& src, list) {
		KUrl dst = destUrl;
		dst.addPath(src.fileName());
		QVERIFY(FileUtils::contentsAreIdentical(src, dst));
	}
}

void ImporterTest::testReadOnlyDestination() {
	KUrl destUrl = KUrl::fromPath(mTempDir->name() + "/foo");
	chmod(QFile::encodeName(mTempDir->name()), 0555);

	Importer importer(0);
	QSignalSpy errorSpy(&importer, SIGNAL(error(const QString&)));
	importer.start(mDocumentList, destUrl);

	QCOMPARE(errorSpy.count(), 1);
	QVERIFY(importer.importedUrlList().isEmpty());
}
