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
#include <kde_file.h>
#include <kdirlister.h>
#include <kfileitem.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kparts/componentfactory.h>
#include <kurl.h>
#include <kurlrequester.h>
#include <kxmlguifactory.h>

// Local
#include "contextmanager.h"
#include "documentview.h"
#include "selectioncontextmanageritem.h"
#include "sidebar.h"
#include <lib/archiveutils.h>
#include <lib/imageviewpart.h>
#include <lib/mimetypeutils.h>
#include <lib/sorteddirmodel.h>
#include <lib/thumbnailview.h>

namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif


static bool urlIsDirectory(QWidget* parent, const KUrl& url) {
	if( url.fileName(KUrl::ObeyTrailingSlash).isEmpty()) {
		return true; // file:/somewhere/<nothing here>
	}

	// Do direct stat instead of using KIO if the file is local (faster)
	if( url.isLocalFile() && !KIO::probably_slow_mounted( url.path())) {
		KDE_struct_stat buff;
		if ( KDE_stat( QFile::encodeName(url.path()), &buff ) == 0 )  {
			return S_ISDIR( buff.st_mode );
		}
	}
	KIO::UDSEntry entry;
	if( KIO::NetAccess::stat( url, entry, parent)) {
		return entry.isDir();
	}
	return false;
}

struct MainWindow::Private {
	MainWindow* mWindow;
	DocumentView* mDocumentView;
	QVBoxLayout* mDocumentLayout;
	QToolButton* mGoUpButton;
	KUrlRequester* mUrlRequester;
	ThumbnailView* mThumbnailView;
	QWidget* mThumbnailViewPanel;
	SideBar* mSideBar;
	KParts::ReadOnlyPart* mPart;
	QString mPartLibrary;

	QActionGroup* mViewModeActionGroup;
	QAction* mThumbsOnlyAction;
	QAction* mThumbsAndImageAction;
	QAction* mImageOnlyAction;
	QAction* mGoUpAction;
	QAction* mGoToPreviousAction;
	QAction* mGoToNextAction;
	QAction* mToggleSideBarAction;

	SortedDirModel* mDirModel;
	ContextManager* mContextManager;

	void setupWidgets() {
		QSplitter* centralSplitter = new QSplitter(Qt::Horizontal, mWindow);
		mWindow->setCentralWidget(centralSplitter);

		QSplitter* viewSplitter = new QSplitter(Qt::Vertical, centralSplitter);
		mSideBar = new SideBar(centralSplitter);
		mDocumentView = new DocumentView(viewSplitter);

		mDocumentLayout = new QVBoxLayout(mDocumentView);
		mDocumentLayout->setMargin(0);

		setupThumbnailView(viewSplitter);
	}

	void setupThumbnailView(QWidget* parent) {
		mThumbnailViewPanel = new QWidget(parent);

		// mThumbnailView
		mThumbnailView = new ThumbnailView(mThumbnailViewPanel);
		mThumbnailView->setModel(mDirModel);
		mThumbnailView->setSelectionMode(QAbstractItemView::ExtendedSelection);
		connect(mThumbnailView, SIGNAL(activated(const QModelIndex&)),
			mWindow, SLOT(openDirUrlFromIndex(const QModelIndex&)) );
		connect(mThumbnailView, SIGNAL(doubleClicked(const QModelIndex&)),
			mWindow, SLOT(openDirUrlFromIndex(const QModelIndex&)) );
		connect(mThumbnailView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
			mWindow, SLOT(slotSelectionChanged()) );

		// mGoUpButton
		mGoUpButton = new QToolButton(mThumbnailViewPanel);
		mGoUpButton->setAutoRaise(true);

		// mUrlRequester
		mUrlRequester = new KUrlRequester(mThumbnailViewPanel);
		mUrlRequester->setMode(KFile::Directory);
		connect(mUrlRequester, SIGNAL(urlSelected(const KUrl&)),
			mWindow, SLOT(openDirUrl(const KUrl&)) );
		connect(mUrlRequester, SIGNAL(returnPressed(const QString&)),
			mWindow, SLOT(openDirUrlFromString(const QString&)) );

		// Layout
		QGridLayout* layout = new QGridLayout(mThumbnailViewPanel);
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

		mGoToPreviousAction = actionCollection->addAction("go_to_previous");
		mGoToPreviousAction->setText(i18n("Previous"));
		mGoToPreviousAction->setIcon(KIcon("go-previous"));
		connect(mGoToPreviousAction, SIGNAL(triggered()),
			mWindow, SLOT(goToPrevious()) );

		mGoToNextAction = actionCollection->addAction("go_to_next");
		mGoToNextAction->setText(i18n("Next"));
		mGoToNextAction->setIcon(KIcon("go-next"));
		connect(mGoToNextAction, SIGNAL(triggered()),
			mWindow, SLOT(goToNext()) );

		mGoUpAction = actionCollection->addAction("go_up");
		mGoUpAction->setText(i18n("Go Up"));
		mGoUpAction->setIcon(KIcon("up"));
		connect(mGoUpAction, SIGNAL(triggered()),
			mWindow, SLOT(goUp()) );

		mGoUpButton->setDefaultAction(mGoUpAction);

		mToggleSideBarAction = actionCollection->addAction("toggle_sidebar");
		connect(mToggleSideBarAction, SIGNAL(triggered()),
			mWindow, SLOT(toggleSideBar()) );
	}


	void setupContextManager() {
		mContextManager = new ContextManager(mWindow);
		mContextManager->setSideBar(mSideBar);
		AbstractContextManagerItem* item = new SelectionContextManagerItem();
		mContextManager->addItem(item);
	}


	void createPartForUrl(const KUrl& url) {

		QString mimeType=KMimeType::findByUrl(url)->name();

		// Get a list of possible parts
		const KService::List offers = KMimeTypeTrader::self()->query( mimeType, QLatin1String("KParts/ReadOnlyPart"));
		if (offers.isEmpty()) {
			kWarning() << "Couldn't find a KPart for " << mimeType << endl;
			if (mPart) {
				mWindow->createGUI(0);
				delete mPart;
				mPartLibrary = QString();
				mPart=0;
			}
			return;
		}

		// Check if we are already using it
		KService::Ptr service = offers.first();
		QString library=service->library();
		Q_ASSERT(!library.isNull());
		if (library == mPartLibrary) {
			LOG("Reusing current part");
			return;
		}

		// Load new part
		LOG("Loading part from library: " << library);
		KParts::ReadOnlyPart* part = KParts::ComponentFactory::createPartInstanceFromService<KParts::ReadOnlyPart>(
			service,
			mDocumentView /*parentWidget*/,
			mDocumentView /*parent*/);
		if (!part) {
			kWarning() << "Failed to instantiate KPart from library " << library << endl;
			return;
		}

		ImageViewPart* ivPart = dynamic_cast<ImageViewPart*>(part);
		mContextManager->setImageView(ivPart);
		mDocumentLayout->addWidget(part->widget());
		mWindow->createGUI(part);

		// Make sure our file list is filled when the part is done.
		connect(part, SIGNAL(completed()), mWindow, SLOT(startDirLister()) );

		// Delete the old part, don't do it before mWindow->createGUI(),
		// otherwise some UI elements from the old part won't be properly
		// removed. 
		delete mPart;
		mPart = part;

		mPartLibrary = library;
	}

	void initDirModel() {
		KDirLister* dirLister = mDirModel->dirLister();
		QStringList mimeTypes;
		mimeTypes += MimeTypeUtils::dirMimeTypes();
		mimeTypes += MimeTypeUtils::imageMimeTypes();
		mimeTypes += MimeTypeUtils::videoMimeTypes();
		dirLister->setMimeFilter(mimeTypes);

		connect(dirLister, SIGNAL(newItems(const KFileItemList&)),
			mWindow, SLOT(slotDirListerNewItems(const KFileItemList&)) );

		connect(dirLister, SIGNAL(deleteItem(KFileItem*)),
			mWindow, SLOT(updatePreviousNextActions()) );
	}

	void updateToggleSideBarAction() {
		if (mSideBar->isVisible()) {
			mToggleSideBarAction->setText(i18n("Hide Side Bar"));
		} else {
			mToggleSideBarAction->setText(i18n("Show Side Bar"));
		}
	}

	void resetDocumentView() {
		if (mPart) {
			mWindow->createGUI(0);
			delete mPart;
			mPartLibrary = QString();
			mPart=0;
		}
	}
	QModelIndex getRelativeIndex(int offset) {
		QItemSelection selection = mThumbnailView->selectionModel()->selection();
		if (selection.size() == 0) {
			return QModelIndex();
		}
		QModelIndex index = selection.indexes()[0];
		int row = index.row() + offset;
		return mDirModel->index(row, 0);
	}

	void goTo(int offset) {
		QModelIndex index = getRelativeIndex(offset);
		if (index.isValid()) {
			mThumbnailView->selectionModel()->select(index, QItemSelectionModel::SelectCurrent);
		}
	}
};


MainWindow::MainWindow()
: KParts::MainWindow(),
d(new MainWindow::Private)
{
	d->mWindow = this;
	d->mDirModel = new SortedDirModel(this);
	d->mPart = 0;
	d->initDirModel();
	d->setupWidgets();
	d->setupActions();
	d->setupContextManager();
	updatePreviousNextActions();

	createShellGUI();
}


void MainWindow::openUrl(const KUrl& url) {
	if (urlIsDirectory(this, url)) {
		d->mThumbsAndImageAction->trigger();
		openDirUrl(url);
	} else {
		d->mImageOnlyAction->trigger();
		d->mSideBar->hide();
		openDocumentUrl(url);
	}
	d->updateToggleSideBarAction();
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
	d->mThumbnailViewPanel->setVisible(showThumbnail);
}


void MainWindow::openDirUrlFromIndex(const QModelIndex& index) {
	if (!index.isValid()) {
		return;
	}

	KFileItem* item = d->mDirModel->itemForIndex(index);
	if (item->isDir()) {
		openDirUrl(item->url());
	}
}


void MainWindow::openSelectedDocument() {
	QItemSelection selection = d->mThumbnailView->selectionModel()->selection();
	if (selection.size() == 0) {
		return;
	}

	QModelIndex index = selection.indexes()[0];
	if (!index.isValid()) {
		return;
	}

	KFileItem* item = d->mDirModel->itemForIndex(index);
	if (!item->isDir()) {
		openDocumentUrl(item->url());
	}
}


void MainWindow::goUp() {
	KUrl url = d->mDirModel->dirLister()->url();
	url = url.upUrl();
	openDirUrl(url);
}


void MainWindow::openDirUrl(const KUrl& url) {
	d->mDirModel->dirLister()->openUrl(url);
	d->mUrlRequester->setUrl(url);
	d->mGoUpAction->setEnabled(url.path() != "/");
	d->resetDocumentView();
}


void MainWindow::openDirUrlFromString(const QString& str) {
	KUrl url(str);
	openDirUrl(url);
}

void MainWindow::openDocumentUrl(const KUrl& url) {
	d->createPartForUrl(url);
	if (!d->mPart) return;
	d->mPart->openUrl(url);
}

void MainWindow::slotSetStatusBarText(const QString&) {
	// FIXME: Show message somewhere
}

void MainWindow::toggleSideBar() {
	d->mSideBar->setVisible(!d->mSideBar->isVisible());
	if (d->mSideBar->isVisible()) {
		updateSideBar();
	}
	d->updateToggleSideBarAction();
}


void MainWindow::updateSideBar() {
	if (!d->mSideBar->isVisible()) {
		return;
	}
	QItemSelection selection = d->mThumbnailView->selectionModel()->selection();
	QModelIndexList indexList = selection.indexes();

	KFileItemList itemList;
	Q_FOREACH(QModelIndex index, indexList) {
		KFileItem* item = d->mDirModel->itemForIndex(index);
		itemList << item;
	}

	d->mContextManager->updateSideBar(itemList);
}

void MainWindow::startDirLister() {
	Q_ASSERT(d->mPart);
	KUrl url = d->mPart->url();
	url.setFileName("");
	if (!url.equals(d->mDirModel->dirLister()->url(), KUrl::CompareWithoutTrailingSlash)) {
		d->mDirModel->dirLister()->openUrl(url);
	}
}


void MainWindow::slotSelectionChanged() {
	openSelectedDocument();
	updateSideBar();
	updatePreviousNextActions();
}


void MainWindow::slotDirListerNewItems(const KFileItemList& list) {
	if (!d->mPart) {
		return;
	}

	QItemSelection selection = d->mThumbnailView->selectionModel()->selection();
	if (selection.size() > 0) {
		updatePreviousNextActions();
		return;
	}

	KUrl url = d->mPart->url();
	Q_FOREACH(KFileItem* item, list) {
		if (item->url() == url) {
			QModelIndex index = d->mDirModel->indexForItem(item);
			d->mThumbnailView->selectionModel()->select(index, QItemSelectionModel::SelectCurrent);
			return;
		}
	}
}


void MainWindow::goToPrevious() {
	d->goTo(-1);
}


void MainWindow::goToNext() {
	d->goTo(1);
}


void MainWindow::updatePreviousNextActions() {
	QModelIndex index = d->getRelativeIndex(-1);
	if (index.isValid()) {
		KFileItem* item = d->mDirModel->itemForIndex(index);
		bool enabled = item && !ArchiveUtils::fileItemIsDirOrArchive(item);
		d->mGoToPreviousAction->setEnabled(enabled);
	} else {
		d->mGoToPreviousAction->setEnabled(false);
	}

	index = d->getRelativeIndex(1);
	d->mGoToNextAction->setEnabled(index.isValid());
}


} // namespace
