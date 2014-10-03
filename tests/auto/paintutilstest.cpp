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
#include <qtest.h>

#include "../lib/paintutils.h"

#include "paintutilstest.h"

QTEST_MAIN(PaintUtilsTest)

void PaintUtilsTest::testScaledRect_data()
{
    QTest::addColumn<QRectF>("input");
    QTest::addColumn<QRect>("expected");

    QTest::newRow("overflow right") << QRectF(1.0, 1.0, 2.7, 3.2) << QRect(1, 1, 3, 4);
    QTest::newRow("overflow left")  << QRectF(0.5, 1.0, 2.0, 3.2) << QRect(0, 1, 3, 4);
    QTest::newRow("overflow both")  << QRectF(0.5, 1.0, 2.6, 3.2) << QRect(0, 1, 4, 4);
}

void PaintUtilsTest::testScaledRect()
{
    QFETCH(QRectF, input);
    QFETCH(QRect,  expected);
    QCOMPARE(Gwenview::PaintUtils::containingRect(input), expected);
}
