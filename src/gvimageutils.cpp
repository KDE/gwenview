// Qt
#include <qimage.h>
#include <qstring.h>
#include <qwmatrix.h>

// KDE
#include <kdebug.h>
#include <kfilemetainfo.h>

// Local
#include "gvimageutils.h"

namespace GVImageUtils {


// Inspired from Kuickshow imlibwidget.cpp::autoRotate
Orientation getOrientation(const QString& pixPath) {
	KFileMetaInfo metaInfo(pixPath);
	if (!metaInfo.isValid()) return NotAvailable;

	KFileMetaInfoItem item = metaInfo.item("Orientation");
	if ( !item.isValid()
#if QT_VERSION >= 0x030100
		|| item.value().isNull()
#endif
		) return NotAvailable;

	int value=item.value().toInt();

	// Sanity check
	if (value<int(Normal) && value>int(Rot270)) return NotAvailable;
	return Orientation(value);
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

