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
#include "urlutilstest.h"

// Qt
#include <QTest>
#include <QDir>

// KF

// Local
#include "../lib/urlutils.h"

QTEST_MAIN(UrlUtilsTest)

using namespace Gwenview;

void UrlUtilsTest::testFixUserEnteredUrl()
{
    QFETCH(QUrl, in);
    QFETCH(QUrl, expected);
    QUrl out = UrlUtils::fixUserEnteredUrl(in);
    QCOMPARE(out.url(), expected.url());
}

#define NEW_ROW(in, expected) QTest::newRow(QString(in).toLocal8Bit().data()) << QUrl(in) << QUrl(expected)
void UrlUtilsTest::testFixUserEnteredUrl_data()
{
    QTest::addColumn<QUrl>("in");
    QTest::addColumn<QUrl>("expected");

    QString pwd = QDir::currentPath();

    NEW_ROW("http://example.com", "http://example.com");
    NEW_ROW("file://" + pwd + "/example.zip", "zip:" + pwd + "/example.zip");
    NEW_ROW("file://" + pwd + "/example.cbz", "zip:" + pwd + "/example.cbz");
    NEW_ROW("file://" + pwd + "/example.jpg", "file://" + pwd + "/example.jpg");
    // Check it does not get turned into gzip://...
    NEW_ROW("file://" + pwd + "/example.svgz", "file://" + pwd + "/example.svgz");
}
