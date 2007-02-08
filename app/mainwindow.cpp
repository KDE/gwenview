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
#include "mainwindow.moc"

// Qt
#include <QDir>
#include <QFrame>
#include <QLabel>
#include <QListView>
#include <QTimer>
#include <QSplitter>

// KDE
#include <kactioncollection.h>
#include <kaction.h>
#include <kdirlister.h>
#include <kdirmodel.h>
#include <klocale.h>
#include <kurl.h>

// Local
#include <lib/thumbnailview.h>

namespace Gwenview {


struct MainWindow::Private {
	MainWindow* mWindow;
	QLabel* mDocumentView;
	ThumbnailView* mThumbnailView;
	QFrame* mSideBar;

	QActionGroup* mViewModeActionGroup;
	QAction* mThumbsOnlyAction;
	QAction* mThumbsAndImageAction;
	QAction* mImageOnlyAction;

	KDirModel* mDirModel;

	void setupWidgets() {
		QSplitter* centralSplitter = new QSplitter(Qt::Horizontal, mWindow);
		mWindow->setCentralWidget(centralSplitter);

		QSplitter* viewSplitter = new QSplitter(Qt::Vertical, centralSplitter);
		mSideBar = new QFrame(centralSplitter);
		mSideBar->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);

		mDocumentView = new QLabel(viewSplitter);
		mDocumentView->setText("Bla");

		mThumbnailView = new ThumbnailView(viewSplitter);
		mThumbnailView->setModel(mDirModel);
	}

	void setupActions() {
		KActionCollection* actionCollection = mWindow->actionCollection();
		mThumbsOnlyAction = actionCollection->addAction("thumbs_only");
		mThumbsOnlyAction->setText(i18n("Thumbnails"));
		mThumbsOnlyAction->setCheckable(true);

		mThumbsAndImageAction = actionCollection->addAction("thumbs_and_image");
		mThumbsAndImageAction->setText(i18n("Thumbnails and Image"));
		mThumbsAndImageAction->setCheckable(true);

		mImageOnlyAction = actionCollection->addAction("image_only");
		mImageOnlyAction->setText(i18n("Image"));
		mImageOnlyAction->setCheckable(true);

		mViewModeActionGroup = new QActionGroup(mWindow);
		mViewModeActionGroup->addAction(mThumbsOnlyAction);
		mViewModeActionGroup->addAction(mThumbsAndImageAction);
		mViewModeActionGroup->addAction(mImageOnlyAction);

		connect(mViewModeActionGroup, SIGNAL(triggered(QAction*)),
			mWindow, SLOT(setActiveViewModeAction(QAction*)) );
	}

};


MainWindow::MainWindow()
: KMainWindow(0),
d(new MainWindow::Private)
{
	d->mWindow = this;
	d->mDirModel = new KDirModel(this);
	d->setupWidgets();
	d->setupActions();
	QTimer::singleShot(0, this, SLOT(initDirModel()) );
	setupGUI();
}


void MainWindow::setActiveViewModeAction(QAction* action) {
	bool showDocument, showThumbnail;
	if (action == d->mThumbsOnlyAction) {
		showDocument = false;
		showThumbnail = true;
	} else if (action == d->mThumbsAndImageAction) {
		showDocument = true;
		showThumbnail = true;
	} else { // image only
		showDocument = true;
		showThumbnail = false;
	}

	d->mDocumentView->setVisible(showDocument);
	d->mThumbnailView->setVisible(showThumbnail);
}


void MainWindow::initDirModel() {
	KUrl url;
	url.setPath(QDir::currentPath());
	d->mDirModel->dirLister()->openUrl(url);
}


} // namespace
