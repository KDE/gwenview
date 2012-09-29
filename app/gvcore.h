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
class KUrl;

class QAbstractItemModel;
class QPalette;

namespace Gwenview
{

class AbstractSemanticInfoBackEnd;
class MainWindow;
class SortedDirModel;

class GvCorePrivate;
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
    AbstractSemanticInfoBackEnd* semanticInfoBackEnd() const;

    void addUrlToRecentFolders(const KUrl&);
    void addUrlToRecentUrls(const KUrl& url);

    /**
     * Checks if the document referenced by url is editable, shows a sorry
     * dialog if it's not.
     * @return true if editable, false if not
     */
    static bool ensureDocumentIsEditable(const KUrl& url);

    QPalette palette(PaletteType type) const;

public Q_SLOTS:
    void saveAll();
    void save(const KUrl&);
    void saveAs(const KUrl&);
    void rotateLeft(const KUrl&);
    void rotateRight(const KUrl&);
    void setRating(const KUrl&, int);

private Q_SLOTS:
    void slotConfigChanged();
    void slotSaveResult(KJob*);

private:
    GvCorePrivate* const d;
};

} // namespace

#endif /* GVCORE_H */
