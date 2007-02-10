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
#include <QGridLayout>
#include <QLabel>
#include <QListView>
#include <QTimer>
#include <QToolButton>
#include <QSplitter>

// KDE
#include <kactioncollection.h>
#include <kaction.h>
#include <kdirlister.h>
#include <kdirmodel.h>
#include <kfileitem.h>
#include <klocale.h>
#include <kurl.h>
#include <kurlrequester.h>

// Local
#include <lib/thumbnailview.h>

namespace Gwenview {


struct MainWindow::Private {
	MainWindow* mWindow;
	QLabel* mDocumentView;
	QToolButton* mGoUpButton;
	KUrlRequester* mUrlRequester;
	ThumbnailView* mThumbnailView;
	QFrame* mSideBar;

	QActionGroup* mViewModeActionGroup;
	QAction* mThumbsOnlyAction;
	QAction* mThumbsAndImageAction;
	QAction* mImageOnlyAction;
	QAction* mGoUpAction;

	KDirModel* mDirModel;

	void setupWidgets() {
		QSplitter* centralSplitter = new QSplitter(Qt::Horizontal, mWindow);
		mWindow->setCentralWidget(centralSplitter);

		QSplitter* viewSplitter = new QSplitter(Qt::Vertical, centralSplitter);
		mSideBar = new QFrame(centralSplitter);
		mSideBar->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);

		mDocumentView = new QLabel(viewSplitter);
		mDocumentView->setText("Bla");

		setupThumbnailView(viewSplitter);
	}

	void setupThumbnailView(QWidget* parent) {
		QWidget* container = new QWidget(parent);

		// mThumbnailView
		mThumbnailView = new ThumbnailView(container);
		mThumbnailView->setModel(mDirModel);
		connect(mThumbnailView, SIGNAL(activated(const QModelIndex&)),
			mWindow, SLOT(openUrlFromIndex(const QModelIndex&)) );
		connect(mThumbnailView, SIGNAL(doubleClicked(const QModelIndex&)),
			mWindow, SLOT(openUrlFromIndex(const QModelIndex&)) );

		// mGoUpButton
		mGoUpButton = new QToolButton(container);
		mGoUpButton->setAutoRaise(true);

		// mUrlRequester
		mUrlRequester = new KUrlRequester(container);
		mUrlRequester->setMode(KFile::Directory);
		connect(mUrlRequester, SIGNAL(urlSelected(const KUrl&)),
			mWindow, SLOT(openUrl(const KUrl&)) );
		connect(mUrlRequester, SIGNAL(returnPressed(const QString&)),
			mWindow, SLOT(openUrlFromString(const QString&)) );

		// Layout
		QGridLayout* layout = new QGridLayout(container);
		layout->setSpacing(0);
		layout->setMargin(0);
		layout->addWidget(mThumbnailView, 0, 0, 1, 2);
		layout->addWidget(mGoUpButton, 1, 0);
		layout->addWidget(mUrlRequester, 1, 1);
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

		mGoUpAction = actionCollection->addAction("go_up");
		mGoUpAction->setText(i18n("Go Up"));
		mGoUpAction->setIcon(KIcon("up"));
		connect(mGoUpAction, SIGNAL(triggered()),
			mWindow, SLOT(goUp()) );

		mGoUpButton->setDefaultAction(mGoUpAction);
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
	openUrl(url);
}


void MainWindow::openUrlFromIndex(const QModelIndex& index) {
	if (!index.isValid()) {
		return;
	}

	KFileItem* item = d->mDirModel->itemForIndex(index);
	if (item->isDir()) {
		openUrl(item->url());
	}
}


void MainWindow::goUp() {
	KUrl url = d->mDirModel->dirLister()->url();
	url = url.upUrl();
	openUrl(url);
}


void MainWindow::openUrl(const KUrl& url) {
	d->mDirModel->dirLister()->openUrl(url);
	d->mUrlRequester->setUrl(url);
	d->mGoUpAction->setEnabled(url.path() != "/");
}


void MainWindow::openUrlFromString(const QString& str) {
	KUrl url(str);
	openUrl(url);
}

} // namespace
