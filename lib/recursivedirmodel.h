// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2012 Aurélien Gâteau <agateau@kde.org>

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
#ifndef RECURSIVEDIRMODEL_H
#define RECURSIVEDIRMODEL_H

// Local
#include <lib/gwenviewlib_export.h>

// KF
#include <KFileItem>

// Qt
#include <QAbstractListModel>

class QUrl;

namespace Gwenview
{
struct RecursiveDirModelPrivate;
/**
 * Recursively list content of a dir
 */
class GWENVIEWLIB_EXPORT RecursiveDirModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit RecursiveDirModel(QObject *parent = nullptr);
    ~RecursiveDirModel() override;

    QUrl url() const;
    void setUrl(const QUrl &);

    int rowCount(const QModelIndex &) const override;
    QVariant data(const QModelIndex &, int role = Qt::DisplayRole) const override;

Q_SIGNALS:
    void completed();

private Q_SLOTS:
    void slotItemsAdded(const QUrl &dirUrl, const KFileItemList &);
    void slotItemsDeleted(const KFileItemList &);
    void slotDirCleared(const QUrl &);
    void slotCleared();

private:
    RecursiveDirModelPrivate *const d;
};

} // namespace

#endif /* RECURSIVEDIRMODEL_H */
