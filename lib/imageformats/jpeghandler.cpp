// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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

--

The content of this file is based on qjpeghandler.cpp
Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.

*/
// Self
#include "jpeghandler.h"

// Qt
#include <QImage>
#include <QSize>
#include <QVariant>

// KDE
#include <KDebug>

// libjpeg
#include <setjmp.h>
#define XMD_H
extern "C" {
#include <jpeglib.h>
}

// Local
#include "../iodevicejpegsourcemanager.h"

namespace Gwenview
{

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) kDebug() << x
#else
#define LOG(x) ;
#endif

struct JpegFatalError : public jpeg_error_mgr
{
    jmp_buf mJmpBuffer;

    static void handler(j_common_ptr cinfo)
    {
        JpegFatalError* error = static_cast<JpegFatalError*>(cinfo->err);
        (error->output_message)(cinfo);
        longjmp(error->mJmpBuffer, 1);
    }
};

static void expand24to32bpp(QImage* image)
{
    for (int j = 0; j < image->height(); ++j) {
        uchar *in = image->scanLine(j) + (image->width() - 1) * 3;
        QRgb *out = (QRgb*)(image->scanLine(j)) + image->width() - 1;

        for (int i = image->width() - 1; i >= 0; --i, --out, in -= 3) {
            *out = qRgb(in[0], in[1], in[2]);
        }
    }
}

static void convertCmykToRgb(QImage* image)
{
    for (int j = 0; j < image->height(); ++j) {
        uchar *in = image->scanLine(j) + image->width() * 4;
        QRgb *out = (QRgb*)image->scanLine(j);

        for (int i = image->width() - 1; i >= 0; --i) {
            in -= 4;
            int k = in[3];
            out[i] = qRgb(k * in[0] / 255, k * in[1] / 255, k * in[2] / 255);
        }
    }
}

static QSize getJpegSize(QIODevice* ioDevice)
{
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

static bool loadJpeg(QImage* image, QIODevice* ioDevice, QSize scaledSize)
{
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
    cinfo.scale_num = 1;
    if (!scaledSize.isEmpty()) {
        // Use !scaledSize.isEmpty(), not scaledSize.isValid() because
        // isValid() returns true if both the width and height is equal to or
        // greater than 0, so it is possible to get a division by 0.
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
    switch (cinfo.output_components) {
    case 3:
    case 4:
        *image = QImage(cinfo.output_width, cinfo.output_height, QImage::Format_RGB32);
        break;
    case 1: // B&W image
        *image = QImage(cinfo.output_width, cinfo.output_height, QImage::Format_Indexed8);
        image->setNumColors(256);
        for (int i = 0; i < 256; ++i) {
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

    switch (cinfo.out_color_space) {
    case JCS_CMYK:
        convertCmykToRgb(image);
        break;
    case JCS_RGB:
    case JCS_GRAYSCALE:
        break;
    default:
        kWarning() << "Unhandled JPEG colorspace" << cinfo.out_color_space;
        break;
    }

    if (cinfo.output_components == 3) {
        expand24to32bpp(image);
    }

    const QSize actualSize(cinfo.output_width, cinfo.output_height);
    if (scaledSize.isValid() && actualSize != scaledSize) {
        *image = image->scaled(scaledSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    return true;
}

/****************************************************************************
This code is a copy of qjpeghandler.cpp because I can't find a way to fallback
to it for image writing.
BEGIN_COPY
****************************************************************************/
struct my_error_mgr : public jpeg_error_mgr
{
    jmp_buf setjmp_buffer;
};

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

    static void my_error_exit(j_common_ptr cinfo)
    {
        my_error_mgr* myerr = (my_error_mgr*) cinfo->err;
        char buffer[JMSG_LENGTH_MAX];
        (*cinfo->err->format_message)(cinfo, buffer);
        qWarning("%s", buffer);
        longjmp(myerr->setjmp_buffer, 1);
    }

#if defined(Q_C_CALLBACKS)
}
#endif

static const int max_buf = 4096;

struct my_jpeg_destination_mgr : public jpeg_destination_mgr
{
    // Nothing dynamic - cannot rely on destruction over longjump
    QIODevice *device;
    JOCTET buffer[max_buf];

public:
    my_jpeg_destination_mgr(QIODevice *);
};

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

    static void qt_init_destination(j_compress_ptr)
    {
    }

    static boolean qt_empty_output_buffer(j_compress_ptr cinfo)
    {
        my_jpeg_destination_mgr* dest = (my_jpeg_destination_mgr*)cinfo->dest;

        int written = dest->device->write((char*)dest->buffer, max_buf);
        if (written == -1)
            (*cinfo->err->error_exit)((j_common_ptr)cinfo);

        dest->next_output_byte = dest->buffer;
        dest->free_in_buffer = max_buf;

#if defined(Q_OS_UNIXWARE)
        return B_TRUE;
#else
        return true;
#endif
    }

    static void qt_term_destination(j_compress_ptr cinfo)
    {
        my_jpeg_destination_mgr* dest = (my_jpeg_destination_mgr*)cinfo->dest;
        qint64 n = max_buf - dest->free_in_buffer;

        qint64 written = dest->device->write((char*)dest->buffer, n);
        if (written == -1)
            (*cinfo->err->error_exit)((j_common_ptr)cinfo);
    }

#if defined(Q_C_CALLBACKS)
}
#endif

inline my_jpeg_destination_mgr::my_jpeg_destination_mgr(QIODevice *device)
{
    jpeg_destination_mgr::init_destination = qt_init_destination;
    jpeg_destination_mgr::empty_output_buffer = qt_empty_output_buffer;
    jpeg_destination_mgr::term_destination = qt_term_destination;
    this->device = device;
    next_output_byte = buffer;
    free_in_buffer = max_buf;
}

static bool write_jpeg_image(const QImage &sourceImage, QIODevice *device, int sourceQuality)
{
    bool success = false;
    const QImage image = sourceImage;
    const QVector<QRgb> cmap = image.colorTable();

    struct jpeg_compress_struct cinfo;
    JSAMPROW row_pointer[1];
    row_pointer[0] = 0;

    struct my_jpeg_destination_mgr *iod_dest = new my_jpeg_destination_mgr(device);
    struct my_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = my_error_exit;

    if (!setjmp(jerr.setjmp_buffer)) {
        // WARNING:
        // this if loop is inside a setjmp/longjmp branch
        // do not create C++ temporaries here because the destructor may never be called
        // if you allocate memory, make sure that you can free it (row_pointer[0])
        jpeg_create_compress(&cinfo);

        cinfo.dest = iod_dest;

        cinfo.image_width = image.width();
        cinfo.image_height = image.height();

        bool gray = false;
        switch (image.depth()) {
        case 1:
        case 8:
            gray = true;
            for (int i = image.numColors(); gray && i--;) {
                gray = gray & (qRed(cmap[i]) == qGreen(cmap[i]) &&
                               qRed(cmap[i]) == qBlue(cmap[i]));
            }
            cinfo.input_components = gray ? 1 : 3;
            cinfo.in_color_space = gray ? JCS_GRAYSCALE : JCS_RGB;
            break;
        case 32:
            cinfo.input_components = 3;
            cinfo.in_color_space = JCS_RGB;
        }

        jpeg_set_defaults(&cinfo);

        qreal diffInch = qAbs(image.dotsPerMeterX() * 2.54 / 100. - qRound(image.dotsPerMeterX() * 2.54 / 100.))
                         + qAbs(image.dotsPerMeterY() * 2.54 / 100. - qRound(image.dotsPerMeterY() * 2.54 / 100.));
        qreal diffCm = (qAbs(image.dotsPerMeterX() / 100. - qRound(image.dotsPerMeterX() / 100.))
                        + qAbs(image.dotsPerMeterY() / 100. - qRound(image.dotsPerMeterY() / 100.))) * 2.54;
        if (diffInch < diffCm) {
            cinfo.density_unit = 1; // dots/inch
            cinfo.X_density = qRound(image.dotsPerMeterX() * 2.54 / 100.);
            cinfo.Y_density = qRound(image.dotsPerMeterY() * 2.54 / 100.);
        } else {
            cinfo.density_unit = 2; // dots/cm
            cinfo.X_density = (image.dotsPerMeterX() + 50) / 100;
            cinfo.Y_density = (image.dotsPerMeterY() + 50) / 100;
        }

        int quality = sourceQuality >= 0 ? qMin(sourceQuality, 100) : 75;
#if defined(Q_OS_UNIXWARE)
        jpeg_set_quality(&cinfo, quality, B_TRUE /* limit to baseline-JPEG values */);
        jpeg_start_compress(&cinfo, B_TRUE);
#else
        jpeg_set_quality(&cinfo, quality, true /* limit to baseline-JPEG values */);
        jpeg_start_compress(&cinfo, true);
#endif

        row_pointer[0] = new uchar[cinfo.image_width * cinfo.input_components];
        int w = cinfo.image_width;
        while (cinfo.next_scanline < cinfo.image_height) {
            uchar *row = row_pointer[0];
            switch (image.depth()) {
            case 1:
                if (gray) {
                    const uchar* data = image.scanLine(cinfo.next_scanline);
                    if (image.format() == QImage::Format_MonoLSB) {
                        for (int i = 0; i < w; i++) {
                            bool bit = !!(*(data + (i >> 3)) & (1 << (i & 7)));
                            row[i] = qRed(cmap[bit]);
                        }
                    } else {
                        for (int i = 0; i < w; i++) {
                            bool bit = !!(*(data + (i >> 3)) & (1 << (7 - (i & 7))));
                            row[i] = qRed(cmap[bit]);
                        }
                    }
                } else {
                    const uchar* data = image.scanLine(cinfo.next_scanline);
                    if (image.format() == QImage::Format_MonoLSB) {
                        for (int i = 0; i < w; i++) {
                            bool bit = !!(*(data + (i >> 3)) & (1 << (i & 7)));
                            *row++ = qRed(cmap[bit]);
                            *row++ = qGreen(cmap[bit]);
                            *row++ = qBlue(cmap[bit]);
                        }
                    } else {
                        for (int i = 0; i < w; i++) {
                            bool bit = !!(*(data + (i >> 3)) & (1 << (7 - (i & 7))));
                            *row++ = qRed(cmap[bit]);
                            *row++ = qGreen(cmap[bit]);
                            *row++ = qBlue(cmap[bit]);
                        }
                    }
                }
                break;
            case 8:
                if (gray) {
                    const uchar* pix = image.scanLine(cinfo.next_scanline);
                    for (int i = 0; i < w; i++) {
                        *row = qRed(cmap[*pix]);
                        ++row; ++pix;
                    }
                } else {
                    const uchar* pix = image.scanLine(cinfo.next_scanline);
                    for (int i = 0; i < w; i++) {
                        *row++ = qRed(cmap[*pix]);
                        *row++ = qGreen(cmap[*pix]);
                        *row++ = qBlue(cmap[*pix]);
                        ++pix;
                    }
                }
                break;
            case 32: {
                QRgb* rgb = (QRgb*)image.scanLine(cinfo.next_scanline);
                for (int i = 0; i < w; i++) {
                    *row++ = qRed(*rgb);
                    *row++ = qGreen(*rgb);
                    *row++ = qBlue(*rgb);
                    ++rgb;
                }
            }
            }
            jpeg_write_scanlines(&cinfo, row_pointer, 1);
        }

        jpeg_finish_compress(&cinfo);
        jpeg_destroy_compress(&cinfo);
        success = true;
    } else {
        jpeg_destroy_compress(&cinfo);
        success = false;
    }

    delete iod_dest;
    delete [] row_pointer[0];
    return success;
}
/****************************************************************************
END_COPY
****************************************************************************/

struct JpegHandlerPrivate
{
    QSize mScaledSize;
    int mQuality;
};

JpegHandler::JpegHandler()
: d(new JpegHandlerPrivate)
{
    d->mQuality = 75;
}

JpegHandler::~JpegHandler()
{
    delete d;
}

bool JpegHandler::canRead() const
{
    if (canRead(device())) {
        setFormat("jpeg");
        return true;
    }
    return false;
}

bool JpegHandler::canRead(QIODevice* device)
{
    if (!device) {
        kWarning() << "called with no device";
        return false;
    }

    return device->peek(2) == "\xFF\xD8";
}

bool JpegHandler::read(QImage* image)
{
    LOG("");
    if (!canRead()) {
        return false;
    }
    return loadJpeg(image, device(), d->mScaledSize);
}

bool JpegHandler::write(const QImage& image)
{
    LOG("");
    return write_jpeg_image(image, device(), d->mQuality);
}

bool JpegHandler::supportsOption(ImageOption option) const
{
    return option == ScaledSize || option == Size || option == Quality;
}

QVariant JpegHandler::option(ImageOption option) const
{
    if (option == ScaledSize) {
        return d->mScaledSize;
    } else if (option == Size) {
        if (canRead() && !device()->isSequential()) {
            qint64 pos = device()->pos();
            QSize size = getJpegSize(device());
            device()->seek(pos);
            return size;
        }
    } else if (option == Quality) {
        return d->mQuality;
    }
    return QVariant();
}

void JpegHandler::setOption(ImageOption option, const QVariant &value)
{
    if (option == ScaledSize) {
        d->mScaledSize = value.toSize();
    } else if (option == Quality) {
        d->mQuality = value.toInt();
    }
}

} // namespace
