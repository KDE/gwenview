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

// Qt
#include <qdir.h>
#include <qfile.h>
#include <qimage.h>
#include <qpainter.h>
#include <qpixmap.h>

// KDE 
#include <kdebug.h>
#include <kfileitem.h>
#include <kiconloader.h>
#include <kmdcodec.h>
#include <kstandarddirs.h>
#include <ktempfile.h>
#include <kio/netaccess.h>

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

//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kdDebug() << k_funcinfo << x << endl
#else
#define LOG(x) ;
#endif



//------------------------------------------------------------------------
//
// ThumbnailLoadJob static methods
//
//------------------------------------------------------------------------
QString ThumbnailLoadJob::thumbnailBaseDir() {
	static QString dir;
	if (!dir.isEmpty()) return dir;

	dir=QDir::cleanDirPath( KGlobal::dirs()->saveLocation("cache", "gwenview") );
	return dir;
}


QString ThumbnailLoadJob::thumbnailDirForURL(const KURL& url) {
	QString originalDir=QDir::cleanDirPath( url.directory() );

	// Check if we're already in a cache dir
	// In that case the cache dir will be the original dir
	QString cacheBaseDir = thumbnailBaseDir();
	if (originalDir.startsWith(cacheBaseDir) ) {
		return originalDir;
	}

	// Generate the thumbnail dir name
	//	kdDebug() << mItems.getFirst()->url().upURL().url(-1) << endl;
	KMD5 md5(QFile::encodeName(KURL(originalDir).url()) );
	QCString hash=md5.hexDigest();
	QString thumbnailDir = thumbnailBaseDir() + "/" +
		QString::fromLatin1( hash.data(), 4 ) + "/" +
		QString::fromLatin1( hash.data()+4, 4 ) + "/" +
		QString::fromLatin1( hash.data()+8 ) + "/";

	// Create the thumbnail cache dir
	KStandardDirs::makeDir(thumbnailDir);
	return thumbnailDir;
}


void ThumbnailLoadJob::deleteImageThumbnail(const KURL& url) {
	QDir dir( thumbnailDirForURL(url) );
	dir.remove(url.fileName());
}


//------------------------------------------------------------------------
//
// ThumbnailLoadJob implementation
//
//------------------------------------------------------------------------
ThumbnailLoadJob::ThumbnailLoadJob(const KFileItemList* items, ThumbnailSize size)
: KIO::Job(false), mThumbnailSize(size)
{
	LOG("");
    
    mBrokenPixmap=KGlobal::iconLoader()->loadIcon("file_broken", 
        KIcon::NoGroup, ThumbnailSize(ThumbnailSize::SMALL).pixelSize());

	// Look for images and store the items in our todo list
	mItems=*items;

	if (mItems.isEmpty()) return;

	// We assume that all items are in the same dir
	mCacheDir=thumbnailDirForURL(mItems.getFirst()->url());
	LOG("mCacheDir=" << mCacheDir);
	
	// Move to the first item
	mItems.first();
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
void ThumbnailLoadJob::appendItem(const KFileItem* item) {
	mItems.append(item);
}


void ThumbnailLoadJob::itemRemoved(const KFileItem* item) {
	mItems.removeRef(item);

	if (item == mCurrentItem) {
		// Abort
		subjobs.first()->kill();
		subjobs.removeFirst();
		determineNextIcon();
	}
}


bool ThumbnailLoadJob::setNextItem(const KFileItem* item) {
	// Move mItems' internal iterator to the next item to be processed
	do {
		if (item==mItems.current()) return true;
	} while (mItems.next());
	
	// not found, move to first item
	mItems.first(); 
	return false;
}


void ThumbnailLoadJob::determineNextIcon() {
	// Skip non images 
	while (true) {
		KFileItem* item=mItems.current();
		if (!item) break;
		if (item->isDir() || GVArchive::fileItemIsArchive(item)) {
			mItems.remove();
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
		mCurrentItem = mItems.current();
		mCurrentURL = mCurrentItem->url();
		KIO::Job* job = KIO::stat(mCurrentURL,false);
		LOG( "KIO::stat orig " << mCurrentURL.url() );
		addSubjob(job);
		mItems.remove();
	}
}


void ThumbnailLoadJob::slotResult(KIO::Job * job) {
	subjobs.remove(job);
	Q_ASSERT(subjobs.isEmpty());	// We should have only one job at a time ...

	switch (mState) {
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
		if (job->error()) {
            emitThumbnailLoadingFailed();
        } else {
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
	LOG("Thumbnail is up to date");

	// Load thumbnail
	QImage img;
	if (!img.load(mThumbURL.path())) return false;
	
	// All done
	emitThumbnailLoaded(img);
	determineNextIcon();
	return true;
}


void ThumbnailLoadJob::createThumbnail(const QString& pixPath) {
	LOG("Creating thumbnail from " << pixPath);
	QImage img;
	bool loaded=false;
	
	// If it's a JPEG, try to load a small image directly from the file
	if (isJPEG(pixPath)) {
		loaded=loadJPEG(pixPath, img);
		if (loaded) {
			// Rotate if necessary
			GVImageUtils::Orientation orientation=GVImageUtils::getOrientation(pixPath);
			img=GVImageUtils::modify(img,orientation);
		}
	}

	// File is not a JPEG, or JPEG optimized load failed, load file using Qt
	if (!loaded) {
		loaded=loadThumbnail(pixPath, img);
	}
	
    if (loaded) {
        img.save(mCacheDir + "/" + mCurrentURL.fileName(),"PNG");
        emitThumbnailLoaded(img);
    } else {
        emitThumbnailLoadingFailed();
    }
}


bool ThumbnailLoadJob::loadThumbnail(const QString& pixPath, QImage& img) {
	LOG("");
	QImage bigImg;
	int biWidth,biHeight;
	int thumbSize=ThumbnailSize::biggest().pixelSize();
	if (!bigImg.load(pixPath)) return false;
	
	biWidth=bigImg.width();
	biHeight=bigImg.height();

	if (biWidth<=thumbSize && biHeight<=thumbSize) {
		img=bigImg;
	} else {
		img=bigImg.smoothScale(thumbSize,thumbSize,QImage::ScaleMin);
	}
	return true;
}


bool ThumbnailLoadJob::isJPEG(const QString& name) {
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

bool ThumbnailLoadJob::loadJPEG( const QString &pixPath, QImage& image) {
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

	image=image.smoothScale( newx, newy);

	jpeg_destroy_decompress(&cinfo);
	fclose(inputFile);

	return true;
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
	QImage img2=img.smoothScale(thumbPixelSize, thumbPixelSize, QImage::ScaleMin);
	emit thumbnailLoaded(mCurrentItem, QPixmap(img2));
}


void ThumbnailLoadJob::emitThumbnailLoadingFailed() {
	emit thumbnailLoaded(mCurrentItem, mBrokenPixmap);
}
