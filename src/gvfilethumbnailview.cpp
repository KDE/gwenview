// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2003 Aurélien Gâteau

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
#include <qlayout.h>
#include <qpainter.h>
#include <qpen.h>
#include <qpixmap.h>
#include <qpixmapcache.h>
#include <qpushbutton.h>
#include <qtimer.h>

// KDE 
#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kurldrag.h>
#include <kwordwrap.h>

// Local 
#include "gvfilethumbnailviewitem.h"
#include "gvarchive.h"
#include "thumbnailloadjob.h"

#include "gvfilethumbnailview.moc"

static const char* CONFIG_THUMBNAIL_SIZE="thumbnail size";
static const char* CONFIG_MARGIN_SIZE="margin size";
static const char* CONFIG_WORD_WRAP_FILENAME="word wrap filename";


GVFileThumbnailView::GVFileThumbnailView(QWidget* parent)
: KIconView(parent), GVFileViewBase(), mUpdateThumbnailsOnNextShow(false), mThumbnailLoadJob(0L)
{
	setAutoArrange(true);
	QIconView::setSorting(true);
	setItemsMovable(false);
	setResizeMode(Adjust);
	setShowToolTips(true);
	setSpacing(0);
	setAcceptDrops(true);
		
	mWaitPixmap=QPixmap(::locate("appdata", "thumbnail/wait.png"));

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
	
	QIconView::setSelectionMode(Extended);
}


GVFileThumbnailView::~GVFileThumbnailView() {
	stopThumbnailUpdate();
}


void GVFileThumbnailView::setThumbnailSize(ThumbnailSize value) {
	if (value==mThumbnailSize) return;
	mThumbnailSize=value;
	updateGrid();
}


void GVFileThumbnailView::setMarginSize(int value) {
	if (value==mMarginSize) return;
	mMarginSize=value;
	updateGrid();
}


void GVFileThumbnailView::setThumbnailPixmap(const KFileItem* fileItem,const QPixmap& thumbnail) {
	GVFileThumbnailViewItem* iconItem=viewItem(fileItem);
	if (!iconItem) return;

	int pixelSize=mThumbnailSize.pixelSize();

	// Draw the thumbnail to the center of the icon
	QPainter painter(iconItem->pixmap());
	painter.eraseRect(0,0,pixelSize,pixelSize);
	painter.drawPixmap(
		(pixelSize-thumbnail.width())/2,
		(pixelSize-thumbnail.height())/2,
		thumbnail);
	iconItem->repaint();

	// Notify others that one thumbnail has been updated
	emit updatedOneThumbnail();
}




void GVFileThumbnailView::setShownFileItem(KFileItem* fileItem) {
	GVFileThumbnailViewItem* oldShownItem=viewItem(mShownFileItem);
	GVFileThumbnailViewItem* newShownItem=viewItem(fileItem);

	GVFileViewBase::setShownFileItem(fileItem);
	if (oldShownItem) repaintItem(oldShownItem);
	if (newShownItem) repaintItem(newShownItem);
}


//-----------------------------------------------------------------------------
//
// Thumbnail code
//
//-----------------------------------------------------------------------------
void GVFileThumbnailView::startThumbnailUpdate() {
	// Delay thumbnail update if the widget is not visible
	if (!isVisible()) {
		mUpdateThumbnailsOnNextShow=true;
		return;
	}
	mUpdateThumbnailsOnNextShow=false;
	stopThumbnailUpdate(); // just in case
	doStartThumbnailUpdate(items());
}


void GVFileThumbnailView::doStartThumbnailUpdate(const KFileItemList* list) {
	mThumbnailLoadJob = new ThumbnailLoadJob(list, mThumbnailSize);

	connect(mThumbnailLoadJob, SIGNAL(thumbnailLoaded(const KFileItem*,const QPixmap&)),
		this, SLOT(setThumbnailPixmap(const KFileItem*,const QPixmap&)) );
	connect(mThumbnailLoadJob, SIGNAL(result(KIO::Job*)),
		this, SIGNAL(updateEnded()) );

	emit updateStarted(list->count());
	mThumbnailLoadJob->start();
}


void GVFileThumbnailView::stopThumbnailUpdate() {
	if (!mThumbnailLoadJob.isNull()) {
		emit updateEnded();
		mThumbnailLoadJob->kill();
	}
}


void GVFileThumbnailView::updateThumbnail(const KFileItem* fileItem) {

	ThumbnailLoadJob::deleteImageThumbnail(fileItem->url());
	if (mThumbnailLoadJob.isNull()) {
		KFileItemList list;
		list.append(fileItem);
		doStartThumbnailUpdate(&list);
	} else {
		mThumbnailLoadJob->appendItem(fileItem);
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

	bool isDirOrArchive=item->isDir() || GVArchive::fileItemIsArchive(item);

	int pixelSize=mThumbnailSize.pixelSize();
	QPixmap thumbnail(pixelSize,pixelSize);
	QPainter painter(&thumbnail);
	painter.eraseRect(0,0,pixelSize,pixelSize);

	if (isDirOrArchive) {
		// Load the icon
		QPixmap itemPix=item->pixmap(pixelSize);
		painter.drawPixmap(
			(pixelSize-itemPix.width())/2,
			(pixelSize-itemPix.height())/2,
			itemPix);
	} else {
		// Create an empty thumbnail
		painter.setPen(colorGroup().button());
		painter.drawRect(0,0,pixelSize,pixelSize);
		painter.drawPixmap(
			(pixelSize-mWaitPixmap.width())/2,
			(pixelSize-mWaitPixmap.height())/2,
			mWaitPixmap);
	}

	// Create icon item
	GVFileThumbnailViewItem* iconItem=new GVFileThumbnailViewItem(this,item->text(),thumbnail,item);
	iconItem->setDropEnabled(isDirOrArchive);

	setSortingKey(iconItem, item);
	item->setExtraData(this,iconItem);
}


void GVFileThumbnailView::updateView(const KFileItem* fileItem) {
	if (!fileItem) return;

	GVFileThumbnailViewItem* iconItem=viewItem(fileItem);
	if (iconItem) {
		iconItem->setText(fileItem->text());
		updateThumbnail(fileItem);
	}
	sort();
}


void GVFileThumbnailView::ensureItemVisible(const KFileItem* fileItem) {
	if (!fileItem) return;

	GVFileThumbnailViewItem* iconItem=viewItem(fileItem);
	if (iconItem) QIconView::ensureItemVisible(iconItem);
}


void GVFileThumbnailView::setCurrentItem(const KFileItem* fileItem) {
	if (!fileItem) return;

	GVFileThumbnailViewItem* iconItem=viewItem(fileItem);
	if (iconItem) QIconView::setCurrentItem(iconItem);
}


void GVFileThumbnailView::setSelected(const KFileItem* fileItem,bool enable) {
	if (!fileItem) return;

	GVFileThumbnailViewItem* iconItem=viewItem(fileItem);
	if (iconItem) QIconView::setSelected(iconItem,enable);
}


bool GVFileThumbnailView::isSelected(const KFileItem* fileItem) const {
	if (!fileItem) return false;

	GVFileThumbnailViewItem* iconItem=viewItem(fileItem);
	if (!iconItem) return false;

	return iconItem->isSelected();
}


void GVFileThumbnailView::removeItem(const KFileItem* fileItem) {
	if (!fileItem) return;

	// Remove it from the image preview job
	if (!mThumbnailLoadJob.isNull())
		mThumbnailLoadJob->itemRemoved(fileItem);

	if (fileItem==mShownFileItem) mShownFileItem=0L;

	// Remove it from our view
	GVFileThumbnailViewItem* iconItem=viewItem(fileItem);
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
	const GVFileThumbnailViewItem* iconItem=viewItem(fileItem);
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
	const GVFileThumbnailViewItem* iconItem=viewItem(fileItem);
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
		QIconViewItem* iconItem=viewItem(item);
		if (iconItem) setSortingKey(iconItem, item);
	}

	KIconView::sort();
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
	if (!mUpdateThumbnailsOnNextShow) return;

	mUpdateThumbnailsOnNextShow=false;
	QTimer::singleShot(0, this, SLOT(startThumbnailUpdate()));
}


//--------------------------------------------------------------------------
//
// Private
// 
//--------------------------------------------------------------------------
void GVFileThumbnailView::updateGrid() {
	setGridX(mThumbnailSize.pixelSize() + mMarginSize);
}


void GVFileThumbnailView::setSortingKey(QIconViewItem *iconItem, const KFileItem *item)
{
	// see also setSorting()
	QDir::SortSpec spec = KFileView::sorting();
	bool isDirOrArchive=item->isDir() || GVArchive::fileItemIsArchive(item);

	if ( spec & QDir::Time )
		iconItem->setKey( sortingKey( item->time( KIO::UDS_MODIFICATION_TIME ),
								  isDirOrArchive, spec ));
	else if ( spec & QDir::Size )
		iconItem->setKey( sortingKey( item->size(), isDirOrArchive, spec ));

	else // Name or Unsorted
		iconItem->setKey( sortingKey( item->text(), isDirOrArchive, spec ));
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
	if (!fileItem) return;

	if (fileItem->isDir() || GVArchive::fileItemIsArchive(fileItem)) {
		emit executed(iconItem);
	}
}


void GVFileThumbnailView::slotClicked(QIconViewItem* iconItem) {
	if (!iconItem) return;
	if (!KGlobalSettings::singleClick()) return;
	GVFileThumbnailViewItem* thumbItem=static_cast<GVFileThumbnailViewItem*>(iconItem);

	KFileItem* fileItem=thumbItem->fileItem();
	if (!fileItem) return;

	if (fileItem->isDir() || GVArchive::fileItemIsArchive(fileItem)) {
		emit executed(iconItem);
	}
}

/**
 * when generating thumbnails, make the first visible (not loaded yet)
 * thumbnail be the next one processed by the thumbnail job
 */
void GVFileThumbnailView::slotContentsMoving( int x, int y ) {
	if (mThumbnailLoadJob.isNull()) return;
	QRect r( x, y, visibleWidth(), visibleHeight());
	GVFileThumbnailViewItem* pos = static_cast< GVFileThumbnailViewItem* >( findFirstVisibleItem( r ));
	GVFileThumbnailViewItem* last = static_cast< GVFileThumbnailViewItem* >( findLastVisibleItem( r ));
	while( pos != NULL ) {
		KFileItem* fileItem = pos->fileItem();
		if( fileItem ) {
			if( mThumbnailLoadJob->setNextItem(fileItem)) return;
		}
		if( pos == last ) break;
		pos = static_cast< GVFileThumbnailViewItem* >( pos->nextItem());
	}
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

	QDragObject* drag=KURLDrag::newDrag(urls, this);
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

	mThumbnailSize=config->readEntry(CONFIG_THUMBNAIL_SIZE);
	mMarginSize=config->readNumEntry(CONFIG_MARGIN_SIZE,5);

	updateGrid();
	setWordWrapIconText(config->readBoolEntry(CONFIG_WORD_WRAP_FILENAME,false));
	arrangeItemsInGrid();
}


void GVFileThumbnailView::writeConfig(KConfig* config,const QString& group) const {
	config->setGroup(group);
	config->writeEntry(CONFIG_THUMBNAIL_SIZE,QString(mThumbnailSize));
	config->writeEntry(CONFIG_MARGIN_SIZE,mMarginSize);
	config->writeEntry(CONFIG_WORD_WRAP_FILENAME,wordWrapIconText());
}

