// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview - A simple image viewer for KDE
Copyright 2000-2006 Aurelien Gateau

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

#include <sys/stat.h> // For S_ISDIR

// Qt
#include <qfileinfo.h>
#include <qguardedptr.h>
#include <qpaintdevicemetrics.h>
#include <qpainter.h>
#include <qwmatrix.h>

// KDE
#include <kapplication.h>
#include <kdebug.h>
#include <kfilemetainfo.h>
#include <kglobalsettings.h>
#include <kimageio.h>
#include <kio/job.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprinter.h>
#include <kstringhandler.h>

// Local
#include "archive.h"
#include "busylevelmanager.h"
#include "cache.h"
#include "documentloadingimpl.h"
#include "documentimpl.h"
#include "imagesavedialog.h"
#include "imageutils/imageutils.h"
#include "jpegformattype.h"
#include "pngformattype.h"
#include "mngformattype.h"
#include "printdialog.h"
#include "qxcfi.h"
#include "xpm.h"
#include "xcursor.h"

#include "document.moc"
namespace Gwenview {


#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

const char* CONFIG_SAVE_AUTOMATICALLY="save automatically";


/**
 * Returns a widget suitable to use as a dialog parent
 */
static QWidget* dialogParentWidget() {
	return KApplication::kApplication()->mainWidget();
}

//-------------------------------------------------------------------
//
// DocumentPrivate
//
//-------------------------------------------------------------------
class DocumentPrivate {
public:
	KURL mURL;
	bool mModified;
	QImage mImage;
	QString mMimeType;
	QCString mImageFormat;
	DocumentImpl* mImpl;
	QGuardedPtr<KIO::StatJob> mStatJob;
	int mFileSize;
};


//-------------------------------------------------------------------
//
// Document
//
//-------------------------------------------------------------------
Document::Document(QObject* parent)
: QObject(parent) {
	d=new DocumentPrivate;
	d->mModified=false;
	d->mImpl=new DocumentEmptyImpl(this);
	d->mStatJob=0L;
	d->mFileSize=-1;

	// Register formats here to make sure they are always enabled
	KImageIO::registerFormats();
	XCFImageFormat::registerFormat();

	// First load Qt's plugins, so that Gwenview's decoders that
	// override some of them are installed later and thus come first.
	QImageIO::inputFormats();
	{
		static Gwenview::JPEGFormatType sJPEGFormatType;
		static Gwenview::PNGFormatType sPNGFormatType;
		static Gwenview::XPM sXPM;
		static Gwenview::MNG sMNG;
		static Gwenview::XCursorFormatType sXCursorFormatType;
	}

	connect( this, SIGNAL( loading()),
		this, SLOT( slotLoading()));
	connect( this, SIGNAL( loaded(const KURL&)),
		this, SLOT( slotLoaded()));
}


Document::~Document() {
	delete d->mImpl;
	delete d;
}


//---------------------------------------------------------------------
//
// Properties
//
//---------------------------------------------------------------------
QString Document::mimeType() const {
	return d->mMimeType;
}

void Document::setMimeType(const QString& mimeType) {
	d->mMimeType = mimeType;
}

MimeTypeUtils::Kind Document::urlKind() const {
	return d->mImpl->urlKind();
}


KURL Document::url() const {
	return d->mURL;
}


void Document::setURL(const KURL& paramURL) {
	if (paramURL==url()) return;
	// Make a copy, we might have to fix the protocol
	KURL localURL(paramURL);
	LOG("url: " << paramURL.prettyURL());

	// Be sure we are not waiting for another stat result
	if (!d->mStatJob.isNull()) {
		d->mStatJob->kill();
	}
	BusyLevelManager::instance()->setBusyLevel(this, BUSY_NONE);

	// Ask to save if necessary.
	saveBeforeClosing();

	if (localURL.isEmpty()) {
		reset();
		return;
	}

	// Set high busy level, so that operations like smoothing are suspended.
	// Otherwise the stat() below done using KIO can take quite long.
	BusyLevelManager::instance()->setBusyLevel( this, BUSY_CHECKING_NEW_IMAGE );


	// Fix wrong protocol
	if (Archive::protocolIsArchive(localURL.protocol())) {
		QFileInfo info(localURL.path());
		if (info.exists()) {
			localURL.setProtocol("file");
		}
	}

	d->mURL = localURL; // this may be fixed after stat() is complete, but set at least something
	d->mStatJob = KIO::stat( localURL, !localURL.isLocalFile() );
	d->mStatJob->setWindow(KApplication::kApplication()->mainWidget());
	connect( d->mStatJob, SIGNAL( result (KIO::Job *) ),
	   this, SLOT( slotStatResult (KIO::Job *) ) );
}


void Document::slotStatResult(KIO::Job* job) {
	LOG("");
	Q_ASSERT(d->mStatJob==job);
	if (d->mStatJob!=job) {
		kdWarning() << k_funcinfo << "We did not get the right job!\n";
		return;
	}
	BusyLevelManager::instance()->setBusyLevel( this, BUSY_NONE );
	if (d->mStatJob->error()) return;

	bool isDir=false;
	KIO::UDSEntry entry = d->mStatJob->statResult();
	d->mURL=d->mStatJob->url();

	KIO::UDSEntry::ConstIterator it;
	for(it=entry.begin();it!=entry.end();++it) {
		if ((*it).m_uds==KIO::UDS_FILE_TYPE) {
			isDir=S_ISDIR( (*it).m_long );
			break;
		}
	}

	if (isDir) {
		d->mURL.adjustPath( +1 ); // add trailing /
		reset();
		return;
	}

	load();
}


void Document::setDirURL(const KURL& paramURL) {
	saveBeforeClosing();
	d->mURL=paramURL;
	d->mURL.adjustPath( +1 ); // add trailing /
	reset();
}


const QImage& Document::image() const {
	return d->mImage;
}

void Document::setImage(QImage img) {
	bool sizechange = d->mImage.size() != img.size();
	d->mImage = img;
	if( sizechange ) emit sizeUpdated();
}


KURL Document::dirURL() const {
	if (filename().isEmpty()) {
		return d->mURL;
	} else {
		KURL url=d->mURL.upURL();
		url.adjustPath(1);
		return url;
	}
}

QString Document::filename() const {
	return d->mURL.filename(false);
}

const QCString& Document::imageFormat() const {
	return d->mImageFormat;
}

void Document::setImageFormat(const QCString& format) {
	d->mImageFormat=format;
}

void Document::setFileSize(int size) {
	d->mFileSize=size;
}

QString Document::comment() const {
	return d->mImpl->comment();
}

QString Document::aperture() const {
	return d->mImpl->aperture();
}

QString Document::exposureTime() const {
       return d->mImpl->exposureTime();
}

QString Document::iso() const {
	return d->mImpl->iso();
}

QString Document::focalLength() const {
	return d->mImpl->focalLength();
}

void Document::setComment(const QString& comment) {
	d->mImpl->setComment(comment);
	d->mModified=true;
	emit modified();
}

Document::CommentState Document::commentState() const {
	return d->mImpl->commentState();
}

/**
 * Returns the duration of the document in seconds, or 0 if there is no
 * duration
 */
int Document::duration() const {
	return d->mImpl->duration();
}

int Document::fileSize() const {
	return d->mFileSize;
}

bool Document::canBeSaved() const {
	return d->mImpl->canBeSaved();
}

bool Document::isModified() const {
	return d->mModified;
}

void Document::slotLoading() {
	BusyLevelManager::instance()->setBusyLevel( this, BUSY_LOADING );
}

void Document::slotLoaded() {
	BusyLevelManager::instance()->setBusyLevel( this, BUSY_NONE );
}

//---------------------------------------------------------------------
//
// Operations
//
//---------------------------------------------------------------------
void Document::reload() {
	Cache::instance()->invalidate( url());
	load();
	emit reloaded(url());
}


void Document::print(KPrinter *pPrinter) {
	QPainter printPainter;
	printPainter.begin(pPrinter);
	doPaint(pPrinter, &printPainter);
	printPainter.end();
}


void Document::doPaint(KPrinter *printer, QPainter *painter) {
	// will contain the final image to print
	QImage image = d->mImage;
	image.detach();

	// We use a QPaintDeviceMetrics to know the actual page size in pixel,
	// this gives the real painting area
	QPaintDeviceMetrics pdMetrics(painter->device());
	const int margin = pdMetrics.logicalDpiY() / 2; // half-inch margin

	painter->setFont( KGlobalSettings::generalFont() );
	QFontMetrics fMetrics = painter->fontMetrics();

	int x = 0;
	int y = 0;
	int pdWidth = pdMetrics.width();
	int pdHeight = pdMetrics.height();

	QString t = "true";
	QString f = "false";

	int alignment = (printer->option("app-gwenview-position").isEmpty() ?
		Qt::AlignCenter : printer->option("app-gwenview-position").toInt());

	// Compute filename offset
	int filenameOffset = 0;
	bool printFilename = printer->option( "app-gwenview-printFilename" ) != f;
	if ( printFilename ) {
		filenameOffset = fMetrics.lineSpacing() + 14;
		pdHeight -= filenameOffset; // filename goes into one line!
	}

	// Compute comment offset
	int commentOffset = 0;
	bool printComment = printer->option( "app-gwenview-printComment" ) != f;
	if ( commentOffset ) {
		commentOffset = fMetrics.lineSpacing() + 14;// #### TODO check if it's correct
		pdHeight -= commentOffset; // #### TODO check if it's correct
	}
	if (commentOffset || printFilename) {
		pdHeight -= margin;
	}

	// Apply scaling
	int scaling = printer->option( "app-gwenview-scale" ).toInt();

	QSize size = image.size();
	if (scaling==GV_FITTOPAGE /* Fit to page */) {
		bool enlargeToFit = printer->option( "app-gwenview-enlargeToFit" ) != f;
		if ((image.width() > pdWidth || image.height() > pdHeight) || enlargeToFit) {
			size.scale( pdWidth, pdHeight, QSize::ScaleMin );
		}
	} else {
		if (scaling==GV_SCALE /* Scale To */) {
			int unit = (printer->option("app-gwenview-scaleUnit").isEmpty() ?
				GV_INCHES : printer->option("app-gwenview-scaleUnit").toInt());
			double inches = 1;
			if (unit == GV_MILLIMETERS) {
				inches = 1/25.4;
			} else if (unit == GV_CENTIMETERS) {
				inches = 1/2.54;
			}
			double wImg = (printer->option("app-gwenview-scaleWidth").isEmpty() ?
				1 : printer->option("app-gwenview-scaleWidth").toDouble()) * inches;
			double hImg = (printer->option("app-gwenview-scaleHeight").isEmpty() ?
				1 : printer->option("app-gwenview-scaleHeight").toDouble()) * inches;
			size.setWidth( int(wImg * printer->resolution()) );
			size.setHeight( int(hImg * printer->resolution()) );
		} else {
			/* GV_NOSCALE: no scaling */
			// try to get the density info so that we can print using original size
			// known if it is am image from scanner for instance
			const float INCHESPERMETER = (100. / 2.54);
			if (image.dotsPerMeterX())
			{
				double wImg = double(size.width()) / double(image.dotsPerMeterX()) * INCHESPERMETER;
				size.setWidth( int(wImg *printer->resolution()) );
			}
			if (image.dotsPerMeterY())
			{
				double hImg = double(size.height()) / double(image.dotsPerMeterY()) * INCHESPERMETER;
				size.setHeight( int(hImg *printer->resolution()) );
			}
		}
    
		if (size.width() > pdWidth || size.height() > pdHeight) {
			int resp = KMessageBox::warningYesNoCancel(dialogParentWidget(),
				i18n("The image will not fit on the page, what do you want to do?"),
				QString::null,KStdGuiItem::cont(),
				i18n("Shrink") );

			if (resp==KMessageBox::Cancel) {
				printer->abort();
				return;
			} else if (resp == KMessageBox::No) { // Shrink
				size.scale(pdWidth, pdHeight, QSize::ScaleMin);
			}
		}
	}

	// Compute x and y
	if ( alignment & Qt::AlignHCenter )
		x = (pdWidth - size.width())/2;
	else if ( alignment & Qt::AlignLeft )
		x = 0;
	else if ( alignment & Qt::AlignRight )
		x = pdWidth - size.width();

	if ( alignment & Qt::AlignVCenter )
		y = (pdHeight - size.height())/2;
	else if ( alignment & Qt::AlignTop )
		y = 0;
	else if ( alignment & Qt::AlignBottom )
		y = pdHeight - size.height();

	// Draw, the image will be scaled to fit the given area if necessary
	painter->drawImage( QRect( x, y, size.width(), size.height()), image );

	if ( printFilename ) {
		QString fname = KStringHandler::cPixelSqueeze( filename(), fMetrics, pdWidth );
		if ( !fname.isEmpty() ) {
			int fw = fMetrics.width( fname );
			int x = (pdWidth - fw)/2;
			int y = pdMetrics.height() - filenameOffset/2 -commentOffset/2 - margin;
			painter->drawText( x, y, fname );
		}
	}
	if ( printComment ) {
		QString comm = comment();
		if ( !comm.isEmpty() ) {
			int fw = fMetrics.width( comm );
			int x = (pdWidth - fw)/2;
			int y = pdMetrics.height() - commentOffset/2 - margin;
			painter->drawText( x, y, comm );
		}
	}
}


void Document::transform(ImageUtils::Orientation orientation) {
	d->mImpl->transform(orientation);
	d->mModified=true;
	emit modified();
}


void Document::save() {
	QString msg=saveInternal(url(), d->mImageFormat);
	if (!msg.isNull()) {
		KMessageBox::error(dialogParentWidget(), msg);
		// If it can't be saved we leave it as modified, because user
		// could choose to save it to another path with saveAs
	}
}


void Document::saveAs() {
	KURL saveURL;

	ImageSaveDialog dialog(saveURL, d->mImageFormat, dialogParentWidget());
	dialog.setSelection(url().fileName());
	if (!dialog.exec()) return;

	QString msg=saveInternal(saveURL, dialog.imageFormat() );
	if (!msg.isNull()) {
		// If it can't be saved we leave it as modified, because user
		// could choose a wrong readonly path from dialog and retry to
		KMessageBox::error(dialogParentWidget(), msg);
	}
}

void Document::saveBeforeClosing() {
	if (!d->mModified) return;

	QString msg=i18n("<qt>The image <b>%1</b> has been modified, do you want to save the changes?</qt>")
		.arg(url().prettyURL());

	int result=KMessageBox::questionYesNo(dialogParentWidget(), msg, QString::null,
		KStdGuiItem::save(), KStdGuiItem::discard(), CONFIG_SAVE_AUTOMATICALLY);		

	if (result == KMessageBox::Yes) {
		saveInternal(url(), d->mImageFormat);	
		// If it can't be saved it's useless to leave it as modified
		// since user is closing this image and changing to another one
		d->mModified=false;
		//FIXME it should be nice to tell the user it failed
	} else {
		d->mModified=false;
	}
}


//---------------------------------------------------------------------
//
// Private stuff
//
//---------------------------------------------------------------------
void Document::switchToImpl(DocumentImpl* impl) {
	// There should always be an implementation defined
	Q_ASSERT(d->mImpl);
	Q_ASSERT(impl);
	delete d->mImpl;
	d->mImpl=impl;

	connect(d->mImpl, SIGNAL(finished(bool)),
		this, SLOT(slotFinished(bool)) );
	connect(d->mImpl, SIGNAL(sizeUpdated()),
		this, SIGNAL(sizeUpdated()) );
	connect(d->mImpl, SIGNAL(rectUpdated(const QRect&)),
		this, SIGNAL(rectUpdated(const QRect&)) );
	d->mImpl->init();
}


void Document::load() {
	KURL pixURL=url();
	Q_ASSERT(!pixURL.isEmpty());
	LOG("url: " << pixURL.prettyURL());

	// DocumentLoadingImpl might emit "finished()" in its "init()" method, so
	// make sure we emit "loading()" before switching
	emit loading();
	switchToImpl(new DocumentLoadingImpl(this));
}


void Document::slotFinished(bool success) {
	LOG("");
	if (success) {
		emit loaded(d->mURL);
	} else {
		// FIXME: Emit a failed signal instead
		emit loaded(d->mURL);
	}
}


QString Document::saveInternal(const KURL& url, const QCString& format) {
	QString msg=d->mImpl->save(url, format);

	if (msg.isNull()) {
		emit saved(url);
		d->mModified=false;
		return QString::null;
	}
	
	LOG("Save failed: " << msg);
	return QString("<qt><b>%1</b><br/>")
		.arg(i18n("Could not save the image to %1.").arg(url.prettyURL()))
		+ msg + "</qt>";
}


void Document::reset() {
	switchToImpl(new DocumentEmptyImpl(this));
	emit loaded(d->mURL);
}

} // namespace
