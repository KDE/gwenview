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

#include <sys/stat.h> // For S_ISDIR

// Qt
#include <qfileinfo.h>
#include <qpaintdevicemetrics.h>
#include <qpainter.h>
#include <qwmatrix.h>

// KDE 
#include <kdebug.h>
#include <kfilemetainfo.h>
#include <kimageio.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprinter.h>
#include <kglobalsettings.h>

// Local 
#include "gvarchive.h"
#include "gvdocumentimpl.h"
#include "gvdocumentdecodeimpl.h"
#include "gvimagesavedialog.h"
#include "gvjpegtran.h"
#include "gvprintdialog.h"

#include "gvdocument.moc"



const char* CONFIG_SAVE_AUTOMATICALLY="save automatically";
const char* CONFIG_NOTIFICATION_MESSAGES_GROUP="Notification Messages";



//-------------------------------------------------------------------
//
// GVDocumentPrivate
//
//-------------------------------------------------------------------
class GVDocumentPrivate {
public:
	KURL mDirURL;
	QString mFilename;
	bool mModified;
	QImage mImage;
	const char* mImageFormat;
	GVDocumentImpl* mImpl;
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
	KURL url=d->mDirURL;
	url.addPath(d->mFilename);
	return url;
}


void GVDocument::setURL(const KURL& paramURL) {
	kdDebug() << k_funcinfo << " url: " << paramURL.prettyURL() << endl;
	KURL localURL(paramURL);
	if (localURL==url()) return;

	// Ask to save if necessary.
	if (!saveBeforeClosing()) {
		// Not saved, notify others that we stay on the image
		emit loaded(d->mDirURL,d->mFilename);
		return;
	}

	if (localURL.isEmpty()) {
		reset();
		return;
	}

	// Fix wrong protocol
	if (GVArchive::protocolIsArchive(localURL.protocol())) {
		QFileInfo info(localURL.path());
		if (info.exists()) {
			localURL.setProtocol("file");
		}
	}

	// Check whether this is a dir or a file
	KIO::UDSEntry entry;
	bool isDir=false;
	if (!KIO::NetAccess::stat(localURL,entry)) return;
	KIO::UDSEntry::ConstIterator it;
	for(it=entry.begin();it!=entry.end();++it) {
		if ((*it).m_uds==KIO::UDS_FILE_TYPE) {
			isDir=S_ISDIR( (*it).m_long );
			break;
		}
	}

	if (isDir) {
		d->mDirURL=localURL;
		d->mFilename="";
	} else {
		d->mDirURL=localURL.upURL();
		d->mFilename=localURL.filename();
	}

	if (d->mFilename.isEmpty()) {
		reset();
		return;
	}

	load();
}


void GVDocument::setDirURL(const KURL& paramURL) {
	if (!saveBeforeClosing()) {
		emit loaded(d->mDirURL,d->mFilename);
		return;
	}
	d->mDirURL=paramURL;
	d->mFilename="";
	reset();
}


void GVDocument::setFilename(const QString& filename) {
	if (d->mFilename==filename) return;

	if (!saveBeforeClosing()) {
		emit loaded(d->mDirURL,d->mFilename);
		return;
	}
	d->mFilename=filename;
	load();
}


const QImage& GVDocument::image() const {
	return d->mImage;
}

void GVDocument::setImage(QImage img) {
	kdDebug() << k_funcinfo << endl;
	d->mImage=img;
}

const KURL& GVDocument::dirURL() const {
	return d->mDirURL;
}

const QString& GVDocument::filename() const {
	return d->mFilename;
}

const char* GVDocument::imageFormat() const {
	return d->mImageFormat;
}

void GVDocument::setImageFormat(const char* format) {
	d->mImageFormat=format;
}

void GVDocument::suspendLoading() {
	d->mImpl->suspendLoading();
}

void GVDocument::resumeLoading() {
	d->mImpl->resumeLoading();
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
	bool scaling = printer->option( "app-gwenview-scale" ) != f;
	if (scaling) {
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
		wImg = wImg * printer->resolution();
		hImg = hImg * printer->resolution();

		image = image.smoothScale( int(wImg), int(hImg), QImage::ScaleMin );
	}

	// Shring image if necessary
	bool shrinkToFit = printer->option( "app-gwenview-shrinkToFit" ) != f;
	if ( shrinkToFit && image.width() > pdWidth || image.height() > pdHeight ) {
		image = image.smoothScale( pdWidth, pdHeight, QImage::ScaleMin );
	}

	// Compute x and y
	if ( alignment & Qt::AlignHCenter )
		x = (pdWidth - image.width())/2;
	else if ( alignment & Qt::AlignLeft )
		x = 0;
	else if ( alignment & Qt::AlignRight )
		x = pdWidth - image.width();

	if ( alignment & Qt::AlignVCenter )
		y = (pdHeight - image.height())/2;
	else if ( alignment & Qt::AlignTop )
		y = 0;
	else if ( alignment & Qt::AlignBottom )
		y = pdHeight - image.height();

	// Draw
	painter->drawImage( x, y, image );

	if ( printFilename ) {
		QString fname = minimizeString( d->mFilename, fMetrics, pdWidth );
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


void GVDocument::modify(GVImageUtils::Orientation orientation) {
	d->mImpl->modify(orientation);
	d->mModified=true;
	emit modified();
}


bool GVDocument::save() {
	return saveInternal(url(), d->mImpl->imageFormat());
}


void GVDocument::saveAs() {
	KURL saveURL;
	if (url().isLocalFile()) saveURL=url();

	// FIXME: Make GVImageSaveDialog uses const char* for image format
	QString tmp(d->mImpl->imageFormat());
	GVImageSaveDialog dialog(saveURL,tmp,0);
	if (!dialog.exec()) return;

	if (!saveInternal(saveURL,d->mImpl->imageFormat())) {
		KMessageBox::sorry(0,i18n(
			"Could not save file. Check that you have the appropriate rights and that there's enough room left on the device."));
	}
}

bool GVDocument::saveBeforeClosing() {
	if (!d->mModified) return true;
	QString msg=i18n("<qt>The image <b>%1</b> has been modified, do you want to save the changes?</qt>")
		.arg(url().prettyURL());

	int result=KMessageBox::questionYesNoCancel(0, msg, QString::null,
		i18n("Save"), i18n("Discard"), CONFIG_SAVE_AUTOMATICALLY);

	switch (result) {
	case KMessageBox::Yes:
		if (save()) return true;
		result= KMessageBox::warningContinueCancel(0, i18n(
			"Could not save file. Check that you have the appropriate rights and that there's enough room left on the device.\n"),
			QString::null,
			i18n("Discard"));
		if (result==KMessageBox::Continue) {
			d->mModified=false;
			return true;
		} else {
			return false;
		}
		
	case KMessageBox::No:
		d->mModified=false;
		return true;

	default:
		return false;
	}
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
}


void GVDocument::load() {
	KURL pixURL=url();
	Q_ASSERT(!pixURL.isEmpty());
	kdDebug() << k_funcinfo << " url: " << pixURL.prettyURL() << endl;

	switchToImpl(new GVDocumentDecodeImpl(this));
	emit loading();
}
	

void GVDocument::slotFinished(bool success) {
	kdDebug() << k_funcinfo << endl;
	if (success) {
		emit loaded(d->mDirURL, d->mFilename);
	} else {
		// FIXME: Emit a failed signal instead
		emit loaded(d->mDirURL, d->mFilename);
	}
}


bool GVDocument::saveInternal(const KURL& url, const char* format) {
	bool result=d->mImpl->save(url, format);
	
	if (result) {
		emit saved(url);
		d->mModified=false;
	}

	return result;
}


void GVDocument::reset() {
	switchToImpl(new GVDocumentEmptyImpl(this));
	emit loaded(d->mDirURL,d->mFilename);
}
