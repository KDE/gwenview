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
    QTest::addColumn<QStringList>("files");
#define NEW_ROW(name, files) QTest::newRow(name) << (files)
    NEW_ROW("empty_dir", QStringList());

    NEW_ROW("images_only", QStringList()
        << "pict01.jpg" << "pict02.jpg" << "pict03.jpg"
        );
    NEW_ROW("images_in_two_dirs", QStringList()
        << "d1/pict101.jpg" << "d1/pict102.jpg"
        << "d2/pict201.jpg"
        );
    NEW_ROW("images_in_two_dirs_w_same_names", QStringList()
        << "d1/a.jpg" << "d1/b.jpg"
        << "d2/a.jpg" << "d2/b.jpg"
        );
#undef NEW_ROW
}

void RecursiveDirModelTest::testBasic()
{
    QFETCH(QStringList, files);
    TestUtils::SandBoxDir sandBoxDir;
    sandBoxDir.fill(files);

    RecursiveDirModel model;
    QEventLoop loop;
    connect(&model, SIGNAL(completed()), &loop, SLOT(quit()));
    model.setUrl(sandBoxDir.absolutePath());
    loop.exec();

    QList<KUrl> out;
    for (int row = 0; row < model.rowCount(QModelIndex()); ++row) {
        QModelIndex index = model.index(row, 0);
        KFileItem item = index.data(KDirModel::FileItemRole).value<KFileItem>();
        out << item.url();
    }
    qSort(out);

    QList<KUrl> expected;
    Q_FOREACH(const QString &name, files) {
        KUrl url(sandBoxDir.absoluteFilePath(name));
        expected << url;
    }
    qSort(expected);

    QCOMPARE(out, expected);
}
