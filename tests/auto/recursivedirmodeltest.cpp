// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2012 Aurélien Gâteau <agateau@kde.org>

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
#include <recursivedirmodeltest.moc>

// Local
#include <testutils.h>
#include <lib/recursivedirmodel.h>

// Qt

// KDE
#include <KDirModel>
#include <qtest_kde.h>

using namespace Gwenview;

QTEST_KDEMAIN(RecursiveDirModelTest, GUI)

void RecursiveDirModelTest::testBasic_data()
{
    QTest::addColumn<QStringList>("initialFiles");
    QTest::addColumn<QStringList>("addedFiles");
    QTest::addColumn<QStringList>("removedFiles");
#define NEW_ROW(name, initialFiles, addedFiles, removedFiles) QTest::newRow(name) << (initialFiles) << (addedFiles) << (removedFiles)
    NEW_ROW("empty_dir",
        QStringList(),
        QStringList()
            << "new.jpg",
        QStringList()
            << "new.jpg"
        );
    NEW_ROW("images_only",
        QStringList()
            << "pict01.jpg"
            << "pict02.jpg"
            << "pict03.jpg",
        QStringList()
            << "pict04.jpg",
        QStringList()
            << "pict02.jpg"
        );
    NEW_ROW("images_in_two_dirs",
        QStringList()
            << "d1/pict101.jpg"
            << "d1/pict102.jpg"
            << "d2/pict201.jpg",
        QStringList()
            << "d1/pict103.jpg"
            << "d2/pict202.jpg",
        QStringList()
            << "d2/pict202.jpg"
        );
    NEW_ROW("images_in_two_dirs_w_same_names",
        QStringList()
            << "d1/a.jpg"
            << "d1/b.jpg"
            << "d2/a.jpg"
            << "d2/b.jpg",
        QStringList()
            << "d3/a.jpg"
            << "d3/b.jpg",
        QStringList()
            << "d1/a.jpg"
            << "d2/a.jpg"
            << "d3/a.jpg"
        );
#undef NEW_ROW
}

static QList<KUrl> listModelUrls(QAbstractItemModel* model)
{
    QList<KUrl> out;
    for (int row = 0; row < model->rowCount(QModelIndex()); ++row) {
        QModelIndex index = model->index(row, 0);
        KFileItem item = index.data(KDirModel::FileItemRole).value<KFileItem>();
        out << item.url();
    }
    qSort(out);
    return out;
}

static QList<KUrl> listExpectedUrls(const QDir& dir, const QStringList& files)
{
    QList<KUrl> lst;
    Q_FOREACH(const QString &name, files) {
        KUrl url(dir.absoluteFilePath(name));
        lst << url;
    }
    qSort(lst);
    return lst;
}

void logLst(const QList<KUrl>& lst)
{
    Q_FOREACH(const KUrl& url, lst) {
        kWarning() << url.fileName();
    }
}

void RecursiveDirModelTest::testBasic()
{
    QFETCH(QStringList, initialFiles);
    QFETCH(QStringList, addedFiles);
    QFETCH(QStringList, removedFiles);
    TestUtils::SandBoxDir sandBoxDir;

    RecursiveDirModel model;
    TestUtils::TimedEventLoop loop;
    connect(&model, SIGNAL(completed()), &loop, SLOT(quit()));

    // Test initial files
    sandBoxDir.fill(initialFiles);
    model.setUrl(sandBoxDir.absolutePath());
    loop.exec();

    QList<KUrl> out = listModelUrls(&model);
    QList<KUrl> expected = listExpectedUrls(sandBoxDir, initialFiles);
    QCOMPARE(out, expected);

    // Test adding new files
    sandBoxDir.fill(addedFiles);
    loop.exec();

    out = listModelUrls(&model);
    expected = listExpectedUrls(sandBoxDir, initialFiles + addedFiles);
    QCOMPARE(out, expected);

# if 0
    /* FIXME: This part of the test is not reliable :/ Sometimes some tests pass,
     * sometimes they don't. It feels like KDirLister::itemsDeleted() is not
     * always emitted.
     */

    // Test removing files
    Q_FOREACH(const QString &name, removedFiles) {
        bool ok = sandBoxDir.remove(name);
        Q_ASSERT(ok);
        expected.removeOne(KUrl(sandBoxDir.absoluteFilePath(name)));
    }
    QTime chrono;
    chrono.start();
    while (chrono.elapsed() < 2000) {
        waitForDeferredDeletes();
    }

    out = listModelUrls(&model);
    if (out != expected) {
        kWarning() << "out:";
        logLst(out);
        kWarning() << "expected:";
        logLst(expected);
    }
    QCOMPARE(out, expected);
#endif
}

void RecursiveDirModelTest::testSetNewUrl()
{
    TestUtils::SandBoxDir sandBoxDir;
    sandBoxDir.fill(
        QStringList()
        << "d1/a.jpg"
        << "d1/b.jpg"
        << "d1/c.jpg"
        << "d1/d.jpg"
        << "d2/e.jpg"
        << "d2/f.jpg"
        );

    RecursiveDirModel model;
    TestUtils::TimedEventLoop loop;
    connect(&model, SIGNAL(completed()), &loop, SLOT(quit()));

    model.setUrl(sandBoxDir.absoluteFilePath("d1"));
    loop.exec();
    QCOMPARE(model.rowCount(QModelIndex()), 4);

    model.setUrl(sandBoxDir.absoluteFilePath("d2"));
    loop.exec();
    QCOMPARE(model.rowCount(QModelIndex()), 2);
}
