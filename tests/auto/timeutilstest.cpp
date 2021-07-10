/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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

#include "timeutilstest.h"

// libc
#include <utime.h>

// Qt
#include <QTemporaryFile>
#include <QTest>

// KF
#include <KFileItem>

// Local
#include "../lib/timeutils.h"

#include "testutils.h"

QTEST_MAIN(TimeUtilsTest)

using namespace Gwenview;

static void touchFile(const QString &path)
{
    utime(QFile::encodeName(path).data(), nullptr);
}

#define NEW_ROW(fileName, dateTime) QTest::newRow(fileName) << fileName << dateTime
void TimeUtilsTest::testBasic_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<QDateTime>("expectedDateTime");

    NEW_ROW("date/exif-datetimeoriginal.jpg", QDateTime::fromString("2003-03-10T17:45:21", Qt::ISODate));
    NEW_ROW("date/exif-datetime-only.jpg", QDateTime::fromString("2003-03-25T02:02:21", Qt::ISODate));

    QUrl url = urlForTestFile("test.png");
    KFileItem item(url);
    NEW_ROW("test.png", item.time(KFileItem::ModificationTime));
}

void TimeUtilsTest::testBasic()
{
    QFETCH(QString, fileName);
    QFETCH(QDateTime, expectedDateTime);
    QDateTime dateTime;
    QUrl url = urlForTestFile(fileName);
    KFileItem item(url);

    dateTime = TimeUtils::dateTimeForFileItem(item);
    QCOMPARE(dateTime, expectedDateTime);

    dateTime = TimeUtils::dateTimeForFileItem(item, TimeUtils::SkipCache);
    QCOMPARE(dateTime, expectedDateTime);
}

void TimeUtilsTest::testCache()
{
    QTemporaryFile tempFile;
    QVERIFY(tempFile.open());
    QUrl url = QUrl::fromLocalFile(tempFile.fileName());
    KFileItem item1(url);
    QDateTime dateTime1 = TimeUtils::dateTimeForFileItem(item1);
    QCOMPARE(dateTime1, item1.time(KFileItem::ModificationTime));

    QTest::qWait(1200);
    touchFile(url.toLocalFile());

    KFileItem item2(url);
    QDateTime dateTime2 = TimeUtils::dateTimeForFileItem(item2);

    QVERIFY(dateTime1 != dateTime2);

    QCOMPARE(dateTime2, item2.time(KFileItem::ModificationTime));
}
