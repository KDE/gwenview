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
#ifndef IMPORTERTEST_H
#define IMPORTERTEST_H

// stdc++
#include <memory>

// Qt
#include <QObject>

// KDE
#include <QTemporaryDir>
#include <QUrl>

class ImporterTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void testContentsAreIdentical();
    void testSuccessfulImport();
    void testAutoRenameFormat();
    void testReadOnlyDestination();
    void testFileNameFormater();
    void testFileNameFormater_data();
    void testSkippedUrlList();
    void testRenamedCount();

private:
    std::auto_ptr<QTemporaryDir> mTempDir;
    QUrl::List mDocumentList;
};

#endif /* IMPORTERTEST_H */
