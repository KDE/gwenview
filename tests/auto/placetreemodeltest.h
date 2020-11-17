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
#ifndef PLACETREEMODELTEST_H
#define PLACETREEMODELTEST_H

// Qt
#include <QObject>
#include <QStringList>
#include <QTemporaryDir>
#include <QUrl>

// KF

class PlaceTreeModelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void initTestCase();
    void testListPlaces();
    void testListUrl1();

private:
    QUrl mUrl1, mUrl2;
    QStringList mUrl1Dirs;
    QTemporaryDir mTempDir;
};

#endif /* PLACETREEMODELTEST_H */
