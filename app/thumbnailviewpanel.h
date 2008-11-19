// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>

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

// Local

class QModelIndex;
class QSlider;

class KActionCollection;
class KUrlNavigator;

namespace Gwenview {

class SortedDirModel;
class ThumbnailView;

class ThumbnailViewPanelPrivate;
/**
 * This class contains all the necessary widgets displayed in browse mode:
 * the thumbnail view, the url navigator, the bottom bar.
 */
class ThumbnailViewPanel : public QWidget {
	Q_OBJECT
public:
	ThumbnailViewPanel(QWidget* parent, SortedDirModel*, KActionCollection*);
	~ThumbnailViewPanel();

	void initActionDependentWidgets();

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

private:
	ThumbnailViewPanelPrivate* const d;
};


} // namespace

#endif /* THUMBNAILVIEWPANEL_H */
