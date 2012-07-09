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
#include "placetreemodeltest.moc"

// Qt
#include <QDir>
#include <QFile>

// KDE
#include <KDebug>
#include <KStandardDirs>
#include <qtest_kde.h>

// Local
#include "../lib/placetreemodel.h"
#include "testutils.h"

//#define KEEP_TEMP_DIR

QTEST_KDEMAIN(PlaceTreeModelTest, GUI)

using namespace Gwenview;

const char* BOOKMARKS_XML =
    "<?xml version='1.0' encoding='UTF-8'?>"
    "<!DOCTYPE xbel>"
    "<xbel xmlns:bookmark='http://www.freedesktop.org/standards/desktop-bookmarks' xmlns:mime='http://www.freedesktop.org/standards/shared-mime-info' xmlns:kdepriv='http://www.kde.org/kdepriv' dbusName='kfilePlaces' >"
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
    Q_ASSERT(mTempDir.exists());
    QDir dir(mTempDir.name());

    Q_ASSERT(dir.mkdir("url1"));
    mUrl1 = KUrl::fromPath(dir.filePath("url1"));

    Q_ASSERT(dir.mkdir("url2"));
    mUrl2 = KUrl::fromPath(dir.filePath("url2"));

    mUrl1Dirs << "aaa" << "zzz" << "bbb";
    Q_FOREACH(const QString & dirName, mUrl1Dirs) {
        dir.mkdir("url1/" + dirName);
    }

#ifdef KEEP_TEMP_DIR
    mTempDir.setAutoRemove(false);
    kDebug() << "mTempDir:" << mTempDir.name();
#endif
}

void PlaceTreeModelTest::init()
{
    TestUtils::purgeUserConfiguration();

    QFile bookmark(KStandardDirs::locateLocal("data", "kfileplaces/bookmarks.xml"));
    Q_ASSERT(bookmark.open(QIODevice::WriteOnly));

    QString xml = QString(BOOKMARKS_XML)
                  .arg(mUrl1.toLocalFile())
                  .arg(mUrl2.toLocalFile())
                  ;
    bookmark.write(xml.toUtf8());

#ifdef KEEP_TEMP_DIR
    mTempDir.setAutoRemove(false);
    kDebug() << "mTempDir:" << mTempDir.name();
#endif
}

void PlaceTreeModelTest::testListPlaces()
{
    PlaceTreeModel model(0);
    QCOMPARE(model.rowCount(), 2);

    QModelIndex index;
    index = model.index(0, 0);
    QCOMPARE(model.urlForIndex(index), mUrl1);
    index = model.index(1, 0);
    QCOMPARE(model.urlForIndex(index), mUrl2);
}

void PlaceTreeModelTest::testListUrl1()
{
    PlaceTreeModel model(0);

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
