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

#include <sys/stat.h> // For S_ISDIR

// Qt includes
#include <qfileinfo.h>
#include <qpainter.h>
#include <qwmatrix.h>
#include <qpaintdevicemetrics.h>

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
	GVPixmap::CommentState mCommentState;
	QString mComment;


	void loadComment(const QString& path) {
		KFileMetaInfo metaInfo=KFileMetaInfo(path);
		KFileMetaInfoItem commentItem;

		mCommentState=GVPixmap::None;

		if (metaInfo.isEmpty()) return;

		// Get the comment
		QString mimeType=metaInfo.mimeType();
		if (mimeType=="image/jpeg") {
			commentItem=metaInfo[JPEG_EXIF_DATA][JPEG_EXIF_COMMENT];
			mCommentState=
				QFileInfo(path).isWritable()?GVPixmap::Writable:GVPixmap::ReadOnly;
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
			mCommentState=GVPixmap::ReadOnly;
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
};


//-------------------------------------------------------------------
//
// GVPixmap
//
//-------------------------------------------------------------------
GVPixmap::GVPixmap(QObject* parent)
		: QObject(parent)
, mModified(false) {
	d=new GVPixmapPrivate;
	d->mCommentState=None;
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
	KURL url=mDirURL;
	url.addPath(mFilename);
	return url;
}


void GVPixmap::setURL(const KURL& paramURL) {
	//kdDebug() << "GVPixmap::setURL " << paramURL.prettyURL() << endl;
	KURL URL(paramURL);
	if (URL.cmp(url())) return;

	// Ask to save if necessary.
	if (!saveIfModified()) {
		// Not saved, notify others that we stay on the image
		emit loaded(mDirURL,mFilename);
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
		mDirURL=URL;
		mFilename="";
	} else {
		mDirURL=URL.upURL();
		mFilename=URL.filename();
	}

	if (mFilename.isEmpty()) {
		reset();
		return;
	}

	load();
}


void GVPixmap::setDirURL(const KURL& paramURL) {
	if (!saveIfModified()) {
		emit loaded(mDirURL,mFilename);
		return;
	}
	mDirURL=paramURL;
	mFilename="";
	reset();
}


void GVPixmap::setFilename(const QString& filename) {
	if (mFilename==filename) return;

	if (!saveIfModified()) {
		emit loaded(mDirURL,mFilename);
		return;
	}
	mFilename=filename;
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
	Q_ASSERT(d->mCommentState==Writable);
	d->mComment=comment;
	mModified=true;
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
	QImage image = mImage;	 // will contain the final image to print

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
		QString fname = minimizeString( mFilename, fMetrics, pdWidth );
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

void GVPixmap::rotateLeft() {
	// Apply the rotation to the compressed data too if available
	if (mImageFormat=="JPEG" && !mCompressedData.isNull()) {
		mCompressedData=GVJPEGTran::apply(mCompressedData,GVImageUtils::Rot270);
	}
	QWMatrix matrix;
	matrix.rotate(-90);
	mImage=mImage.xForm(matrix);
	mModified=true;
	emit modified();
}


void GVPixmap::rotateRight() {
	if (mImageFormat=="JPEG" && !mCompressedData.isNull()) {
		mCompressedData=GVJPEGTran::apply(mCompressedData,GVImageUtils::Rot90);
	}
	QWMatrix matrix;
	matrix.rotate(90);
	mImage=mImage.xForm(matrix);
	mModified=true;
	emit modified();
}


void GVPixmap::mirror() {
	if (mImageFormat=="JPEG" && !mCompressedData.isNull()) {
		mCompressedData=GVJPEGTran::apply(mCompressedData,GVImageUtils::HFlip);
	}
	QWMatrix matrix;
	matrix.scale(-1,1);
	mImage=mImage.xForm(matrix);
	mModified=true;
	emit modified();
}


void GVPixmap::flip() {
	if (mImageFormat=="JPEG" && !mCompressedData.isNull()) {
		mCompressedData=GVJPEGTran::apply(mCompressedData,GVImageUtils::VFlip);
	}
	QWMatrix matrix;
	matrix.scale(1,-1);
	mImage=mImage.xForm(matrix);
	mModified=true;
	emit modified();
}


bool GVPixmap::save() {
	if (!saveInternal(url(),mImageFormat)) {
		KMessageBox::sorry(0,i18n("Could not save file."));
		return false;
	}
	return true;
}


void GVPixmap::saveAs() {
	QString format=mImageFormat;
	KURL saveURL;
	if (url().isLocalFile()) saveURL=url();

	GVImageSaveDialog dialog(saveURL,format,0);
	if (!dialog.exec()) return;

	if (!saveInternal(saveURL,format)) {
		KMessageBox::sorry(0,i18n("Could not save file."));
	}
}

bool GVPixmap::saveIfModified() {
	if (!mModified) return true;
	QString msg=i18n("<qt>The image <b>%1</b> has been modified, do you want to save the changes?</qt>")
				.arg(url().prettyURL());

	int result=KMessageBox::questionYesNoCancel(0, msg, QString::null,
			   i18n("Save"), i18n("Discard"), CONFIG_SAVE_AUTOMATICALLY);

	switch (result) {
	case KMessageBox::Yes:
		return save();
	case KMessageBox::No:
		mModified=false;
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
	if (str=="yes") return GVPixmap::SaveSilently;
	if (str=="no") return GVPixmap::DiscardChanges;
	return GVPixmap::Ask;
}

static QString modifiedBehaviorToString(GVPixmap::ModifiedBehavior behaviour) {
	if (behaviour==GVPixmap::SaveSilently) return "yes";
	if (behaviour==GVPixmap::DiscardChanges) return "no";
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
	emit loading();
	KURL pixURL=url();
	Q_ASSERT(!pixURL.isEmpty());
	//kdDebug() << "GVPixmap::load() " << pixURL.prettyURL() << endl;

	QString path;
	if (!KIO::NetAccess::download(pixURL,path)) {
		mImage.reset();
		emit loaded(mDirURL,"");
		return;
	}

	// Load file. We load it ourself so that we can keep a copy of the
	// compressed data. This is used by JPEG lossless manipulations.
	QFile file(path);
	file.open(IO_ReadOnly);
	QDataStream stream(&file);
	mCompressedData.resize(file.size());
	stream.readRawBytes(mCompressedData.data(),mCompressedData.size());

	// Decode image
	mImageFormat=QString(QImage::imageFormat(path));
	if (mImage.loadFromData(mCompressedData,mImageFormat.ascii())) {
		// Convert depth if necessary
		// (32 bit depth is necessary for alpha-blending)
		if (mImage.depth()<32 && mImage.hasAlphaBuffer()) {
			mImage=mImage.convertDepth(32);
		}

		// If the image is a JPEG, rotate the decode image and the compressed
		// data if necessary, otherwise throw the compressed data away
		if (mImageFormat=="JPEG") {
			GVImageUtils::Orientation orientation=GVImageUtils::getOrientation(mCompressedData);

			if (orientation!=GVImageUtils::NotAvailable && orientation!=GVImageUtils::Normal) {
				mImage=GVImageUtils::rotate(mImage, orientation);
				mCompressedData=GVJPEGTran::apply(mCompressedData,orientation);
				mCompressedData=GVImageUtils::setOrientation(mCompressedData,GVImageUtils::Normal);
			}
		} else {
			mCompressedData.resize(0);
		}

		d->loadComment(path);
	} else {
		mImage.reset();
	}

	KIO::NetAccess::removeTempFile(path);
	emit loaded(mDirURL,mFilename);
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

	if (format=="JPEG" && !mCompressedData.isNull()) {
		//kdDebug() << "Lossless save\n";
		QFile file(path);
		result=file.open(IO_WriteOnly);
		if (!result) return false;
		QDataStream stream(&file);
		stream.writeRawBytes(mCompressedData.data(),mCompressedData.size());
		file.close();
	} else {
		result=mImage.save(path,format.ascii());
		if (!result) return false;
	}

	// Save comment
	d->saveComment(path);

	if (!url.isLocalFile()) {
		result=KIO::NetAccess::upload(tmp.name(),url);
	}

	if (result) {
		emit saved(url);
		mModified=false;
	}

	return result;
}


void GVPixmap::reset() {
	mImage.reset();
	mCompressedData.resize(0);
	emit loaded(mDirURL,mFilename);
}


