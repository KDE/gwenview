// vim: set tabstop=4 shiftwidth=4 expandtab:
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
#include "sorteddirmodeltest.h"

// Local
#include <lib/semanticinfo/sorteddirmodel.h>

// Qt
#include <QTemporaryDir>
#include <QTest>

// KF
#include <KDirLister>

using namespace Gwenview;

QTEST_MAIN(SortedDirModelTest)

void SortedDirModelTest::initTestCase()
{
    mSandBoxDir.mkdir("empty_dir");
    mSandBoxDir.mkdir("dirs_only");
    mSandBoxDir.mkdir("dirs_only/dir1");
    mSandBoxDir.mkdir("dirs_only/dir2");
    mSandBoxDir.mkdir("dirs_and_docs");
    mSandBoxDir.mkdir("dirs_and_docs/dir");
    createEmptyFile(mSandBoxDir.absoluteFilePath("dirs_and_docs/file.png"));
    mSandBoxDir.mkdir("docs_only");
    createEmptyFile(mSandBoxDir.absoluteFilePath("docs_only/file.png"));
}

void SortedDirModelTest::testHasDocuments_data()
{
    QTest::addColumn<QString>("dir");
    QTest::addColumn<bool>("hasDocuments");
#define NEW_ROW(dir, hasDocuments) QTest::newRow(QString(dir).toLocal8Bit().data()) << mSandBoxDir.absoluteFilePath(dir) << hasDocuments
    NEW_ROW("empty_dir", false);
    NEW_ROW("dirs_only", false);
    NEW_ROW("dirs_and_docs", true);
    NEW_ROW("docs_only", true);
#undef NEW_ROW
}

void SortedDirModelTest::testHasDocuments()
{
    QFETCH(QString, dir);
    QFETCH(bool, hasDocuments);
    QUrl url = QUrl::fromLocalFile(dir);

    SortedDirModel model;
    QEventLoop loop;
    connect(model.dirLister(), SIGNAL(completed()), &loop, SLOT(quit()));
    model.dirLister()->openUrl(url);
    loop.exec();
    QCOMPARE(model.hasDocuments(), hasDocuments);
}
