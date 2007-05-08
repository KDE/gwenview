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
