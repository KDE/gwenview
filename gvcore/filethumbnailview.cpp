// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2004 Aurélien Gâteau

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// Qt
#include <qframe.h>
#include <qlayout.h>
#include <qpainter.h>
#include <qpen.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qtimer.h>
#include <qvaluevector.h>

// KDE
#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kprogress.h>
#include <kstandarddirs.h>
#include <kurldrag.h>
#include <kwordwrap.h>

// Local
#include "fileviewconfig.h"
#include "filethumbnailviewitem.h"
#include "archive.h"
#include "dragpixmapgenerator.h"
#include "thumbnailloadjob.h"
#include "busylevelmanager.h"
#include "imageloader.h"
#include "timeutils.h"
#include "thumbnailsize.h"
#include "thumbnaildetailsdialog.h"

#undef ENABLE_LOG
#undef LOG
#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

#include "filethumbnailview.moc"
namespace Gwenview {

static const int THUMBNAIL_UPDATE_DELAY=500;
	
static const int RIGHT_TEXT_WIDTH=128;
static const int BOTTOM_MIN_TEXT_WIDTH=96;

class ProgressWidget : public QFrame {
	KProgress* mProgressBar;
	QPushButton* mStop;
public:
	ProgressWidget(FileThumbnailView* view, int count)
	: QFrame(view)
	{
		QHBoxLayout* layout=new QHBoxLayout(this, 3, 3);
		layout->setAutoAdd(true);
		setFrameStyle( QFrame::StyledPanel | QFrame::Raised );
		
		mStop=new QPushButton(this);
		mStop->setPixmap(SmallIcon("stop"));
		mStop->setFlat(true);

		mProgressBar=new KProgress(count, this);
		mProgressBar->setFormat("%v/%m");

		view->clipper()->installEventFilter(this);
	}
	
	void polish() {
		QFrame::polish();
		setMinimumWidth(layout()->minimumSize().width());
		//setFixedHeight( mProgressBar->height() );
		setFixedHeight( mStop->height() );
	}

	void showEvent(QShowEvent*) {
		updatePosition();
	}

	bool eventFilter(QObject*, QEvent* event) {
		if (event->type()==QEvent::Resize) {
			updatePosition();
		}
		return false;
	}

	void updatePosition() {
		FileThumbnailView* view=static_cast<FileThumbnailView*>(parent());
		QSize tmp=view->clipper()->size() - size();
		move(tmp.width() - 2, tmp.height() - 2);
	}

	KProgress* progressBar() const { return mProgressBar; }
	QPushButton* stopButton() const { return mStop; }
};


struct FileThumbnailView::Private {
	int mThumbnailSize;
	int mMarginSize;
	bool mUpdateThumbnailsOnNextShow;
	QPixmap mWaitPixmap; // The wait pixmap (32 x 32)
	QPixmap mWaitThumbnail; // The wait thumbnail (mThumbnailSize x mThumbnailSize)
	ProgressWidget* mProgressWidget;

	QGuardedPtr<ThumbnailLoadJob> mThumbnailLoadJob;
	
	QTimer* mThumbnailUpdateTimer;

	int mItemDetails;

	ImageLoader* mPrefetch;
	ThumbnailDetailsDialog* mThumbnailsDetailDialog;

	void updateWaitThumbnail(const FileThumbnailView* view) {
		mWaitThumbnail=QPixmap(mThumbnailSize, mThumbnailSize);
		mWaitThumbnail.fill(view->paletteBackgroundColor());
		QPainter painter(&mWaitThumbnail);
		
		painter.setPen(view->colorGroup().button());
		painter.drawRect(0,0,mThumbnailSize,mThumbnailSize);
		painter.drawPixmap(
			(mThumbnailSize-mWaitPixmap.width())/2,
			(mThumbnailSize-mWaitPixmap.height())/2,
			mWaitPixmap);
		painter.end();
	}
};


static FileThumbnailViewItem* viewItem(const FileThumbnailView* view, const KFileItem* fileItem) {
	if (!fileItem) return 0L;
	return static_cast<FileThumbnailViewItem*>( const_cast<void*>(fileItem->extraData(view) ) );
}


FileThumbnailView::FileThumbnailView(QWidget* parent)
: KIconView(parent), FileViewBase()
{
	d=new Private;
	d->mUpdateThumbnailsOnNextShow=false;
	d->mThumbnailLoadJob=0L;
	d->mWaitPixmap=QPixmap(::locate("appdata", "thumbnail/wait.png"));
	d->mProgressWidget=0L;
	d->mThumbnailUpdateTimer=new QTimer(this);
	d->mMarginSize=FileViewConfig::thumbnailMarginSize();
	d->mItemDetails=FileViewConfig::thumbnailDetails();
	d->mPrefetch = NULL;
	d->mThumbnailSize = 0;
	d->mThumbnailsDetailDialog = 0;

	setItemTextPos( QIconView::ItemTextPos(FileViewConfig::thumbnailTextPos()) );
	setAutoArrange(true);
	QIconView::setSorting(true);
	setItemsMovable(false);
	setResizeMode(Adjust);
	setShowToolTips(true);
	setSpacing(0);
	setAcceptDrops(true);

	// We can't use KIconView::Execute mode because in this mode the current
	// item is unselected after being clicked, so we use KIconView::Select mode
	// and emit the execute() signal with slotClicked() ourself.
	setMode(KIconView::Select);
	connect(this, SIGNAL(clicked(QIconViewItem*)),
		this, SLOT(slotClicked(QIconViewItem*)) );
	connect(this, SIGNAL(doubleClicked(QIconViewItem*)),
		this, SLOT(slotDoubleClicked(QIconViewItem*)) );

	connect(this, SIGNAL(dropped(QDropEvent*,const QValueList<QIconDragItem>&)),
		this, SLOT(slotDropped(QDropEvent*)) );
	connect(this, SIGNAL( contentsMoving( int, int )),
		this, SLOT( slotContentsMoving( int, int )));
	connect(this, SIGNAL(currentChanged(QIconViewItem*)),
		this, SLOT(slotCurrentChanged(QIconViewItem*)) );

	QIconView::setSelectionMode(Extended);

	connect(BusyLevelManager::instance(), SIGNAL(busyLevelChanged(BusyLevel)),
		this, SLOT( slotBusyLevelChanged(BusyLevel)));

	connect(d->mThumbnailUpdateTimer, SIGNAL(timeout()),
		this, SLOT( startThumbnailUpdate()) );
}


FileThumbnailView::~FileThumbnailView() {
	stopThumbnailUpdate();
	FileViewConfig::setThumbnailDetails(d->mItemDetails);
	FileViewConfig::setThumbnailTextPos( int(itemTextPos()) );
	FileViewConfig::writeConfig();
	delete d;
}


void FileThumbnailView::setThumbnailSize(int value) {
	if (value==d->mThumbnailSize) return;
	d->mThumbnailSize=value;
	updateGrid();
		
	KFileItemListIterator it( *items() );
	for ( ; it.current(); ++it ) {
		KFileItem *item=it.current();
		QPixmap pixmap=createItemPixmap(item);
		QIconViewItem* iconItem=viewItem(this, item);
		if (iconItem) iconItem->setPixmap(pixmap);
	}
	arrangeItemsInGrid();
	d->mThumbnailUpdateTimer->start(THUMBNAIL_UPDATE_DELAY, true);
}


int FileThumbnailView::thumbnailSize() const {
	return d->mThumbnailSize;
}


/**
 * Overriden to call updateGrid
 */
void FileThumbnailView::setItemTextPos(ItemTextPos pos) {
	QIconView::setItemTextPos(pos);
	updateGrid();
}


void FileThumbnailView::setMarginSize(int value) {
	if (value==d->mMarginSize) return;
	d->mMarginSize=value;
	updateGrid();
}


int FileThumbnailView::marginSize() const {
	return d->mMarginSize;
}


void FileThumbnailView::setItemDetails(int details) {
	d->mItemDetails=details;
	for (QIconViewItem* item=firstItem(); item; item=item->nextItem()) {
		static_cast<FileThumbnailViewItem*>(item)->updateLines();
	}
	arrangeItemsInGrid();
}


int FileThumbnailView::itemDetails() const {
	return d->mItemDetails;
}


void FileThumbnailView::setThumbnailPixmap(const KFileItem* fileItem, const QPixmap& thumbnail, const QSize& size) {
	FileThumbnailViewItem* iconItem=viewItem(this, fileItem);
	if (!iconItem) return;

	iconItem->setPixmap(thumbnail);

	// Update item info
	if (size.isValid()) {
		iconItem->setImageSize(size);
	}
	iconItem->repaint();

	// Notify progress
	if (d->mProgressWidget) {
		// mProgressWidget might be null if we get called after the thumbnail
		// job finished. This can happen when the thumbnail job use KPreviewJob
		// to generate a thumbnail.
		d->mProgressWidget->progressBar()->advance(1);
	}
}




void FileThumbnailView::setShownFileItem(KFileItem* fileItem) {
	if( fileItem == mShownFileItem ) return;
	FileThumbnailViewItem* oldShownItem=viewItem(this, mShownFileItem);
	FileThumbnailViewItem* newShownItem=viewItem(this, fileItem);

	FileViewBase::setShownFileItem(fileItem);
	if (oldShownItem) repaintItem(oldShownItem);
	if (newShownItem) repaintItem(newShownItem);
}


//-----------------------------------------------------------------------------
//
// Thumbnail code
//
//-----------------------------------------------------------------------------
QPixmap FileThumbnailView::createItemPixmap(const KFileItem* item) const {
	bool isDirOrArchive=item->isDir() || Archive::fileItemIsArchive(item);
	if (!isDirOrArchive) {
		if (d->mWaitThumbnail.width()!=d->mThumbnailSize) {
			d->updateWaitThumbnail(this);
		}
		return d->mWaitThumbnail;
	}

	QPixmap thumbnail(d->mThumbnailSize, d->mThumbnailSize);
	thumbnail.fill(paletteBackgroundColor());
	QPainter painter(&thumbnail);

	// Load the icon
	QPixmap itemPix=item->pixmap(QMIN(d->mThumbnailSize, ThumbnailSize::NORMAL));
	painter.drawPixmap(
		(d->mThumbnailSize-itemPix.width())/2,
		(d->mThumbnailSize-itemPix.height())/2,
		itemPix);

	return thumbnail;
}


void FileThumbnailView::startThumbnailUpdate() {
	// Delay thumbnail update if the widget is not visible
	if (!isVisible()) {
		d->mUpdateThumbnailsOnNextShow=true;
		return;
	}
	d->mUpdateThumbnailsOnNextShow=false;
	stopThumbnailUpdate(); // just in case
	doStartThumbnailUpdate(items());
}


void FileThumbnailView::doStartThumbnailUpdate(const KFileItemList* list) {
	QValueVector<const KFileItem*> imageList;
	imageList.reserve( list->count());
	QPtrListIterator<KFileItem> it(*list);
	for (;it.current(); ++it) {
		KFileItem* item=it.current();
		if (!item->isDir() && !Archive::fileItemIsArchive(item)) {
			imageList.append( item );
		}
	}
	if (imageList.empty()) return;

	BusyLevelManager::instance()->setBusyLevel( this, BUSY_THUMBNAILS );

	Q_ASSERT(!d->mProgressWidget);
	d->mProgressWidget=new ProgressWidget(this, imageList.count() );
	
	connect(d->mProgressWidget->stopButton(), SIGNAL(clicked()),
		this, SLOT(stopThumbnailUpdate()) );
	d->mProgressWidget->show();
	
	d->mThumbnailLoadJob = new ThumbnailLoadJob(&imageList, d->mThumbnailSize);

	connect(d->mThumbnailLoadJob, SIGNAL(thumbnailLoaded(const KFileItem*, const QPixmap&, const QSize&)),
		this, SLOT(setThumbnailPixmap(const KFileItem*,const QPixmap&, const QSize&)) );
	connect(d->mThumbnailLoadJob, SIGNAL(result(KIO::Job*)),
		this, SLOT(slotUpdateEnded()) );

	slotBusyLevelChanged( BusyLevelManager::instance()->busyLevel());
	// start updating at visible position
	slotContentsMoving( contentsX(), contentsY());
	d->mThumbnailLoadJob->start();
}


void FileThumbnailView::stopThumbnailUpdate() {
	if (!d->mThumbnailLoadJob.isNull()) {
		d->mThumbnailLoadJob->kill(false);
	}
}


void FileThumbnailView::slotUpdateEnded() {
	Q_ASSERT(d->mProgressWidget);
	delete d->mProgressWidget;
	d->mProgressWidget=0L;

	BusyLevelManager::instance()->setBusyLevel( this, BUSY_NONE );
}


void FileThumbnailView::updateThumbnail(const KFileItem* fileItem) {
	if (fileItem->isDir() || Archive::fileItemIsArchive(fileItem)) {
		return;
	}

	ThumbnailLoadJob::deleteImageThumbnail(fileItem->url());
	if (d->mThumbnailLoadJob.isNull()) {
		KFileItemList list;
		list.append(fileItem);
		doStartThumbnailUpdate(&list);
	} else {
		d->mThumbnailLoadJob->appendItem(fileItem);
	}
}

// temporarily stop loading thumbnails when busy loading the selected image,
// otherwise thumbnail loading slows it down
void FileThumbnailView::slotBusyLevelChanged(BusyLevel level) {
	if( !d->mThumbnailLoadJob.isNull()) {
		if( level > BUSY_THUMBNAILS ) {
			d->mThumbnailLoadJob->suspend();
		} else {
			d->mThumbnailLoadJob->resume();
		}
	}
}

//-----------------------------------------------------------------------------
//
// KFileView methods
//
//-----------------------------------------------------------------------------
void FileThumbnailView::clearView() {
	stopThumbnailUpdate();
	mShownFileItem=0L;
	QIconView::clear();
}


void FileThumbnailView::insertItem(KFileItem* item) {
	if (!item) return;
	bool isDirOrArchive=item->isDir() || Archive::fileItemIsArchive(item);

	QPixmap thumbnail=createItemPixmap(item);
	FileThumbnailViewItem* iconItem=new FileThumbnailViewItem(this,item->text(),thumbnail,item);
	iconItem->setDropEnabled(isDirOrArchive);

	setSortingKey(iconItem, item);
	item->setExtraData(this,iconItem);
}


void FileThumbnailView::updateView(const KFileItem* fileItem) {
	if (!fileItem) return;

	FileThumbnailViewItem* iconItem=viewItem(this, fileItem);
	if (iconItem) {
		iconItem->setText(fileItem->text());
		updateThumbnail(fileItem);
	}
	sort();
}


void FileThumbnailView::ensureItemVisible(const KFileItem* fileItem) {
	if (!fileItem) return;

	FileThumbnailViewItem* iconItem=viewItem(this, fileItem);
	if (iconItem) QIconView::ensureItemVisible(iconItem);
}


void FileThumbnailView::setCurrentItem(const KFileItem* fileItem) {
	if (!fileItem) return;

	FileThumbnailViewItem* iconItem=viewItem(this, fileItem);
	if (iconItem) QIconView::setCurrentItem(iconItem);
}


void FileThumbnailView::setSelected(const KFileItem* fileItem,bool enable) {
	if (!fileItem) return;

	FileThumbnailViewItem* iconItem=viewItem(this, fileItem);
	if (iconItem) QIconView::setSelected(iconItem, enable, true /* do not unselect others */);
}


bool FileThumbnailView::isSelected(const KFileItem* fileItem) const {
	if (!fileItem) return false;

	FileThumbnailViewItem* iconItem=viewItem(this, fileItem);
	if (!iconItem) return false;

	return iconItem->isSelected();
}


void FileThumbnailView::removeItem(const KFileItem* fileItem) {
	if (!fileItem) return;

	// Remove it from the image preview job
	if (!d->mThumbnailLoadJob.isNull())
		d->mThumbnailLoadJob->itemRemoved(fileItem);

	if (fileItem==mShownFileItem) mShownFileItem=0L;

	// Remove it from our view
	FileThumbnailViewItem* iconItem=viewItem(this, fileItem);
	if (iconItem) delete iconItem;
	KFileView::removeItem(fileItem);
	arrangeItemsInGrid();
}


KFileItem* FileThumbnailView::firstFileItem() const {
	FileThumbnailViewItem* iconItem=static_cast<FileThumbnailViewItem*>(firstItem());
	if (!iconItem) return 0L;
	return iconItem->fileItem();
}


KFileItem* FileThumbnailView::prevItem(const KFileItem* fileItem) const {
	const FileThumbnailViewItem* iconItem=viewItem(this, fileItem);
	if (!iconItem) return 0L;

	iconItem=static_cast<const FileThumbnailViewItem*>(iconItem->prevItem());
	if (!iconItem) return 0L;

	return iconItem->fileItem();
}


KFileItem* FileThumbnailView::currentFileItem() const {
	const QIconViewItem* iconItem=currentItem();
	if (!iconItem) return 0L;

	return static_cast<const FileThumbnailViewItem*>(iconItem)->fileItem();
}


KFileItem* FileThumbnailView::nextItem(const KFileItem* fileItem) const {
	const FileThumbnailViewItem* iconItem=viewItem(this, fileItem);
	if (!iconItem) return 0L;

	iconItem=static_cast<const FileThumbnailViewItem*>(iconItem->nextItem());
	if (!iconItem) return 0L;

	return iconItem->fileItem();
}


void FileThumbnailView::setSorting(QDir::SortSpec spec) {
	KFileView::setSorting(spec);

	KFileItem *item;
	KFileItemListIterator it( *items() );

	for ( ; (item = it.current() ); ++it ) {
		QIconViewItem* iconItem=viewItem(this, item);
		if (iconItem) setSortingKey(iconItem, item);
	}

	KIconView::sort(! (spec & QDir::Reversed) );
}

//--------------------------------------------------------------------------
//
// Drop support
//
//--------------------------------------------------------------------------
void FileThumbnailView::contentsDragEnterEvent(QDragEnterEvent* event) {
	return event->accept( KURLDrag::canDecode(event) );
}


void FileThumbnailView::slotDropped(QDropEvent* event) {
	emit dropped(event,0L);
}


void FileThumbnailView::showEvent(QShowEvent* event) {
	KIconView::showEvent(event);
	if (!d->mUpdateThumbnailsOnNextShow) return;

	d->mUpdateThumbnailsOnNextShow=false;
	QTimer::singleShot(0, this, SLOT(startThumbnailUpdate()));
}


//--------------------------------------------------------------------------
//
// Private
//
//--------------------------------------------------------------------------
void FileThumbnailView::updateGrid() {
	if (itemTextPos()==Right) {
		setGridX(
			d->mThumbnailSize
			+ FileThumbnailViewItem::PADDING*3
			+ RIGHT_TEXT_WIDTH);
	} else {
		setGridX(
			QMAX(d->mThumbnailSize, BOTTOM_MIN_TEXT_WIDTH)
			+ FileThumbnailViewItem::PADDING*2);
	}
	setSpacing(d->mMarginSize);
}


void FileThumbnailView::setSortingKey(QIconViewItem *iconItem, const KFileItem *item)
{
	// see also setSorting()
	QDir::SortSpec spec = KFileView::sorting();
	bool isDirOrArchive=item->isDir() || Archive::fileItemIsArchive(item);

	QString key;
	if ( spec & QDir::Time ) {
		time_t time = TimeUtils::getTime(item);
		key=sortingKey(time, isDirOrArchive, spec);
		
	} else if ( spec & QDir::Size ) {
		key=sortingKey( item->size(), isDirOrArchive, spec );
		
	} else {
		// Name or Unsorted
		key=sortingKey( item->text(), isDirOrArchive, spec );
	}

	iconItem->setKey(key);
}


//--------------------------------------------------------------------------
//
// Private slots
//
//--------------------------------------------------------------------------
void FileThumbnailView::slotDoubleClicked(QIconViewItem* iconItem) {
	if (!iconItem) return;
	if (KGlobalSettings::singleClick()) return;
	FileThumbnailViewItem* thumbItem=static_cast<FileThumbnailViewItem*>(iconItem);

	KFileItem* fileItem=thumbItem->fileItem();

	if (fileItem->isDir() || Archive::fileItemIsArchive(fileItem)) {
		emit executed(iconItem);
	}
}


void FileThumbnailView::slotClicked(QIconViewItem* iconItem) {
	if (!iconItem) return;
	if (!KGlobalSettings::singleClick()) return;
	FileThumbnailViewItem* thumbItem=static_cast<FileThumbnailViewItem*>(iconItem);

	KFileItem* fileItem=thumbItem->fileItem();

	if (fileItem->isDir() || Archive::fileItemIsArchive(fileItem)) {
		emit executed(iconItem);
	}
}

void FileThumbnailView::slotContentsMoving( int x, int y ) {
	updateVisibilityInfo( x, y ); // use x,y, the signal is emitted before moving
}

void FileThumbnailView::slotCurrentChanged(QIconViewItem* item ) {
	// trigger generating thumbnails from the current one
	updateVisibilityInfo( contentsX(), contentsY());
	prefetchDone();
	// if the first image is selected, no matter how, preload the next one
	for( QIconViewItem* pos = item;
	     pos != NULL;
	     pos = pos->nextItem()) {
		FileThumbnailViewItem* cur = static_cast< FileThumbnailViewItem* >( pos );
		if( cur->fileItem()->isDir() || Archive::fileItemIsArchive(cur->fileItem())) continue;
		if( pos == item && pos->nextItem() != NULL ) {
			d->mPrefetch = ImageLoader::loader(
				static_cast<const FileThumbnailViewItem*>( cur->nextItem() )->fileItem()->url(),
				this, BUSY_PRELOADING );
			connect( d->mPrefetch, SIGNAL( imageLoaded( bool )), SLOT( prefetchDone()));
		}
	}
}

/**
 * when generating thumbnails, make the current thumbnail
 * to be the next one processed by the thumbnail job, if visible,
 * otherwise use the first visible thumbnail
 */
void FileThumbnailView::updateVisibilityInfo( int x, int y ) {
	if (d->mThumbnailLoadJob.isNull()) return;
	
	QRect rect( x, y, visibleWidth(), visibleHeight());
	FileThumbnailViewItem* first = static_cast< FileThumbnailViewItem* >( findFirstVisibleItem( rect ));
	if (!first) {
		d->mThumbnailLoadJob->setPriorityItems(NULL,NULL,NULL);
		return;
	}
	
	FileThumbnailViewItem* last = static_cast< FileThumbnailViewItem* >( findLastVisibleItem( rect ));
	Q_ASSERT(last); // If we get a first item, then there must be a last
	
	if (currentItem() && currentItem()->intersects(rect)) {
		KFileItem* fileItem = currentFileItem();
		d->mThumbnailLoadJob->setPriorityItems(fileItem,
			first->fileItem(), last->fileItem());
		return;
	}
	
	d->mThumbnailLoadJob->setPriorityItems(
		first->fileItem(),
		first->fileItem(),
		last->fileItem());
}

void FileThumbnailView::keyPressEvent( QKeyEvent* e ) {
// When the user presses e.g. the Down key, try to preload the next image in that direction.
	if( e->key() != Key_Left
		&& e->key() != Key_Right
		&& e->key() != Key_Up
		&& e->key() != Key_Down ) return KIconView::keyPressEvent( e );

	QIconViewItem* current = currentItem();
	KIconView::keyPressEvent( e );
	QIconViewItem* next = NULL;
	if( current != currentItem() && currentItem() != NULL ) { // it actually moved
		switch( e->key()) {
			case Key_Left:
				next = currentItem()->prevItem();
				break;
			case Key_Right:
				next = currentItem()->nextItem();
				break;
			case Key_Up:
			// This relies on the thumbnails being in a grid ( x() == x() )
				for( next = currentItem()->prevItem();
				     next != NULL && next->x() != currentItem()->x();
				     next = next->prevItem())
					;
				break;
			case Key_Down:
				for( next = currentItem()->nextItem();
				     next != NULL && next->x() != currentItem()->x();
				     next = next->nextItem())
					;
				break;
		}

	}
	prefetchDone();
	if( next != NULL ) {
		d->mPrefetch = ImageLoader::loader(
			static_cast<const FileThumbnailViewItem*>( next )->fileItem()->url(),
			this, BUSY_PRELOADING );
		connect( d->mPrefetch, SIGNAL( imageLoaded( bool )), SLOT( prefetchDone()));
	}
}

void FileThumbnailView::prefetchDone() {
	if( d->mPrefetch != NULL ) {
		d->mPrefetch->release( this );
		d->mPrefetch = NULL;
	}
}

//--------------------------------------------------------------------------
//
// Protected
//
//--------------------------------------------------------------------------
void FileThumbnailView::startDrag() {
	/**
	 * The item drawer for DragPixmapGenerator
	 */
	struct ItemDrawer : public DragPixmapItemDrawer<KFileItem*> {
		ItemDrawer(FileThumbnailView* view)
		: mView(view) {}

		QSize itemSize(KFileItem* fileItem) {
			QPixmap* pix = pixmapFromFileItem(fileItem);
			if (!pix) return QSize();
					
			QSize size = pix->size();
			int maxWidth = mGenerator->maxWidth();
			if (size.width() > maxWidth) {
				size.rheight() = size.height() * maxWidth / size.width();
				size.rwidth() = maxWidth;
			}
			return size;
		}

		int spacing() const {
			return 2;
		}
			
		void drawItem(QPainter* painter, int left, int top, KFileItem* fileItem) {
			QPixmap* pix = pixmapFromFileItem(fileItem);
			if (!pix) return;

			QSize size = itemSize(fileItem);
			left += (mGenerator->pixmapWidth() - size.width()) / 2;
			if (size == pix->size()) {
				painter->drawPixmap(left, top, *pix);
				return;
			}
			
			QImage img = pix->convertToImage();
			img = img.smoothScale(size, QImage::ScaleMin);
			painter->drawImage(left, top, img);
		}

		QPixmap* pixmapFromFileItem(KFileItem* fileItem) {
			FileThumbnailViewItem* iconItem = viewItem(mView, fileItem);
			Q_ASSERT(iconItem);
			if (!iconItem) return 0;
			
			QPixmap* pix = iconItem->pixmap();
			Q_ASSERT(pix);
			if (!pix) return 0;
			return pix;
		}
		
		FileThumbnailView* mView;
	};
	ItemDrawer drawer(this);


	KURL::List urls;
	KFileItemListIterator it(*KFileView::selectedItems());

	DragPixmapGenerator<KFileItem*> generator;
	generator.setItemDrawer(&drawer);
	
	for ( ; it.current(); ++it ) {
		urls.append(it.current()->url());
		generator.addItem(it.current());
	}

	if (urls.isEmpty()) {
		kdWarning() << "No item to drag\n";
		return;
	}

	QDragObject* drag=new KURLDrag(urls, this, 0);
	QPixmap dragPixmap = generator.generate();

	drag->setPixmap( dragPixmap, QPoint(generator.DRAG_OFFSET, -generator.DRAG_OFFSET));
	drag->dragCopy();
}


void FileThumbnailView::showThumbnailDetailsDialog() {
	if (!d->mThumbnailsDetailDialog) {
		d->mThumbnailsDetailDialog = new ThumbnailDetailsDialog(this);
	}
	d->mThumbnailsDetailDialog->show();
}


} // namespace
