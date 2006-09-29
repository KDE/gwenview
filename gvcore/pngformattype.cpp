// this code is copied from Qt, with code added to actually call consumer
// methods that inform about the progress of loading

/****************************************************************************
** 
**
** Implementation of PNG QImage IOHandler
**
** Created : 970521
**
** Copyright (C) 1992-2003 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "pngformattype.h"

#include <png.h>

namespace Gwenview {

class PNGFormat : public QImageFormat {
public:
    PNGFormat();
    virtual ~PNGFormat();

    int decode(QImage& img, QImageConsumer* consumer,
	    const uchar* buffer, int length);

    void info(png_structp png_ptr, png_infop info);
    void row(png_structp png_ptr, png_bytep new_row,
		png_uint_32 row_num, int pass);
    void end(png_structp png_ptr, png_infop info);
#ifdef PNG_USER_CHUNKS_SUPPORTED
    int user_chunk(png_structp png_ptr,
	    png_bytep data, png_uint_32 length);
#endif

private:
    // Animation-level information
    enum { MovieStart, FrameStart, Inside, End } state;
    int first_frame;
    int base_offx;
    int base_offy;

    // Image-level information
    png_structp png_ptr;
    png_infop info_ptr;

    // Temporary locals during single data-chunk processing
    QImageConsumer* consumer;
    QImage* image;
    int unused_data;
    QRect changed_rect;
};

/*
  \class QPNGFormat qpngio.h
  \brief The QPNGFormat class provides an incremental image decoder for PNG
  image format.

  \ingroup images
  \ingroup graphics

  This subclass of QImageFormat decodes PNG format images,
  including animated PNGs.

  Animated PNG images are standard PNG images. The PNG standard
  defines two extension chunks that are useful for animations:

  \list
    \i gIFg - GIF-like Graphic Control Extension.
     This includes frame disposal, user input flag (we ignore this),
	    and inter-frame delay.
   \i gIFx - GIF-like Application Extension.
    This is multi-purpose, but we just use the Netscape extension
	    which specifies looping.
  \endlist

  The subimages usually contain a offset chunk (oFFs) but need not.

  The first image defines the "screen" size. Any subsequent image that
  doesn't fit is clipped.
*/
/* ###TODO: decide on this point. gIFg gives disposal types, so it can be done.
  All images paste (\e not composite, just place all-channel copying)
  over the previous image to produce a subsequent frame.
*/

/*
  \class QPNGFormatType qasyncimageio.h
  \brief The QPNGFormatType class provides an incremental image decoder
  for PNG image format.

  \ingroup images
  \ingroup graphics
  \ingroup io

  This subclass of QImageFormatType recognizes PNG format images, creating
  a QPNGFormat when required. An instance of this class is created
  automatically before any other factories, so you should have no need for
  such objects.
*/

QImageFormat* PNGFormatType::decoderFor(
    const uchar* buffer, int length)
{
    if (length < 8) return 0;
    if (buffer[0]==137
     && buffer[1]=='P'
     && buffer[2]=='N'
     && buffer[3]=='G'
     && buffer[4]==13
     && buffer[5]==10
     && buffer[6]==26
     && buffer[7]==10)
	return new PNGFormat;
    return 0;
}

const char* PNGFormatType::formatName() const
{
    return "PNG";
}

extern "C" {

static void
info_callback(png_structp png_ptr, png_infop info)
{
    PNGFormat* that = (PNGFormat*)png_get_progressive_ptr(png_ptr);
    that->info(png_ptr,info);
}

static void
row_callback(png_structp png_ptr, png_bytep new_row,
   png_uint_32 row_num, int pass)
{
    PNGFormat* that = (PNGFormat*)png_get_progressive_ptr(png_ptr);
    that->row(png_ptr,new_row,row_num,pass);
}

static void
end_callback(png_structp png_ptr, png_infop info)
{
    PNGFormat* that = (PNGFormat*)png_get_progressive_ptr(png_ptr);
    that->end(png_ptr,info);
}

#if 0
#ifdef PNG_USER_CHUNKS_SUPPORTED
static int
CALLBACK_CALL_TYPE user_chunk_callback(png_structp png_ptr,
         png_unknown_chunkp chunk)
{
    PNGFormat* that = (PNGFormat*)png_get_progressive_ptr(png_ptr);
    return that->user_chunk(png_ptr,chunk->data,chunk->size);
}
#endif
#endif

static void qt_png_warning(png_structp /*png_ptr*/, png_const_charp message)
{
    qWarning("libpng warning: %s", message);
}

}

static
void setup_qt( QImage& image, png_structp png_ptr, png_infop info_ptr )
{
    // For now, we will use the PC monitor gamma, if you own a Mac, you'd better use 1.8
    const double SCREEN_GAMMA=2.2;
    if ( png_get_valid(png_ptr, info_ptr, PNG_INFO_gAMA) ) {
	double file_gamma;
	png_get_gAMA(png_ptr, info_ptr, &file_gamma);
	png_set_gamma( png_ptr, SCREEN_GAMMA, file_gamma );
    }

    png_uint_32 width;
    png_uint_32 height;
    int bit_depth;
    int color_type;
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
	0, 0, 0);

    if ( color_type == PNG_COLOR_TYPE_GRAY ) {
	// Black & White or 8-bit grayscale
	if ( bit_depth == 1 && info_ptr->channels == 1 ) {
	    png_set_invert_mono( png_ptr );
	    png_read_update_info( png_ptr, info_ptr );
	    if (!image.create( width, height, 1, 2, QImage::BigEndian ))
		return;
	    image.setColor( 1, qRgb(0,0,0) );
	    image.setColor( 0, qRgb(255,255,255) );
	} else if (bit_depth == 16 && png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
	    png_set_expand(png_ptr);
	    png_set_strip_16(png_ptr);
	    png_set_gray_to_rgb(png_ptr);

	    if (!image.create(width, height, 32))
		return;
	    image.setAlphaBuffer(TRUE);

	    if (QImage::systemByteOrder() == QImage::BigEndian)
		png_set_swap_alpha(png_ptr);

	    png_read_update_info(png_ptr, info_ptr);
	} else {
	    if ( bit_depth == 16 )
		png_set_strip_16(png_ptr);
	    else if ( bit_depth < 8 )
		png_set_packing(png_ptr);
	    int ncols = bit_depth < 8 ? 1 << bit_depth : 256;
	    png_read_update_info(png_ptr, info_ptr);
	    if (!image.create(width, height, 8, ncols))
		return;
	    for (int i=0; i<ncols; i++) {
		int c = i*255/(ncols-1);
		image.setColor( i, qRgba(c,c,c,0xff) );
	    }
	    if ( png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) ) {
		const int g = info_ptr->trans_values.gray;
		if (g < ncols) {
		    image.setAlphaBuffer(TRUE);
		    image.setColor(g, image.color(g) & RGB_MASK);
		}
	    }
	}
    } else if ( color_type == PNG_COLOR_TYPE_PALETTE
     && png_get_valid(png_ptr, info_ptr, PNG_INFO_PLTE)
     && info_ptr->num_palette <= 256 )
    {
	// 1-bit and 8-bit color
	if ( bit_depth != 1 )
	    png_set_packing( png_ptr );
	png_read_update_info( png_ptr, info_ptr );
	png_get_IHDR(png_ptr, info_ptr,
	    &width, &height, &bit_depth, &color_type, 0, 0, 0);
	if (!image.create(width, height, bit_depth, info_ptr->num_palette,
	    QImage::BigEndian))
	    return;
	int i = 0;
	if ( png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) ) {
	    image.setAlphaBuffer( TRUE );
	    while ( i < info_ptr->num_trans ) {
		image.setColor(i, qRgba(
		    info_ptr->palette[i].red,
		    info_ptr->palette[i].green,
		    info_ptr->palette[i].blue,
		    info_ptr->trans[i]
		    )
		);
		i++;
	    }
	}
	while ( i < info_ptr->num_palette ) {
	    image.setColor(i, qRgba(
		info_ptr->palette[i].red,
		info_ptr->palette[i].green,
		info_ptr->palette[i].blue,
		0xff
		)
	    );
	    i++;
	}
    } else {
	// 32-bit
	if ( bit_depth == 16 )
	    png_set_strip_16(png_ptr);

	png_set_expand(png_ptr);

	if ( color_type == PNG_COLOR_TYPE_GRAY_ALPHA )
	    png_set_gray_to_rgb(png_ptr);

	if (!image.create(width, height, 32))
	    return;

	// Only add filler if no alpha, or we can get 5 channel data.
	if (!(color_type & PNG_COLOR_MASK_ALPHA)
	   && !png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
	    png_set_filler(png_ptr, 0xff,
		QImage::systemByteOrder() == QImage::BigEndian ?
		    PNG_FILLER_BEFORE : PNG_FILLER_AFTER);
	    // We want 4 bytes, but it isn't an alpha channel
	} else {
	    image.setAlphaBuffer(TRUE);
	}

	if ( QImage::systemByteOrder() == QImage::BigEndian ) {
	    png_set_swap_alpha(png_ptr);
	}

	png_read_update_info(png_ptr, info_ptr);
    }

    // Qt==ARGB==Big(ARGB)==Little(BGRA)
    if ( QImage::systemByteOrder() == QImage::LittleEndian ) {
	png_set_bgr(png_ptr);
    }
}



/*!
    Constructs a QPNGFormat object.
*/
PNGFormat::PNGFormat()
{
    state = MovieStart;
    first_frame = 1;
    base_offx = 0;
    base_offy = 0;
    png_ptr = 0;
    info_ptr = 0;
}


/*!
    Destroys a QPNGFormat object.
*/
PNGFormat::~PNGFormat()
{
    if ( png_ptr )
        png_destroy_read_struct(&png_ptr, &info_ptr, 0);
}


/*!
    This function decodes some data into image changes.

    Returns the number of bytes consumed.
*/
int PNGFormat::decode(QImage& img, QImageConsumer* cons,
	const uchar* buffer, int length)
{
    consumer = cons;
    image = &img;

    if ( state != Inside ) {
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (!png_ptr) {
	    info_ptr = 0;
	    image = 0;
		return -1;
	}

	png_set_error_fn(png_ptr, 0, 0, qt_png_warning);
	png_set_compression_level(png_ptr, 9);

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
	    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	    image = 0;
	    return -1;
	}

	if (setjmp((png_ptr)->jmpbuf)) {
	    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	    image = 0;
	    return -1;
	}

	png_set_progressive_read_fn(png_ptr, (void *)this,
	                            info_callback, row_callback, end_callback);

#ifdef PNG_USER_CHUNKS_SUPPORTED
	// Can't do this yet. libpng has a crash bug with unknown (user) chunks.
	// Warwick has sent them a patch.
	// png_set_read_user_chunk_fn(png_ptr, 0, user_chunk_callback);
	// png_set_keep_unknown_chunks(png_ptr, 2/*HANDLE_CHUNK_IF_SAFE*/, 0, 0);
#endif

	if ( state != MovieStart && *buffer != 0211 ) {
	    // Good, no signature - the preferred way to concat PNG images.
	    // Skip them.
	    png_set_sig_bytes(png_ptr, 8);
	}

	state = Inside;
        changed_rect = QRect();
    }

    if ( !png_ptr ) return 0;

    if (setjmp(png_ptr->jmpbuf)) {
	png_destroy_read_struct(&png_ptr, &info_ptr, 0);
	image = 0;
	state = MovieStart;
	return -1;
    }
    unused_data = 0;
    png_process_data(png_ptr, info_ptr, (png_bytep)buffer, length);
    int l = length - unused_data;

    if( !changed_rect.isNull()) {
        consumer->changed( changed_rect );
        changed_rect = QRect();
    }

    if ( state != Inside ) {
	if ( png_ptr )
	    png_destroy_read_struct(&png_ptr, &info_ptr, 0);
    }

    image = 0;
    return l;
}

void PNGFormat::info(png_structp png, png_infop)
{
    png_set_interlace_handling(png);
    setup_qt(*image, png, info_ptr);
    consumer->setSize( image->width(), image->height());
}

void PNGFormat::row(png_structp png, png_bytep new_row,
    png_uint_32 row_num, int)
{
    uchar* old_row = image->scanLine(row_num);
    png_progressive_combine_row(png, old_row, new_row);
    changed_rect |= QRect( 0, row_num, image->width(), 1 );
}


void PNGFormat::end(png_structp png, png_infop info)
{
    int offx = png_get_x_offset_pixels(png,info) - base_offx;
    int offy = png_get_y_offset_pixels(png,info) - base_offy;
    if ( first_frame ) {
	base_offx = offx;
	base_offy = offy;
	first_frame = 0;
    }
    image->setOffset(QPoint(offx,offy));
    image->setDotsPerMeterX(png_get_x_pixels_per_meter(png,info));
    image->setDotsPerMeterY(png_get_y_pixels_per_meter(png,info));
    png_textp text_ptr;
    int num_text=0;
    png_get_text(png,info,&text_ptr,&num_text);
    while (num_text--) {
	image->setText(text_ptr->key,0,text_ptr->text);
	text_ptr++;
    }
    if( !changed_rect.isNull()) {
        consumer->changed( changed_rect );
        changed_rect = QRect();
    }
    QRect r(0,0,image->width(),image->height());
    consumer->frameDone(QPoint(offx,offy),r);
    consumer->end();
    state = FrameStart;
    unused_data = (int)png->buffer_size; // Since libpng doesn't tell us
}

#ifdef PNG_USER_CHUNKS_SUPPORTED

/*
#ifndef QT_NO_IMAGE_TEXT
static bool skip(png_uint_32& max, png_bytep& data)
{
    while (*data) {
	if ( !max ) return FALSE;
	max--;
	data++;
    }
    if ( !max ) return FALSE;
    max--;
    data++; // skip to after NUL
    return TRUE;
}
#endif
*/

int PNGFormat::user_chunk(png_structp png,
	    png_bytep data, png_uint_32 length)
{
#if 0 // NOT SUPPORTED: experimental PNG animation.
    // qDebug("Got %ld-byte %s chunk", length, png->chunk_name);
    if ( 0==qstrcmp((char*)png->chunk_name, "gIFg")
	    && length == 4 ) {

	//QPNGImageWriter::DisposalMethod disposal =
	//  (QPNGImageWriter::DisposalMethod)data[0];
	// ### TODO: use the disposal method
	int ms_delay = ((data[2] << 8) | data[3])*10;
	consumer->setFramePeriod(ms_delay);
	return 1;
    } else if ( 0==qstrcmp((char*)png->chunk_name, "gIFx")
	    && length == 13 ) {
	if ( qstrncmp((char*)data,"NETSCAPE2.0",11)==0 ) {
	    int looping = (data[0xC]<<8)|data[0xB];
	    consumer->setLooping(looping);
	    return 1;
	}
    }
#else
    Q_UNUSED( png )
    Q_UNUSED( data )
    Q_UNUSED( length )
#endif

    /*

    libpng now supports this chunk.


    if ( 0==qstrcmp((char*)png->chunk_name, "iTXt") && length>=6 ) {
	const char* keyword = (const char*)data;
	if ( !skip(length,data) ) return 0;
	if ( length >= 4 ) {
	    char compression_flag = *data++;
	    char compression_method = *data++;
	    if ( compression_flag == compression_method ) {
		// fool the compiler into thinking they're used
	    }
	    const char* lang = (const char*)data;
	    if ( !skip(length,data) ) return 0;
	    // const char* keyword_utf8 = (const char*)data;
	    if ( !skip(length,data) ) return 0;
	    const char* text_utf8 = (const char*)data;
	    if ( !skip(length,data) ) return 0;
	    QString text = QString::fromUtf8(text_utf8);
	    image->setText(keyword,lang[0] ? lang : 0,text);
	    return 1;
	}
    }
    */

    return 0;
}
} // namespace
#endif
