/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <aurelien.gateau@free.fr>

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
#include <kdirsortfilterproxymodel.h>

// Local
#include <lib/gwenviewlib_export.h>


class KDirLister;
class KFileItem;
class KUrl;

namespace Gwenview {

class AbstractSemanticInfoBackEnd;
class SortedDirModelPrivate;
class TagSet;

#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
class SemanticInfo;
#endif

class SortedDirModel;
class GWENVIEWLIB_EXPORT AbstractSortedDirModelFilter : public QObject {
public:
	AbstractSortedDirModelFilter(SortedDirModel* model);
	~AbstractSortedDirModelFilter();
	SortedDirModel* model() const { return mModel; }

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
class GWENVIEWLIB_EXPORT SortedDirModel : public KDirSortFilterProxyModel {
	Q_OBJECT
public:
	SortedDirModel(QObject* parent);
	~SortedDirModel();
	KDirLister* dirLister();
	KFileItem itemForIndex(const QModelIndex& index) const;
	QModelIndex indexForItem(const KFileItem& item) const;
	QModelIndex indexForUrl(const KUrl& url) const;

	/**
	 * A list of file extensions we should skip
	 */
	void setBlackListedExtensions(const QStringList& list);

	virtual void setMimeExcludeFilter(const QStringList &mimeList);

	void setTagSetFilter(const TagSet& tagSet);

	void addFilter(AbstractSortedDirModelFilter*);

	void removeFilter(AbstractSortedDirModelFilter*);

	void reload();

	AbstractSemanticInfoBackEnd* semanticInfoBackEnd() const;

// FIXME: Shouldn't semanticInfoBackEnd() be in the #ifndef too?
#ifndef GWENVIEW_SEMANTICINFO_BACKEND_NONE
	// FIXME: Having to pass sourceIndex is quite surprising, but that's what
	// the filter receives
	SemanticInfo semanticInfoForIndex(const QModelIndex& sourceIndex) const;
#endif

	// Make invalidateFilter public
    void invalidateFilter() { QSortFilterProxyModel::invalidateFilter(); }

public Q_SLOTS:
	void setMinimumRating(int);

protected:
	bool filterAcceptsRow(int row, const QModelIndex& parent) const;

private:
	friend class SortedDirModelPrivate;
	SortedDirModelPrivate * const d;
};

} // namespace

#endif /* SORTEDDIRMODEL_H */
