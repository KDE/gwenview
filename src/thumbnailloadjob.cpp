// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*  Gwenview - A simple image viewer for KDE
    Copyright 2000-2004 Aurélien Gâteau
    This class is based on the ImagePreviewJob class from Konqueror.
    Original copyright follows.
*/
/*  This file is part of the KDE project
    Copyright (C) 2000 David Faure <faure@kde.org>
                  2000 Carsten Pfeiffer <pfeiffer@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

// Qt
#include <qdir.h>
#include <qfile.h>
#include <qimage.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qtimer.h>

// KDE 
#include <kdebug.h>
#include <kfileitem.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <klargefile.h>
#include <kmdcodec.h>
#include <kstandarddirs.h>
#include <ktempfile.h>

// libjpeg 
#include <setjmp.h>
#define XMD_H
extern "C" {
#include <jpeglib.h>
}

// Local
#include "gvarchive.h"
#include "gvimageutils.h"
#include "thumbnailloadjob.moc"

#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif




QString generateOriginalURI(KURL url) {
	// Don't include the password if any
	url.setPass(QString::null);
	
	// The TMS defines local files as file:///path/to/file instead of KDE's
	// way (file:/path/to/file)
	if (url.protocol() == "file") {
		return "file://" + url.path();
	} else {
		return url.url();
	}
}


QString generateThumbnailPath(const QString& uri) {
	KMD5 md5( QFile::encodeName(uri) );
	QString baseDir=ThumbnailLoadJob::thumbnailBaseDir();
	return baseDir + QString(QFile::encodeName( md5.hexDigest())) + ".png";
}

//------------------------------------------------------------------------
//
// ThumbnailThread
//
//------------------------------------------------------------------------
void ThumbnailThread::load(
	const QString& originalURI, time_t originalTime, int originalSize, const QString& originalMimeType,
	const QString& pixPath,
	const QString& thumbnailPath)
{
	QMutexLocker lock( &mMutex );
	assert( mPixPath.isNull());

	mOriginalURI = TSDeepCopy(originalURI);
	mOriginalTime = originalTime;
	mOriginalSize = originalSize;
	mOriginalMimeType = TSDeepCopy(originalMimeType);
	mPixPath = TSDeepCopy(pixPath);
	mThumbnailPath = TSDeepCopy(thumbnailPath);
	if(!running()) start();
	mCond.wakeOne();
}

void ThumbnailThread::run() {
	QMutexLocker lock( &mMutex );
	while( !testCancel()) {
		// empty mPixPath means nothing to do
		while( mPixPath.isNull()) {
			TSCancellable c( &mCond );
			mCond.wait( &mMutex );
			if( testCancel()) return;
		}
		loadThumbnail();
		mPixPath = QString(); // done, ready for next
		postSignal( SIGNAL( done()));
	}
}

void ThumbnailThread::loadThumbnail() {
	mImage = QImage();
	bool loaded=false;
	int width, height;
	
	// If it's a JPEG, try to load a small image directly from the file
	if (isJPEG(mPixPath)) {
		loaded=loadJPEG(mPixPath, mImage, width, height);
		if (loaded) {
			// Rotate if necessary
			GVImageUtils::Orientation orientation=GVImageUtils::getOrientation(mPixPath);
			mImage=GVImageUtils::modify(mImage,orientation);
		}
	}
	// File is not a JPEG, or JPEG optimized load failed, load file using Qt
	if (!loaded) {
		QImage bigImg;
		if (bigImg.load(mPixPath)) {
			width=bigImg.width();
			height=bigImg.height();
			int thumbSize=ThumbnailSize::biggest().pixelSize();

			if( testCancel()) return;

			if (width<=thumbSize && height<=thumbSize) {
				mImage=bigImg;
			} else {
				mImage=GVImageUtils::scale(bigImg,thumbSize,thumbSize,GVImageUtils::SMOOTH_NONE,QImage::ScaleMin);
			}
			loaded = true;
		}
	}

	if( testCancel()) return;

	if( loaded ) {
		mImage.setText("Thumb::URI", 0, mOriginalURI);
		mImage.setText("Thumb::MTime", 0, QString::number(mOriginalTime));
		mImage.setText("Thumb::Size", 0, QString::number(mOriginalSize));
		mImage.setText("Thumb::Mimetype", 0, mOriginalMimeType);
		mImage.setText("Thumb::Image::Width", 0, QString::number(width));
		mImage.setText("Thumb::Image::Height", 0, QString::number(height));
		mImage.setText("Software", 0, "Gwenview");
		KStandardDirs::makeDir(ThumbnailLoadJob::thumbnailBaseDir(), 0700);
		mImage.save(mThumbnailPath, "PNG");
	}
}


bool ThumbnailThread::isJPEG(const QString& name) {
	QString format=QImageIO::imageFormat(name);
	return format=="JPEG";
}



struct GVJPEGFatalError : public jpeg_error_mgr {
	jmp_buf mJmpBuffer;

	static void handler(j_common_ptr cinfo) {
		GVJPEGFatalError* error=static_cast<GVJPEGFatalError*>(cinfo->err);
		(error->output_message)(cinfo);
		longjmp(error->mJmpBuffer,1);
	}
};

bool ThumbnailThread::loadJPEG( const QString &pixPath, QImage& image, int& width, int& height) {
	struct jpeg_decompress_struct cinfo;

	// Open file
	FILE* inputFile=fopen(QFile::encodeName( pixPath ).data(), "rb");
	if(!inputFile) return false;
	
	// Error handling
	struct GVJPEGFatalError jerr;
	cinfo.err = jpeg_std_error(&jerr);
	cinfo.err->error_exit = GVJPEGFatalError::handler;
	if (setjmp(jerr.mJmpBuffer)) {
		jpeg_destroy_decompress(&cinfo);
		fclose(inputFile);
		return false;
	}

	// Init decompression
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, inputFile);
	jpeg_read_header(&cinfo, TRUE);
	width=cinfo.image_width;
	height=cinfo.image_height;

	// Get image size and check if we need a thumbnail
	int size=ThumbnailSize::biggest().pixelSize();
	int imgSize = QMAX(cinfo.image_width, cinfo.image_height);

	if (imgSize<=size) {
		fclose(inputFile);
		return image.load(pixPath);
	}	

	// Compute scale value
	int scale=1;
	while(size*scale*2<=imgSize) {
		scale*=2;
	}
	if(scale>8) scale=8;

	cinfo.scale_num=1;
	cinfo.scale_denom=scale;

	// Create QImage
	jpeg_start_decompress(&cinfo);

	switch(cinfo.output_components) {
	case 3:
	case 4:
		image.create( cinfo.output_width, cinfo.output_height, 32 );
		break;
	case 1: // B&W image
		image.create( cinfo.output_width, cinfo.output_height, 8, 256 );
		for (int i=0; i<256; i++)
			image.setColor(i, qRgb(i,i,i));
		break;
	default:
		jpeg_destroy_decompress(&cinfo);
		fclose(inputFile);
		return false;
	}

	uchar** lines = image.jumpTable();
	while (cinfo.output_scanline < cinfo.output_height) {
		jpeg_read_scanlines(&cinfo, lines + cinfo.output_scanline, cinfo.output_height);
	}
	jpeg_finish_decompress(&cinfo);

// Expand 24->32 bpp
	if ( cinfo.output_components == 3 ) {
		for (uint j=0; j<cinfo.output_height; j++) {
			uchar *in = image.scanLine(j) + cinfo.output_width*3;
			QRgb *out = (QRgb*)( image.scanLine(j) );

			for (uint i=cinfo.output_width; i--; ) {
				in-=3;
				out[i] = qRgb(in[0], in[1], in[2]);
			}
		}
	}

	int newMax = QMAX(cinfo.output_width, cinfo.output_height);
	int newx = size*cinfo.output_width / newMax;
	int newy = size*cinfo.output_height / newMax;

	image=GVImageUtils::scale(image,newx, newy,GVImageUtils::SMOOTH_NONE);

	jpeg_destroy_decompress(&cinfo);
	fclose(inputFile);

	return true;
}


QImage ThumbnailThread::popThumbnail() {
	QMutexLocker lock( &mMutex );
	QImage ret = mImage; // no deep copy
	mImage = QImage(); // thread no longer accesses the image
	mPixPath = QString(); // waiting for next job
	return ret;
}


//------------------------------------------------------------------------
//
// ThumbnailLoadJob static methods
//
//------------------------------------------------------------------------
QString ThumbnailLoadJob::thumbnailBaseDir() {
	static QString dir;
	if (!dir.isEmpty()) return dir;
    dir = QDir::homeDirPath() + "/.thumbnails/normal/";
	return dir;
}


void ThumbnailLoadJob::deleteImageThumbnail(const KURL& url) {
	QString uri=generateOriginalURI(url);
	QFile::remove(generateThumbnailPath(uri));
}


//------------------------------------------------------------------------
//
// ThumbnailLoadJob implementation
//
//------------------------------------------------------------------------
ThumbnailLoadJob::ThumbnailLoadJob(const KFileItemList* items, ThumbnailSize size)
: KIO::Job(false), mState( STATE_NEXTTHUMB ), mThumbnailSize(size), mSuspended( false )
{
	LOG("");
    
	mBrokenPixmap=KGlobal::iconLoader()->loadIcon("file_broken", 
		KIcon::NoGroup, ThumbnailSize(ThumbnailSize::SMALL).pixelSize());

	// Look for images and store the items in our todo list
	mItems=*items;

	if (mItems.isEmpty()) return;

	connect( &mThumbnailThread, SIGNAL( done()), SLOT( thumbnailReady()));
	
	// Move to the first item
	mNextItem = mItems.first();
}


ThumbnailLoadJob::~ThumbnailLoadJob() {
	LOG("");
	mThumbnailThread.cancel();
	mThumbnailThread.wait();
}


void ThumbnailLoadJob::start() {
	if (mItems.isEmpty()) {
		LOG("Nothing to do");
		emit result(this);
		delete this;
		return;
	}

	determineNextIcon();
}

void ThumbnailLoadJob::suspend() {
	mSuspended = true;
}

void ThumbnailLoadJob::resume() {
	if( !mSuspended ) return;
	mSuspended = false;
	if( mState == STATE_NEXTTHUMB ) // don't load next while already loading
		determineNextIcon();
}

//-Internal--------------------------------------------------------------
void ThumbnailLoadJob::appendItem(const KFileItem* item) {
	mItems.append(item);
}


void ThumbnailLoadJob::itemRemoved(const KFileItem* item) {
	mItems.removeRef(item);
	if( item == mNextItem ) mNextItem = mItems.current();

	if (item == mCurrentItem) {
		// Abort
		subjobs.first()->kill();
		subjobs.removeFirst();
		determineNextIcon();
	}
}


bool ThumbnailLoadJob::setNextItem(const KFileItem* item) {
	// Move mNextItem to the next item to be processed
	if( mItems.containsRef( item )) {
		mNextItem = item;
		return true;
	}
	mNextItem = mItems.first();
	return false;
}


void ThumbnailLoadJob::determineNextIcon() {
	mState = STATE_NEXTTHUMB;
	if( mSuspended ) {
		return;
	}
	if( mNextItem != mItems.current()) mItems.findRef( mNextItem );
	assert( mNextItem == mItems.current());
	// Skip non images 
	while (true) {
		KFileItem* item=mItems.current();
		if (!item) break;
		if (item->isDir() || GVArchive::fileItemIsArchive(item)) {
			mItems.remove();
			mNextItem = mItems.current();
		} else {
			break;
		}
	}
	
	// No more items ?
	if (mItems.isEmpty()) {
		// Done
		LOG("emitting result");
		emit result(this);
		delete this;
	} else {
		// First, stat the orig file
		mState = STATE_STATORIG;
		mOriginalTime = 0;
		assert( mNextItem == mItems.current());
		mCurrentItem = mNextItem;
		mCurrentURL = mCurrentItem->url();
		mCurrentURL.cleanPath();
		// Do direct stat instead of using KIO if the file is local (faster)
		if( mCurrentURL.isLocalFile()
			&& !KIO::probably_slow_mounted( mCurrentURL.path())) {
			KDE_struct_stat buff;
			if ( KDE_stat( QFile::encodeName(mCurrentURL.path()), &buff ) == 0 )  {
				mOriginalTime = buff.st_mtime;
				QTimer::singleShot( 0, this, SLOT( checkThumbnail()));
			}
		}
		if( mOriginalTime == 0 ) { // KIO must be used
			KIO::Job* job = KIO::stat(mCurrentURL,false);
			LOG( "KIO::stat orig " << mCurrentURL.url() );
			addSubjob(job);
		}
		mItems.remove();
		mNextItem = mItems.current();
	}
}


void ThumbnailLoadJob::slotResult(KIO::Job * job) {
	subjobs.remove(job);
	Q_ASSERT(subjobs.isEmpty());	// We should have only one job at a time ...

	switch (mState) {
	case STATE_NEXTTHUMB:
		Q_ASSERT( false );
		determineNextIcon();
		return;
	case STATE_STATORIG: {
		// Could not stat original, drop this one and move on to the next one
		if (job->error()) {
			emitThumbnailLoadingFailed();
			determineNextIcon();
			return;
		}

		// Get modification time of the original file
		KIO::UDSEntry entry = static_cast<KIO::StatJob*>(job)->statResult();
		KIO::UDSEntry::ConstIterator it= entry.begin();
		mOriginalTime = 0;
		for (; it!=entry.end(); it++) {
			if ((*it).m_uds == KIO::UDS_MODIFICATION_TIME) {
				mOriginalTime = (time_t)((*it).m_long);
				break;
			}
		}
		checkThumbnail();
		return;
	}

	case STATE_DOWNLOADORIG: 
		if (job->error()) {
			emitThumbnailLoadingFailed();
			mState=STATE_DELETETEMP;
			LOG("Delete temp file " << mTempURL.prettyURL());
			addSubjob(KIO::file_delete(mTempURL,false));
			mTempURL = KURL();
		} else {
			startCreatingThumbnail(mTempURL.path());
		}
		return;

	case STATE_CREATETHUMB:
		assert( false ); // thumbnailReady() must handle this

	case STATE_DELETETEMP:
		determineNextIcon();
		return;
	}
}


void ThumbnailLoadJob::thumbnailReady() {
	QImage img = mThumbnailThread.popThumbnail();
	if ( !img.isNull()) {
		emitThumbnailLoaded(img);
	} else {
		emitThumbnailLoadingFailed();
	}
	if( !mTempURL.isEmpty()) {
		mState=STATE_DELETETEMP;
		LOG("Delete temp file " << mTempURL.prettyURL());
		addSubjob(KIO::file_delete(mTempURL,false));
		mTempURL = KURL();
	} else {
		determineNextIcon();
	}
}

void ThumbnailLoadJob::checkThumbnail() {
	// If we are in the thumbnail dir, just load the file
	if (mCurrentURL.isLocalFile()
		&& mCurrentURL.directory(false)==ThumbnailLoadJob::thumbnailBaseDir())
	{
		emitThumbnailLoaded(QImage(mCurrentURL.path()));
		determineNextIcon();
		return;
	}
	
	mOriginalURI=generateOriginalURI(mCurrentURL);
	mThumbnailPath=generateThumbnailPath(mOriginalURI);

	LOG("Stat thumb " << mThumbnailPath);
	
	QImage thumb;
	if ( thumb.load(mThumbnailPath) ) { 
		if (thumb.text("Thumb::URI", 0) == mOriginalURI &&
			thumb.text("Thumb::MTime", 0).toInt() == mOriginalTime )
		{
			emitThumbnailLoaded(thumb);
			determineNextIcon();
			return;
		}
	}

	// Thumbnail not found or not valid
	// If the original is a local file, create the thumbnail
	// and proceed to next icon, otherwise download the original
	if (mCurrentURL.isLocalFile()) {
		startCreatingThumbnail(mCurrentURL.path());
	} else {
		mState=STATE_DOWNLOADORIG;
		KTempFile tmpFile;
		mTempURL.setPath(tmpFile.name());
		KIO::Job* job=KIO::file_copy(mCurrentURL,mTempURL,-1,true,false,false);
		LOG("Download remote file " << mCurrentURL.prettyURL());
		addSubjob(job);
	}
}

void ThumbnailLoadJob::startCreatingThumbnail(const QString& pixPath) {
	LOG("Creating thumbnail from " << pixPath);
	mThumbnailThread.load( mOriginalURI, mOriginalTime, mCurrentItem->size(), mCurrentItem->mimetype(), pixPath, mThumbnailPath);
	mState = STATE_CREATETHUMB;
}


void ThumbnailLoadJob::emitThumbnailLoaded(const QImage& img) {
	ThumbnailSize biggest=ThumbnailSize::biggest();

	if (mThumbnailSize==biggest) {
		emit thumbnailLoaded(mCurrentItem, QPixmap(img));
		return;
	}
	
	// Do we need to scale down the thumbnail ?
	int biggestDimension=QMAX(img.width(), img.height());
	int thumbPixelSize=mThumbnailSize.pixelSize();
	if (biggestDimension<=thumbPixelSize) {
		emit thumbnailLoaded(mCurrentItem, QPixmap(img));
		return;
	}

	// Scale thumbnail
	QImage img2=GVImageUtils::scale(img,thumbPixelSize, thumbPixelSize, GVImageUtils::SMOOTH_FAST,QImage::ScaleMin);
	emit thumbnailLoaded(mCurrentItem, QPixmap(img2));
}


void ThumbnailLoadJob::emitThumbnailLoadingFailed() {
	emit thumbnailLoaded(mCurrentItem, mBrokenPixmap);
}



