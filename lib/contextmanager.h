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

#include <lib/gwenviewlib_export.h>

// Qt
#include <QObject>
#include <QUrl>

// KF
#include <KFileItem>

class QItemSelectionModel;
class QModelIndex;

namespace Gwenview
{

class SortedDirModel;

struct ContextManagerPrivate;

/**
 * Manages the state of the application.
 * TODO: Most of GvCore should be merged in this class
 */
class GWENVIEWLIB_EXPORT ContextManager : public QObject
{
    Q_OBJECT
public:
    ContextManager(SortedDirModel*, QObject* parent);

    ~ContextManager() override;

    void loadConfig();
    void saveConfig() const;

    QUrl currentUrl() const;

    void setCurrentDirUrl(const QUrl&);

    QUrl currentDirUrl() const;

    void setCurrentUrl(const QUrl &currentUrl);

    KFileItemList selectedFileItemList() const;

    SortedDirModel* dirModel() const;

    QItemSelectionModel* selectionModel() const;

    bool currentUrlIsRasterImage() const;

    QUrl urlToSelect() const;

    void setUrlToSelect(const QUrl&);

    QUrl targetDirUrl() const;

    void setTargetDirUrl(const QUrl&);

Q_SIGNALS:
    void currentDirUrlChanged(const QUrl&);
    void currentUrlChanged(const QUrl&);
    void selectionChanged();
    void selectionDataChanged();

public Q_SLOTS:
    void slotSelectionChanged();

private Q_SLOTS:
    void slotDirModelDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
    void slotCurrentChanged(const QModelIndex&);
    void emitQueuedSignals();
    void slotRowsAboutToBeRemoved(const QModelIndex& /*parent*/, int start, int end);
    void slotRowsInserted();
    void selectUrlToSelect();
    void slotDirListerRedirection(const QUrl&);
    void slotDirListerCompleted();

private:
    ContextManagerPrivate* const d;
};

} // namespace

#endif /* CONTEXTMANAGER_H */
