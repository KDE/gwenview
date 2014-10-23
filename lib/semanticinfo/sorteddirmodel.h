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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#ifndef SORTEDDIRMODEL_H
#define SORTEDDIRMODEL_H

#include <config-gwenview.h>

// Qt
#include <QPointer>

// KDE
#include <KDirSortFilterProxyModel>

// Local
#include <lib/gwenviewlib_export.h>
#include <lib/mimetypeutils.h>

class KDirLister;
class KFileItem;
class QUrl;

namespace Gwenview
{

class AbstractSemanticInfoBackEnd;
struct SortedDirModelPrivate;

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
struct SemanticInfo;
#endif

class SortedDirModel;
class GWENVIEWLIB_EXPORT AbstractSortedDirModelFilter : public QObject
{
public:
    AbstractSortedDirModelFilter(SortedDirModel* model);
    ~AbstractSortedDirModelFilter();
    SortedDirModel* model() const
    {
        return mModel;
    }

    virtual bool needsSemanticInfo() const = 0;
    /**
     * Returns true if index should be accepted.
     * Warning: index is a source index of SortedDirModel
     */
    virtual bool acceptsIndex(const QModelIndex& index) const = 0;

private:
    QPointer<SortedDirModel> mModel;
};

/**
 * This model makes it possible to show all images in a folder.
 * It can filter images based on name and metadata.
 */
class GWENVIEWLIB_EXPORT SortedDirModel : public KDirSortFilterProxyModel
{
    Q_OBJECT
public:
    SortedDirModel(QObject* parent = 0);
    ~SortedDirModel();
    KDirLister* dirLister() const;
    /**
     * Redefines the dir lister, useful for debugging
     */
    void setDirLister(KDirLister*);
    KFileItem itemForIndex(const QModelIndex& index) const;
    QUrl urlForIndex(const QModelIndex& index) const;
    KFileItem itemForSourceIndex(const QModelIndex& sourceIndex) const;
    QModelIndex indexForItem(const KFileItem& item) const;
    QModelIndex indexForUrl(const QUrl &url) const;

    void setKindFilter(MimeTypeUtils::Kinds);
    MimeTypeUtils::Kinds kindFilter() const;

    void adjustKindFilter(MimeTypeUtils::Kinds, bool set);

    /**
     * A list of file extensions we should skip
     */
    void setBlackListedExtensions(const QStringList& list);

    void addFilter(AbstractSortedDirModelFilter*);

    void removeFilter(AbstractSortedDirModelFilter*);

    void reload();

    AbstractSemanticInfoBackEnd* semanticInfoBackEnd() const;

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
    SemanticInfo semanticInfoForSourceIndex(const QModelIndex& sourceIndex) const;
#endif

    bool hasDocuments() const;

public Q_SLOTS:
    void applyFilters();

protected:
    bool filterAcceptsRow(int row, const QModelIndex& parent) const Q_DECL_OVERRIDE;
    bool lessThan(const QModelIndex& left, const QModelIndex& right) const Q_DECL_OVERRIDE;

private Q_SLOTS:
    void doApplyFilters();

private:
    friend struct SortedDirModelPrivate;
    SortedDirModelPrivate * const d;
};

} // namespace

#endif /* SORTEDDIRMODEL_H */
