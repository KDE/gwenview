/*
Copyright 2004 Jonathan Riddell <jr@jriddell.org>

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
Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.

*/
#include "gvimagepart.moc"

#include <qapplication.h>
#include <qcursor.h>
#include <qpoint.h>

#include <kaction.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kparts/genericfactory.h>

#include <gvcore/cache.h>
#include <gvcore/document.h>
#include <gvcore/printdialog.h>
#include <gvcore/imageview.h>
#include <gvcore/imageloader.h>
#include <gvcore/mimetypeutils.h>

#include "config.h"

namespace Gwenview {
// For now let's duplicate
const char CONFIG_CACHE_GROUP[]="cache";

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif


//Factory Code
typedef KParts::GenericFactory<GVImagePart> GVImageFactory;
K_EXPORT_COMPONENT_FACTORY( libgvimagepart /*library name*/, GVImageFactory )

GVImagePart::GVImagePart(QWidget* parentWidget, const char* /*widgetName*/, QObject* parent,
			 const char* name, const QStringList &)
	: KParts::ReadOnlyPart( parent, name )
	, mPrefetch( NULL )
	, mLastDirection( DirectionUnknown )  {
	GVImageFactory::instance()->iconLoader()->addAppDir( "gwenview");
	setInstance( GVImageFactory::instance() );
	KGlobal::locale()->insertCatalogue( "gwenview" );

	Cache::instance()->ref();

	mBrowserExtension = new GVImagePartBrowserExtension(this);

	// Create the widgets
	mDocument = new Document(this);
	connect( mDocument, SIGNAL( loading()), SLOT( slotLoading()));
	connect( mDocument, SIGNAL( loaded(const KURL&)), SLOT( slotLoaded(const KURL&)));
	mImageView = new ImageView(parentWidget, mDocument, actionCollection());
	connect( mImageView, SIGNAL(requestContextMenu(const QPoint&)),
		mBrowserExtension, SLOT(openContextMenu(const QPoint&)) );
	setWidget(mImageView);

	mDirLister = new KDirLister;
	mDirLister->setAutoErrorHandlingEnabled( false, 0 );
	mDirLister->setMainWindow( parentWidget->topLevelWidget());
	connect( mDirLister, SIGNAL( clear()), SLOT( dirListerClear()));
	connect( mDirLister, SIGNAL( newItems( const KFileItemList& )),
		SLOT( dirListerNewItems( const KFileItemList& )));
	connect(mDirLister,SIGNAL(deleteItem(KFileItem*)),
		SLOT(dirListerDeleteItem(KFileItem*)) );
	
	QStringList mimeTypes=MimeTypeUtils::rasterImageMimeTypes();
	mDirLister->setMimeFilter(mimeTypes);
	mPreviousImage=new KAction(i18n("&Previous Image"),
		QApplication::reverseLayout() ? "1rightarrow":"1leftarrow", Key_BackSpace,
		this,SLOT(slotSelectPrevious()), actionCollection(), "previous");
	mNextImage=new KAction(i18n("&Next Image"),
		QApplication::reverseLayout() ? "1leftarrow":"1rightarrow", Key_Space,
		this,SLOT(slotSelectNext()), actionCollection(), "next");
	updateNextPrevious();

	KStdAction::saveAs( mDocument, SLOT(saveAs()), actionCollection(), "saveAs" );
	new KAction(i18n("Rotate &Right"), "rotate_cw", CTRL + Key_R, this, SLOT(rotateRight()), actionCollection(), "rotate_right");

	setXMLFile( "gvimagepart/gvimagepart.rc" );
}

GVImagePart::~GVImagePart() {
	Cache::instance()->deref();
	delete mDirLister;
}


void GVImagePart::partActivateEvent(KParts::PartActivateEvent* event) {
	if (event->activated()) {
		KConfig* config=new KConfig("gwenviewrc");
		Cache::instance()->readConfig(config,CONFIG_CACHE_GROUP);
		delete config;
	}
	KParts::ReadOnlyPart::partActivateEvent( event );
}


KAboutData* GVImagePart::createAboutData() {
	KAboutData* aboutData = new KAboutData( "gvimagepart", I18N_NOOP("GVImagePart"),
						"0.1", I18N_NOOP("Image Viewer"),
						KAboutData::License_GPL,
						"(c) 2004, Jonathan Riddell <jr@jriddell.org>");
	return aboutData;
}

bool GVImagePart::openURL(const KURL& url) {
	if (!url.isValid())  {
		return false;
	}
	KURL oldURLDir = m_url;
	oldURLDir.setFileName( QString::null );
	KURL newURLDir = url;
	newURLDir.setFileName( QString::null );
	bool sameDir = oldURLDir == newURLDir;
	m_url = url;
	emit started( 0 );
	if( mDocument->url() == url ) { // reload button in Konqy - setURL would return immediately
		mDocument->reload();
	} else {
		mDocument->setURL(url);
	}
	if( !sameDir ) {
		mDirLister->openURL(mDocument->dirURL());
		mLastDirection = DirectionUnknown;
	}
	return true;
}

QString GVImagePart::filePath() {
	return m_file;
}

void GVImagePart::slotLoading() {
	emit setWindowCaption(mDocument->url().filename() + " - " + i18n("Loading..."));
	// Set the location bar URL because we can arrive here if the user click on
	// previous/next, which do not use openURLRequest
	emit mBrowserExtension->setLocationBarURL(mDocument->url().pathOrURL());
	updateNextPrevious();
}

void GVImagePart::slotLoaded(const KURL& url) {
	QString caption = url.filename() + QString(" - %1x%2").arg(mDocument->width()).arg(mDocument->height());
	emit setWindowCaption(caption);
	emit completed();
	emit setStatusBarText(i18n("Done."));
	prefetchDone();
	mPrefetch = ImageLoader::loader( mLastDirection == DirectionPrevious ? previousURL() : nextURL(),
		this, BUSY_PRELOADING );
	connect( mPrefetch, SIGNAL( imageLoaded( bool )), SLOT( prefetchDone()));
}

void GVImagePart::prefetchDone() {
	if( mPrefetch != NULL ) { 
		mPrefetch->release( this );
	}
	mPrefetch = NULL;
}

void GVImagePart::print() {
	KPrinter printer;

	printer.setDocName( m_url.filename() );
	KPrinter::addDialogPage( new PrintDialogPage( mDocument, mImageView, "GV page"));

	if (printer.setup(mImageView, QString::null, true)) {
		mDocument->print(&printer);
	}
}

void GVImagePart::rotateRight() {
	mDocument->transform(ImageUtils::ROT_90);
}

void GVImagePart::dirListerClear() {
	mImagesInDirectory.clear();
	updateNextPrevious();
}

void GVImagePart::dirListerNewItems( const KFileItemList& list ) {
	QPtrListIterator<KFileItem> it(list);
	for( ; it.current(); ++it ) {
		mImagesInDirectory.append( (*it)->name());
	}
	qHeapSort( mImagesInDirectory );
	updateNextPrevious();
}

void GVImagePart::dirListerDeleteItem( KFileItem* item ) {
	mImagesInDirectory.remove( item->name());
	updateNextPrevious();
}

void GVImagePart::updateNextPrevious() {
	QStringList::ConstIterator current = mImagesInDirectory.find( mDocument->filename());
	if( current == mImagesInDirectory.end()) {
		mNextImage->setEnabled( false );
		mPreviousImage->setEnabled( false );
		return;
	}
	mPreviousImage->setEnabled( current != mImagesInDirectory.begin());
	++current;
	mNextImage->setEnabled( current != mImagesInDirectory.end());
}

KURL GVImagePart::nextURL() const {
	QStringList::ConstIterator current = mImagesInDirectory.find( mDocument->filename());
	if( current == mImagesInDirectory.end()) {
		return KURL();
	}
	++current;
	if( current == mImagesInDirectory.end()) {
		return KURL();
	}
	KURL newURL = mDocument->dirURL();
	newURL.setFileName( *current );
	return newURL;
}

void GVImagePart::slotSelectNext() {
	KURL newURL = nextURL();
	if( newURL.isEmpty()) return;
	mLastDirection = DirectionNext;
	// Do not use mBrowserExtension->openURLRequest to avoid switching to
	// another KPart
	openURL(newURL);
	emit mBrowserExtension->openURLNotify();
}

KURL GVImagePart::previousURL() const {
	QStringList::ConstIterator current = mImagesInDirectory.find( mDocument->filename());
	if( current == mImagesInDirectory.end() || current == mImagesInDirectory.begin()) {
		return KURL();
	}
	--current;
	KURL newURL = mDocument->dirURL();
	newURL.setFileName( *current );
	return newURL;
}

void GVImagePart::slotSelectPrevious() {
	KURL newURL = previousURL();
	if( newURL.isEmpty()) return;
	mLastDirection = DirectionPrevious;
	openURL(newURL);
	emit mBrowserExtension->openURLNotify();
}

/***** GVImagePartBrowserExtension *****/

GVImagePartBrowserExtension::GVImagePartBrowserExtension(GVImagePart* viewPart, const char* name)
	:KParts::BrowserExtension(viewPart, name) {
	mGVImagePart = viewPart;
	emit enableAction("print", true );
}

GVImagePartBrowserExtension::~GVImagePartBrowserExtension() {
}

void GVImagePartBrowserExtension::openContextMenu(const QPoint& pos) {
	KURL url=mGVImagePart->url();
	QString mimeType=KMimeType::findByURL(url)->name();
	KFileItem item(url, mimeType, S_IFREG);
	KFileItemList list;
	list.append(&item);
	emit popupMenu(pos, list);
}

void GVImagePartBrowserExtension::print() {
	mGVImagePart->print();
}

} // namespace
