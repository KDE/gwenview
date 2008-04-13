// vim: set tabstop=4 shiftwidth=4 noexpandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <aurelien.gateau@free.fr>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "jpeghandler.h"

// Qt
#include <QImage>
#include <QSize>
#include <QVariant>

// KDE
#include <kdebug.h>

// libjpeg
#include <setjmp.h>
#define XMD_H
extern "C" {
#include <jpeglib.h>
}

// Local
#include "../iodevicejpegsourcemanager.h"

namespace Gwenview {

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << x
#else
#define LOG(x) ;
#endif


struct JpegFatalError : public jpeg_error_mgr {
	jmp_buf mJmpBuffer;

	static void handler(j_common_ptr cinfo) {
		JpegFatalError* error=static_cast<JpegFatalError*>(cinfo->err);
		(error->output_message)(cinfo);
		longjmp(error->mJmpBuffer,1);
	}
};


static QSize getJpegSize(QIODevice* ioDevice) {
	struct jpeg_decompress_struct cinfo;
	QSize size;

	// Error handling
	struct JpegFatalError jerr;
	cinfo.err = jpeg_std_error(&jerr);
	cinfo.err->error_exit = JpegFatalError::handler;
	if (setjmp(jerr.mJmpBuffer)) {
		jpeg_destroy_decompress(&cinfo);
		return size;
	}

	// Init decompression
	jpeg_create_decompress(&cinfo);
	Gwenview::IODeviceJpegSourceManager::setup(&cinfo, ioDevice);
	jpeg_read_header(&cinfo, true);

	size = QSize(cinfo.image_width, cinfo.image_height);
	jpeg_destroy_decompress(&cinfo);
	return size;
}


static bool loadJpeg(QImage* image, QIODevice* ioDevice, QSize scaledSize) {
	struct jpeg_decompress_struct cinfo;

	// Error handling
	struct JpegFatalError jerr;
	cinfo.err = jpeg_std_error(&jerr);
	cinfo.err->error_exit = JpegFatalError::handler;
	if (setjmp(jerr.mJmpBuffer)) {
		jpeg_destroy_decompress(&cinfo);
		return false;
	}

	// Init decompression
	jpeg_create_decompress(&cinfo);
	Gwenview::IODeviceJpegSourceManager::setup(&cinfo, ioDevice);
	jpeg_read_header(&cinfo, true);

	// Compute scale value
	cinfo.scale_num=1;
	if (scaledSize.isValid()) {
		cinfo.scale_denom = qMin(cinfo.image_width / scaledSize.width(),
								 cinfo.image_height / scaledSize.height());
		if (cinfo.scale_denom < 2) {
			cinfo.scale_denom = 1;
		} else if (cinfo.scale_denom < 4) {
			cinfo.scale_denom = 2;
		} else if (cinfo.scale_denom < 8) {
			cinfo.scale_denom = 4;
		} else {
			cinfo.scale_denom = 8;
		}
	} else {
		cinfo.scale_denom = 1;
	}
	LOG("cinfo.scale_denom=" << cinfo.scale_denom);

	// Init image
	jpeg_start_decompress(&cinfo);
	switch(cinfo.output_components) {
	case 3:
	case 4:
		*image = QImage(cinfo.output_width, cinfo.output_height, QImage::Format_RGB32);
		break;
	case 1: // B&W image
		*image = QImage(cinfo.output_width, cinfo.output_height, QImage::Format_Indexed8);
		image->setNumColors(256);
		for (int i=0; i<256; i++) {
			image->setColor(i, qRgba(i, i, i, 255));
		}
		break;
	default:
		jpeg_destroy_decompress(&cinfo);
		return false;
	}

	while (cinfo.output_scanline < cinfo.output_height) {
		uchar *line = image->scanLine(cinfo.output_scanline);
		jpeg_read_scanlines(&cinfo, &line, 1);
	}

	jpeg_finish_decompress(&cinfo);

	// Expand 24->32 bpp
	if ( cinfo.output_components == 3 ) {
		for (uint j=0; j<cinfo.output_height; j++) {
			uchar *in = image->scanLine(j) + (cinfo.output_width - 1)*3;
			QRgb *out = (QRgb*)( image->scanLine(j) ) + cinfo.output_width - 1;

			for (int i=cinfo.output_width - 1; i>=0; --i, --out, in -= 3) {
				*out = qRgb(in[0], in[1], in[2]);
			}
		}
	}

	if (scaledSize.isValid()) {
		*image = image->scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	}

	jpeg_destroy_decompress(&cinfo);

	return true;
}



struct JpegHandlerPrivate {
	QSize mScaledSize;
};


JpegHandler::JpegHandler()
: d(new JpegHandlerPrivate) {
}


JpegHandler::~JpegHandler() {
	delete d;
}


bool JpegHandler::canRead() const {
	if (canRead(device())) {
		setFormat("jpeg");
		return true;
	}
	return false;
}


bool JpegHandler::canRead(QIODevice* device) {
	if (!device) {
		kWarning() << "called with no device";
		return false;
	}

	return device->peek(2) == "\xFF\xD8";
}


bool JpegHandler::read(QImage* image) {
	LOG("");
	if (!canRead()) {
		return false;
	}
	return loadJpeg(image, device(), d->mScaledSize);
}


bool JpegHandler::supportsOption(ImageOption option) const {
    return option == ScaledSize || option == Size;
}


QVariant JpegHandler::option(ImageOption option) const {
	if (option == ScaledSize) {
		return d->mScaledSize;
	} else if (option == Size) {
		if (canRead() && !device()->isSequential()) {
			qint64 pos = device()->pos();
			QSize size = getJpegSize(device());
			device()->seek(pos);
			return size;
		}
	}
	return QVariant();
}


void JpegHandler::setOption(ImageOption option, const QVariant &value) {
	if (option == ScaledSize) {
		d->mScaledSize = value.toSize();
	}
}


QByteArray JpegHandler::name() const {
	return "jpeg";
}

} // namespace
