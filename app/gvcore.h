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
#ifndef GVCORE_H
#define GVCORE_H

// Qt
#include <QObject>

// KDE

// Local

class KJob;
class QUrl;

class QAbstractItemModel;
class QPalette;

namespace Gwenview
{

class AbstractSemanticInfoBackEnd;
class MainWindow;
class SortedDirModel;

struct GvCorePrivate;
class GvCore : public QObject
{
    Q_OBJECT
public:
    GvCore(MainWindow* mainWindow, SortedDirModel*);
    ~GvCore();

    enum PaletteType {
        NormalPalette = 0,
        NormalViewPalette,
        FullScreenPalette,
        FullScreenViewPalette
    };

    QAbstractItemModel* recentFoldersModel() const;
    QAbstractItemModel* recentUrlsModel() const;
    SortedDirModel* sortedDirModel() const;
    AbstractSemanticInfoBackEnd* semanticInfoBackEnd() const;

    void addUrlToRecentFolders(QUrl);
    void addUrlToRecentUrls(const QUrl &url);

    QPalette palette(PaletteType type) const;

public Q_SLOTS:
    void saveAll();
    void save(const QUrl&);
    void saveAs(const QUrl&);
    void rotateLeft(const QUrl&);
    void rotateRight(const QUrl&);
    void setRating(const QUrl&, int);

private Q_SLOTS:
    void slotConfigChanged();
    void slotSaveResult(KJob*);

private:
    GvCorePrivate* const d;
};

} // namespace

#endif /* GVCORE_H */
