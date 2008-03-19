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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Qt
#include <QAction>

// KDE
#include <kparts/mainwindow.h>

class QModelIndex;

class KFileItemList;
class KUrl;

namespace KParts { class ReadOnlyPart; }

namespace Gwenview {

class MainWindow : public KParts::MainWindow {
Q_OBJECT
public:
	MainWindow();
	~MainWindow();
	/**
	 * Defines the url to display when the window is shown for the first time.
	 */
	void setInitialUrl(const KUrl&);

public Q_SLOTS:
	virtual void setCaption(const QString&);

	virtual void setCaption(const QString&, bool modified);

protected:
	virtual void slotSetStatusBarText(const QString&);
	virtual bool queryClose();
	virtual QSize sizeHint() const;
	virtual void showEvent(QShowEvent*);

private Q_SLOTS:
	void setActiveViewModeAction(QAction* action);
	void openDirUrl(const KUrl&);
	void slotThumbnailViewIndexActivated(const QModelIndex&);

	void openDocumentUrl(const KUrl&);
	void goToUrl(const KUrl&);
	void goUp();
	void toggleSideBar();
	void updateContextManager();
	void updateModifiedFlag();

	/**
	 * Init all the file list stuff. This should only be necessary when
	 * Gwenview is started with an image as a parameter (in this case we load
	 * the image before looking at the content of the image folder)
	 */
	void slotPartCompleted();
	
	/**
	 * If an image is loaded but there is no item selected for it in the file
	 * view, this function will select the corresponding item if it comes up in
	 * list.
	 */
	void slotDirModelNewItems();

	/**
	 * If no image is selected, select the first one available.
	 */
	void slotDirListerCompleted();

	void slotSelectionChanged();

	void goToPrevious();
	void goToNext();
	void updatePreviousNextActions();

	void exitFullScreen();
	void toggleFullScreen();
	void toggleSlideShow();
	void updateSlideShowAction();

	void saveCurrent();
	void saveCurrentAs();
	void openFile();
	void reload();
	void save(const KUrl&);
	void saveAs(const KUrl&);

	void showDocumentInFullScreen(const KUrl&);
	void rotateCurrentLeft();
	void rotateLeft(const KUrl&);
	void rotateCurrentRight();
	void rotateRight(const KUrl&);
	void mirror();
	void flip();
	void resizeImage();
	void crop();

	void generateThumbnailForUrl(const KUrl&);
	void showConfigDialog();
	void loadConfig();
	void print();

	void handleResizeRequest(const QSize&);

	void hideTemporarySideBar();

	void preloadNextUrl();

	void configureToolbars();
	void slotNewToolbarConfig();

	void toggleMenuBar();

	void editLocation();

	void updateFullScreenInformation();
	void applyNameFilter();
	void toggleFilterBarVisibility(bool);

private:
	class Private;
	MainWindow::Private* const d;

	void openSelectedDocument();
	void saveConfig();
};

} // namespace

#endif /* MAINWINDOW_H */
