// Qt
#include <qcstring.h>
#include <qimage.h>
#include <qstring.h>
#include <qwmatrix.h>

// KDE
#include <kdebug.h>

// Exif
#include "libgvexif/exif-data.h"
#include "libgvexif/exif-ifd.h"
#include "libgvexif/exif-utils.h"
#include "libgvexif/jpeg-data.h"

// Local
#include "gvimageutils.h"

namespace GVImageUtils {


/**
 * Simple wrapper to raw data pointer. It's main goal in life is to prevent
 * from forgetting to unref the data. Gotta love C++ :-)
 */
template<class T, void(*unrefFcn)(T*)>
class DataPtr {
public:
	DataPtr(T* data) : mData(data) { }
	
	~DataPtr() {
		if (mData) unrefFcn(mData);
	}
	
	operator T*() const    { return mData; }
	T* operator->() const  { return mData; }
	bool operator!() const { return !mData; }

protected:
	T* mData;

private:
	// Better safe than sorry
	DataPtr(const DataPtr&) {}
	DataPtr& operator=(const DataPtr&) {}
};

typedef DataPtr<JPEGData,jpeg_data_unref> JPEGDataPtr;
typedef DataPtr<ExifData,exif_data_unref> ExifDataPtr;


static Orientation getOrientation(ExifData* data) {
	if (!data) {
		return NotAvailable;
	}
	ExifByteOrder byteOrder=exif_data_get_byte_order(data);
	
	ExifEntry* entry(
		exif_content_get_entry(data->ifd[EXIF_IFD_0],
			EXIF_TAG_ORIENTATION) );
	if (!entry) {
		return NotAvailable;
	}
	short value=exif_get_short(entry->data, byteOrder);
	if (value<Normal || value>Rot270) return NotAvailable;
	return Orientation(value);
}


Orientation getOrientation(const QString& pixPath) {
	ExifDataPtr exifData( exif_data_new_from_file(pixPath.ascii()) );
	return getOrientation(exifData);
}


Orientation getOrientation(const QByteArray& jpegContent) {
	JPEGDataPtr jpegData( jpeg_data_new_from_data((unsigned char*)jpegContent.data(),jpegContent.size()) );
	if (!jpegData) {
		return NotAvailable;
	}
	
	ExifDataPtr exifData( jpeg_data_get_exif_data(jpegData) );
	return getOrientation(exifData);
}




QByteArray setOrientation(const QByteArray& jpegContent, Orientation orientation) {
	JPEGDataPtr jpegData( jpeg_data_new_from_data((unsigned char*)jpegContent.data(),jpegContent.size()) );
	if (!jpegData) {
		return jpegContent;
	}
	
	ExifDataPtr exifData( jpeg_data_get_exif_data(jpegData) );
	if (!exifData) {
		return jpegContent;
	}
	ExifByteOrder byteOrder=exif_data_get_byte_order(exifData);
	
	ExifEntry* entry( exif_content_get_entry(
			exifData->ifd[EXIF_IFD_0], EXIF_TAG_ORIENTATION) );
	if (!entry) {
		return jpegContent;
	}
	exif_set_short(entry->data, byteOrder, short(orientation));

	jpeg_data_set_exif_data(jpegData, exifData);
	unsigned char* dest=0L;
	unsigned int destSize=0;
	jpeg_data_save_data(jpegData, &dest, &destSize);
	
	QByteArray destArray;
	destArray.assign((char*)dest,destSize);
	return destArray;
}


QImage rotate(const QImage& img, Orientation orientation) {
	QWMatrix matrix;
	switch (orientation) {
	case NotAvailable:
	case Normal:
		return img;

	case HFlip:
		matrix.scale(-1,1);
		break;

	case Rot180:
		matrix.rotate(180);
		break;

	case VFlip:
		matrix.scale(1,-1);
		break;
	
	case Rot90HFlip:
		matrix.scale(-1,1);
		matrix.rotate(90);
		break;
		
	case Rot90:		
		matrix.rotate(90);
		break;
	
	case Rot90VFlip:
		matrix.scale(1,-1);
		matrix.rotate(90);
		break;
		
	case Rot270:		
		matrix.rotate(270);
		break;
	}

	return img.xForm(matrix);
}


} // Namespace

