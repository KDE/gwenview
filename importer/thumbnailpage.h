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
#ifndef THUMBNAILPAGE_H
#define THUMBNAILPAGE_H

// Qt
#include <QModelIndex>
#include <QUrl>
#include <QWidget>

// KF

// Local
#include "documentdirfinder.h"

namespace Gwenview
{
struct ThumbnailPagePrivate;
class ThumbnailPage : public QWidget
{
    Q_OBJECT
public:
    ThumbnailPage();
    ~ThumbnailPage() override;

    /**
     * Returns the list of urls to import
     * Only valid after importRequested() has been emitted
     */
    QList<QUrl> urlList() const;

    QUrl destinationUrl() const;
    void setDestinationUrl(const QUrl &);

    void setSourceUrl(const QUrl &, const QString &icon, const QString &label);

Q_SIGNALS:
    void importRequested();
    void rejected();

private Q_SLOTS:
    void slotImportSelected();
    void slotImportAll();
    void updateImportButtons();
    void openUrl(const QUrl &);
    void slotDocumentDirFinderDone(const QUrl &url, DocumentDirFinder::Status status);
    void showConfigDialog();
    void openUrlFromIndex(const QModelIndex &index);
    void setupSrcUrlTreeView();
    void toggleSrcUrlTreeView();
    void slotSrcUrlModelExpand(const QModelIndex &index);

private:
    bool mDiscoverAvailable = false;
    friend struct ThumbnailPagePrivate;
    ThumbnailPagePrivate *const d;
    void importList(const QModelIndexList &);
    void installProtocolSupport() const;
};

} // namespace

#endif /* THUMBNAILPAGE_H */
