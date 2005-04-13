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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include <qapplication.h>
#include <qcursor.h>
#include <qpoint.h>

#include <kaction.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kfilemetainfo.h>
#include <kiconloader.h>
#include <kimageio.h>
#include <klocale.h>
#include <kparts/browserextension.h>
#include <kparts/genericfactory.h>

#include "gvimagepart.h"
#include <src/gvcache.h>
#include <src/gvdocument.h>
#include <src/gvprintdialog.h>
#include <src/gvscrollpixmapview.h>
#include <src/gvimageloader.h>

#include "config.h"

// For now let's duplicate
const char CONFIG_VIEW_GROUP[]="GwenviewPart View";
const char CONFIG_JPEGTRAN_GROUP[]="jpegtran";
const char CONFIG_CACHE_GROUP[]="cache";


class GVImagePartView : public GVScrollPixmapView {
public:
	GVImagePartView(QWidget* parent, GVDocument* document, KActionCollection* actionCollection, GVImagePartBrowserExtension* browserExtension)
	: GVScrollPixmapView(parent, document, actionCollection), mBrowserExtension(browserExtension)
	{}

protected:
	void openContextMenu(const QPoint&) {
		mBrowserExtension->contextMenu();
	}

private:
	GVImagePartBrowserExtension* mBrowserExtension;
};


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

	mBrowserExtension = new GVImagePartBrowserExtension(this);

	// Create the widgets
	mDocument = new GVDocument(this);
	connect( mDocument, SIGNAL( loaded(const KURL&)), SLOT( loaded(const KURL&)));
	mPixmapView = new GVImagePartView(parentWidget, mDocument, actionCollection(), mBrowserExtension);
	setWidget(mPixmapView);

	mDirLister = new KDirLister;
	mDirLister->setAutoErrorHandlingEnabled( false, 0 );
	mDirLister->setMainWindow( parentWidget->topLevelWidget());
	connect( mDirLister, SIGNAL( clear()), SLOT( dirListerClear()));
	connect( mDirLister, SIGNAL( newItems( const KFileItemList& )),
		SLOT( dirListerNewItems( const KFileItemList& )));
	connect(mDirLister,SIGNAL(deleteItem(KFileItem*)),
		SLOT(dirListerDeleteItem(KFileItem*)) );
	// partially duped from GVFileViewStack::initDirListerFilter()
	QStringList mimeTypes=KImageIO::mimeTypes(KImageIO::Reading);
	mimeTypes.append("image/x-xcf-gimp");
	mimeTypes.append("image/pjpeg"); // KImageIO does not return this one :'(
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
	prefetchDone( false );
	delete mDirLister;
}


void GVImagePart::partActivateEvent(KParts::PartActivateEvent* event) {
	KConfig* config=new KConfig("gwenviewrc");
	if (event->activated()) {
		GVCache::instance()->readConfig(config,CONFIG_CACHE_GROUP);
		mPixmapView->readConfig(config, CONFIG_VIEW_GROUP);
	} else {
		mPixmapView->writeConfig(config, CONFIG_VIEW_GROUP);
	}
	delete config;
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
	} else {
		updateNextPrevious();
	}
	emit setWindowCaption(url.prettyURL());
	return true;
}

QString GVImagePart::filePath() {
	return m_file;
}

void GVImagePart::loaded(const KURL& url) {
	QString caption = url.filename() + QString(" - %1x%2").arg(mDocument->width()).arg(mDocument->height());
	emit setWindowCaption(caption);
	emit completed();
	emit setStatusBarText(i18n("Done."));
	prefetchDone( false );
	mPrefetch = GVImageLoader::loader( mLastDirection == DirectionPrevious ? previousURL() : nextURL());
	connect( mPrefetch, SIGNAL( imageLoaded( bool )), SLOT( prefetchDone( bool )));
}

void GVImagePart::prefetchDone( bool ) {
	if( mPrefetch != NULL ) { 
		mPrefetch->disconnect( this );
		mPrefetch->release();
	}
	mPrefetch = NULL;
}

void GVImagePart::print() {
	KPrinter printer;

	printer.setDocName( m_url.filename() );
	KPrinter::addDialogPage( new GVPrintDialogPage( mDocument, mPixmapView, "GV page"));

	if (printer.setup(mPixmapView, QString::null, true)) {
		mDocument->print(&printer);
	}
}

void GVImagePart::rotateRight() {
	mDocument->transform(GVImageUtils::ROT_90);
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
	KParts::URLArgs args;        // Prevent adding all images to history, it feels
	args.setLockHistory( true ); // better that way when finally going back.
	mBrowserExtension->openURLRequest( newURL, args );
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
	KParts::URLArgs args;
	args.setLockHistory( true );
	mBrowserExtension->openURLRequest( newURL, args );
}

/***** GVImagePartBrowserExtension *****/

GVImagePartBrowserExtension::GVImagePartBrowserExtension(GVImagePart* viewPart, const char* name)
	:KParts::BrowserExtension(viewPart, name) {
	mGVImagePart = viewPart;
	emit enableAction("print", true );
}

GVImagePartBrowserExtension::~GVImagePartBrowserExtension() {
}

void GVImagePartBrowserExtension::contextMenu() {
	/*FIXME Why is this KFileMetaInfo invalid?
	KFileMetaInfo metaInfo = KFileMetaInfo(mGVImagePart->filePath());
	kdDebug() << k_funcinfo << "mGVImagePart->filePath(): " << mGVImagePart->filePath() << endl;
	kdDebug() << k_funcinfo << "metaInfo.isValid(): " << metaInfo.isValid() << endl;
	kdDebug() << k_funcinfo << "above" << endl;
	QString mimeType = metaInfo.mimeType();
	kdDebug() << k_funcinfo << "below" << endl;
	emit popupMenu(QCursor::pos(), mGVImagePart->url(), mimeType);
	*/
	emit popupMenu(QCursor::pos(), mGVImagePart->url(), 0);
}

void GVImagePartBrowserExtension::print() {
	mGVImagePart->print();
}

#include "gvimagepart.moc"
