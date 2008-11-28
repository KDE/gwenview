/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>

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

#include "timeutilstest.moc"

// libc
#include <utime.h>

// KDE
#include <kfileitem.h>
#include <qtest_kde.h>

// Local
#include "../lib/timeutils.h"

#include "testutils.h"

QTEST_KDEMAIN( TimeUtilsTest, GUI )

using namespace Gwenview;

static void touchFile(const QString& path) {
	utime(QFile::encodeName(path).data(), 0);
}

void TimeUtilsTest::testPng() {
	KUrl url = urlForTestFile("test.png");
	KFileItem item(KFileItem::Unknown, KFileItem::Unknown, url);
	KDateTime dateTime = TimeUtils::dateTimeForFileItem(item);
	QCOMPARE(dateTime, item.time(KFileItem::ModificationTime));
}


void TimeUtilsTest::testJpeg() {
	KUrl url = urlForTestFile("orient6.jpg");
	KFileItem item(KFileItem::Unknown, KFileItem::Unknown, url);
	KDateTime dateTime = TimeUtils::dateTimeForFileItem(item);

	const KDateTime orient6DateTime = KDateTime::fromString("2003-03-25T02:02:21");
	QCOMPARE(dateTime, orient6DateTime);
}


void TimeUtilsTest::testCache() {
	KUrl url = urlForTestFile("test.png");
	KFileItem item1(KFileItem::Unknown, KFileItem::Unknown, url);
	KDateTime dateTime1 = TimeUtils::dateTimeForFileItem(item1);
	QCOMPARE(dateTime1, item1.time(KFileItem::ModificationTime));

	QTest::qWait(1200);
	touchFile(url.path());

	KFileItem item2(KFileItem::Unknown, KFileItem::Unknown, url);
	KDateTime dateTime2 = TimeUtils::dateTimeForFileItem(item2);

	QVERIFY(dateTime1 != dateTime2);

	QCOMPARE(dateTime2, item2.time(KFileItem::ModificationTime));
}
