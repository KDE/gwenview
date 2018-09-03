// vim: set tabstop=4 shiftwidth=4 expandtab:
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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
#ifndef SEMANTICINFODIRMODEL_H
#define SEMANTICINFODIRMODEL_H

// Qt

// KDE
#include <KDirModel>

// Local

class QUrl;

namespace Gwenview
{

class AbstractSemanticInfoBackEnd;
struct SemanticInfo;
struct SemanticInfoDirModelPrivate;
/**
 * Extends KDirModel by providing read/write access to image metadata such as
 * rating, tags and descriptions.
 */
class SemanticInfoDirModel : public KDirModel
{
    Q_OBJECT
public:
    enum {
        RatingRole = 0x21a43a51,
        DescriptionRole = 0x26FB33FA,
        TagsRole = 0x0462F0A8
    };
    SemanticInfoDirModel(QObject* parent);
    ~SemanticInfoDirModel() override;

    void clearSemanticInfoCache();

    bool semanticInfoAvailableForIndex(const QModelIndex&) const;

    void retrieveSemanticInfoForIndex(const QModelIndex&);

    SemanticInfo semanticInfoForIndex(const QModelIndex&) const;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    bool setData(const QModelIndex& index, const QVariant& data, int role = Qt::EditRole) override;

    AbstractSemanticInfoBackEnd* semanticInfoBackEnd() const;

Q_SIGNALS:
    void semanticInfoRetrieved(const QUrl&, const SemanticInfo&);

private:
    SemanticInfoDirModelPrivate* const d;

private Q_SLOTS:
    void slotSemanticInfoRetrieved(const QUrl &url, const SemanticInfo&);

    void slotRowsAboutToBeRemoved(const QModelIndex&, int, int);
    void slotModelAboutToBeReset();
};

} // namespace

#endif /* SEMANTICINFODIRMODEL_H */
