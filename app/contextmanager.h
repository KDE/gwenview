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
#ifndef CONTEXTMANAGER_H
#define CONTEXTMANAGER_H

// Qt
#include <QObject>

// KDE
#include <KUrl>
#include <KFileItem>

class QItemSelectionModel;
class QModelIndex;

namespace Gwenview
{

class SortedDirModel;

class AbstractContextManagerItem;

struct ContextManagerPrivate;

/**
 * Manage the update of the contextual parts of the applications,
 * like the sidebar or the context menu.
 */
class ContextManager : public QObject
{
    Q_OBJECT
public:
    ContextManager(SortedDirModel*, QObject* parent);

    ~ContextManager();

    void addItem(AbstractContextManagerItem* item);

    KUrl currentUrl() const;

    void setCurrentDirUrl(const KUrl&);

    KUrl currentDirUrl() const;

    void setCurrentUrl(const KUrl& currentUrl);

    KFileItemList selectedFileItemList() const;

    SortedDirModel* dirModel() const;

    QItemSelectionModel* selectionModel() const;

Q_SIGNALS:
    void selectionChanged();
    void selectionDataChanged();
    void currentDirUrlChanged();

private Q_SLOTS:
    void slotDirModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
    void slotSelectionChanged();
    void slotCurrentChanged(const QModelIndex&);
    void emitQueuedSignals();
    void slotRowsAboutToBeRemoved(const QModelIndex& /*parent*/, int start, int end);

private:
    ContextManagerPrivate* const d;
};

} // namespace

#endif /* CONTEXTMANAGER_H */
