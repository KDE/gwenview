// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>
Copyright 2014 Tomas Mecir <mecirt@gmail.com>

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
#ifndef RECENTFILESMODEL_H
#define RECENTFILESMODEL_H

// Qt
#include <QStandardItemModel>

// KF

// Local
#include <lib/gwenviewlib_export.h>

class QUrl;

namespace Gwenview
{

struct RecentFilesModelPrivate;
/**
 * A model which maintains a list of urls in the dir specified by the
 * storageDir parameter of its ctor.
 * Each url is stored in a separate KConfig file to avoid concurrency issues.
 */
class GWENVIEWLIB_EXPORT RecentFilesModel : public QStandardItemModel
{
    Q_OBJECT
public:
    explicit RecentFilesModel(QObject* parent);
    ~RecentFilesModel() override;

    void addUrl(const QUrl&);

    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

private:
    RecentFilesModelPrivate* const d;
};

} // namespace

#endif /* RECENTFILESMODEL_H */
