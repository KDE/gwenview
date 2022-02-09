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
#include "placetreemodeltest.h"

// Qt
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTest>

// KF
#include <kio_version.h>

// Local
#include "../lib/placetreemodel.h"
#include "testutils.h"

//#define KEEP_TEMP_DIR

QTEST_MAIN(PlaceTreeModelTest)

using namespace Gwenview;

const char *BOOKMARKS_XML =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<!DOCTYPE xbel>"
    "<xbel xmlns:bookmark='http://www.freedesktop.org/standards/desktop-bookmarks' xmlns:mime='http://www.freedesktop.org/standards/shared-mime-info' "
    "xmlns:kdepriv='http://www.kde.org/kdepriv' dbusName='kfilePlaces' >"
    " <bookmark href='%1' >"
    "  <title>url1</title>"
    "  <info>"
    "   <metadata owner='http://freedesktop.org' >"
    "    <bookmark:icon name='user-home' />"
    "   </metadata>"
    "   <metadata owner='http://www.kde.org' >"
    "    <ID>1214343736/0</ID>"
    "    <isSystemItem>true</isSystemItem>"
    "   </metadata>"
    "  </info>"
    " </bookmark>"
    " <bookmark href='%2' >"
    "  <title>url2</title>"
    "  <info>"
    "   <metadata owner='http://freedesktop.org' >"
    "    <bookmark:icon name='user-home' />"
    "   </metadata>"
    "   <metadata owner='http://www.kde.org' >"
    "    <ID>1214343736/1</ID>"
    "    <isSystemItem>true</isSystemItem>"
    "   </metadata>"
    "  </info>"
    " </bookmark>"
    "</xbel>";

void PlaceTreeModelTest::initTestCase()
{
    Q_ASSERT(mTempDir.isValid());
    QDir dir(mTempDir.path());

    const bool dir1created = dir.mkdir("url1");
    Q_ASSERT(dir1created);
    Q_UNUSED(dir1created);
    mUrl1 = QUrl::fromLocalFile(dir.filePath("url1"));

    const bool dir2created = dir.mkdir("url2");
    Q_ASSERT(dir2created);
    Q_UNUSED(dir2created);
    mUrl2 = QUrl::fromLocalFile(dir.filePath("url2"));

    mUrl1Dirs << "aaa"
              << "zzz"
              << "bbb";
    for (const QString &dirName : qAsConst(mUrl1Dirs)) {
        dir.mkdir("url1/" + dirName);
    }

#ifdef KEEP_TEMP_DIR
    mTempDir.setAutoRemove(false);
    // qDebug() << "mTempDir:" << mTempDir.name();
#endif
}

void PlaceTreeModelTest::init()
{
    QStandardPaths::setTestModeEnabled(true);

    TestUtils::purgeUserConfiguration();

    const QString confDir = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    QDir().mkpath(confDir);
    QFile bookmark(confDir + "/user-places.xbel");
    const bool bookmarkOpened = bookmark.open(QIODevice::WriteOnly);
    Q_ASSERT(bookmarkOpened);
    Q_UNUSED(bookmarkOpened);

    QString xml = QString(BOOKMARKS_XML).arg(mUrl1.url(), mUrl2.url());
    bookmark.write(xml.toUtf8());

#ifdef KEEP_TEMP_DIR
    mTempDir.setAutoRemove(false);
    // qDebug() << "mTempDir:" << mTempDir.name();
#endif
}

void PlaceTreeModelTest::testListPlaces()
{
    PlaceTreeModel model(nullptr);

    QCOMPARE(model.rowCount(), 8);

    QModelIndex index;
    index = model.index(0, 0);
    QCOMPARE(model.urlForIndex(index), mUrl1);
    index = model.index(1, 0);
    QCOMPARE(model.urlForIndex(index), mUrl2);
}

void PlaceTreeModelTest::testListUrl1()
{
    PlaceTreeModel model(nullptr);

    QModelIndex index = model.index(0, 0);
    QCOMPARE(model.urlForIndex(index), mUrl1);

    // We should not have fetched content yet
    QCOMPARE(model.rowCount(index), 0);
    QVERIFY(model.canFetchMore(index));

    while (model.canFetchMore(index)) {
        model.fetchMore(index);
    }
    QTest::qWait(1000);
    QCOMPARE(model.rowCount(index), mUrl1Dirs.length());

    QStringList dirs = mUrl1Dirs;
    dirs.sort();

    for (int row = 0; row < dirs.count(); ++row) {
        QModelIndex subIndex = model.index(row, 0, index);
        QVERIFY(subIndex.isValid());

        QString dirName = model.data(subIndex).toString();
        QCOMPARE(dirName, dirs.value(row));
    }
}
