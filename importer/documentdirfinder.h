// vim: set tabstop=4 shiftwidth=4 expandtab:
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
#ifndef DOCUMENTDIRFINDER_H
#define DOCUMENTDIRFINDER_H

// Qt
#include <QObject>

// KF
#include <KFileItem>

// Local

namespace Gwenview
{

struct DocumentDirFinderPrivate;

/**
 * This class is a worker which tries to find the document dir given a root
 * url. This is useful for digital camera cards, which often have a dir
 * hierarchy like this:
 * /DCIM
 *   /FOOBAR
 *     /PICT0001.JPG
 *     /PICT0002.JPG
 *     ...
 *     /PICTnnnn.JPG
 */
class DocumentDirFinder : public QObject
{
    Q_OBJECT
public:
    enum Status {
        NoDocumentFound,
        DocumentDirFound,
        MultipleDirsFound
    };

    DocumentDirFinder(const QUrl& rootUrl);
    ~DocumentDirFinder() override;

    void start();

Q_SIGNALS:
    void done(const QUrl&, DocumentDirFinder::Status);

private Q_SLOTS:
    void slotItemsAdded(const QUrl&, const KFileItemList&);
    void slotCompleted();

private:
    DocumentDirFinderPrivate* const d;
    void finish(const QUrl&, Status);
};

} // namespace

#endif /* DOCUMENTDIRFINDER_H */
