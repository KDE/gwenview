// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*	Gwenview - A simple image viewer for KDE
	Copyright 2000-2004 Aurélien Gâteau
	This class is based on the ImagePreviewJob class from Konqueror.
	Original copyright follows.
*/
/*	This file is part of the KDE project
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
#include "thumbnailloadjob.moc"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

// Qt
#include <qdir.h>
#include <qfile.h>
#include <qimage.h>
#include <qpixmap.h>
#include <qtimer.h>

// KDE 
#include <kapplication.h>
#include <kdebug.h>
#include <kfileitem.h>
#include <kiconloader.h>
#include <kio/previewjob.h>
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
#include "cache.h"
#include "mimetypeutils.h"
#include "miscconfig.h"
#include "imageutils/jpegcontent.h"
#include "imageutils/imageutils.h"
#include "thumbnailsize.h"
#include "fileviewconfig.h"
namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif


static QString generateOriginalURI(KURL url) {
	// Don't include the password if any
	url.setPass(QString::null);
	return url.url();
}


static QString generateThumbnailPath(const QString& uri, int size) {
	KMD5 md5( QFile::encodeName(uri) );
	QString baseDir=ThumbnailLoadJob::thumbnailBaseDir(size);
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
	const QString& thumbnailPath,
	int size, bool storeThumbnail)
{
	QMutexLocker lock( &mMutex );
	assert( mPixPath.isNull());

	mOriginalURI = TSDeepCopy(originalURI);
	mOriginalTime = originalTime;
	mOriginalSize = originalSize;
	mOriginalMimeType = TSDeepCopy(originalMimeType);
	mPixPath = TSDeepCopy(pixPath);
	mThumbnailPath = TSDeepCopy(thumbnailPath);
	mThumbnailSize = size;
	mStoreThumbnailsInCache = storeThumbnail;
	if(!running()) start();
	mCond.wakeOne();
}

void ThumbnailThread::run() {
	QMutexLocker lock( &mMutex );
	while( !testCancel()) {
		// empty mPixPath means nothing to do
		while( mPixPath.isNull()) {
			mCond.cancellableWait( &mMutex );
			if( testCancel()) return;
		}
		loadThumbnail();
		mPixPath = QString(); // done, ready for next
		QSize size(mOriginalWidth, mOriginalHeight);
		emitCancellableSignal( this, SIGNAL( done( const QImage&, const QSize&)), mImage, size);
	}
}

void ThumbnailThread::loadThumbnail() {
	mImage = QImage();
	bool loaded=false;
	bool needCaching=true;
	
	// If it's a JPEG, try to load a small image directly from the file
	if (isJPEG()) {
		ImageUtils::JPEGContent content;
		content.load(mPixPath);
		mOriginalWidth = content.size().width();
		mOriginalHeight = content.size().height();
		mImage = content.thumbnail();
	
		if( !mImage.isNull()
			&& ( mImage.width() >= mThumbnailSize // don't use small thumbnails
			|| mImage.height() >= mThumbnailSize )) {
			loaded = true;
			needCaching = false;
		}
		if(!loaded) {
			loaded=loadJPEG();
		}
		if (loaded && MiscConfig::autoRotateImages()) {
			// Rotate if necessary
			ImageUtils::Orientation orientation = content.orientation();
			mImage=ImageUtils::transform(mImage,orientation);
		}
	}
	// File is not a JPEG, or JPEG optimized load failed, load file using Qt
	if (!loaded) {
		QImage originalImage;
		if (originalImage.load(mPixPath)) {
			mOriginalWidth=originalImage.width();
			mOriginalHeight=originalImage.height();
			int thumbSize=mThumbnailSize<=ThumbnailSize::NORMAL ? ThumbnailSize::NORMAL : ThumbnailSize::LARGE;

			if( testCancel()) return;

			if (QMAX(mOriginalWidth, mOriginalHeight)<=thumbSize ) {
				mImage=originalImage;
				needCaching = false;
			} else {
				mImage=ImageUtils::scale(originalImage,thumbSize,thumbSize,ImageUtils::SMOOTH_FAST,QImage::ScaleMin);
			}
			loaded = true;
		}
	}

	if( testCancel()) return;

	if( mStoreThumbnailsInCache && needCaching ) {
		mImage.setText("Thumb::URI", 0, mOriginalURI);
		mImage.setText("Thumb::MTime", 0, QString::number(mOriginalTime));
		mImage.setText("Thumb::Size", 0, QString::number(mOriginalSize));
		mImage.setText("Thumb::Mimetype", 0, mOriginalMimeType);
		mImage.setText("Thumb::Image::Width", 0, QString::number(mOriginalWidth));
		mImage.setText("Thumb::Image::Height", 0, QString::number(mOriginalHeight));
		mImage.setText("Software", 0, "Gwenview");

		QString thumbnailDir = ThumbnailLoadJob::thumbnailBaseDir(mThumbnailSize);
		KStandardDirs::makeDir(thumbnailDir, 0700);

		KTempFile tmp(thumbnailDir + "/gwenview", ".png");
		tmp.setAutoDelete(true);
		if (tmp.status()!=0) {
			QString reason( strerror(tmp.status()) );
			kdWarning() << "Could not create a temporary file.\nReason: " << reason << endl;
			return;
		}

		if (!mImage.save(tmp.name(), "PNG")) {
			kdWarning() << "Could not save thumbnail for file " << mOriginalURI << endl;
			return;
		}
		
		rename(QFile::encodeName(tmp.name()), QFile::encodeName(mThumbnailPath));
	}
}


bool ThumbnailThread::isJPEG() {
	QString format=QImageIO::imageFormat(mPixPath);
	return format=="JPEG";
}



struct JPEGFatalError : public jpeg_error_mgr {
	jmp_buf mJmpBuffer;

	static void handler(j_common_ptr cinfo) {
		JPEGFatalError* error=static_cast<JPEGFatalError*>(cinfo->err);
		(error->output_message)(cinfo);
		longjmp(error->mJmpBuffer,1);
	}
};

bool ThumbnailThread::loadJPEG() {
	struct jpeg_decompress_struct cinfo;

	// Open file
	FILE* inputFile=fopen(QFile::encodeName( mPixPath ).data(), "rb");
	if(!inputFile) return false;
	
	// Error handling
	struct JPEGFatalError jerr;
	cinfo.err = jpeg_std_error(&jerr);
	cinfo.err->error_exit = JPEGFatalError::handler;
	if (setjmp(jerr.mJmpBuffer)) {
		jpeg_destroy_decompress(&cinfo);
		fclose(inputFile);
		return false;
	}

	// Init decompression
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, inputFile);
	jpeg_read_header(&cinfo, TRUE);

	// Get image size and check if we need a thumbnail
	int size= mThumbnailSize <= ThumbnailSize::NORMAL ? ThumbnailSize::NORMAL : ThumbnailSize::LARGE;
	int imgSize = QMAX(cinfo.image_width, cinfo.image_height);

	if (imgSize<=size) {
		fclose(inputFile);
		return mImage.load(mPixPath);
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
		mImage.create( cinfo.output_width, cinfo.output_height, 32 );
		break;
	case 1: // B&W image
		mImage.create( cinfo.output_width, cinfo.output_height, 8, 256 );
		for (int i=0; i<256; i++)
			mImage.setColor(i, qRgb(i,i,i));
		break;
	default:
		jpeg_destroy_decompress(&cinfo);
		fclose(inputFile);
		return false;
	}

	uchar** lines = mImage.jumpTable();
	while (cinfo.output_scanline < cinfo.output_height) {
		jpeg_read_scanlines(&cinfo, lines + cinfo.output_scanline, cinfo.output_height);
	}
	jpeg_finish_decompress(&cinfo);

// Expand 24->32 bpp
	if ( cinfo.output_components == 3 ) {
		for (uint j=0; j<cinfo.output_height; j++) {
			uchar *in = mImage.scanLine(j) + cinfo.output_width*3;
			QRgb *out = (QRgb*)( mImage.scanLine(j) );

			for (uint i=cinfo.output_width; i--; ) {
				in-=3;
				out[i] = qRgb(in[0], in[1], in[2]);
			}
		}
	}

	int newMax = QMAX(cinfo.output_width, cinfo.output_height);
	int newx = size*cinfo.output_width / newMax;
	int newy = size*cinfo.output_height / newMax;

	mImage=ImageUtils::scale(mImage,newx, newy,ImageUtils::SMOOTH_FAST);

	jpeg_destroy_decompress(&cinfo);
	fclose(inputFile);

	return true;
}


//------------------------------------------------------------------------
//
// ThumbnailLoadJob static methods
//
//------------------------------------------------------------------------
QString ThumbnailLoadJob::thumbnailBaseDir() {
	static QString dir;
	if (!dir.isEmpty()) return dir;
	dir=QDir::homeDirPath() + "/.thumbnails/";
	return dir;
}


QString ThumbnailLoadJob::thumbnailBaseDir(int size) {
	QString dir = thumbnailBaseDir();
	if (size<=ThumbnailSize::NORMAL) {
		dir+="normal/";
	} else {
		dir+="large/";
	}
	return dir;
}


void ThumbnailLoadJob::deleteImageThumbnail(const KURL& url) {
	QString uri=generateOriginalURI(url);
	QFile::remove(generateThumbnailPath(uri, ThumbnailSize::NORMAL));
	QFile::remove(generateThumbnailPath(uri, ThumbnailSize::LARGE));
}


//------------------------------------------------------------------------
//
// ThumbnailLoadJob implementation
//
//------------------------------------------------------------------------


/*

 This class tries to first generate the most important thumbnails, i.e.
 first the currently selected one, then the ones that are visible, and then
 the rest, the closer the the currently selected one the sooner

 mAllItems contains all thumbnails
 mItems contains pending thumbnails, in the priority order
 mCurrentItem is currently processed thumbnail, already removed from mItems
 mProcessedState needs to match mAllItems, and contains information about every
   thumbnail whether it has been already processed

 thumbnailIndex() returns index of a thumbnail in mAllItems, or -1
 updateItemsOrder() builds mItems from mAllItems
*/

ThumbnailLoadJob::ThumbnailLoadJob(const QValueVector<const KFileItem*>* items, int size)
: KIO::Job(false), mState( STATE_NEXTTHUMB ),
  mCurrentVisibleIndex( -1 ), mFirstVisibleIndex( -1 ), mLastVisibleIndex( -1 ),
  mThumbnailSize(size), mSuspended( false )
{
	LOG("");
	
	mBrokenPixmap=KGlobal::iconLoader()->loadIcon("file_broken", 
		KIcon::NoGroup, ThumbnailSize::MIN);

	// Look for images and store the items in our todo list
	Q_ASSERT(!items->empty());
	mAllItems=*items;
	mProcessedState.resize( mAllItems.count());
	qFill( mProcessedState.begin(), mProcessedState.end(), false );
	mCurrentItem = NULL;

	connect(&mThumbnailThread, SIGNAL(done(const QImage&, const QSize&)),
		SLOT(thumbnailReady(const QImage&, const QSize&)) );
	Cache::instance()->updateAge(); // see addThumbnail in Cache
}


ThumbnailLoadJob::~ThumbnailLoadJob() {
	LOG("");
	mThumbnailThread.cancel();
	mThumbnailThread.wait();
}


void ThumbnailLoadJob::start() {
	// build mItems from mAllItems if not done yet
	if (mLastVisibleIndex == -1 ) {
		setPriorityItems( NULL, NULL, NULL );
	}
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
	int index = thumbnailIndex( item );
	if( index >= 0 ) {
		mProcessedState[ index ] = false;
		return;
	}
	mAllItems.append(item);
	mProcessedState.append( false );
	updateItemsOrder();
}


void ThumbnailLoadJob::itemRemoved(const KFileItem* item) {
	Q_ASSERT(item);

	// If we are removing the next item, update to be the item after or the
	// first if we removed the last item
	mItems.remove( item );
	int index = thumbnailIndex( item );
	if( index >= 0 ) {
		mAllItems.erase( mAllItems.begin() + index );
		mProcessedState.erase( mProcessedState.begin() + index );
	}

	if (item == mCurrentItem) {
		// Abort
		mCurrentItem = NULL;
		if (subjobs.first()) {
			subjobs.first()->kill();
			subjobs.removeFirst();
		}
		determineNextIcon();
	}
}


void ThumbnailLoadJob::setPriorityItems(const KFileItem* current, const KFileItem* first, const KFileItem* last) {
	if( mAllItems.isEmpty()) {
		mCurrentVisibleIndex = mFirstVisibleIndex = mLastVisibleIndex = 0;
		return;
	}
	mFirstVisibleIndex = -1;
	mLastVisibleIndex = - 1;
	mCurrentVisibleIndex = -1;
	if( first != NULL ) mFirstVisibleIndex = thumbnailIndex( first );
	if( last != NULL ) mLastVisibleIndex = thumbnailIndex( last );
	if( current != NULL ) mCurrentVisibleIndex = thumbnailIndex( current );
	if( mFirstVisibleIndex == -1 ) mFirstVisibleIndex = 0;
	if( mLastVisibleIndex == -1 ) mLastVisibleIndex = mAllItems.count() - 1;
	if( mCurrentVisibleIndex == -1 ) mCurrentVisibleIndex = mFirstVisibleIndex;
	updateItemsOrder();
}

void ThumbnailLoadJob::updateItemsOrder() {
	mItems.clear();
	int forward = mCurrentVisibleIndex + 1;
	int backward = mCurrentVisibleIndex;
	int first = mFirstVisibleIndex;
	int last = mLastVisibleIndex;
	updateItemsOrderHelper( forward, backward, first, last );
	if( first != 0 || last != int( mAllItems.count()) - 1 ) {
		// add non-visible items
		updateItemsOrderHelper( last + 1, first - 1, 0, mAllItems.count() - 1);
	}
}

void ThumbnailLoadJob::updateItemsOrderHelper( int forward, int backward, int first, int last ) {
	// start from the current item, add one following it, and one preceding it, for all visible items
	while( forward <= last || backward >= first ) {
		// start with backward - that's the curent item for the first time
		while( backward >= first ) {
			if( !mProcessedState[ backward ] ) {
				mItems.append( mAllItems[ backward ] );
				--backward;
				break;
			}
			--backward;
		}
		while( forward <= last ) {
			if( !mProcessedState[ forward ] ) {
				mItems.append( mAllItems[ forward ] );
				++forward;
				break;
			}
			++forward;
		}
	}
}

void ThumbnailLoadJob::determineNextIcon() {
	mState = STATE_NEXTTHUMB;
	if( mSuspended ) {
		return;
	}

	// No more items ?
	if (mItems.isEmpty()) {
		// Done
		LOG("emitting result");
		emit result(this);
		delete this;
		return;
	}

	mCurrentItem=mItems.first();
	mItems.pop_front();
	Q_ASSERT( !mProcessedState[ thumbnailIndex( mCurrentItem )] );
	mProcessedState[ thumbnailIndex( mCurrentItem )] = true;

	// First, stat the orig file
	mState = STATE_STATORIG;
	mOriginalTime = 0;
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
		job->setWindow(KApplication::kApplication()->mainWidget());
		LOG( "KIO::stat orig " << mCurrentURL.url() );
		addSubjob(job);
	}
}


void ThumbnailLoadJob::slotResult(KIO::Job * job) {
	LOG(mState);
	subjobs.remove(job);
	Q_ASSERT(subjobs.isEmpty());	// We should have only one job at a time ...

	switch (mState) {
	case STATE_NEXTTHUMB:
		Q_ASSERT(false);
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
		for (; it!=entry.end(); ++it) {
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
			LOG("Delete temp file " << mTempPath);
			QFile::remove(mTempPath);
			mTempPath = QString::null;
			determineNextIcon();
		} else {
			startCreatingThumbnail(mTempPath);
		}
		return;
	
	case STATE_PREVIEWJOB:
		determineNextIcon();
		return;
	}
}


void ThumbnailLoadJob::thumbnailReady( const QImage& im, const QSize& _size) {
	QImage img = TSDeepCopy( im );
	QSize size = _size;
	if ( !img.isNull()) {
		emitThumbnailLoaded(img, size);
	} else {
		emitThumbnailLoadingFailed();
	}
	if( !mTempPath.isEmpty()) {
		LOG("Delete temp file " << mTempPath);
		QFile::remove(mTempPath);
		mTempPath = QString::null;
	}
	determineNextIcon();
}

void ThumbnailLoadJob::checkThumbnail() {
	// If we are in the thumbnail dir, just load the file
	if (mCurrentURL.isLocalFile()
		&& mCurrentURL.directory(false).startsWith(thumbnailBaseDir()) )
	{
		QImage image(mCurrentURL.path());
		emitThumbnailLoaded(image, image.size());
		determineNextIcon();
		return;
	}
	QSize imagesize;
	if( mOriginalTime == time_t( Cache::instance()->timestamp( mCurrentURL ).toTime_t())) {
		QPixmap cached = Cache::instance()->thumbnail( mCurrentURL, imagesize );
		if( !cached.isNull()) {
			emit thumbnailLoaded(mCurrentItem, cached, imagesize);
			determineNextIcon();
			return;
		}
	}
	
	mOriginalURI=generateOriginalURI(mCurrentURL);
	mThumbnailPath=generateThumbnailPath(mOriginalURI, mThumbnailSize);

	LOG("Stat thumb " << mThumbnailPath);
	
	QImage thumb;
	if ( thumb.load(mThumbnailPath) ) { 
		if (thumb.text("Thumb::URI", 0) == mOriginalURI &&
			thumb.text("Thumb::MTime", 0).toInt() == mOriginalTime )
		{
			int width=0, height=0;
			QSize size;
			bool ok;

			width=thumb.text("Thumb::Image::Width", 0).toInt(&ok);
			if (ok) height=thumb.text("Thumb::Image::Height", 0).toInt(&ok);
			if (ok) {
				size=QSize(width, height);
			} else {
				LOG("Thumbnail for " << mOriginalURI << " does not contain correct image size information");
				KFileMetaInfo fmi(mCurrentURL);
				if (fmi.isValid()) {
					KFileMetaInfoItem item=fmi.item("Dimensions");
					if (item.isValid()) {
						size=item.value().toSize();
					} else {
						LOG("KFileMetaInfoItem for " << mOriginalURI << " did not get image size information");
					}
				} else {
					LOG("Could not get a valid KFileMetaInfo instance for " << mOriginalURI);
				}
			}
			emitThumbnailLoaded(thumb, size);
			determineNextIcon();
			return;
		}
	}

	// Thumbnail not found or not valid
	if ( MimeTypeUtils::rasterImageMimeTypes().contains(mCurrentItem->mimetype()) ) {
		// This is a raster image
		if (mCurrentURL.isLocalFile()) {
			// Original is a local file, create the thumbnail
			startCreatingThumbnail(mCurrentURL.path());
		} else {
			// Original is remote, download it
			mState=STATE_DOWNLOADORIG;
			KTempFile tmpFile;
			mTempPath=tmpFile.name();
			KURL url;
			url.setPath(mTempPath);
			KIO::Job* job=KIO::file_copy(mCurrentURL, url,-1,true,false,false);
			job->setWindow(KApplication::kApplication()->mainWidget());
			LOG("Download remote file " << mCurrentURL.prettyURL());
			addSubjob(job);
		}
	} else {
		// Not a raster image, use a KPreviewJob
		mState=STATE_PREVIEWJOB;
		KFileItemList list;
		list.append(mCurrentItem);
		KIO::Job* job=KIO::filePreview(list, mThumbnailSize);
		job->setWindow(KApplication::kApplication()->mainWidget());
		connect(job, SIGNAL(gotPreview(const KFileItem*, const QPixmap&)),
			this, SLOT(slotGotPreview(const KFileItem*, const QPixmap&)) );
		connect(job, SIGNAL(failed(const KFileItem*)),
			this, SLOT(emitThumbnailLoadingFailed()) );
		addSubjob(job);
		return;
	}
}

void ThumbnailLoadJob::startCreatingThumbnail(const QString& pixPath) {
	LOG("Creating thumbnail from " << pixPath);
	mThumbnailThread.load( mOriginalURI, mOriginalTime, mCurrentItem->size(),
		mCurrentItem->mimetype(), pixPath, mThumbnailPath, mThumbnailSize,
		FileViewConfig::storeThumbnailsInCache());
}


void ThumbnailLoadJob::slotGotPreview(const KFileItem* item, const QPixmap& pixmap) {
	LOG("");
	QSize size;
	emit thumbnailLoaded(item, pixmap, size);
}


void ThumbnailLoadJob::emitThumbnailLoaded(const QImage& img, QSize size) {
	int biggestDimension=QMAX(img.width(), img.height());

	QImage thumbImg;
	if (biggestDimension>mThumbnailSize) {
		// Scale down thumbnail if necessary
		thumbImg=ImageUtils::scale(img,mThumbnailSize, mThumbnailSize, ImageUtils::SMOOTH_FAST,QImage::ScaleMin);
	} else {
		thumbImg=img;
	}
	QDateTime tm;
	tm.setTime_t( mOriginalTime );
	QPixmap thumb( thumbImg ); // store as QPixmap in cache (faster to retrieve, no conversion needed)
	Cache::instance()->addThumbnail( mCurrentURL, thumb, size, tm );
	emit thumbnailLoaded(mCurrentItem, thumb, size);
}


void ThumbnailLoadJob::emitThumbnailLoadingFailed() {
	QSize size;
	emit thumbnailLoaded(mCurrentItem, mBrokenPixmap, size);
}


} // namespace
