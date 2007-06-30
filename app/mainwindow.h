/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau

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

#include <memory>

class QModelIndex;

class KFileItemList;
class KUrl;

namespace Gwenview {

class MainWindow : public KParts::MainWindow {
Q_OBJECT
public:
	MainWindow();
	void openUrl(const KUrl&);

protected:
	virtual void slotSetStatusBarText(const QString&);
	   
private Q_SLOTS:
	void setActiveViewModeAction(QAction* action);
	void openDirUrl(const KUrl&);
	void openDirUrlFromString(const QString& str);
	void openDirUrlFromIndex(const QModelIndex&);

	void openDocumentUrl(const KUrl&);
	void goUp();
	void toggleSideBar();
	void updateSideBar();

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
	void slotDirListerNewItems(const KFileItemList& list);

	void slotSelectionChanged();

	void goToPrevious();
	void goToNext();
	void updatePreviousNextActions();

	void toggleFullScreen();

	void save();

	void saveAs();

	void rotateLeft();
	void rotateRight();
	void mirror();
	void flip();

private:
	class Private;
	std::auto_ptr<Private> d;

	void openSelectedDocument();
};

} // namespace

#endif /* MAINWINDOW_H */
