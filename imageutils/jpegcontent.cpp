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
// System
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern "C" {
#include <jpeglib.h>
#include "transupp.h"
}

// Qt
#include <qbuffer.h>
#include <qfile.h>
#include <qimage.h>
#include <qmap.h>
#include <qwmatrix.h>

// KDE
#include <kdebug.h>

// Exif
#include "exif-data.h"
#include "exif-ifd.h"
#include "exif-utils.h"

// Local
#include "imageutils/imageutils.h"
#include "imageutils/jpegcontent.h"
#include "imageutils/jpeg-data.h"
#include "imageutils/jpegerrormanager.h"

namespace ImageUtils {

const int INMEM_DST_DELTA=4096;


//------------------------------------------
//
// In-memory data source manager for libjpeg
//
//------------------------------------------
struct inmem_src_mgr : public jpeg_source_mgr {
	QByteArray* mInput;
};

void inmem_init_source(j_decompress_ptr cinfo) {
	inmem_src_mgr* src=(inmem_src_mgr*)(cinfo->src);
	src->next_input_byte=(const JOCTET*)( src->mInput->data() );
	src->bytes_in_buffer=src->mInput->size();
}

/**
 * If this function is called, it means the JPEG file is broken. We feed the
 * decoder with fake EOI has specified in the libjpeg documentation.
 */
int inmem_fill_input_buffer(j_decompress_ptr cinfo) {
	static JOCTET fakeEOI[2]={ JOCTET(0xFF), JOCTET(JPEG_EOI)};
	kdWarning() << k_funcinfo << " Image is incomplete" << endl;
	cinfo->src->next_input_byte=fakeEOI;
	cinfo->src->bytes_in_buffer=2;
	return true;
}

void inmem_skip_input_data(j_decompress_ptr cinfo, long num_bytes) {
	if (num_bytes<=0) return;
	Q_ASSERT(num_bytes>=long(cinfo->src->bytes_in_buffer));
	cinfo->src->next_input_byte+=num_bytes;
	cinfo->src->bytes_in_buffer-=num_bytes;
}

void inmem_term_source(j_decompress_ptr /*cinfo*/) {
}


//-----------------------------------------------
//
// In-memory data destination manager for libjpeg
//
//-----------------------------------------------
struct inmem_dest_mgr : public jpeg_destination_mgr {
	QByteArray* mOutput;

	void dump() {
		kdDebug() << "dest_mgr:\n";
		kdDebug() << "- next_output_byte: " << next_output_byte << endl;
		kdDebug() << "- free_in_buffer: " << free_in_buffer << endl;
		kdDebug() << "- output size: " << mOutput->size() << endl;
	}
};

void inmem_init_destination(j_compress_ptr cinfo) {
	inmem_dest_mgr* dest=(inmem_dest_mgr*)(cinfo->dest);
	if (dest->mOutput->size()==0) {
		bool result=dest->mOutput->resize(INMEM_DST_DELTA);
		Q_ASSERT(result);
	}
	dest->free_in_buffer=dest->mOutput->size();
	dest->next_output_byte=(JOCTET*)(dest->mOutput->data() );
}

int inmem_empty_output_buffer(j_compress_ptr cinfo) {
	inmem_dest_mgr* dest=(inmem_dest_mgr*)(cinfo->dest);
	bool result=dest->mOutput->resize(dest->mOutput->size() + INMEM_DST_DELTA);
	Q_ASSERT(result);
	dest->next_output_byte=(JOCTET*)( dest->mOutput->data() + dest->mOutput->size() - INMEM_DST_DELTA );
	dest->free_in_buffer=INMEM_DST_DELTA;

	return true;
}

void inmem_term_destination(j_compress_ptr cinfo) {
	inmem_dest_mgr* dest=(inmem_dest_mgr*)(cinfo->dest);
	int finalSize=dest->next_output_byte - (JOCTET*)(dest->mOutput->data());
	Q_ASSERT(finalSize>=0);
	dest->mOutput->resize(finalSize);
}


//---------------------
//
// JPEGContent::Private
//
//---------------------
struct JPEGContent::Private {
	QByteArray mRawData;
	QSize mSize;
	QString mComment;
	bool mPendingChanges;
	QWMatrix mTransformMatrix;
	ExifData* mExifData;
	ExifEntry* mOrientationEntry;
	ExifByteOrder mByteOrder;

	Private() {
		mPendingChanges = false;
		mExifData=0;
		mOrientationEntry=0;
	}

	void setupInmemSource(j_decompress_ptr cinfo) {
		Q_ASSERT(!cinfo->src);
		inmem_src_mgr* src = (inmem_src_mgr*)
			(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
										sizeof(inmem_src_mgr));
		cinfo->src=(struct jpeg_source_mgr*)(src);

		src->init_source=inmem_init_source;
		src->fill_input_buffer=inmem_fill_input_buffer;
		src->skip_input_data=inmem_skip_input_data;
		src->resync_to_restart=jpeg_resync_to_restart;
		src->term_source=inmem_term_source;

		src->mInput=&mRawData;
	}

	
	void setupInmemDestination(j_compress_ptr cinfo, QByteArray* outputData) {
		Q_ASSERT(!cinfo->dest);
		inmem_dest_mgr* dest = (inmem_dest_mgr*)
			(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
										sizeof(inmem_dest_mgr));
		cinfo->dest=(struct jpeg_destination_mgr*)(dest);

		dest->init_destination=inmem_init_destination;
		dest->empty_output_buffer=inmem_empty_output_buffer;
		dest->term_destination=inmem_term_destination;

		dest->mOutput=outputData;
	}
	bool readJPEGInfo() {
		struct jpeg_decompress_struct srcinfo;
		jpeg_saved_marker_ptr mark;
		
		// Init JPEG structs 
		JPEGErrorManager errorManager;

		// Initialize the JPEG decompression object
		srcinfo.err = &errorManager;
		jpeg_create_decompress(&srcinfo);
		if (setjmp(errorManager.jmp_buffer)) {
			kdError() << k_funcinfo << "libjpeg fatal error\n";
			return false;
		}

		// Specify data source for decompression
		setupInmemSource(&srcinfo);

		// Read the header
		jcopy_markers_setup(&srcinfo, JCOPYOPT_ALL);
		int result=jpeg_read_header(&srcinfo, true);
		if (result!=JPEG_HEADER_OK) {
			kdError() << "Could not read jpeg header\n";
			jpeg_destroy_decompress(&srcinfo);
			return false;
		}
		mSize=QSize(srcinfo.image_width, srcinfo.image_height);
		
		// Read the comment, if any
		mComment=QString::null;
		for (mark = srcinfo.marker_list; mark; mark = mark->next) {
			if (mark->marker == JPEG_COM) {
				mComment=QString::fromUtf8((const char*)(mark->data), mark->data_length);
				break;
			}
		}
		
		jpeg_destroy_decompress(&srcinfo);
		return true;
	}
};


//------------
//
// JPEGContent
//
//------------
JPEGContent::JPEGContent() {
	d=new JPEGContent::Private();
}


JPEGContent::~JPEGContent() {
	if (d->mExifData) {
		exif_data_unref(d->mExifData);
	}
	delete d;
}


bool JPEGContent::load(const QString& path) {
	QFile file(path);
	if (!file.open(IO_ReadOnly)) {
		kdError() << "Could not open '" << path << "' for reading\n";
		return false;
	}
	return loadFromData(file.readAll());
}


bool JPEGContent::loadFromData(const QByteArray& data) {
	d->mPendingChanges = false;
	d->mTransformMatrix.reset();
	if (d->mExifData) {
		exif_data_unref(d->mExifData);
		d->mExifData=0;
	}

	d->mRawData = data;
	if (d->mRawData.size()==0) {
		kdError() << "No data\n";
		return false;
	}

	if (!d->readJPEGInfo()) return false;

	d->mExifData = exif_data_new_from_data((unsigned char*)data.data(), data.size());
	if (!d->mExifData) {
		kdError() << "Could not load exif data\n";
		return false;
	}
	d->mByteOrder = exif_data_get_byte_order(d->mExifData);
	
	d->mOrientationEntry =
		exif_content_get_entry(d->mExifData->ifd[EXIF_IFD_0],
			EXIF_TAG_ORIENTATION);

	// Adjust the size according to the orientation
	switch (orientation()) {
	case TRANSPOSE:
	case ROT_90:
	case TRANSVERSE:
	case ROT_270:
		d->mSize.transpose();
		break;
	default:
		break;
	}

	return true;
}


Orientation JPEGContent::orientation() const {
	if (!d->mOrientationEntry) {
		return NOT_AVAILABLE;
	}
	short value=exif_get_short(d->mOrientationEntry->data, d->mByteOrder);
	if (value<NORMAL || value>ROT_270) return NOT_AVAILABLE;
	return Orientation(value);
}


void JPEGContent::resetOrientation() {
	if (!d->mOrientationEntry) {
		return;
	}
	exif_set_short(d->mOrientationEntry->data, d->mByteOrder,
		short(ImageUtils::NORMAL));
}


QSize JPEGContent::size() const {
	return d->mSize;
}


QString JPEGContent::comment() const {
	return d->mComment;
}


void JPEGContent::setComment(const QString& comment) {
	d->mPendingChanges = true;
	d->mComment = comment;
}



// This code is inspired by jpegtools.c from fbida
static void doSetComment(struct jpeg_decompress_struct *src, const QString& comment) {
	jpeg_saved_marker_ptr mark;
	int size;

	/* find or create comment marker */
	for (mark = src->marker_list;; mark = mark->next) {
		if (mark->marker == JPEG_COM)
			break;
		if (NULL == mark->next) {
			mark->next = (jpeg_marker_struct*)
				src->mem->alloc_large((j_common_ptr)src,JPOOL_IMAGE,
							   sizeof(*mark));
			mark = mark->next;
			memset(mark,0,sizeof(*mark));
			mark->marker = JPEG_COM;
			break;
		}
	}

	/* update comment marker */
	QCString utf8=comment.utf8();
	size = utf8.length();
	mark->data = (JOCTET*)
		src->mem->alloc_large((j_common_ptr)src,JPOOL_IMAGE,size);
	mark->original_length = size;
	mark->data_length = size;
	memcpy(mark->data, utf8, size);
}


static QWMatrix createRotMatrix(int angle) {
	QWMatrix matrix;
	matrix.rotate(angle);
	return matrix;
}


static QWMatrix createScaleMatrix(int dx, int dy) {
	QWMatrix matrix;
	matrix.scale(dx, dy);
	return matrix;
}



struct OrientationInfo {
	OrientationInfo() {}
	OrientationInfo(Orientation o, QWMatrix m, JXFORM_CODE j)
	: orientation(o), matrix(m), jxform(j) {}

	Orientation orientation;
	QWMatrix matrix;
	JXFORM_CODE jxform;
};
typedef QValueList<OrientationInfo> OrientationInfoList;

static const OrientationInfoList& orientationInfoList() {
	static OrientationInfoList list;
	if (list.size() == 0) {
		QWMatrix rot90 = createRotMatrix(90);
		QWMatrix hflip = createScaleMatrix(-1, 1);
		QWMatrix vflip = createScaleMatrix(1, -1);

		list
			<< OrientationInfo(NOT_AVAILABLE, QWMatrix(), JXFORM_NONE)
			<< OrientationInfo(NORMAL, QWMatrix(), JXFORM_NONE)
			<< OrientationInfo(HFLIP, hflip, JXFORM_FLIP_H)
			<< OrientationInfo(ROT_180, createRotMatrix(180), JXFORM_ROT_180)
			<< OrientationInfo(VFLIP, vflip, JXFORM_FLIP_V)
			<< OrientationInfo(TRANSPOSE, hflip * rot90, JXFORM_TRANSPOSE)
			<< OrientationInfo(ROT_90, rot90, JXFORM_ROT_90)
			<< OrientationInfo(TRANSVERSE, vflip * rot90, JXFORM_TRANSVERSE)
			<< OrientationInfo(ROT_270, createRotMatrix(270), JXFORM_ROT_270)
			;
	}
	return list;
}


void JPEGContent::transform(Orientation orientation) {
	if (orientation != NOT_AVAILABLE && orientation != NORMAL) {
		d->mPendingChanges = true;
		OrientationInfoList::ConstIterator it(orientationInfoList().begin()), end(orientationInfoList().end());
		for (; it!=end; ++it) {
			if ( (*it).orientation == orientation ) {
				d->mTransformMatrix = (*it).matrix * d->mTransformMatrix;
				break;
			}
		}
		if (it == end) {
			kdWarning() << k_funcinfo << "Could not find matrix for orientation\n";
		}
	}
}


#if 0
static void dumpMatrix(const QWMatrix& matrix) {
	kdDebug() << "matrix | " << matrix.m11() << ", " << matrix.m12() << " |\n";
	kdDebug() << "       | " << matrix.m21() << ", " << matrix.m22() << " |\n";
	kdDebug() << "       ( " << matrix.dx()  << ", " << matrix.dy()  << " )\n";
}
#endif


static bool matricesAreSame(const QWMatrix& m1, const QWMatrix& m2, double tolerance) {
	return fabs( m1.m11() - m2.m11() ) < tolerance
		&& fabs( m1.m12() - m2.m12() ) < tolerance
		&& fabs( m1.m21() - m2.m21() ) < tolerance
		&& fabs( m1.m22() - m2.m22() ) < tolerance
		&& fabs( m1.dx()  - m2.dx()  ) < tolerance
		&& fabs( m1.dy()  - m2.dy()  ) < tolerance;
}


static JXFORM_CODE findJxform(const QWMatrix& matrix) {
	OrientationInfoList::ConstIterator it(orientationInfoList().begin()), end(orientationInfoList().end());
	for (; it!=end; ++it) {
		if ( matricesAreSame( (*it).matrix, matrix, 0.001) ) {
			return (*it).jxform;
		}
	}
	kdWarning() << "findJxform: failed\n";
	return JXFORM_NONE;
}


void JPEGContent::applyPendingChanges() {
	if (d->mRawData.size()==0) {
		kdError() << "No data loaded\n";
		return;
	}

	// The following code is inspired by jpegtran.c from the libjpeg

	// Init JPEG structs 
	struct jpeg_decompress_struct srcinfo;
	struct jpeg_compress_struct dstinfo;
	jvirt_barray_ptr * src_coef_arrays;
	jvirt_barray_ptr * dst_coef_arrays;

	// Initialize the JPEG decompression object
	JPEGErrorManager srcErrorManager;
	srcinfo.err = &srcErrorManager;
	jpeg_create_decompress(&srcinfo);
	if (setjmp(srcErrorManager.jmp_buffer)) {
		kdError() << k_funcinfo << "libjpeg error in src\n";
		return;
	}

	// Initialize the JPEG compression object
	JPEGErrorManager dstErrorManager;
	dstinfo.err = &dstErrorManager;
	jpeg_create_compress(&dstinfo);
	if (setjmp(dstErrorManager.jmp_buffer)) {
		kdError() << k_funcinfo << "libjpeg error in dst\n";
		return;
	}

	// Specify data source for decompression
	d->setupInmemSource(&srcinfo);

	// Enable saving of extra markers that we want to copy
	jcopy_markers_setup(&srcinfo, JCOPYOPT_ALL);

	(void) jpeg_read_header(&srcinfo, TRUE);

	doSetComment(&srcinfo, d->mComment);

	// Init transformation
	jpeg_transform_info transformoption;
	transformoption.transform = findJxform(d->mTransformMatrix);
	transformoption.force_grayscale = false;
	transformoption.trim = false;
	jtransform_request_workspace(&srcinfo, &transformoption);

	/* Read source file as DCT coefficients */
	src_coef_arrays = jpeg_read_coefficients(&srcinfo);

	/* Initialize destination compression parameters from source values */
	jpeg_copy_critical_parameters(&srcinfo, &dstinfo);

	/* Adjust destination parameters if required by transform options;
	* also find out which set of coefficient arrays will hold the output.
	*/
	dst_coef_arrays = jtransform_adjust_parameters(&srcinfo, &dstinfo,
		src_coef_arrays,
		&transformoption);

	/* Specify data destination for compression */
	QByteArray output;
	output.resize(d->mRawData.size());
	d->setupInmemDestination(&dstinfo, &output);

	/* Start compressor (note no image data is actually written here) */
	jpeg_write_coefficients(&dstinfo, dst_coef_arrays);
	
	/* Copy to the output file any extra markers that we want to preserve */
	jcopy_markers_execute(&srcinfo, &dstinfo, JCOPYOPT_ALL);

	/* Execute image transformation, if any */
	jtransform_execute_transformation(&srcinfo, &dstinfo,
		src_coef_arrays,
		&transformoption);

	/* Finish compression and release memory */
	jpeg_finish_compress(&dstinfo);
	jpeg_destroy_compress(&dstinfo);
	(void) jpeg_finish_decompress(&srcinfo);
	jpeg_destroy_decompress(&srcinfo);

	// Set rawData to our new JPEG 
	d->mRawData = output;
}


QImage JPEGContent::thumbnail() const {
	QImage image;
	if( d->mExifData && d->mExifData->data ) {
		image.loadFromData( d->mExifData->data, d->mExifData->size, "JPEG" );
	}
	return image;
}


void JPEGContent::setThumbnail(const QImage& thumbnail) {
	if( !d->mExifData) {
		return;
	}
	
	if(d->mExifData->data) {
		free(d->mExifData->data);
		d->mExifData->data=0;
	}
	d->mExifData->size=0;

	QByteArray array;
	QBuffer buffer(array);
	buffer.open(IO_WriteOnly);
	QImageIO iio(&buffer, "JPEG");
	iio.setImage(thumbnail);
	if (!iio.write()) {
		kdError() << "Could not write thumbnail\n";
		return;
	}
	
	d->mExifData->size=array.size();
	d->mExifData->data=(unsigned char*)malloc(d->mExifData->size);
	if (!d->mExifData->data) {
		kdError() << "Could not allocate memory for thumbnail\n";
		return;
	}
	memcpy(d->mExifData->data, array.data(), array.size());
}


bool JPEGContent::save(const QString& path) {
	QFile file(path);
	if (!file.open(IO_WriteOnly)) {
		kdError() << "Could not open '" << path << "' for writing\n";
		return false;
	}

	return save(&file);
}


bool JPEGContent::save(QFile* file) {
	if (d->mRawData.size()==0) {
		kdError() << "No data to store in '" << file->name() << "'\n";
		return false;
	}

	if (d->mPendingChanges) {
		applyPendingChanges();
		d->mPendingChanges = false;
	}

	if (d->mExifData) {
		// Store Exif info
		JPEGData* jpegData=jpeg_data_new_from_data((unsigned char*)d->mRawData.data(), d->mRawData.size());
		if (!jpegData) {
			kdError() << "Could not create jpegData object\n";
			return false;
		}
		
		jpeg_data_set_exif_data(jpegData, d->mExifData);
		unsigned char* dest=0L;
		unsigned int destSize=0;
		jpeg_data_save_data(jpegData, &dest, &destSize);
		jpeg_data_unref(jpegData);

		// Update mRawData
		d->mRawData.assign((char*)dest, destSize);
	}
	
	QDataStream stream(file);
	stream.writeRawBytes(d->mRawData.data(), d->mRawData.size());

	// Make sure we are up to date
	loadFromData(d->mRawData);
	return true;
}


} // namespace
