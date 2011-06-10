// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2011 Aurélien Gâteau <agateau@kde.org>

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
// Self
#include <sorteddirmodeltest.moc>

// Local
#include <testutils.h>
#include <lib/semanticinfo/sorteddirmodel.h>

// Qt

// KDE
#include <qtest_kde.h>
#include <kdirlister.h>

using namespace Gwenview;

QTEST_KDEMAIN(SortedDirModelTest, GUI)

void SortedDirModelTest::testHasDocuments_data() {
	QTest::addColumn<QString>("sandBoxDir");
	QTest::addColumn<bool>("hasDocuments");
	#define NEW_ROW(dir, hasDocuments) \
		QTest::newRow(QString(dir).toLocal8Bit().data()) << pathForTestFile(dir) << hasDocuments
	NEW_ROW("empty_dir", false);
	NEW_ROW("dirs_only", false);
	NEW_ROW("dirs_and_docs", true);
	NEW_ROW("docs_only", true);
	#undef NEW_ROW
}

void SortedDirModelTest::testHasDocuments() {
	QFETCH(QString, sandBoxDir);
	QFETCH(bool, hasDocuments);
	SortedDirModel model;
	QEventLoop loop;
	connect(model.dirLister(), SIGNAL(completed()), &loop, SLOT(quit()));
	model.dirLister()->openUrl(KUrl::fromPath(sandBoxDir));
	loop.exec();
	QCOMPARE(model.hasDocuments(), hasDocuments);
}
