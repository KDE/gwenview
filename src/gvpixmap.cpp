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

// Qt includes
#include <qasyncimageio.h>
#include <qfileinfo.h>
#include <qpaintdevicemetrics.h>
#include <qpainter.h>
#include <qtimer.h>
#include <qwmatrix.h>

// KDE includes
#include <kdebug.h>
#include <kfilemetainfo.h>
#include <kimageio.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprinter.h>
#include <ktempfile.h>
#include <kglobalsettings.h>

// Our includes
#include <gvarchive.h>
#include <gvimagesavedialog.h>
#include <gvjpegtran.h>
#include <gvprintdialog.h>

#include <gvpixmap.moc>



const char* CONFIG_SAVE_AUTOMATICALLY="save automatically";
const char* CONFIG_NOTIFICATION_MESSAGES_GROUP="Notification Messages";

const char* JPEG_EXIF_DATA="Jpeg EXIF Data";
const char* JPEG_EXIF_COMMENT="Comment";
const char* PNG_COMMENT="Comment";



//-------------------------------------------------------------------
//
// GVPixmapPrivate
//
//-------------------------------------------------------------------
class GVPixmapPrivate {
public:
	// Declare it here to avoid breaking --enable-final builds
	static const int CHUNK_SIZE=4096;
	GVPixmap::CommentState mCommentState;
	QString mComment;
	QImageDecoder* mDecoder;

	QImage mImage;
	KURL mDirURL;
	QString mFilename;
	QString mImageFormat;
	bool mModified;

	QString mTempFilePath;
	// Store compressed data. Usefull for lossless manipulations.
	QByteArray mCompressedData;
	int mReadSize;
	QTimer mDecoderTimer;


	GVPixmapPrivate()
			: mDecoder(0) {}

	~GVPixmapPrivate() {
		if (mDecoder) delete mDecoder;
	}

	void loadComment(const QString& path) {
		KFileMetaInfo metaInfo=KFileMetaInfo(path);
		KFileMetaInfoItem commentItem;

		mCommentState=GVPixmap::NONE;

		if (metaInfo.isEmpty()) return;

		// Get the comment
		QString mimeType=metaInfo.mimeType();
		if (mimeType=="image/jpeg") {
			commentItem=metaInfo[JPEG_EXIF_DATA][JPEG_EXIF_COMMENT];
			mCommentState=QFileInfo(path).isWritable()?GVPixmap::WRITABLE:GVPixmap::READ_ONLY;
			mComment=QString::fromUtf8( commentItem.string().ascii() );

		} else if (mimeType=="image/png") {

			// we take all comments
			QStringList keys=metaInfo[PNG_COMMENT].keys();
			if (keys.size()==0) return;

			QStringList tmp;
			QStringList::ConstIterator it;
			for(it=keys.begin(); it!=keys.end(); ++it) {
				KFileMetaInfoItem metaInfoItem=metaInfo[PNG_COMMENT][*it];
				QString line=QString("%1: %2")
					.arg(metaInfoItem.translatedKey())
					.arg(metaInfoItem.string());
				tmp.append(line);
			}
			mComment=tmp.join("\n");
			mCommentState=GVPixmap::READ_ONLY;
		} else {
			return;
		}

#if 0
		// Some code to debug
		QStringList groups, keys;
		groups = metaInfo.groups();
		for (uint i=0; i<groups.size(); ++i) {
			keys = metaInfo[groups[i]].keys();
			kdDebug() << groups[i] << endl;
			for (uint j=0; j<keys.size(); ++j) {
				kdDebug() << "- " << keys[j] << endl;
			}
		}
#endif

	}


	void saveComment(const QString& path) {
		KFileMetaInfo metaInfo=KFileMetaInfo(path);
		KFileMetaInfoItem commentItem;
		if (metaInfo.isEmpty()) return;

		QString mimeType=metaInfo.mimeType();
		if (mimeType=="image/jpeg") {
			commentItem=metaInfo[JPEG_EXIF_DATA][JPEG_EXIF_COMMENT];
		} else {
			return;
		}

		if (commentItem.isEditable()) {
			commentItem.setValue(mComment);
		}
		metaInfo.applyChanges();
	}

	void finishLoading() {
		Q_ASSERT(mDecoderTimer.isActive());
		Q_ASSERT(mDecoder);
		mDecoderTimer.stop();
		delete mDecoder;
		mDecoder=0;
		KIO::NetAccess::removeTempFile(mTempFilePath);
	}

};


//-------------------------------------------------------------------
//
// GVPixmap
//
//-------------------------------------------------------------------
GVPixmap::GVPixmap(QObject* parent)
		: QObject(parent) {
	d=new GVPixmapPrivate();
	d->mModified=false;
	d->mCommentState=NONE;
	kdDebug() << "GVPixmap::GVPixmap supported decoder formats: "
		<< QStringList::fromStrList(QImageDecoder::inputFormats()).join(",")
		<< endl;
	connect(&d->mDecoderTimer, SIGNAL(timeout()), this, SLOT(loadChunk()) );
}


GVPixmap::~GVPixmap() {
	delete d;
}


//---------------------------------------------------------------------
//
// Properties
//
//---------------------------------------------------------------------
KURL GVPixmap::url() const {
	KURL url=d->mDirURL;
	url.addPath(d->mFilename);
	return url;
}


void GVPixmap::setURL(const KURL& paramURL) {
	//kdDebug() << "GVPixmap::setURL " << paramURL.prettyURL() << endl;
	KURL URL(paramURL);
	if (URL.cmp(url())) return;

	// Ask to save if necessary.
	if (!saveBeforeClosing()) {
		// Not saved, notify others that we stay on the image
		emit loaded(d->mDirURL,d->mFilename);
		return;
	}

	if (URL.isEmpty()) {
		reset();
		return;
	}

	// Fix wrong protocol
	if (GVArchive::protocolIsArchive(URL.protocol())) {
		QFileInfo info(URL.path());
		if (info.exists()) {
			URL.setProtocol("file");
		}
	}

	// Check whether this is a dir or a file
	KIO::UDSEntry entry;
	bool isDir=false;
	if (!KIO::NetAccess::stat(URL,entry)) return;
	KIO::UDSEntry::ConstIterator it;
	for(it=entry.begin();it!=entry.end();++it) {
		if ((*it).m_uds==KIO::UDS_FILE_TYPE) {
			isDir=S_ISDIR( (*it).m_long );
			break;
		}
	}

	if (isDir) {
		d->mDirURL=URL;
		d->mFilename="";
	} else {
		d->mDirURL=URL.upURL();
		d->mFilename=URL.filename();
	}

	if (d->mFilename.isEmpty()) {
		reset();
		return;
	}

	load();
}


void GVPixmap::setDirURL(const KURL& paramURL) {
	if (!saveBeforeClosing()) {
		emit loaded(d->mDirURL,d->mFilename);
		return;
	}
	d->mDirURL=paramURL;
	d->mFilename="";
	reset();
}


void GVPixmap::setFilename(const QString& filename) {
	if (d->mFilename==filename) return;

	if (!saveBeforeClosing()) {
		emit loaded(d->mDirURL,d->mFilename);
		return;
	}
	d->mFilename=filename;
	load();
}


GVPixmap::CommentState GVPixmap::commentState() const {
	return d->mCommentState;
}


QString GVPixmap::comment() const {
	return d->mComment;
#if 0
	if (d->mCommentItem.isValid()) {
		return QString::fromUtf8( d->mCommentItem.string().ascii());
	} else {
		return QString::null;
	}
#endif
}


void GVPixmap::setComment(const QString& comment) {
	Q_ASSERT(d->mCommentState==WRITABLE);
	d->mComment=comment;
	d->mModified=true;
}

const QImage& GVPixmap::image() const {
	return d->mImage;
}

const KURL& GVPixmap::dirURL() const {
	return d->mDirURL;
}
const QString& GVPixmap::filename() const {
	return d->mFilename;
}
const QString& GVPixmap::imageFormat() const {
	return d->mImageFormat;
}

//---------------------------------------------------------------------
//
// Operations
//
//---------------------------------------------------------------------
void GVPixmap::reload() {
	load();
	emit reloaded(url());
}


void GVPixmap::print(KPrinter *pPrinter) {
	QPainter printPainter;
	printPainter.begin(pPrinter);
	doPaint(pPrinter, &printPainter);
	printPainter.end();
}

QString GVPixmap::minimizeString( const QString& text, const QFontMetrics&
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

void GVPixmap::doPaint(KPrinter *printer, QPainter *painter) {
	// will contain the final image to print
	QImage image = d->mImage;

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


void GVPixmap::modify(GVImageUtils::Orientation orientation) {
	// Apply the rotation to the compressed data too if available
	if (d->mImageFormat=="JPEG" && !d->mCompressedData.isNull()) {
		d->mCompressedData=GVJPEGTran::apply(d->mCompressedData, orientation);
	}

	d->mImage=GVImageUtils::modify(d->mImage, orientation);
	d->mModified=true;
	emit modified();
}


bool GVPixmap::save() {
	return saveInternal(url(), d->mImageFormat);
}


void GVPixmap::saveAs() {
	QString format=d->mImageFormat;
	KURL saveURL;
	if (url().isLocalFile()) saveURL=url();

	GVImageSaveDialog dialog(saveURL,format,0);
	if (!dialog.exec()) return;

	if (!saveInternal(saveURL,format)) {
		KMessageBox::sorry(0,i18n(
			"Could not save file. Check that you have the appropriate rights and that there's enough room left on the device."));
	}
}

bool GVPixmap::saveBeforeClosing() {
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
static GVPixmap::ModifiedBehavior stringToModifiedBehavior(const QString& str) {
	if (str=="yes") return GVPixmap::SAVE_SILENTLY;
	if (str=="no") return GVPixmap::DISCARD_CHANGES;
	return GVPixmap::ASK;
}

static QString modifiedBehaviorToString(GVPixmap::ModifiedBehavior behaviour) {
	if (behaviour==GVPixmap::SAVE_SILENTLY) return "yes";
	if (behaviour==GVPixmap::DISCARD_CHANGES) return "no";
	return "";
}


void GVPixmap::setModifiedBehavior(ModifiedBehavior value) {
	KConfig* config=KGlobal::config();
	KConfigGroupSaver saver(config, CONFIG_NOTIFICATION_MESSAGES_GROUP);
	config->setGroup(CONFIG_NOTIFICATION_MESSAGES_GROUP);
	config->writeEntry(CONFIG_SAVE_AUTOMATICALLY,
		modifiedBehaviorToString(value));
}


GVPixmap::ModifiedBehavior GVPixmap::modifiedBehavior() const {
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
void GVPixmap::load() {
	KURL pixURL=url();
	Q_ASSERT(!pixURL.isEmpty());
	//kdDebug() << "GVPixmap::load " << pixURL.prettyURL() << endl;

	// Abort any active download
	if (d->mDecoderTimer.isActive()) {
		kdDebug() << "GVPixmap::load found an already active loading\n";
		d->finishLoading();
		// FIXME: Emit a cancelled signal instead
		emit loaded(d->mDirURL,d->mFilename);
	}

	// Start the new download
	emit loading();
	if (!KIO::NetAccess::download(pixURL, d->mTempFilePath)) {
		d->mImage.reset();
		emit loaded(d->mDirURL,"");
		return;
	}
	d->mImageFormat=QString(QImage::imageFormat(d->mTempFilePath));

	// Load file. We load it ourself so that we can keep a copy of the
	// compressed data. This is used by JPEG lossless manipulations.
	QFile file(d->mTempFilePath);
	file.open(IO_ReadOnly);
	QDataStream stream(&file);
	d->mCompressedData.resize(file.size());
	stream.readRawBytes(d->mCompressedData.data(),d->mCompressedData.size());
	d->mReadSize=0;

	d->mDecoder=new QImageDecoder(this);
	d->mDecoderTimer.start(0, false);
}


void GVPixmap::loadChunk() {
	//kdDebug() << "GVPixmap::loadChunk\n";
	Q_ASSERT(d->mDecoder);
	int decodedSize=d->mDecoder->decode(
		(const uchar*)(d->mCompressedData.data()+d->mReadSize),
		QMIN(GVPixmapPrivate::CHUNK_SIZE, int(d->mCompressedData.size())-d->mReadSize));

	// Continue loading
	if (decodedSize>0) {
		d->mReadSize+=decodedSize;
		return;
	}

	// Loading finished
	bool ok=decodedSize==0;
	// If async loading failed, try synchronous loading
	if (!ok) {
		//kdDebug() << "GVPixmap::loadChunk async loading failed\n";
		ok=d->mImage.loadFromData(d->mCompressedData);
	} else {
		d->mImage=d->mDecoder->image();
	}

	if (!ok) {
		//kdDebug() << "GVPixmap::loadChunk loading failed\n";
		// Oups, loading failed
		// FIXME: Emit a failed signal instead
		emit loaded(d->mDirURL,d->mFilename);
	} else {
		//kdDebug() << "GVPixmap::loadChunk loading succeded\n";

		// Convert depth if necessary
		// (32 bit depth is necessary for alpha-blending)
		if (d->mImage.depth()<32 && d->mImage.hasAlphaBuffer()) {
			d->mImage=d->mImage.convertDepth(32);
		}

		// If the image is a JPEG, rotate the decode image and the compressed
		// data if necessary, otherwise throw the compressed data away
		if (d->mImageFormat=="JPEG") {
			GVImageUtils::Orientation orientation=GVImageUtils::getOrientation(d->mCompressedData);

			if (orientation!=GVImageUtils::NOT_AVAILABLE && orientation!=GVImageUtils::NORMAL) {
				d->mImage=GVImageUtils::modify(d->mImage, orientation);
				d->mCompressedData=GVJPEGTran::apply(d->mCompressedData,orientation);
				d->mCompressedData=GVImageUtils::setOrientation(d->mCompressedData,GVImageUtils::NORMAL);
			}
		} else {
			d->mCompressedData.resize(0);
		}

		d->loadComment(d->mTempFilePath);
		emit loaded(d->mDirURL,d->mFilename);
	}
	d->finishLoading();
}



bool GVPixmap::saveInternal(const KURL& url, const QString& format) {
	bool result;

	KTempFile tmp;
	tmp.setAutoDelete(true);
	QString path;
	if (url.isLocalFile()) {
		path=url.path();
	} else {
		path=tmp.name();
	}

	if (format=="JPEG" && !d->mCompressedData.isNull()) {
		kdDebug() << "Lossless save\n";
		QFile file(path);
		result=file.open(IO_WriteOnly);
		if (!result) return false;
		QDataStream stream(&file);
		stream.writeRawBytes(d->mCompressedData.data(),d->mCompressedData.size());
		file.close();
	} else {
		result=d->mImage.save(path,format.ascii());
		if (!result) return false;
	}

	// Save comment
	d->saveComment(path);

	if (!url.isLocalFile()) {
		result=KIO::NetAccess::upload(tmp.name(),url);
	}

	if (result) {
		emit saved(url);
		d->mModified=false;
	}

	return result;
}


void GVPixmap::reset() {
	d->mImage.reset();
	d->mCompressedData.resize(0);
	emit loaded(d->mDirURL,d->mFilename);
}



//---------------------------------------------------------------------
//
// QImageConsumer
//
//---------------------------------------------------------------------
void GVPixmap::end() {
	kdDebug() << "GVPixmap::end\n";
}

void GVPixmap::changed(const QRect& rect) {
	kdDebug() << "GVPixmap::changed" << rect << "\n";
	d->mImage=d->mDecoder->image();
	// FIXME: Use an updated(cons QRect& signal)
	emit modified();
}

void GVPixmap::frameDone() {
	kdDebug() << "GVPixmap::frameDone\n";
}

void GVPixmap::frameDone(const QPoint& /*offset*/, const QRect& /*rect*/) {
	kdDebug() << "GVPixmap::frameDone\n";
}

void GVPixmap::setLooping(int) {
	kdDebug() << "GVPixmap::setLooping\n";
}

void GVPixmap::setFramePeriod(int /*milliseconds*/) {
	kdDebug() << "GVPixmap::setFramePeriod\n";
}

void GVPixmap::setSize(int width, int height) {
	kdDebug() << "GVPixmap::setSize " << width << "x" << height << "\n";
	d->mImage=d->mDecoder->image();
}

