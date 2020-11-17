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
#include "historymodeltest.h"

// Qt
#include <QDir>

// KDE
#include <QDebug>
#include <KFilePlacesModel>
#include <QTemporaryDir>
#include <QTest>

// Local
#include "../lib/historymodel.h"

QTEST_MAIN(HistoryModelTest)

using namespace Gwenview;

void testModel(const HistoryModel& model, const QUrl &u1, const QUrl& u2)
{
    QModelIndex index;
    QUrl url;
    QCOMPARE(model.rowCount(), 2);

    index = model.index(0, 0);
    QVERIFY(index.isValid());
    url = model.data(index, KFilePlacesModel::UrlRole).toUrl();
    QCOMPARE(url, u1);

    index = model.index(1, 0);
    QVERIFY(index.isValid());
    url = model.data(index, KFilePlacesModel::UrlRole).toUrl();
    QCOMPARE(url, u2);

}

void HistoryModelTest::testAddUrl()
{
    QUrl u1 = QUrl::fromLocalFile("/home");
    QDateTime d1 = QDateTime::fromString("2008-02-03T12:34:56", Qt::ISODate);
    QUrl u2 = QUrl::fromLocalFile("/root");
    QDateTime d2 = QDateTime::fromString("2009-01-29T23:01:47", Qt::ISODate);
    QTemporaryDir dir;
    {
        HistoryModel model(nullptr, dir.path());
        model.addUrl(u1, d1);
        model.addUrl(u2, d2);
        testModel(model, u2, u1);
    }

    HistoryModel model(nullptr, dir.path());
    testModel(model, u2, u1);

    // Make u1 the most recent
    QDateTime d3 = QDateTime::fromString("2009-03-24T22:42:15", Qt::ISODate);
    model.addUrl(u1, d3);
    testModel(model, u1, u2);
}

void HistoryModelTest::testGarbageCollect()
{
    QUrl u1 = QUrl::fromLocalFile("/home");
    QDateTime d1 = QDateTime::fromString("2008-02-03T12:34:56", Qt::ISODate);
    QUrl u2 = QUrl::fromLocalFile("/root");
    QDateTime d2 = QDateTime::fromString("2009-01-29T23:01:47", Qt::ISODate);
    QUrl u3 = QUrl::fromLocalFile("/usr");
    QDateTime d3 = QDateTime::fromString("2009-03-24T22:42:15", Qt::ISODate);

    QTemporaryDir dir;
    {
        HistoryModel model(nullptr, dir.path(), 2);
        model.addUrl(u1, d1);
        model.addUrl(u2, d2);
        testModel(model, u2, u1);
        model.addUrl(u3, d3);
    }

    // Create a model with a larger history so that if garbage collecting fails
    // to remove the collected url, the size of the model won't pass
    // testModel()
    HistoryModel model(nullptr, dir.path(), 10);
    testModel(model, u3, u2);
}

void HistoryModelTest::testRemoveRows()
{
    QUrl u1 = QUrl::fromLocalFile("/home");
    QDateTime d1 = QDateTime::fromString("2008-02-03T12:34:56", Qt::ISODate);
    QUrl u2 = QUrl::fromLocalFile("/root");
    QDateTime d2 = QDateTime::fromString("2009-01-29T23:01:47", Qt::ISODate);

    QTemporaryDir dir;
    HistoryModel model(nullptr, dir.path(), 2);
    model.addUrl(u1, d1);
    model.addUrl(u2, d2);
    model.removeRows(0, 1);
    QCOMPARE(model.rowCount(), 1);
    QDir qDir(dir.path());
    QCOMPARE(qDir.entryList(QDir::Files | QDir::NoDotAndDotDot).count(), 1);
}
