/*  Gwenview - A simple image viewer for KDE
    Copyright (C) 2000-2002 Aurélien Gâteau
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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

// Qt includes
#include <qdir.h>
#include <qfile.h>
#include <qimage.h>
#include <qpainter.h>
#include <qpixmap.h>

// KDE includes
#include <kdebug.h>
#include <kfileitem.h>
#include <kmdcodec.h>
#include <kstddirs.h>
#include <ktempfile.h>
#include <kio/netaccess.h>

// Other includes
#define XMD_H
extern "C" {
#include <jpeglib.h>
}

// Our includes
#include "thumbnailloadjob.moc"

//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif



const QString& ThumbnailLoadJob::thumbnailDir() {
	static QString dir;
	if (!dir.isEmpty()) return dir;

	KGlobal::dirs()->addResourceType("thumbnails","share/thumbnails/");
	dir = QDir::cleanDirPath( KGlobal::dirs()->resourceDirs("thumbnails")[0] );
	return dir;
}


ThumbnailLoadJob::ThumbnailLoadJob(const KFileItemList* itemList,ThumbnailSize size)
: KIO::Job(false), mThumbnailSize(size)
{
	LOG("");

	// Look for images and store the items in our todo list
	mItems=*itemList;

	if (mItems.isEmpty()) return;

	QString originalDir=QDir::cleanDirPath( mItems.first()->url().directory() );

	// Check if we're already in a cache dir
	// In that case the cache dir will be the original dir
	QString cacheBaseDir = thumbnailDir();
	if ( originalDir.startsWith(cacheBaseDir) ) {
		mCacheDir=originalDir;
		return;
	}

	// Generate the thumbnail dir name
	//	kdDebug() << mItems.first()->url().upURL().url(-1) << endl;
	KMD5 md5(QFile::encodeName(KURL(originalDir).url()) );
	QCString hash=md5.hexDigest();
	QString thumbPath = QString::fromLatin1( hash.data(), 4 ) + "/" +
		QString::fromLatin1( hash.data()+4, 4 ) + "/" +
		QString::fromLatin1( hash.data()+8 ) + "/";

	// Create the thumbnail cache dir
	mCacheDir = locateLocal( "thumbnails", thumbPath + QString(ThumbnailSize::biggest()) + "/" );
	LOG("mCacheDir=" << mCacheDir);
}


ThumbnailLoadJob::~ThumbnailLoadJob() {
	LOG("");
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


//-Internal--------------------------------------------------------------
void ThumbnailLoadJob::itemRemoved(const KFileItem* item) {
	mItems.removeRef(item);

	if (item == mCurrentItem) {
		// Abort
		subjobs.first()->kill();
		subjobs.removeFirst();
		determineNextIcon();
	}
}


void ThumbnailLoadJob::determineNextIcon() {
	// Skip dirs
	while (!mItems.isEmpty() && mItems.first()->isDir()) {
		mItems.removeFirst();
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
		mCurrentItem = mItems.first();
		mCurrentURL = mCurrentItem->url();
		KIO::Job* job = KIO::stat(mCurrentURL,false);
		LOG( "KIO::stat orig " << mCurrentURL.url() );
		addSubjob(job);
		mItems.removeFirst();
	}
}


void ThumbnailLoadJob::slotResult(KIO::Job * job) {
	subjobs.remove(job);
	ASSERT(subjobs.isEmpty());	// We should have only one job at a time ...

	switch (mState) {
	case STATE_STATORIG: {
		// Could not stat original, drop this one and move on to the next one
		if (job->error()) {
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

		// Now stat the thumbnail
		mThumbURL.setPath(mCacheDir + "/" + mCurrentURL.fileName());
		mState = STATE_STATTHUMB;
		KIO::Job * job = KIO::stat(mThumbURL, false);
		LOG("Stat thumb " << mThumbURL.url());
		addSubjob(job);

		return;
	}

	case STATE_STATTHUMB:
		// Try to load this thumbnail - takes care of determineNextIcon
		if (statResultThumbnail(static_cast < KIO::StatJob * >(job)))
			return;

		// Thumbnail not found or not valid
		// If the original is a local file, create the thumbnail
		// and proceed to next icon, otherwise download the original
		if (mCurrentURL.isLocalFile()) {
			createThumbnail(mCurrentURL.path());
			determineNextIcon();
		} else {
			mState=STATE_DOWNLOADORIG;
			KTempFile tmpFile;
			mTempURL.setPath(tmpFile.name());
			KIO::Job* job=KIO::file_copy(mCurrentURL,mTempURL,-1,true,false,false);
			LOG("Download remote file " << mCurrentURL.prettyURL());
			addSubjob(job);
		}
		return;

	case STATE_DOWNLOADORIG: 
		if (!job->error()) {
			createThumbnail(mTempURL.path());
		}
		
		mState=STATE_DELETETEMP;
		LOG("Delete temp file " << mTempURL.prettyURL());
		addSubjob(KIO::file_delete(mTempURL,false));
		return;

	case STATE_DELETETEMP:
		determineNextIcon();
		return;
	}
}


bool ThumbnailLoadJob::statResultThumbnail(KIO::StatJob * job) {
	// Quit if thumbnail not found
	if (job->error()) return false;

	// Get thumbnail modification time
	KIO::UDSEntry entry = job->statResult();
	KIO::UDSEntry::ConstIterator it = entry.begin();
	time_t thumbnailTime = 0;
	for (; it != entry.end(); it++) {
		if ((*it).m_uds == KIO::UDS_MODIFICATION_TIME) {
			thumbnailTime = (time_t) ((*it).m_long);
			break;
		}
	}

	// Quit if thumbnail is older than file
	if (thumbnailTime<mOriginalTime) return false;

	// Load thumbnail
	QPixmap pix;
	if (!pix.load(mThumbURL.path())) return false;
	
	// All done
	emitThumbnailLoaded(pix);
	determineNextIcon();
	return true;
}


void ThumbnailLoadJob::createThumbnail(const QString& pixPath) {
	LOG("Creating thumbnail from " << pixPath);
	QPixmap pix;
	
	// Create thumbnail
	if( isJPEG(pixPath)) {
		if( loadJPEG( pixPath, pix)) {
			pix.save(mCacheDir + "/" + mCurrentURL.fileName(),"PNG");
			emitThumbnailLoaded(pix);
			return;
		}
		//load failed try via Qt
	}
	if( loadThumbnail(pixPath, pix)) {
		pix.save(mCacheDir + "/" + mCurrentURL.fileName(),"PNG");
		emitThumbnailLoaded(pix);
	}
}


bool ThumbnailLoadJob::loadThumbnail(const QString& pixPath, QPixmap &pix) {
	LOG("");
	QImage bigImg,img;
	int biWidth,biHeight;
	int thumbSize=ThumbnailSize::biggest().pixelSize();
	if (!bigImg.load(pixPath)) return false;
	
	biWidth=bigImg.width();
	biHeight=bigImg.height();

	if (biWidth<=thumbSize && biHeight<=thumbSize) {
		pix.convertFromImage(bigImg);
		return true;
	}
	
	img=bigImg.smoothScale(thumbSize,thumbSize,QImage::ScaleMin);
	pix.convertFromImage(img);

	return true;
}


bool ThumbnailLoadJob::isJPEG(const QString& name) {
	QString format=QImageIO::imageFormat(name);
	return format=="JPEG";
}


bool ThumbnailLoadJob::loadJPEG( const QString &pixPath, QPixmap &pix) {
	QImage image;

	// Open file
	FILE* inputFile=fopen(pixPath.data(), "rb");
	if(!inputFile) return false;

	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	jpeg_stdio_src(&cinfo, inputFile);
	jpeg_read_header(&cinfo, TRUE);

	// Get image size and check if we need a thumbnail
	int size=ThumbnailSize::biggest().pixelSize();
	int imgSize = QMAX(cinfo.image_width, cinfo.image_height);

	if (imgSize<=size) {
		fclose(inputFile);
		return pix.load(pixPath);
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
	while (cinfo.output_scanline < cinfo.output_height)
		jpeg_read_scanlines(&cinfo, lines + cinfo.output_scanline, cinfo.output_height);
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

	pix=image.smoothScale( newx, newy);

	jpeg_destroy_decompress(&cinfo);
	fclose(inputFile);

	return true;
}


void ThumbnailLoadJob::emitThumbnailLoaded(const QPixmap& pix) {
	ThumbnailSize biggest=ThumbnailSize::biggest();

	if (mThumbnailSize==biggest) {
		emit thumbnailLoaded(mCurrentItem,pix);
		return;
	}
	
// Do we need to scale down the thumbnail ?
	int biggestDimension=QMAX(pix.width(),pix.height());
	int thumbPixelSize=mThumbnailSize.pixelSize();
	if (biggestDimension<=thumbPixelSize) {
		emit thumbnailLoaded(mCurrentItem,pix);
		return;
	}

// Scale thumbnail
	double scale=double(thumbPixelSize)/double(biggestDimension);
	QPixmap pix2(thumbPixelSize,thumbPixelSize);
	QPainter painter;
	painter.begin(&pix2);
	painter.eraseRect(0,0,thumbPixelSize,thumbPixelSize);
	painter.scale(scale,scale);
	painter.drawPixmap((biggestDimension-pix.width())/2, (biggestDimension-pix.height())/2, pix);
	painter.end();

	emit thumbnailLoaded(mCurrentItem,pix2);
}
