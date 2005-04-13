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

#include <sys/stat.h> // For S_ISDIR

// Qt
#include <qfileinfo.h>
#include <qguardedptr.h>
#include <qpaintdevicemetrics.h>
#include <qpainter.h>
#include <qwmatrix.h>

// KDE
#include <kdebug.h>
#include <kfilemetainfo.h>
#include <kimageio.h>
#include <kio/job.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprinter.h>
#include <kglobalsettings.h>

// Local
#include "gvarchive.h"
#include "gvbusylevelmanager.h"
#include "gvdocumentloadingimpl.h"
#include "gvdocumentimpl.h"
#include "gvimagesavedialog.h"
#include "gvimageutils/gvimageutils.h"
#include "gvjpegformattype.h"
#include "gvpngformattype.h"
#include "gvmngformattype.h"
#include "gvprintdialog.h"
#include "qxcfi.h"
#include "gvxpm.h"

#include "gvdocument.moc"


//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif

const char* CONFIG_SAVE_AUTOMATICALLY="save automatically";
const char* CONFIG_NOTIFICATION_MESSAGES_GROUP="Notification Messages";

//-------------------------------------------------------------------
//
// GVDocumentPrivate
//
//-------------------------------------------------------------------
class GVDocumentPrivate {
public:
	KURL mURL;
	bool mModified;
	QImage mImage;
	QCString mImageFormat;
	GVDocumentImpl* mImpl;
	QGuardedPtr<KIO::StatJob> mStatJob;
	int mFileSize;
};


//-------------------------------------------------------------------
//
// GVDocument
//
//-------------------------------------------------------------------
GVDocument::GVDocument(QObject* parent)
: QObject(parent) {
	d=new GVDocumentPrivate;
	d->mModified=false;
	d->mImpl=new GVDocumentEmptyImpl(this);
	d->mStatJob=0L;
	d->mFileSize=-1;

	// Register formats here to make sure they are always enabled
	KImageIO::registerFormats();
	XCFImageFormat::registerFormat();

	// First load Qt's plugins, so that Gwenview's decoders that
	// override some of them are installed later and thus come first.
	QImageIO::inputFormats();
	{
		static GVJPEGFormatType sJPEGFormatType;
		static GVPNGFormatType sPNGFormatType;
		static GVXPM sXPM;
		static GVMNG sMNG;
	}

	connect( this, SIGNAL( loading()),
		this, SLOT( slotLoading()));
	connect( this, SIGNAL( loaded(const KURL&)),
		this, SLOT( slotLoaded()));
}


GVDocument::~GVDocument() {
	delete d->mImpl;
	delete d;
}


//---------------------------------------------------------------------
//
// Properties
//
//---------------------------------------------------------------------
KURL GVDocument::url() const {
	return d->mURL;
}


void GVDocument::setURL(const KURL& paramURL) {
	if (paramURL==url()) return;
	// Make a copy, we might have to fix the protocol
	KURL localURL(paramURL);
	LOG("url: " << paramURL.prettyURL());

	// Be sure we are not waiting for another stat result
	if (!d->mStatJob.isNull()) {
		d->mStatJob->kill();
	}
	GVBusyLevelManager::instance()->setBusyLevel(this, BUSY_NONE);

	// Ask to save if necessary.
	if (!saveBeforeClosing()) {
		// Not saved, notify others that we stay on the image
		emit loaded(d->mURL);
		return;
	}

	if (localURL.isEmpty()) {
		reset();
		return;
	}

	// Set high busy level, so that operations like smoothing are suspended.
	// Otherwise the stat() below done using KIO can take quite long.
	GVBusyLevelManager::instance()->setBusyLevel( this, BUSY_CHECKING_NEW_IMAGE );


	// Fix wrong protocol
	if (GVArchive::protocolIsArchive(localURL.protocol())) {
		QFileInfo info(localURL.path());
		if (info.exists()) {
			localURL.setProtocol("file");
		}
	}

	d->mURL = localURL; // this may be fixed after stat() is complete, but set at least something
	d->mStatJob = KIO::stat( localURL, !localURL.isLocalFile() );
	connect( d->mStatJob, SIGNAL( result (KIO::Job *) ),
	   this, SLOT( slotStatResult (KIO::Job *) ) );
}


void GVDocument::slotStatResult(KIO::Job* job) {
	LOG("");
	Q_ASSERT(d->mStatJob==job);
	if (d->mStatJob!=job) {
		kdWarning() << k_funcinfo << "We did not get the right job!\n";
		return;
	}
	GVBusyLevelManager::instance()->setBusyLevel( this, BUSY_NONE );
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


void GVDocument::setDirURL(const KURL& paramURL) {
	if (!saveBeforeClosing()) {
		emit loaded(d->mURL);
		return;
	}
	d->mURL=paramURL;
	d->mURL.adjustPath( +1 ); // add trailing /
	reset();
}


const QImage& GVDocument::image() const {
	return d->mImage;
}

void GVDocument::setImage(QImage img, bool update) {
	bool sizechange = d->mImage.size() != img.size();
	d->mImage=img;
	if( update ) {
		if( sizechange ) emit sizeUpdated( img.width(), img.height());
		emit rectUpdated(QRect(QPoint(0,0), img.size()) );
	}
}

KURL GVDocument::dirURL() const {
	if (filename().isEmpty()) {
		return d->mURL;
	} else {
		KURL url=d->mURL.upURL();
		url.adjustPath(1);
		return url;
	}
}

QString GVDocument::filename() const {
	return d->mURL.filename(false);
}

const QCString& GVDocument::imageFormat() const {
	return d->mImageFormat;
}

void GVDocument::setImageFormat(const QCString& format) {
	d->mImageFormat=format;
}

void GVDocument::setFileSize(int size) {
	d->mFileSize=size;
}

QString GVDocument::comment() const {
	return d->mImpl->comment();
}

void GVDocument::setComment(const QString& comment) {
	d->mImpl->setComment(comment);
	d->mModified=true;
	emit modified();
}

GVDocument::CommentState GVDocument::commentState() const {
	return d->mImpl->commentState();
}

int GVDocument::fileSize() const {
	return d->mFileSize;
}

void GVDocument::slotLoading() {
	GVBusyLevelManager::instance()->setBusyLevel( this, BUSY_LOADING );
}

void GVDocument::slotLoaded() {
	GVBusyLevelManager::instance()->setBusyLevel( this, BUSY_NONE );
}

//---------------------------------------------------------------------
//
// Operations
//
//---------------------------------------------------------------------
void GVDocument::reload() {
	load();
	emit reloaded(url());
}


void GVDocument::print(KPrinter *pPrinter) {
	QPainter printPainter;
	printPainter.begin(pPrinter);
	doPaint(pPrinter, &printPainter);
	printPainter.end();
}

static QString minimizeString( const QString& text, const QFontMetrics&
                                  metrics, int maxWidth ) {
	if ( text.length() <= 5 )
		return text; // no sense to cut that tiny little string

	QString txt = text;
	bool changed = false;
	while ( metrics.width( txt ) > maxWidth ) {
		int mid = txt.length() / 2;
		txt.remove( mid, 2 ); // remove 2 characters in the middle
		changed = true;
	}

	if ( changed ) // add "..." in the middle
	{
		int mid = txt.length() / 2;
		if ( mid <= 5 ) // sanity check
			return txt;

		txt.replace( mid - 1, 3, "..." );
	}

	return txt;
}

void GVDocument::doPaint(KPrinter *printer, QPainter *painter) {
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
	if (scaling==1 /* Fit to page */) {
		bool enlargeToFit = printer->option( "app-gwenview-enlargeToFit" ) != f;
		if ((image.width() > pdWidth || image.height() > pdHeight) || enlargeToFit) {
			size.scale( pdWidth, pdHeight, QSize::ScaleMin );
		}
	} else {
		if (scaling==2 /* Scale To */) {
			int unit = (printer->option("app-gwenview-scaleUnit").isEmpty() ?
				GV_INCHES : printer->option("app-gwenview-scaleUnit").toInt());
			float inches = 1;
			if (unit == GV_MILLIMETERS) {
				inches = 1/25.5;
			} else if (unit == GV_CENTIMETERS) {
				inches = 1/2.54;
			}
			float wImg = (printer->option("app-gwenview-scaleWidth").isEmpty() ?
				1 : printer->option("app-gwenview-scaleWidth").toInt()) * inches;
			float hImg = (printer->option("app-gwenview-scaleHeight").isEmpty() ?
				1 : printer->option("app-gwenview-scaleHeight").toInt()) * inches;
			size.setWidth( int(wImg * printer->resolution()) );
			size.setHeight( int(hImg * printer->resolution()) );
		} else {
			/* No scaling */
			size = image.size();
		}


		if (size.width() > pdWidth || size.height() > pdHeight) {
			int resp = KMessageBox::warningYesNoCancel(0,
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
		QString fname = minimizeString( filename(), fMetrics, pdWidth );
		if ( !fname.isEmpty() ) {
			int fw = fMetrics.width( fname );
			int x = (pdWidth - fw)/2;
			int y = pdMetrics.height() - filenameOffset/2 -commentOffset/2 - margin;
			painter->drawText( x, y, fname );
		}
	}
	if ( printComment ) {
		QString comm = comment(); //minimizeString( mFilename, fMetrics, pdWidth );
		if ( !comm.isEmpty() ) {
			int fw = fMetrics.width( comm );
			int x = (pdWidth - fw)/2;
			int y = pdMetrics.height() - commentOffset/2 - margin;
			painter->drawText( x, y, comm );
		}
	}
}


void GVDocument::transform(GVImageUtils::Orientation orientation) {
	d->mImpl->transform(orientation);
	d->mModified=true;
	emit modified();
}


void GVDocument::save() {
	QString msg=saveInternal(url(), d->mImageFormat);
	if (!msg.isNull()) {
		KMessageBox::error(0, msg);
	}
}


void GVDocument::saveAs() {
	KURL saveURL;
	if (url().isLocalFile()) saveURL=url();

	GVImageSaveDialog dialog(saveURL, d->mImageFormat, 0);
	if (!dialog.exec()) return;

	QString msg=saveInternal(saveURL, dialog.imageFormat() );
	if (!msg.isNull()) {
		KMessageBox::error(0, msg);
	}
}

bool GVDocument::saveBeforeClosing() {
	if (!d->mModified) return true;

	QString msg=i18n("<qt>The image <b>%1</b> has been modified, do you want to save the changes?</qt>")
		.arg(url().prettyURL());

	// FIXME: If we definitly switch from questionYesNoCancel to questionYesNo,
	// change this method to not return anything and change the calling code.
	int result=KMessageBox::questionYesNo(0, msg, QString::null,
		KStdGuiItem::save(), KStdGuiItem::discard(), CONFIG_SAVE_AUTOMATICALLY);		

	if (result == KMessageBox::Yes) {
		msg=saveInternal(url(), d->mImageFormat);	
	} else {
		d->mModified=false;
	}

	return true;
}


//---------------------------------------------------------------------
//
// Config
//
//---------------------------------------------------------------------
static GVDocument::ModifiedBehavior stringToModifiedBehavior(const QString& str) {
	if (str=="yes") return GVDocument::SAVE_SILENTLY;
	if (str=="no") return GVDocument::DISCARD_CHANGES;
	return GVDocument::ASK;
}

static QString modifiedBehaviorToString(GVDocument::ModifiedBehavior behaviour) {
	if (behaviour==GVDocument::SAVE_SILENTLY) return "yes";
	if (behaviour==GVDocument::DISCARD_CHANGES) return "no";
	return "";
}


void GVDocument::setModifiedBehavior(ModifiedBehavior value) {
	KConfig* config=KGlobal::config();
	KConfigGroupSaver saver(config, CONFIG_NOTIFICATION_MESSAGES_GROUP);
	config->setGroup(CONFIG_NOTIFICATION_MESSAGES_GROUP);
	config->writeEntry(CONFIG_SAVE_AUTOMATICALLY,
		modifiedBehaviorToString(value));
}


GVDocument::ModifiedBehavior GVDocument::modifiedBehavior() const {
	KConfig* config=KGlobal::config();
	KConfigGroupSaver saver(config, CONFIG_NOTIFICATION_MESSAGES_GROUP);
	QString str=config->readEntry(CONFIG_SAVE_AUTOMATICALLY);
	return stringToModifiedBehavior(str.lower());
}


//---------------------------------------------------------------------
//
// Private stuff
//
//---------------------------------------------------------------------
void GVDocument::switchToImpl(GVDocumentImpl* impl) {
	// There should always be an implementation defined
	Q_ASSERT(d->mImpl);
	Q_ASSERT(impl);
	delete d->mImpl;
	d->mImpl=impl;

	connect(d->mImpl, SIGNAL(finished(bool)),
		this, SLOT(slotFinished(bool)) );
	connect(d->mImpl, SIGNAL(sizeUpdated(int, int)),
		this, SIGNAL(sizeUpdated(int, int)) );
	connect(d->mImpl, SIGNAL(rectUpdated(const QRect&)),
		this, SIGNAL(rectUpdated(const QRect&)) );
	d->mImpl->init();
}


void GVDocument::load() {
	KURL pixURL=url();
	Q_ASSERT(!pixURL.isEmpty());
	LOG("url: " << pixURL.prettyURL());

	switchToImpl(new GVDocumentLoadingImpl(this));
	emit loading();
}


void GVDocument::slotFinished(bool success) {
	LOG("");
	if (success) {
		emit loaded(d->mURL);
	} else {
		// FIXME: Emit a failed signal instead
		emit loaded(d->mURL);
	}
}


QString GVDocument::saveInternal(const KURL& url, const QCString& format) {
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


void GVDocument::reset() {
	switchToImpl(new GVDocumentEmptyImpl(this));
	emit loaded(d->mURL);
}
