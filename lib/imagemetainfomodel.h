// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
#ifndef IMAGEMETAINFO_H
#define IMAGEMETAINFO_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QAbstractItemModel>

// KDE

// Local

class QUrl;

namespace Exiv2
{
class Image;
}

namespace Gwenview
{

struct ImageMetaInfoModelPrivate;
class GWENVIEWLIB_EXPORT ImageMetaInfoModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    ImageMetaInfoModel();
    ~ImageMetaInfoModel();

    void setUrl(const QUrl&);
    void setImageSize(const QSize&);
    void setExiv2Image(const Exiv2::Image*);

    QString keyForIndex(const QModelIndex&) const;
    void getInfoForKey(const QString& key, QString* label, QString* value) const;
    QString getValueForKey(const QString& key) const;

    virtual QModelIndex index(int row, int col, const QModelIndex& parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex&) const;
    virtual int rowCount(const QModelIndex& = QModelIndex()) const;
    virtual int columnCount(const QModelIndex& = QModelIndex()) const;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const;

private:
    ImageMetaInfoModelPrivate* const d;
    friend struct ImageMetaInfoModelPrivate;
};

} // namespace

#endif /* IMAGEMETAINFO_H */
