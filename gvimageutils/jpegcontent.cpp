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
// System
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

// KDE
#include <kdebug.h>
#include <ktempfile.h>

// Exif
#include "libgvexif/exif-data.h"
#include "libgvexif/exif-ifd.h"
#include "libgvexif/exif-utils.h"
#include "libgvexif/jpeg-data.h"

// Local
#include "gvimageutils/gvimageutils.h"
#include "gvimageutils/jpegcontent.h"

namespace GVImageUtils {

	
struct JPEGContent::Private {
	QByteArray mRawData;
	ExifData* mExifData;
	ExifEntry* mOrientationEntry;
	ExifByteOrder mByteOrder;

	Private() {
		mExifData=0;
		mOrientationEntry=0;
	}
};


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
	if (d->mExifData) {
		exif_data_unref(d->mExifData);
	}

	d->mRawData = data;
	d->mExifData = exif_data_new_from_data((unsigned char*)data.data(), data.size());
	if (!d->mExifData) {
		kdError() << "Could not load exif data\n";
		return false;
	}
	d->mByteOrder = exif_data_get_byte_order(d->mExifData);
	
	d->mOrientationEntry =
		exif_content_get_entry(d->mExifData->ifd[EXIF_IFD_0],
			EXIF_TAG_ORIENTATION);

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
		short(GVImageUtils::NORMAL));
}


// FIXME: This shoud use in-memory jpeg readers and writers
void JPEGContent::transform(Orientation orientation) {
	QMap<Orientation,JXFORM_CODE> orientation2jxform;
	orientation2jxform[NOT_AVAILABLE]= JXFORM_NONE;
	orientation2jxform[NORMAL]=        JXFORM_NONE;
	orientation2jxform[HFLIP]=         JXFORM_FLIP_H;
	orientation2jxform[ROT_180]=       JXFORM_ROT_180;
	orientation2jxform[VFLIP]=         JXFORM_FLIP_V;
	orientation2jxform[ROT_90_HFLIP]=  JXFORM_TRANSPOSE;
	orientation2jxform[ROT_90]=        JXFORM_ROT_90;
	orientation2jxform[ROT_90_VFLIP]=  JXFORM_TRANSVERSE;
	orientation2jxform[ROT_270]=       JXFORM_ROT_270;

	// NOTE: We do not use the default values here, because this would cause
	// KTempFile to use the appname. Since it's not set by the test program,
	// it crashes (as of KDE 3.3.0)
	KTempFile srcTemp("src", ".jpg");
	srcTemp.setAutoDelete(true);
	KTempFile dstTemp("dst", ".jpg");
	dstTemp.setAutoDelete(true);

	// Copy array to srcTemp
	QFile file(srcTemp.name());
	if (!file.open(IO_WriteOnly)) {
		kdError() << "Could not open '" << srcTemp.name() << "' for writing\n";
		return;
	}
	QDataStream stream(&file);
	stream.writeRawBytes(d->mRawData.data(), d->mRawData.size());
	file.close();
	
	// The following code is inspired by jpegtran.c from the libjpeg

	// Init JPEG structs 
	struct jpeg_decompress_struct srcinfo;
	struct jpeg_compress_struct dstinfo;
	struct jpeg_error_mgr jsrcerr, jdsterr;
	jvirt_barray_ptr * src_coef_arrays;
	jvirt_barray_ptr * dst_coef_arrays;

	// Initialize the JPEG decompression object with default error handling
	srcinfo.err = jpeg_std_error(&jsrcerr);
	jpeg_create_decompress(&srcinfo);

	// Initialize the JPEG compression object with default error handling
	dstinfo.err = jpeg_std_error(&jdsterr);
	jpeg_create_compress(&dstinfo);

	// Open files
	FILE *input_file=fopen(QFile::encodeName(srcTemp.name()), "r");
	if (!input_file) {
		kdError() << "Could not open temp file for reading\n";
		return;
	}
	FILE *output_file=fopen(QFile::encodeName(dstTemp.name()), "w");
	if (!output_file) {
		fclose(input_file);
		kdError() << "Could not open temp file for writing\n";
		return;
	}

	// Specify data source for decompression
	jpeg_stdio_src(&srcinfo, input_file);

	// Enable saving of extra markers that we want to copy
	jcopy_markers_setup(&srcinfo, JCOPYOPT_ALL);

	// Read file header
	(void) jpeg_read_header(&srcinfo, TRUE);

	// Init transformation
	jpeg_transform_info transformoption;
	transformoption.transform = orientation2jxform[orientation];
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
	jpeg_stdio_dest(&dstinfo, output_file);

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

	/* Close files, if we opened them */
	fclose(input_file);
	fclose(output_file);
	
	// Reload the new JPEG
	load(dstTemp.name());
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


bool JPEGContent::save(const QString& path) const {
	QFile file(path);
	if (!file.open(IO_WriteOnly)) {
		kdError() << "Could not open '" << path << "' for writing\n";
		return false;
	}

	if (!d->mExifData) {
		// There's no Exif info, let's just save the byte array
		QDataStream stream(&file);
		stream.writeRawBytes(d->mRawData.data(), d->mRawData.size());
		return true;
	}

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
	
	QDataStream stream(&file);
	stream.writeRawBytes((char*)dest, destSize);
	free(dest);
	return true;
}


} // namespace
