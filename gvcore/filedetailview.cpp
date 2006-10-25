// vim: set tabstop=4 shiftwidth=4 noexpandtab
/* This file is based on kfiledetailview.cpp v1.43 from the KDE libs. Original
   copyright follows.
*/
/* This file is part of the KDE libraries
   Copyright (C) 1997 Stephan Kulow <coolo@kde.org>
                 2000, 2001 Carsten Pfeiffer <pfeiffer@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.	If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

// Qt
#include <qbitmap.h>
#include <qevent.h>
#include <qheader.h>
#include <qkeycode.h>
#include <qpainter.h>
#include <qpixmap.h>

// KDE 
#include <kapplication.h>
#include <kdebug.h>
#include <kfileitem.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kurldrag.h>
#include <kwordwrap.h>

// Local
#include "archive.h"
#include "dragpixmapgenerator.h"
#include "filedetailviewitem.h"
#include "filedetailview.moc"
#include "timeutils.h"
namespace Gwenview {


static QPixmap createShownItemPixmap(int size, const QColor& color) {
	QPixmap pix(size, size);
	pix.fill(Qt::red);
	QPainter painter(&pix);
	int margin = 2;
	
	QPointArray pa(3);
	int arrowSize = size/2 - margin;
	int center = size/2 - 1;
	pa[0] = QPoint((size - arrowSize) / 2, center - arrowSize);
	pa[1] = QPoint((size + arrowSize) / 2, center);
	pa[2] = QPoint(pa[0].x(),              center + arrowSize);

	painter.setBrush(color);
	painter.setPen(color);
	painter.drawPolygon(pa);
	painter.end();

	pix.setMask(pix.createHeuristicMask());
	
	return pix;
}


FileDetailView::FileDetailView(QWidget *parent, const char *name)
	: KListView(parent, name), FileViewBase()
{
	mSortingCol = COL_NAME;
	mBlockSortingSignal = false;

	addColumn( i18n( "Name" ) );
	addColumn( i18n( "Size" ) );
	addColumn( i18n( "Date" ) );
	addColumn( i18n( "Permissions" ) );
	addColumn( i18n( "Owner" ) );
	addColumn( i18n( "Group" ) );
	setShowSortIndicator( TRUE );
	setAllColumnsShowFocus( TRUE );

	connect( header(), SIGNAL( sectionClicked(int)),
			 SLOT(slotSortingChanged(int) ));


	connect( this, SIGNAL( returnPressed(QListViewItem *) ),
			 SLOT( slotActivate( QListViewItem *) ) );

	connect( this, SIGNAL( clicked(QListViewItem *, const QPoint&, int)),
			 SLOT( selected( QListViewItem *) ) );
	connect( this, SIGNAL( doubleClicked(QListViewItem *, const QPoint&, int)),
			 SLOT( slotActivate( QListViewItem *) ) );

	connect( this, SIGNAL(contextMenuRequested( QListViewItem *,
												const QPoint &, int )),
			 this, SLOT( slotActivateMenu( QListViewItem *, const QPoint& )));

	QListView::setSelectionMode( QListView::Extended );
	connect( this, SIGNAL( selectionChanged() ),
			 SLOT( slotSelectionChanged() ));

	// FileViewStack need to be aware of sort changes, to update the sort menu
	connect( sig, SIGNAL(sortingChanged(QDir::SortSpec)),
		this, SIGNAL(sortingChanged(QDir::SortSpec)) );
	
	setSorting( sorting() );


	mResolver =
		new KMimeTypeResolver<FileDetailViewItem,FileDetailView>( this );

	setDragEnabled(true);
	
	setAcceptDrops(true);
	setDropVisualizer(false);
	setDropHighlighter(false);

	int size = IconSize(KIcon::Small);
	mShownItemUnselectedPixmap = createShownItemPixmap(size, colorGroup().highlight());
	mShownItemSelectedPixmap = createShownItemPixmap(size, colorGroup().highlightedText());
}


FileDetailView::~FileDetailView()
{
	delete mResolver;
}


void FileDetailView::setSelected( const KFileItem *info, bool enable )
{
	if (!info) return;
	FileDetailViewItem *item = viewItem(info);
	if (item) KListView::setSelected(item, enable);
}

void FileDetailView::setCurrentItem( const KFileItem *item )
{
	if (!item) return;
	FileDetailViewItem *listItem = viewItem(item);
	if (listItem) KListView::setCurrentItem(listItem);
}

KFileItem * FileDetailView::currentFileItem() const
{
	FileDetailViewItem *current = static_cast<FileDetailViewItem*>( currentItem() );
	if ( current ) return current->fileInfo();

	return 0L;
}

void FileDetailView::clearSelection()
{
	KListView::clearSelection();
}

void FileDetailView::selectAll()
{
	KListView::selectAll( true );
}

void FileDetailView::invertSelection()
{
	KListView::invertSelection();
}

void FileDetailView::slotActivateMenu (QListViewItem *item,const QPoint& pos )
{
	if ( !item ) {
		sig->activateMenu( 0, pos );
		return;
	}
	FileDetailViewItem *i = (FileDetailViewItem*) item;
	sig->activateMenu( i->fileInfo(), pos );
}

void FileDetailView::clearView()
{
	mResolver->m_lstPendingMimeIconItems.clear();
	mShownFileItem=0L;
	KListView::clear();
}

void FileDetailView::insertItem( KFileItem *i )
{
	KFileView::insertItem( i );

	FileDetailViewItem *item = new FileDetailViewItem( (QListView*) this, i );

	setSortingKey( item, i );

	i->setExtraData( this, item );

	if ( !i->isMimeTypeKnown() )
		mResolver->m_lstPendingMimeIconItems.append( item );
}

void FileDetailView::slotActivate( QListViewItem *item )
{
	if ( !item ) return;

	const KFileItem *fi = ( (FileDetailViewItem*)item )->fileInfo();
	if ( fi ) sig->activate( fi );
}

void FileDetailView::selected( QListViewItem *item )
{
	if ( !item ) return;

	if ( KGlobalSettings::singleClick() ) {
		const KFileItem *fi = ( (FileDetailViewItem*)item )->fileInfo();
		if ( fi && (fi->isDir() || !onlyDoubleClickSelectsFiles()) )
			sig->activate( fi );
	}
}

void FileDetailView::highlighted( QListViewItem *item )
{
	if ( !item ) return;

	const KFileItem *fi = ( (FileDetailViewItem*)item )->fileInfo();
	if ( fi ) sig->highlightFile( fi );
}


bool FileDetailView::isSelected(const KFileItem* fileItem) const
{
	if (!fileItem) return false;

	FileDetailViewItem *item = viewItem(fileItem);
	return item && item->isSelected();
}


void FileDetailView::updateView( bool b )
{
	if ( !b ) return;

	QListViewItemIterator it( (QListView*)this );
	for ( ; it.current(); ++it ) {
		FileDetailViewItem *item=static_cast<FileDetailViewItem *>(it.current());
		item->setPixmap( 0, item->fileInfo()->pixmap(KIcon::SizeSmall) );
	}
}

void FileDetailView::updateView( const KFileItem *i )
{
	if ( !i ) return;

	FileDetailViewItem *item = viewItem(i);
	if ( !item ) return;

	item->init();
	setSortingKey( item, i );
}


void FileDetailView::setSortingKey( FileDetailViewItem *dvItem, const KFileItem *item)
{
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

	dvItem->setKey(key);
}


void FileDetailView::removeItem( const KFileItem *i )
{
	if ( !i ) return;

	FileDetailViewItem *item = viewItem(i);
	mResolver->m_lstPendingMimeIconItems.remove( item );
	if(mShownFileItem==i) mShownFileItem=0L;
	delete item;

	KFileView::removeItem( i );
}

void FileDetailView::slotSortingChanged( int col )
{
	QDir::SortSpec sort = sorting();
	int sortSpec = -1;
	bool reversed = col == mSortingCol && (sort & QDir::Reversed) == 0;
	mSortingCol = col;

	switch( col ) {
	case COL_NAME:
		sortSpec = (sort & ~QDir::SortByMask | QDir::Name);
		break;
	case COL_SIZE:
		sortSpec = (sort & ~QDir::SortByMask | QDir::Size);
		break;
	case COL_DATE:
		sortSpec = (sort & ~QDir::SortByMask | QDir::Time);
		break;

	// the following columns have no equivalent in QDir, so we set it
	// to QDir::Unsorted and remember the column (mSortingCol)
	case COL_OWNER:
	case COL_GROUP:
	case COL_PERM:
		// grmbl, QDir::Unsorted == SortByMask.
		sortSpec = (sort & ~QDir::SortByMask);// | QDir::Unsorted;
		break;
	default:
		break;
	}

	if ( reversed )
		sortSpec |= QDir::Reversed;
	else
		sortSpec &= ~QDir::Reversed;

	if ( sort & QDir::IgnoreCase )
		sortSpec |= QDir::IgnoreCase;
	else
		sortSpec &= ~QDir::IgnoreCase;


	KFileView::setSorting( static_cast<QDir::SortSpec>( sortSpec ) );

	KFileItem *item;
	KFileItemListIterator it( *items() );

	for ( ; (item = it.current() ); ++it ) {
		FileDetailViewItem* thumbItem=viewItem( item );
		if (thumbItem) setSortingKey(thumbItem,item);
	}

	KListView::setSorting( mSortingCol, !reversed );
	KListView::sort();

	if (!mBlockSortingSignal) sig->changeSorting( static_cast<QDir::SortSpec>( sortSpec ) );
}


void FileDetailView::setSorting( QDir::SortSpec spec )
{
	int col = 0;
	if ( spec & QDir::Time )
		col = COL_DATE;
	else if ( spec & QDir::Size )
		col = COL_SIZE;
	else if ( spec & QDir::Unsorted )
		col = mSortingCol;
	else
		col = COL_NAME;

	// inversed, because slotSortingChanged will reverse it
	if ( spec & QDir::Reversed )
		spec = (QDir::SortSpec) (spec & ~QDir::Reversed);
	else
		spec = (QDir::SortSpec) (spec | QDir::Reversed);

	mSortingCol = col;
	KFileView::setSorting( (QDir::SortSpec) spec );


	// don't emit sortingChanged() when called via setSorting()
	mBlockSortingSignal = true; // can't use blockSignals()
	slotSortingChanged( col );
	mBlockSortingSignal = false;
}

void FileDetailView::ensureItemVisible( const KFileItem *i )
{
	if ( !i ) return;

	FileDetailViewItem *item = viewItem(i);
		
	if ( item ) KListView::ensureItemVisible( item );
}

// we're in multiselection mode
void FileDetailView::slotSelectionChanged()
{
	sig->highlightFile( 0L );
}

KFileItem * FileDetailView::firstFileItem() const
{
	FileDetailViewItem *item = static_cast<FileDetailViewItem*>( firstChild() );
	if ( item ) return item->fileInfo();
	return 0L;
}

KFileItem * FileDetailView::nextItem( const KFileItem *fileItem ) const
{
	if ( fileItem ) {
		FileDetailViewItem *item = viewItem( fileItem );
		if ( item && item->itemBelow() )
			return ((FileDetailViewItem*) item->itemBelow())->fileInfo();
		else
			return 0L;
	}
	else
		return firstFileItem();
}

KFileItem * FileDetailView::prevItem( const KFileItem *fileItem ) const
{
	if ( fileItem ) {
		FileDetailViewItem *item = viewItem( fileItem );
		if ( item && item->itemAbove() )
			return ((FileDetailViewItem*) item->itemAbove())->fileInfo();
		else
			return 0L;
	}
	else
		return firstFileItem();
}

void FileDetailView::keyPressEvent( QKeyEvent *e )
{
	KListView::keyPressEvent( e );

	if ( e->key() == Key_Return || e->key() == Key_Enter ) {
		if ( e->state() & ControlButton )
			e->ignore();
		else
			e->accept();
	}
}

//
// mimetype determination on demand
//
void FileDetailView::mimeTypeDeterminationFinished()
{
	// anything to do?
}

void FileDetailView::determineIcon( FileDetailViewItem *item )
{
	(void) item->fileInfo()->determineMimeType();
	updateView( item->fileInfo() );
}

void FileDetailView::listingCompleted()
{
	mResolver->start();
}

void FileDetailView::startDrag()
{
	/**
	 * The item drawer for DragPixmapGenerator
	 */
	struct ItemDrawer : public DragPixmapItemDrawer<KFileItem*> {
		ItemDrawer(const QFontMetrics& fontMetrics)
		: mFontMetrics(fontMetrics) {}

		QSize itemSize(KFileItem* fileItem) {
			if (!fileItem) return QSize();
			QString name = fileItem->name();
			int width = QMIN(mGenerator->maxWidth(), mFontMetrics.width(name));
			int height = mFontMetrics.height();
			return QSize(width, height);
		}
				
		void drawItem(QPainter* painter, int left, int top, KFileItem* fileItem) {
			QString name = fileItem->name();
			painter->save();
			KWordWrap::drawFadeoutText(painter, 
				left, top + mFontMetrics.ascent(), 
				mGenerator->maxWidth(), name);
			painter->restore();
		}
		
		QFontMetrics mFontMetrics;
	};
	ItemDrawer drawer(fontMetrics());


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

	drag->setPixmap( dragPixmap, QPoint(-generator.DRAG_OFFSET, -generator.DRAG_OFFSET));
	drag->dragCopy();
}


void FileDetailView::setShownFileItem(KFileItem* fileItem)
{
	if( fileItem == mShownFileItem ) return;
	FileDetailViewItem* oldShownItem=viewItem(mShownFileItem);
	FileDetailViewItem* newShownItem=viewItem(fileItem);
	
	FileViewBase::setShownFileItem(fileItem);
	if (oldShownItem) oldShownItem->repaint();
	if (newShownItem) newShownItem->repaint();
}


//----------------------------------------------------------------------
//
// Drop support
//
//----------------------------------------------------------------------
bool FileDetailView::acceptDrag(QDropEvent* event) const {
	return KURLDrag::canDecode(event);
}

void FileDetailView::contentsDropEvent(QDropEvent *event) {
	KFileItem* fileItem=0L;
	QListViewItem *item=itemAt(contentsToViewport(event->pos() ) );
	
	if (item) {
		fileItem=static_cast<FileDetailViewItem*>(item)->fileInfo();
	}
	emit dropped(event,fileItem);
}

} // namespace
