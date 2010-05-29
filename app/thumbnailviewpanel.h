// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
#ifndef THUMBNAILVIEWPANEL_H
#define THUMBNAILVIEWPANEL_H

// Qt
#include <QWidget>

// KDE
#include <kurl.h>

// Local

class QDropEvent;
class QModelIndex;

class KActionCollection;
class KUrlNavigator;

namespace Gwenview {

class SortedDirModel;
class ThumbnailView;

struct ThumbnailViewPanelPrivate;
/**
 * This class contains all the necessary widgets displayed in browse mode:
 * the thumbnail view, the url navigator, the bottom bar.
 */
class ThumbnailViewPanel : public QWidget {
	Q_OBJECT
public:
	ThumbnailViewPanel(QWidget* parent, SortedDirModel*, KActionCollection*);
	~ThumbnailViewPanel();

	void applyPalette(const QPalette&);

	ThumbnailView* thumbnailView() const;
	KUrlNavigator* urlNavigator() const;

	void loadConfig();
	void saveConfig() const;

private Q_SLOTS:
	void editLocation();
	void addFolderToPlaces();

	void slotDirModelRowsInserted(const QModelIndex& parent, int start, int end);
	void slotDirModelRowsAboutToBeRemoved(const QModelIndex& parent, int start, int end);
	void slotDirModelReset();
	void updateSortOrder();
	void updateThumbnailDetails();
	void slotUrlsDropped(const KUrl& destUrl, QDropEvent*);
	void showMenuForDroppedUrls(const KUrl::List&, const KUrl& destUrl);

private:
	ThumbnailViewPanelPrivate* const d;
};


} // namespace

#endif /* THUMBNAILVIEWPANEL_H */
