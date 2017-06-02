// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2013 Aurélien Gâteau <agateau@kde.org>

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
#include "contextmanagertest.h"

// Local
#include <lib/contextmanager.h>
#include <lib/semanticinfo/sorteddirmodel.h>
#include <testutils.h>

// Qt
#include <QEventLoop>
#include <QItemSelectionModel>

// KDE
#include <QDebug>
#include <KDirLister>
#include <qtest.h>

using namespace Gwenview;
using namespace TestUtils;

QTEST_MAIN(ContextManagerTest)

void ContextManagerTest::testRemove()
{
    // When the current image is removed Gwenview must go to the next image if
    // there is any, otherwise to the previous image.

    SandBoxDir sandBox;
    sandBox.fill(QStringList() << "a" << "b" << "c");
    QUrl dirUrl = QUrl::fromLocalFile(sandBox.absolutePath());

    SortedDirModel dirModel;
    {
        QEventLoop loop;
        connect(dirModel.dirLister(), SIGNAL(completed()), &loop, SLOT(quit()));
        dirModel.dirLister()->openUrl(dirUrl);
        loop.exec();
    }

    QCOMPARE(dirModel.rowCount(), 3);

    ContextManager manager(&dirModel, 0);
    // Select second row
    manager.selectionModel()->setCurrentIndex(dirModel.index(1, 0), QItemSelectionModel::Select);

    // Remove "b", `manager` should select "c"
    sandBox.remove("b");
    dirModel.dirLister()->updateDirectory(dirUrl);
    while (dirModel.rowCount() == 3) {
        QTest::qWait(100);
    }

    QModelIndex currentIndex = manager.selectionModel()->currentIndex();
    QCOMPARE(currentIndex.row(), 1);
    QCOMPARE(currentIndex.data(Qt::DisplayRole).toString(), QString("c"));

    // Remove "c", `manager` should select "a"
    sandBox.remove("c");
    dirModel.dirLister()->updateDirectory(dirUrl);
    while (dirModel.rowCount() == 2) {
        QTest::qWait(100);
    }

    currentIndex = manager.selectionModel()->currentIndex();
    QCOMPARE(currentIndex.row(), 0);
    QCOMPARE(currentIndex.data(Qt::DisplayRole).toString(), QString("a"));
}

void ContextManagerTest::testInvalidDirUrl()
{
    class DirLister : public KDirLister
    {
    public:
        DirLister()
        : mOpenUrlCalled(false)
        {
            setAutoErrorHandlingEnabled(false, 0);
        }

        bool openUrl(const QUrl &url, OpenUrlFlags flags = NoFlags) Q_DECL_OVERRIDE
        {
            mOpenUrlCalled = true;
            return KDirLister::openUrl(url, flags);
        }

        bool mOpenUrlCalled;
    };

    SortedDirModel dirModel;
    DirLister* dirLister = new DirLister;
    dirModel.setDirLister(dirLister);
    ContextManager manager(&dirModel, 0);

    manager.setCurrentDirUrl(QUrl());
    QVERIFY(!dirLister->mOpenUrlCalled);
}

