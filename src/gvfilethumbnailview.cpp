// vim: set tabstop=4 shiftwidth=4 noexpandtab
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
#include <qpixmapcache.h>
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
#include "gvfilethumbnailviewitem.h"
#include "gvarchive.h"
#include "thumbnailloadjob.h"
#include "gvbusylevelmanager.h"
#include "gvthumbnailsize.h"

#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

#include "gvfilethumbnailview.moc"

static const char* CONFIG_ITEM_TEXT_POS="item text pos";
static const char* CONFIG_THUMBNAIL_SIZE="thumbnail size";
static const char* CONFIG_MARGIN_SIZE="margin size";
static const char* CONFIG_WORD_WRAP_FILENAME="word wrap filename";

static const int THUMBNAIL_TEXT_SIZE=128;

class ProgressWidget : public QFrame {
	KProgress* mProgressBar;
	QPushButton* mStop;
public:
	ProgressWidget(GVFileThumbnailView* view, int count)
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
		GVFileThumbnailView* view=static_cast<GVFileThumbnailView*>(parent());
		QSize tmp=view->clipper()->size() - size();
		move(tmp.width() - 2, tmp.height() - 2);
	}

	KProgress* progressBar() const { return mProgressBar; }
	QPushButton* stopButton() const { return mStop; }
};


struct GVFileThumbnailView::Private {
	int mThumbnailSize;
	int mMarginSize;
	bool mUpdateThumbnailsOnNextShow;
	QPixmap mWaitPixmap;
	ProgressWidget* mProgressWidget;

	QGuardedPtr<ThumbnailLoadJob> mThumbnailLoadJob;
	
};


static GVFileThumbnailViewItem* viewItem(const GVFileThumbnailView* view, const KFileItem* fileItem) {
	if (!fileItem) return 0L;
	return static_cast<GVFileThumbnailViewItem*>( const_cast<void*>(fileItem->extraData(view) ) );
}


GVFileThumbnailView::GVFileThumbnailView(QWidget* parent)
: KIconView(parent), GVFileViewBase()
{
	d=new Private;
	d->mUpdateThumbnailsOnNextShow=false;
	d->mThumbnailLoadJob=0L;
	d->mWaitPixmap=QPixmap(::locate("appdata", "thumbnail/wait.png"));
	d->mProgressWidget=0L;

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

	connect(GVBusyLevelManager::instance(), SIGNAL(busyLevelChanged(GVBusyLevel)),
		this, SLOT( slotBusyLevelChanged(GVBusyLevel)));
}


GVFileThumbnailView::~GVFileThumbnailView() {
	stopThumbnailUpdate();
	delete d;
}


void GVFileThumbnailView::setThumbnailSize(int value) {
	if (value==d->mThumbnailSize) return;
	d->mThumbnailSize=value;
	
	KFileItemListIterator it( *items() );
	for ( ; it.current(); ++it ) {
		KFileItem *item=it.current();
		QPixmap pixmap=createItemPixmap(item);
		QIconViewItem* iconItem=viewItem(this, item);
		if (iconItem) iconItem->setPixmap(pixmap);
	}
	updateGrid();
	startThumbnailUpdate();
}


int GVFileThumbnailView::thumbnailSize() const {
	return d->mThumbnailSize;
}


/**
 * Overriden to call updateGrid
 */
void GVFileThumbnailView::setItemTextPos(ItemTextPos pos) {
	QIconView::setItemTextPos(pos);
	updateGrid();
}


void GVFileThumbnailView::setMarginSize(int value) {
	if (value==d->mMarginSize) return;
	d->mMarginSize=value;
	updateGrid();
}


int GVFileThumbnailView::marginSize() const {
	return d->mMarginSize;
}


void GVFileThumbnailView::setThumbnailPixmap(const KFileItem* fileItem, const QPixmap& thumbnail, const QSize& size) {
	GVFileThumbnailViewItem* iconItem=viewItem(this, fileItem);
	if (!iconItem) return;

	int pixelSize=d->mThumbnailSize;

	// Draw the thumbnail to the center of the icon
	QPainter painter(iconItem->pixmap());
	painter.fillRect(0,0,pixelSize,pixelSize,paletteBackgroundColor());
	painter.drawPixmap(
		(pixelSize-thumbnail.width())/2,
		(pixelSize-thumbnail.height())/2,
		thumbnail);

	// Update item info
	if (size.isValid()) {
		iconItem->setImageSize(size);
	}
	iconItem->repaint();

	// Notify progress
	Q_ASSERT(d->mProgressWidget);
	d->mProgressWidget->progressBar()->advance(1);
}




void GVFileThumbnailView::setShownFileItem(KFileItem* fileItem) {
	if( fileItem == mShownFileItem ) return;
	GVFileThumbnailViewItem* oldShownItem=viewItem(this, mShownFileItem);
	GVFileThumbnailViewItem* newShownItem=viewItem(this, fileItem);

	GVFileViewBase::setShownFileItem(fileItem);
	if (oldShownItem) repaintItem(oldShownItem);
	if (newShownItem) repaintItem(newShownItem);
}


//-----------------------------------------------------------------------------
//
// Thumbnail code
//
//-----------------------------------------------------------------------------
QPixmap GVFileThumbnailView::createItemPixmap(const KFileItem* item) const {
	bool isDirOrArchive=item->isDir() || GVArchive::fileItemIsArchive(item);

	QPixmap thumbnail(d->mThumbnailSize, d->mThumbnailSize);
	QPainter painter(&thumbnail);
	painter.fillRect(0,0, d->mThumbnailSize, d->mThumbnailSize, paletteBackgroundColor());

	if (isDirOrArchive) {
		// Load the icon
		QPixmap itemPix=item->pixmap(d->mThumbnailSize);
		painter.drawPixmap(
			(d->mThumbnailSize-itemPix.width())/2,
			(d->mThumbnailSize-itemPix.height())/2,
			itemPix);
	} else {
		// Create an empty thumbnail
		painter.setPen(colorGroup().button());
		painter.drawRect(0,0,d->mThumbnailSize,d->mThumbnailSize);
		painter.drawPixmap(
			(d->mThumbnailSize-d->mWaitPixmap.width())/2,
			(d->mThumbnailSize-d->mWaitPixmap.height())/2,
			d->mWaitPixmap);
	}

	return thumbnail;
}


void GVFileThumbnailView::startThumbnailUpdate() {
	// Delay thumbnail update if the widget is not visible
	if (!isVisible()) {
		d->mUpdateThumbnailsOnNextShow=true;
		return;
	}
	d->mUpdateThumbnailsOnNextShow=false;
	stopThumbnailUpdate(); // just in case
	doStartThumbnailUpdate(items());
}


void GVFileThumbnailView::doStartThumbnailUpdate(const KFileItemList* list) {
	QValueVector<const KFileItem*> imageList;
	imageList.reserve( list->count());
	QPtrListIterator<KFileItem> it(*list);
	for (;it.current(); ++it) {
		KFileItem* item=it.current();
		if (!item->isDir() && !GVArchive::fileItemIsArchive(item)) {
			imageList.append( item );
		}
	}
	if (imageList.empty()) return;

	GVBusyLevelManager::instance()->setBusyLevel( this, BUSY_THUMBNAILS );
	
	d->mThumbnailLoadJob = new ThumbnailLoadJob(&imageList, d->mThumbnailSize);

	connect(d->mThumbnailLoadJob, SIGNAL(thumbnailLoaded(const KFileItem*, const QPixmap&, const QSize&)),
		this, SLOT(setThumbnailPixmap(const KFileItem*,const QPixmap&, const QSize&)) );
	connect(d->mThumbnailLoadJob, SIGNAL(result(KIO::Job*)),
		this, SLOT(slotUpdateEnded()) );

	Q_ASSERT(!d->mProgressWidget);
	d->mProgressWidget=new ProgressWidget(this, imageList.count() );
	connect(d->mProgressWidget->stopButton(), SIGNAL(clicked()),
		this, SLOT(stopThumbnailUpdate()) );
	d->mProgressWidget->show();
	slotBusyLevelChanged( GVBusyLevelManager::instance()->busyLevel());
	// start updating at visible position
	slotContentsMoving( contentsX(), contentsY());
	d->mThumbnailLoadJob->start();
}


void GVFileThumbnailView::stopThumbnailUpdate() {
	if (!d->mThumbnailLoadJob.isNull()) {
		d->mThumbnailLoadJob->kill(false);
	}
}


void GVFileThumbnailView::slotUpdateEnded() {
	Q_ASSERT(d->mProgressWidget);
	delete d->mProgressWidget;
	d->mProgressWidget=0L;
	// This is necessary because the size info might have been added to the
	// text
	if (itemTextPos()==Bottom) {
		arrangeItemsInGrid();
	}
	GVBusyLevelManager::instance()->setBusyLevel( this, BUSY_NONE );
}


void GVFileThumbnailView::updateThumbnail(const KFileItem* fileItem) {
	if (fileItem->isDir() || GVArchive::fileItemIsArchive(fileItem)) {
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
void GVFileThumbnailView::slotBusyLevelChanged(GVBusyLevel level) {
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
void GVFileThumbnailView::clearView() {
	stopThumbnailUpdate();
	mShownFileItem=0L;
	QIconView::clear();
}


void GVFileThumbnailView::insertItem(KFileItem* item) {
	if (!item) return;

	QPixmap thumbnail=createItemPixmap(item);

	GVFileThumbnailViewItem* iconItem=new GVFileThumbnailViewItem(this,item->text(),thumbnail,item);
	bool isDirOrArchive=item->isDir() || GVArchive::fileItemIsArchive(item);
	iconItem->setDropEnabled(isDirOrArchive);

	setSortingKey(iconItem, item);
	item->setExtraData(this,iconItem);
}


void GVFileThumbnailView::updateView(const KFileItem* fileItem) {
	if (!fileItem) return;

	GVFileThumbnailViewItem* iconItem=viewItem(this, fileItem);
	if (iconItem) {
		iconItem->setText(fileItem->text());
		updateThumbnail(fileItem);
	}
	sort();
}


void GVFileThumbnailView::ensureItemVisible(const KFileItem* fileItem) {
	if (!fileItem) return;

	GVFileThumbnailViewItem* iconItem=viewItem(this, fileItem);
	if (iconItem) QIconView::ensureItemVisible(iconItem);
}


void GVFileThumbnailView::setCurrentItem(const KFileItem* fileItem) {
	if (!fileItem) return;

	GVFileThumbnailViewItem* iconItem=viewItem(this, fileItem);
	if (iconItem) QIconView::setCurrentItem(iconItem);
}


void GVFileThumbnailView::setSelected(const KFileItem* fileItem,bool enable) {
	if (!fileItem) return;

	GVFileThumbnailViewItem* iconItem=viewItem(this, fileItem);
	if (iconItem) QIconView::setSelected(iconItem,enable);
}


bool GVFileThumbnailView::isSelected(const KFileItem* fileItem) const {
	if (!fileItem) return false;

	GVFileThumbnailViewItem* iconItem=viewItem(this, fileItem);
	if (!iconItem) return false;

	return iconItem->isSelected();
}


void GVFileThumbnailView::removeItem(const KFileItem* fileItem) {
	if (!fileItem) return;

	// Remove it from the image preview job
	if (!d->mThumbnailLoadJob.isNull())
		d->mThumbnailLoadJob->itemRemoved(fileItem);

	if (fileItem==mShownFileItem) mShownFileItem=0L;

	// Remove it from our view
	GVFileThumbnailViewItem* iconItem=viewItem(this, fileItem);
	if (iconItem) delete iconItem;
	KFileView::removeItem(fileItem);
	arrangeItemsInGrid();
}


KFileItem* GVFileThumbnailView::firstFileItem() const {
	GVFileThumbnailViewItem* iconItem=static_cast<GVFileThumbnailViewItem*>(firstItem());
	if (!iconItem) return 0L;
	return iconItem->fileItem();
}


KFileItem* GVFileThumbnailView::prevItem(const KFileItem* fileItem) const {
	const GVFileThumbnailViewItem* iconItem=viewItem(this, fileItem);
	if (!iconItem) return 0L;

	iconItem=static_cast<const GVFileThumbnailViewItem*>(iconItem->prevItem());
	if (!iconItem) return 0L;

	return iconItem->fileItem();
}


KFileItem* GVFileThumbnailView::currentFileItem() const {
	const QIconViewItem* iconItem=currentItem();
	if (!iconItem) return 0L;

	return static_cast<const GVFileThumbnailViewItem*>(iconItem)->fileItem();
}


KFileItem* GVFileThumbnailView::nextItem(const KFileItem* fileItem) const {
	const GVFileThumbnailViewItem* iconItem=viewItem(this, fileItem);
	if (!iconItem) return 0L;

	iconItem=static_cast<const GVFileThumbnailViewItem*>(iconItem->nextItem());
	if (!iconItem) return 0L;

	return iconItem->fileItem();
}


void GVFileThumbnailView::setSorting(QDir::SortSpec spec) {
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
void GVFileThumbnailView::contentsDragEnterEvent(QDragEnterEvent* event) {
	return event->accept( KURLDrag::canDecode(event) );
}


void GVFileThumbnailView::slotDropped(QDropEvent* event) {
	emit dropped(event,0L);
}


void GVFileThumbnailView::showEvent(QShowEvent* event) {
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
void GVFileThumbnailView::updateGrid() {
	if (itemTextPos()==Bottom) {
		setGridX(d->mThumbnailSize + d->mMarginSize);
	} else {
		setGridX(d->mThumbnailSize + d->mMarginSize + THUMBNAIL_TEXT_SIZE);
	}
}


void GVFileThumbnailView::setSortingKey(QIconViewItem *iconItem, const KFileItem *item)
{
	// see also setSorting()
	QDir::SortSpec spec = KFileView::sorting();
	bool isDirOrArchive=item->isDir() || GVArchive::fileItemIsArchive(item);

	QString key;
	if ( spec & QDir::Time )
		key=sortingKey( item->time( KIO::UDS_MODIFICATION_TIME ),
								  isDirOrArchive, spec );
	else if ( spec & QDir::Size )
		key=sortingKey( item->size(), isDirOrArchive, spec );

	else // Name or Unsorted
		key=sortingKey( item->text(), isDirOrArchive, spec );

	iconItem->setKey(key);
}


//--------------------------------------------------------------------------
//
// Private slots
//
//--------------------------------------------------------------------------
void GVFileThumbnailView::slotDoubleClicked(QIconViewItem* iconItem) {
	if (!iconItem) return;
	if (KGlobalSettings::singleClick()) return;
	GVFileThumbnailViewItem* thumbItem=static_cast<GVFileThumbnailViewItem*>(iconItem);

	KFileItem* fileItem=thumbItem->fileItem();

	if (fileItem->isDir() || GVArchive::fileItemIsArchive(fileItem)) {
		emit executed(iconItem);
	}
}


void GVFileThumbnailView::slotClicked(QIconViewItem* iconItem) {
	if (!iconItem) return;
	if (!KGlobalSettings::singleClick()) return;
	GVFileThumbnailViewItem* thumbItem=static_cast<GVFileThumbnailViewItem*>(iconItem);

	KFileItem* fileItem=thumbItem->fileItem();

	if (fileItem->isDir() || GVArchive::fileItemIsArchive(fileItem)) {
		emit executed(iconItem);
	}
}

void GVFileThumbnailView::slotContentsMoving( int x, int y ) {
	updateVisibilityInfo( x, y ); // use x,y, the signal is emitted before moving
}

void GVFileThumbnailView::slotCurrentChanged(QIconViewItem*) {
	// trigger generating thumbnails from the current one
	updateVisibilityInfo( contentsX(), contentsY());
}

/**
 * when generating thumbnails, make the current thumbnail
 * to be the next one processed by the thumbnail job, if visible,
 * otherwise use the first visible thumbnail
 */
void GVFileThumbnailView::updateVisibilityInfo( int x, int y ) {
	if (d->mThumbnailLoadJob.isNull()) return;
	
	QRect rect( x, y, visibleWidth(), visibleHeight());
	GVFileThumbnailViewItem* first = static_cast< GVFileThumbnailViewItem* >( findFirstVisibleItem( rect ));
	if (!first) {
		d->mThumbnailLoadJob->setPriorityItems(NULL,NULL,NULL);
		return;
	}
	
	GVFileThumbnailViewItem* last = static_cast< GVFileThumbnailViewItem* >( findLastVisibleItem( rect ));
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

//--------------------------------------------------------------------------
//
// Protected
//
//--------------------------------------------------------------------------
void GVFileThumbnailView::startDrag() {
	KURL::List urls;
	KFileItemListIterator it(*KFileView::selectedItems());

	for ( ; it.current(); ++it ) {
		urls.append(it.current()->url());
	}

	if (urls.isEmpty()) {
		kdWarning() << "No item to drag\n";
		return;
	}

	QDragObject* drag=new KURLDrag(urls, this, 0);
	QPixmap dragPixmap;
	if (urls.count()>1) {
		dragPixmap=SmallIcon("kmultiple");
	} else {
		dragPixmap=KFileView::selectedItems()->getFirst()->pixmap(16);
	}
	drag->setPixmap( dragPixmap, QPoint(dragPixmap.width()/2, dragPixmap.height()/2) );

	drag->dragCopy();
}


//--------------------------------------------------------------------------
//
// Configuration
//
//--------------------------------------------------------------------------
void GVFileThumbnailView::readConfig(KConfig* config,const QString& group) {
	config->setGroup(group);

	d->mThumbnailSize=config->readNumEntry(CONFIG_THUMBNAIL_SIZE, 48);
	d->mMarginSize=config->readNumEntry(CONFIG_MARGIN_SIZE,5);
	int pos=config->readNumEntry(CONFIG_ITEM_TEXT_POS, QIconView::Right);
	setItemTextPos(QIconView::ItemTextPos(pos));

	updateGrid();
	setWordWrapIconText(config->readBoolEntry(CONFIG_WORD_WRAP_FILENAME,false));
	arrangeItemsInGrid();
}

void GVFileThumbnailView::kpartConfig() {
	d->mThumbnailSize=GVThumbnailSize::MAX;
	d->mMarginSize=5;

	updateGrid();
	setWordWrapIconText(false);
	arrangeItemsInGrid();
}


void GVFileThumbnailView::writeConfig(KConfig* config,const QString& group) const {
	config->setGroup(group);
	config->writeEntry(CONFIG_THUMBNAIL_SIZE,d->mThumbnailSize);
	config->writeEntry(CONFIG_MARGIN_SIZE,d->mMarginSize);
	config->writeEntry(CONFIG_WORD_WRAP_FILENAME,wordWrapIconText());
	config->writeEntry(CONFIG_ITEM_TEXT_POS, int(itemTextPos()));
}

