// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*	Gwenview - A simple image viewer for KDE
	Copyright 2000-2007 Aurélien Gâteau
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
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.
*/
#include "thumbnailloadjob.moc"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

// Qt
#include <QDir>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QMatrix>
#include <QPixmap>
#include <QTimer>

// KDE
#include <kapplication.h>
#include <kcodecs.h>
#include <kdebug.h>
#include <kde_file.h>
#include <kfileitem.h>
#include <kiconloader.h>
#include <kio/jobuidelegate.h>
#include <kio/previewjob.h>
#include <kmountpoint.h>
#include <kstandarddirs.h>
#include <ktemporaryfile.h>

// libjpeg
#include <setjmp.h>
#define XMD_H
extern "C" {
#include <jpeglib.h>
}

// Local
#include "mimetypeutils.h"
#include "jpegcontent.h"
#include "imageutils.h"
namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << x << endl
#else
#define LOG(x) ;
#endif


static const int THUMBNAILSIZE_MIN = 48;
static const int THUMBNAILSIZE_NORMAL = 128;
static const int THUMBNAILSIZE_LARGE = 256;

static QString generateOriginalUri(KUrl url) {
	// Don't include the password if any
	url.setPass(QString::null);	//krazy:exclude=nullstrassign for old broken gcc
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
ThumbnailThread::ThumbnailThread()
: mCancel(false) {}

void ThumbnailThread::load(
	const QString& originalUri, time_t originalTime, int originalSize, const QString& originalMimeType,
	const QString& pixPath,
	const QString& thumbnailPath,
	int size)
{
	QMutexLocker lock( &mMutex );
	assert( mPixPath.isNull());

	mOriginalUri = originalUri;
	mOriginalTime = originalTime;
	mOriginalSize = originalSize;
	mOriginalMimeType = originalMimeType;
	mPixPath = pixPath;
	mThumbnailPath = thumbnailPath;
	mThumbnailSize = size;
	if(!isRunning()) start();
	mCond.wakeOne();
}

bool ThumbnailThread::testCancel() {
	QMutexLocker lock( &mMutex );
	return mCancel;
}

void ThumbnailThread::cancel() {
	QMutexLocker lock( &mMutex );
	mCancel = true;
	mCond.wakeOne();
}

void ThumbnailThread::run() {
	LOG("");
	while( !testCancel()) {
		{
			QMutexLocker lock(&mMutex);
			// empty mPixPath means nothing to do
			LOG("Waiting for mPixPath");
			if (mPixPath.isNull()) {
				LOG("mPixPath.isNull");
				mCond.wait(&mMutex);
			}
		}
		if(testCancel()) {
			return;
		}
		{
			QMutexLocker lock(&mMutex);
			Q_ASSERT(!mPixPath.isNull());
			LOG("Loading" << mPixPath);
			loadThumbnail();
			mPixPath = QString(); // done, ready for next
		}
		if(testCancel()) {
			return;
		}
		{
			LOG("emitting done signal");
			QSize size(mOriginalWidth, mOriginalHeight);
			QMutexLocker lock(&mMutex);
			done(mImage, size);
			LOG("Done");
		}
	}
	LOG("Ending thread");
}

void ThumbnailThread::loadThumbnail() {
	mImage = QImage();
	bool loaded=false;
	bool needCaching=true;

	// If it's a Jpeg, try to load a small image directly from the file
	if (isJpeg()) {
		JpegContent content;
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
			loaded = loadJpeg();
		}
		if (loaded) {
			// Rotate if necessary
			Orientation orientation = content.orientation();
			QMatrix matrix = ImageUtils::transformMatrix(orientation);
			mImage = mImage.transformed(matrix);
		}
	}

	// File is not a Jpeg, or Jpeg optimized load failed, load file using Qt
	if (!loaded) {
		QImage originalImage;
		if (originalImage.load(mPixPath)) {
			mOriginalWidth=originalImage.width();
			mOriginalHeight=originalImage.height();
			int thumbSize=mThumbnailSize<=THUMBNAILSIZE_NORMAL ? THUMBNAILSIZE_NORMAL : THUMBNAILSIZE_LARGE;

			if (qMax(mOriginalWidth, mOriginalHeight)<=thumbSize ) {
				mImage=originalImage;
				needCaching = false;
			} else {
				mImage = originalImage.scaled(thumbSize, thumbSize, Qt::KeepAspectRatio);
			}
			loaded = true;
		}
	}

	if (needCaching) {
		mImage.setText("Thumb::Uri", 0, mOriginalUri);
		mImage.setText("Thumb::MTime", 0, QString::number(mOriginalTime));
		mImage.setText("Thumb::Size", 0, QString::number(mOriginalSize));
		mImage.setText("Thumb::Mimetype", 0, mOriginalMimeType);
		mImage.setText("Thumb::Image::Width", 0, QString::number(mOriginalWidth));
		mImage.setText("Thumb::Image::Height", 0, QString::number(mOriginalHeight));
		mImage.setText("Software", 0, "Gwenview");

		QString thumbnailDir = ThumbnailLoadJob::thumbnailBaseDir(mThumbnailSize);
		KStandardDirs::makeDir(thumbnailDir, 0700);

		KTemporaryFile tmp;
		tmp.setPrefix(thumbnailDir + "/gwenview");
		tmp.setSuffix(".png");
		if (!tmp.open()) {
			kWarning() << "Could not create a temporary file.";
			return;
		}

		if (!mImage.save(tmp.fileName(), "png")) {
			kWarning() << "Could not save thumbnail for file" << mOriginalUri;
			return;
		}

		rename(QFile::encodeName(tmp.fileName()), QFile::encodeName(mThumbnailPath));
	}
}


bool ThumbnailThread::isJpeg() {
	QString format = QImageReader::imageFormat(mPixPath);
	return format == "jpeg";
}



struct JpegFatalError : public jpeg_error_mgr {
	jmp_buf mJmpBuffer;

	static void handler(j_common_ptr cinfo) {
		JpegFatalError* error=static_cast<JpegFatalError*>(cinfo->err);
		(error->output_message)(cinfo);
		longjmp(error->mJmpBuffer,1);
	}
};

bool ThumbnailThread::loadJpeg() {
	struct jpeg_decompress_struct cinfo;

	// Open file
	FILE* inputFile=fopen(QFile::encodeName( mPixPath ).data(), "rb");
	if(!inputFile) return false;

	// Error handling
	struct JpegFatalError jerr;
	cinfo.err = jpeg_std_error(&jerr);
	cinfo.err->error_exit = JpegFatalError::handler;
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
	int size= mThumbnailSize <= THUMBNAILSIZE_NORMAL ? THUMBNAILSIZE_NORMAL : THUMBNAILSIZE_LARGE;
	int imgSize = qMax(cinfo.image_width, cinfo.image_height);

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
		mImage = QImage(cinfo.output_width, cinfo.output_height, QImage::Format_RGB32);
		break;
	case 1: // B&W image
		mImage = QImage(cinfo.output_width, cinfo.output_height, QImage::Format_Indexed8);
		mImage.setNumColors(256);
		for (int i=0; i<256; i++) {
			mImage.setColor(i, qRgba(i, i, i, 255));
		}
		break;
	default:
		jpeg_destroy_decompress(&cinfo);
		fclose(inputFile);
		return false;
	}

	while (cinfo.output_scanline < cinfo.output_height) {
		uchar *line = mImage.scanLine(cinfo.output_scanline);
		jpeg_read_scanlines(&cinfo, &line, 1);
	}
	jpeg_finish_decompress(&cinfo);

// Expand 24->32 bpp
	if ( cinfo.output_components == 3 ) {
		for (uint j=0; j<cinfo.output_height; j++) {
			uchar *in = mImage.scanLine(j) + (cinfo.output_width - 1)*3;
			QRgb *out = (QRgb*)( mImage.scanLine(j) ) + cinfo.output_width - 1;

			for (int i=cinfo.output_width - 1; i>=0; --i, --out, in -= 3) {
				*out = qRgb(in[0], in[1], in[2]);
			}
		}
	}

	int newMax = qMax(cinfo.output_width, cinfo.output_height);
	int newx = size*cinfo.output_width / newMax;
	int newy = size*cinfo.output_height / newMax;

	mImage = mImage.scaled(newx, newy, Qt::KeepAspectRatio);

	jpeg_destroy_decompress(&cinfo);
	fclose(inputFile);

	return true;
}


//------------------------------------------------------------------------
//
// ThumbnailLoadJob static methods
//
//------------------------------------------------------------------------
static QString sThumbnailBaseDir;
QString ThumbnailLoadJob::thumbnailBaseDir() {
	if (sThumbnailBaseDir.isEmpty()) {
		sThumbnailBaseDir = QDir::homePath() + "/.thumbnails/";
	}
	return sThumbnailBaseDir;
}


void ThumbnailLoadJob::setThumbnailBaseDir(const QString& dir) {
	sThumbnailBaseDir = dir;
}


QString ThumbnailLoadJob::thumbnailBaseDir(int size) {
	QString dir = thumbnailBaseDir();
	if (size<=THUMBNAILSIZE_NORMAL) {
		dir+="normal/";
	} else {
		dir+="large/";
	}
	return dir;
}


void ThumbnailLoadJob::deleteImageThumbnail(const KUrl& url) {
	QString uri=generateOriginalUri(url);
	QFile::remove(generateThumbnailPath(uri, THUMBNAILSIZE_NORMAL));
	QFile::remove(generateThumbnailPath(uri, THUMBNAILSIZE_LARGE));
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

ThumbnailLoadJob::ThumbnailLoadJob(const QList<KFileItem>& items, int size)
: KIO::Job(), mState( STATE_NEXTTHUMB ),
  mCurrentVisibleIndex( -1 ), mFirstVisibleIndex( -1 ), mLastVisibleIndex( -1 ),
  mThumbnailSize(size)
{
	LOG((int)this);

	mBrokenPixmap = KIconLoader::global()->loadIcon("file_broken",
		KIconLoader::NoGroup, THUMBNAILSIZE_MIN);

	// Look for images and store the items in our todo list
	Q_ASSERT(!items.empty());
	mAllItems = QVector<KFileItem>::fromList(items);
	mProcessedState.resize(mAllItems.count());
	qFill(mProcessedState.begin(), mProcessedState.end(), false);
	mCurrentItem = KFileItem();

	connect(&mThumbnailThread, SIGNAL(done(const QImage&, const QSize&)),
		SLOT(thumbnailReady(const QImage&, const QSize&)),
		Qt::QueuedConnection);
}


ThumbnailLoadJob::~ThumbnailLoadJob() {
	LOG((int)this);
	if (hasSubjobs()) {
		LOG("Killing subjob");
		KJob* job = subjobs().first();
		job->kill();
		removeSubjob(job);
	}
	mThumbnailThread.cancel();
	mThumbnailThread.wait();
}


void ThumbnailLoadJob::start() {
	// build mItems from mAllItems if not done yet
	if (mLastVisibleIndex == -1 ) {
		setPriorityItems(KFileItem(), KFileItem(), KFileItem());
	}
	if (mItems.isEmpty()) {
		LOG("Nothing to do");
		emitResult();
		delete this;
		return;
	}

	determineNextIcon();
}

//-Internal--------------------------------------------------------------
void ThumbnailLoadJob::appendItem(const KFileItem& item) {
	int index = thumbnailIndex( item );
	if( index >= 0 ) {
		mProcessedState[ index ] = false;
		return;
	}
	mAllItems.append(item);
	mProcessedState.append( false );
	updateItemsOrder();
}


void ThumbnailLoadJob::itemRemoved(const KFileItem& item) {
	// If we are removing the next item, update to be the item after or the
	// first if we removed the last item
	mItems.removeAll( item );
	int index = thumbnailIndex( item );
	if( index >= 0 ) {
		mAllItems.erase( mAllItems.begin() + index );
		mProcessedState.erase( mProcessedState.begin() + index );
	}

	if (item == mCurrentItem) {
		// Abort
		mCurrentItem = KFileItem();
		if (hasSubjobs()) {
			KJob* job = subjobs().first();
			job->kill();
			removeSubjob(job);
		}
		determineNextIcon();
	}
}

void ThumbnailLoadJob::setPriorityItems(const KFileItem& current, const KFileItem& first, const KFileItem& last) {
	if( mAllItems.isEmpty()) {
		mCurrentVisibleIndex = mFirstVisibleIndex = mLastVisibleIndex = 0;
		return;
	}
	mFirstVisibleIndex = -1;
	mLastVisibleIndex = - 1;
	mCurrentVisibleIndex = -1;
	if( first != KFileItem() ) mFirstVisibleIndex = thumbnailIndex( first );
	if( last != KFileItem() ) mLastVisibleIndex = thumbnailIndex( last );
	if( current != KFileItem() ) mCurrentVisibleIndex = thumbnailIndex( current );
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
	LOG((int)this);
	mState = STATE_NEXTTHUMB;

	// No more items ?
	if (mItems.isEmpty()) {
		// Done
		LOG("emitting result");
		emitResult();
		delete this;
		return;
	}

	mCurrentItem = mItems.first();
	mItems.pop_front();
	int index = mAllItems.indexOf(mCurrentItem);
	LOG("mCurrentItem.url=" << mCurrentItem.url());
	Q_ASSERT(index != -1);
	Q_ASSERT( !mProcessedState[index] );
	mProcessedState[index] = true;

	// First, stat the orig file
	mState = STATE_STATORIG;
	mOriginalTime = 0;
	mCurrentUrl = mCurrentItem.url();
	mCurrentUrl.cleanPath();

	// Do direct stat instead of using KIO if the file is local (faster)
	if( mCurrentUrl.isLocalFile()) {
		KMountPoint::Ptr mountPoint = KMountPoint::currentMountPoints().findByPath(mCurrentUrl.path());
		if (!mountPoint->probablySlow()) {
			KDE_struct_stat buff;
			if ( KDE_stat( QFile::encodeName(mCurrentUrl.path()), &buff ) == 0 )  {
				mOriginalTime = buff.st_mtime;
				QTimer::singleShot( 0, this, SLOT( checkThumbnail()));
			}
		}
	}
	if( mOriginalTime == 0 ) { // KIO must be used
		KIO::Job* job = KIO::stat(mCurrentUrl,false);
		job->ui()->setWindow(KApplication::kApplication()->activeWindow());
		LOG( "KIO::stat orig" << mCurrentUrl.url() );
		addSubjob(job);
	}
	LOG("/determineNextIcon" << (int)this);
}


void ThumbnailLoadJob::slotResult(KJob * job) {
	LOG(mState);
	removeSubjob(job);
	Q_ASSERT(subjobs().isEmpty()); // We should have only one job at a time

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
		mOriginalTime = entry.numberValue(KIO::UDSEntry::UDS_MODIFICATION_TIME, -1);
		checkThumbnail();
		return;
	}

	case STATE_DOWNLOADORIG:
		if (job->error()) {
			emitThumbnailLoadingFailed();
			LOG("Delete temp file" << mTempPath);
			QFile::remove(mTempPath);
			mTempPath = QString();
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
	QImage img = im;
	QSize size = _size;
	if ( !img.isNull()) {
		emitThumbnailLoaded(img, size);
	} else {
		emitThumbnailLoadingFailed();
	}
	if( !mTempPath.isEmpty()) {
		LOG("Delete temp file" << mTempPath);
		QFile::remove(mTempPath);
		mTempPath = QString();
	}
	determineNextIcon();
}

void ThumbnailLoadJob::checkThumbnail() {
	// If we are in the thumbnail dir, just load the file
	if (mCurrentUrl.isLocalFile()
		&& mCurrentUrl.directory().startsWith(thumbnailBaseDir()) )
	{
		QImage image(mCurrentUrl.path());
		emitThumbnailLoaded(image, image.size());
		determineNextIcon();
		return;
	}
	QSize imagesize;

	mOriginalUri=generateOriginalUri(mCurrentUrl);
	mThumbnailPath=generateThumbnailPath(mOriginalUri, mThumbnailSize);

	LOG("Stat thumb" << mThumbnailPath);

	QImage thumb;
	if ( thumb.load(mThumbnailPath) ) {
		if (thumb.text("Thumb::Uri", 0) == mOriginalUri &&
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
				LOG("Thumbnail for" << mOriginalUri << "does not contain correct image size information");
				KFileMetaInfo fmi(mCurrentUrl);
				if (fmi.isValid()) {
					KFileMetaInfoItem item=fmi.item("Dimensions");
					if (item.isValid()) {
						size=item.value().toSize();
					} else {
						LOG("KFileMetaInfoItem for" << mOriginalUri << "did not get image size information");
					}
				} else {
					LOG("Could not get a valid KFileMetaInfo instance for" << mOriginalUri);
				}
			}
			emitThumbnailLoaded(thumb, size);
			determineNextIcon();
			return;
		}
	}

	// Thumbnail not found or not valid
	if ( MimeTypeUtils::rasterImageMimeTypes().contains(mCurrentItem.mimetype()) ) {
		// This is a raster image
		if (mCurrentUrl.isLocalFile()) {
			// Original is a local file, create the thumbnail
			startCreatingThumbnail(mCurrentUrl.path());
		} else {
			// Original is remote, download it
			mState=STATE_DOWNLOADORIG;
			
			KTemporaryFile tempFile;
			tempFile.setAutoRemove(false);
			if (!tempFile.open()) {
				kWarning() << "Couldn't create temp file to download " << mCurrentUrl.prettyUrl();
				emitThumbnailLoadingFailed();
				determineNextIcon();
				return;
			}
			mTempPath = tempFile.fileName();

			KUrl url;
			url.setPath(mTempPath);
			KIO::Job* job=KIO::file_copy(mCurrentUrl, url,-1,true,false,false);
			job->ui()->setWindow(KApplication::kApplication()->activeWindow());
			LOG("Download remote file" << mCurrentUrl.prettyUrl() << "to" << url.pathOrUrl());
			addSubjob(job);
		}
	} else {
		// Not a raster image, use a KPreviewJob
		LOG("Starting a KPreviewJob for" << mCurrentItem.url());
		mState=STATE_PREVIEWJOB;
		QList<KFileItem> list;
		list.append(mCurrentItem);
		KIO::Job* job=KIO::filePreview(list, mThumbnailSize);
		//job->ui()->setWindow(KApplication::kApplication()->activeWindow());
		connect(job, SIGNAL(gotPreview(const KFileItem&, const QPixmap&)),
			this, SLOT(slotGotPreview(const KFileItem&, const QPixmap&)) );
		connect(job, SIGNAL(failed(const KFileItem&)),
			this, SLOT(emitThumbnailLoadingFailed()) );
		addSubjob(job);
	}
}

void ThumbnailLoadJob::startCreatingThumbnail(const QString& pixPath) {
	LOG("Creating thumbnail from" << pixPath);
	mThumbnailThread.load( mOriginalUri, mOriginalTime, mCurrentItem.size(),
		mCurrentItem.mimetype(), pixPath, mThumbnailPath, mThumbnailSize);
}


void ThumbnailLoadJob::slotGotPreview(const KFileItem& item, const QPixmap& pixmap) {
	LOG(mCurrentItem.url());
	QSize size;
	emit thumbnailLoaded(item, pixmap, size);
}


void ThumbnailLoadJob::emitThumbnailLoaded(const QImage& img, QSize size) {
	LOG(mCurrentItem.url());
	int biggestDimension=qMax(img.width(), img.height());

	QImage thumbImg;
	if (biggestDimension>mThumbnailSize) {
		// Scale down thumbnail if necessary
		thumbImg = img.scaled(mThumbnailSize, mThumbnailSize, Qt::KeepAspectRatio);
	} else {
		thumbImg = img;
	}
	QPixmap thumb = QPixmap::fromImage(thumbImg);
	emit thumbnailLoaded(mCurrentItem, thumb, size);
}


void ThumbnailLoadJob::emitThumbnailLoadingFailed() {
	LOG(mCurrentItem.url());
	QSize size;
	emit thumbnailLoaded(mCurrentItem, mBrokenPixmap, size);
}


} // namespace
