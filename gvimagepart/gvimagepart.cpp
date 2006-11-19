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
#include <qfile.h>
#include <qpoint.h>

#include <kaction.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kfiledialog.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetype.h>
#include <kparts/genericfactory.h>

#include <gvcore/cache.h>
#include <gvcore/document.h>
#include <gvcore/fileoperation.h>
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
#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif


static bool storeData(QWidget* parent, QFile* file, const QByteArray& data) {
	uint sizeWritten = file->writeBlock(data);
	if (sizeWritten != data.size()) {
		KMessageBox::error(
			parent,
			i18n("Could not save image to a temporary file"));
		return false;
	}
	return true;
}


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
	mDocument = new Document(this);
	connect( mDocument, SIGNAL( loading()), SLOT( slotLoading()));
	connect( mDocument, SIGNAL( loaded(const KURL&)), SLOT( slotLoaded(const KURL&)));
	mImageView = new ImageView(parentWidget, mDocument, actionCollection());
	connect( mImageView, SIGNAL(requestContextMenu(const QPoint&)),
		this, SLOT(openContextMenu(const QPoint&)) );
	setWidget(mImageView);

	mDirLister = new KDirLister;
	mDirLister->setAutoErrorHandlingEnabled( false, 0 );
	mDirLister->setMainWindow(KApplication::kApplication()->mainWidget());
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

	KStdAction::saveAs( this, SLOT(saveAs()), actionCollection(), "saveAs" );
	new KAction(i18n("Rotate &Left"), "rotate_ccw", CTRL + Key_L, this, SLOT(rotateLeft()), actionCollection(), "rotate_left");
	new KAction(i18n("Rotate &Right"), "rotate_cw", CTRL + Key_R, this, SLOT(rotateRight()), actionCollection(), "rotate_right");

	setXMLFile( "gvimagepart/gvimagepart.rc" );
}

GVImagePart::~GVImagePart() {
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


void GVImagePart::guiActivateEvent( KParts::GUIActivateEvent* event) {
	// Stolen from KHTMLImage
	//
	// prevent the base implementation from emitting setWindowCaption with
	// our url. It destroys our pretty, previously caption. Konq saves/restores
	// the caption for us anyway.
	if (event->activated()) {
		return;
	}
	KParts::ReadOnlyPart::guiActivateEvent(event);
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

void GVImagePart::rotateLeft() {
	mDocument->transform(ImageUtils::ROT_270);
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


void GVImagePart::saveAs() {
	if (!mDocument->isModified()) {
		saveOriginalAs();
		return;
	}

	if (mDocument->canBeSaved()) {
		mDocument->saveAs();
		return;
	}

	KGuiItem saveItem(i18n("&Save Original"), "filesaveas");
	int result = KMessageBox::warningContinueCancel(
		widget(), 
		i18n("Gwenview KPart can't save the modifications you made. Do you want to save the original image?"),
		i18n("Warning"),
		saveItem);

	if (result == KMessageBox::Cancel) return;

	saveOriginalAs();
}


void GVImagePart::showJobError(KIO::Job* job) {
	if (job->error() != 0) { 
		job->showErrorDialog(widget());
	}
}


void GVImagePart::saveOriginalAs() {
	KURL srcURL = mDocument->url();
	KURL dstURL = KFileDialog::getSaveURL(
		srcURL.fileName(),
		QString::null,
		widget());
	if (!dstURL.isValid()) return;

	// Try to get data from the cache to avoid downloading the image again.
	QByteArray data = Cache::instance()->file(srcURL);

	if (data.size() == 0) {
		// We need to read the image again. Let KIO::copy do the work.
		KIO::Job* job = KIO::copy(srcURL, dstURL);
		job->setWindow(widget());
		connect(job, SIGNAL(result(KIO::Job*)),
			this, SLOT(showJobError(KIO::Job*)) );
		return;
	}

	if (dstURL.isLocalFile()) {
		// Destination is a local file, store it ourself
		QString path = dstURL.path();
		QFile file(path);
		if (!file.open(IO_WriteOnly)) {
			KMessageBox::error(
				widget(),
				i18n("Could not open '%1' for writing.").arg(path));
			return;
		}
		storeData(widget(), &file, data);
		return;
	}

	// We need to send the data to a remote location
	new DataUploader(widget(), data, dstURL);
}


DataUploader::DataUploader(QWidget* dialogParent, const QByteArray& data, const KURL& dstURL)
: mDialogParent(dialogParent)
{
	mTempFile.setAutoDelete(true);

	// Store it in a temp file
	if (! storeData(dialogParent, mTempFile.file(), data) ) return;

	// Now upload it
	KURL tmpURL;
	tmpURL.setPath(mTempFile.name());
	KIO::Job* job = KIO::copy(tmpURL, dstURL);
	job->setWindow(dialogParent);
	connect(job, SIGNAL(result(KIO::Job*)),
		this, SLOT(slotJobFinished(KIO::Job*)) );
}


void DataUploader::slotJobFinished(KIO::Job* job) {
	if (job->error() != 0) { 
		job->showErrorDialog(mDialogParent);
	}

	delete this;
}


/**
 * Overload KXMLGUIClient so that we can call setXML
 */
class PopupGUIClient : public KXMLGUIClient {
public:
	PopupGUIClient( KInstance *inst, const QString &doc ) {
		setInstance( inst );
		setXML( doc );
	}
};


void GVImagePart::openContextMenu(const QPoint& pos) {
	QString doc = KXMLGUIFactory::readConfigFile( "gvimagepartpopup.rc", true, instance() );
	PopupGUIClient guiClient(instance(), doc);
	
	KStdAction::saveAs( this, SLOT(saveAs()), guiClient.actionCollection(), "saveAs" );
	
	KParts::URLArgs urlArgs;
	urlArgs.serviceType = mDocument->mimeType();

	KParts::BrowserExtension::PopupFlags flags = 
		KParts::BrowserExtension::ShowNavigationItems 
		| KParts::BrowserExtension::ShowUp
		| KParts::BrowserExtension::ShowReload;
	
	emit mBrowserExtension->popupMenu(&guiClient, pos, m_url, urlArgs, flags, S_IFREG);
}


/***** GVImagePartBrowserExtension *****/

GVImagePartBrowserExtension::GVImagePartBrowserExtension(GVImagePart* viewPart, const char* name)
	:KParts::BrowserExtension(viewPart, name) {
	mGVImagePart = viewPart;
	emit enableAction("print", true );
}

GVImagePartBrowserExtension::~GVImagePartBrowserExtension() {
}

void GVImagePartBrowserExtension::print() {
	mGVImagePart->print();
}

} // namespace
