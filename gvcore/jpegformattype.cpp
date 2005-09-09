/* This file is based on kdelibs-3.2.0/khtml/misc/loader_jpeg.cpp. Original
 * copyright follows.
 */
/*
    This file is part of the KDE libraries
 
    Copyright (C) 2000 Dirk Mueller (mueller@kde.org)
 
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
 
    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.
 
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
    AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
    AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
*/


// System
#include <stdio.h>
#include <setjmp.h>
extern "C" {
#define XMD_H
#include <jpeglib.h>
#undef const
}

// Qt
#include <qdatetime.h>

// KDE
#include <kdebug.h>
#include <kglobal.h>

// Local
#include "jpegformattype.h"
#include "imageutils/jpegerrormanager.h"

namespace Gwenview {

#undef BUFFER_DEBUG
//#define BUFFER_DEBUG

#undef JPEG_DEBUG
//#define JPEG_DEBUG


static const int MAX_BUFFER = 32768;
// how long it will consume data before starting outputing progressive scan
static const int MAX_CONSUMING_TIME = 500;

//-----------------------------------------------------------------------------
//
// JPEGSourceManager
// (Does not follow HACKING file recommandation to be consistent with
// jpeg_source_mgr naming)
//
//-----------------------------------------------------------------------------
struct JPEGSourceManager : public jpeg_source_mgr {
	JOCTET jpeg_buffer[MAX_BUFFER];

	int valid_buffer_length;
	size_t skip_input_bytes;
	bool at_eof;
	QRect change_rect;
	QRect old_change_rect;
	QTime decoder_timestamp;
	bool final_pass;
	bool decoding_done;
	bool do_progressive;

	JPEGSourceManager() {
		// jpeg_source_mgr fields
		init_source = gvJPEGDummyDecompress;
		fill_input_buffer = gvFillInputBuffer;
		skip_input_data = gvSkipInputData;
		resync_to_restart = jpeg_resync_to_restart;
		term_source = gvJPEGDummyDecompress;
		
		bytes_in_buffer = 0;
		next_input_byte = jpeg_buffer;
		
		// JPEGSourceManager fields
		valid_buffer_length = 0;
		skip_input_bytes = 0;
		at_eof = 0;
		final_pass = false;
		decoding_done = false;
	}
	
	static void gvJPEGDummyDecompress(j_decompress_ptr) {}

	/* Do not replace boolean with bool, it's the libjpeg boolean type */
	static boolean gvFillInputBuffer(j_decompress_ptr cinfo) {
#ifdef BUFFER_DEBUG
		qDebug("FillInputBuffer called!");
#endif

		JPEGSourceManager* src = (JPEGSourceManager*)cinfo->src;

		if ( src->at_eof ) {
			/* Insert a fake EOI marker - as per jpeglib recommendation */
			src->jpeg_buffer[0] = (JOCTET) 0xFF;
			src->jpeg_buffer[1] = (JOCTET) JPEG_EOI;
			src->bytes_in_buffer = 2;
			src->next_input_byte = (JOCTET *) src->jpeg_buffer;
#ifdef BUFFER_DEBUG
			qDebug("...returning true!");
#endif
			return true;
		} else {
			return false;  /* I/O suspension mode */
		}
	}

	static void gvSkipInputData(j_decompress_ptr cinfo, long num_bytes) {
		if(num_bytes <= 0) return; /* required noop */

#ifdef BUFFER_DEBUG
		qDebug("SkipInputData (%d) called!", num_bytes);
#endif

		JPEGSourceManager* src = (JPEGSourceManager*)cinfo->src;
		src->skip_input_bytes += num_bytes;

		unsigned int skipbytes = kMin(src->bytes_in_buffer, src->skip_input_bytes);

#ifdef BUFFER_DEBUG
		qDebug("skip_input_bytes is now %d", src->skip_input_bytes);
		qDebug("skipbytes is now %d", skipbytes);
		qDebug("valid_buffer_length is before %d", src->valid_buffer_length);
		qDebug("bytes_in_buffer is %d", src->bytes_in_buffer);
#endif

		if(skipbytes < src->bytes_in_buffer) {
			memmove(src->jpeg_buffer,
				src->next_input_byte+skipbytes,
				src->bytes_in_buffer - skipbytes);
		}

		src->bytes_in_buffer -= skipbytes;
		src->valid_buffer_length = src->bytes_in_buffer;
		src->skip_input_bytes -= skipbytes;

		/* adjust data for jpeglib */
		cinfo->src->next_input_byte = (JOCTET *) src->jpeg_buffer;
		cinfo->src->bytes_in_buffer = (size_t) src->valid_buffer_length;
#ifdef BUFFER_DEBUG
		qDebug("valid_buffer_length is afterwards %d", src->valid_buffer_length);
		qDebug("skip_input_bytes is now %d", src->skip_input_bytes);
#endif

	}
};


//-----------------------------------------------------------------------------
//
// JPEGFormat
//
//-----------------------------------------------------------------------------
class JPEGFormat : public QImageFormat {
public:
	JPEGFormat();

	virtual ~JPEGFormat();

	virtual int decode(QImage& img, QImageConsumer* consumer,
		const uchar* buffer, int length);
private:

	enum {
		INIT,
		START_DECOMPRESS,
		DECOMPRESS_STARTED,
		CONSUME_INPUT,
		PREPARE_OUTPUT_SCAN,
		DO_OUTPUT_SCAN,
		READ_DONE,
		INVALID
	} mState;

	// structs for the jpeglib
	jpeg_decompress_struct mDecompress;
	ImageUtils::JPEGErrorManager mErrorManager;
	JPEGSourceManager mSourceManager;
};


JPEGFormat::JPEGFormat() {
	memset(&mDecompress, 0, sizeof(mDecompress));
	mDecompress.err = &mErrorManager;
	jpeg_create_decompress(&mDecompress);
	mDecompress.src = &mSourceManager;
	mState = INIT;
}


JPEGFormat::~JPEGFormat() {
	(void) jpeg_destroy_decompress(&mDecompress);
}

/*
 * return  > 0 means "consumed x bytes, need more"
 * return == 0 means "end of frame reached"
 * return  < 0 means "fatal error in image decoding, don't call me ever again"
 */

int JPEGFormat::decode(QImage& image, QImageConsumer* consumer, const uchar* buffer, int length) {
#ifdef JPEG_DEBUG
	qDebug("JPEGFormat::decode(%08lx, %08lx, %08lx, %d)",
	       &image, consumer, buffer, length);
#endif

	if(mSourceManager.at_eof) {
#ifdef JPEG_DEBUG
		qDebug("at_eof, eating");
#endif
		return length;
	}

	if(setjmp(mErrorManager.jmp_buffer)) {
#ifdef JPEG_DEBUG
		qDebug("jump into state INVALID");
#endif
		if(consumer) consumer->end();

		// this is fatal
		return -1;
	}

	int consumed = kMin(length, MAX_BUFFER - mSourceManager.valid_buffer_length);

#ifdef BUFFER_DEBUG
	qDebug("consuming %d bytes", consumed);
#endif

	// filling buffer with the new data
	memcpy(mSourceManager.jpeg_buffer + mSourceManager.valid_buffer_length, buffer, consumed);
	mSourceManager.valid_buffer_length += consumed;

	if(mSourceManager.skip_input_bytes) {
#ifdef BUFFER_DEBUG
		qDebug("doing skipping");
		qDebug("valid_buffer_length %d", mSourceManager.valid_buffer_length);
		qDebug("skip_input_bytes %d", mSourceManager.skip_input_bytes);
#endif
		int skipbytes = kMin((size_t) mSourceManager.valid_buffer_length, mSourceManager.skip_input_bytes);

		if(skipbytes < mSourceManager.valid_buffer_length) {
			memmove(mSourceManager.jpeg_buffer,
				mSourceManager.jpeg_buffer+skipbytes,
				mSourceManager.valid_buffer_length - skipbytes);
		}

		mSourceManager.valid_buffer_length -= skipbytes;
		mSourceManager.skip_input_bytes -= skipbytes;

		// still more bytes to skip
		if(mSourceManager.skip_input_bytes) {
			if(consumed <= 0) qDebug("ERROR!!!");
			return consumed;
		}

	}

	mDecompress.src->next_input_byte = (JOCTET *) mSourceManager.jpeg_buffer;
	mDecompress.src->bytes_in_buffer = (size_t) mSourceManager.valid_buffer_length;

#ifdef BUFFER_DEBUG
	qDebug("buffer contains %d bytes", mSourceManager.valid_buffer_length);
#endif

	if(mState == INIT) {
		if(jpeg_read_header(&mDecompress, true) != JPEG_SUSPENDED) {
			if (consumer) {
				consumer->setSize(
					mDecompress.image_width/mDecompress.scale_denom,
					mDecompress.image_height/mDecompress.scale_denom);
			}

			mState = START_DECOMPRESS;
		}
	}

	if(mState == START_DECOMPRESS) {
		mSourceManager.do_progressive = jpeg_has_multiple_scans( &mDecompress );

#ifdef JPEG_DEBUG
		qDebug( "**** DOPROGRESSIVE: %d",  mSourceManager.do_progressive );
#endif
		mDecompress.buffered_image = mSourceManager.do_progressive;

		// setup image sizes
		jpeg_calc_output_dimensions( &mDecompress );

		if (mDecompress.jpeg_color_space == JCS_YCbCr) {
			mDecompress.out_color_space = JCS_RGB;
		}

		mDecompress.do_fancy_upsampling = true;
		mDecompress.do_block_smoothing = false;
		mDecompress.quantize_colors = false;

		// false: IO suspension
		if(jpeg_start_decompress(&mDecompress)) {
			if ( mDecompress.output_components == 3 || mDecompress.output_components == 4) {
				image.create( mDecompress.output_width, mDecompress.output_height, 32 );
			} else if ( mDecompress.output_components == 1 ) {
				image.create( mDecompress.output_width, mDecompress.output_height, 8, 256 );
				for (int i=0; i<256; i++) {
					image.setColor(i, qRgb(i,i,i));
				}
			}

#ifdef JPEG_DEBUG
			qDebug("will create a picture %d/%d in size", mDecompress.output_width, mDecompress.output_height);
#endif

#ifdef JPEG_DEBUG
			qDebug("ok, going to DECOMPRESS_STARTED");
#endif

			mSourceManager.decoder_timestamp.start();
			mState = mSourceManager.do_progressive ? DECOMPRESS_STARTED : DO_OUTPUT_SCAN;
		}
	}

again:

	if(mState == DECOMPRESS_STARTED) {
		mState =  (!mSourceManager.final_pass && mSourceManager.decoder_timestamp.elapsed() < MAX_CONSUMING_TIME)
			 ? CONSUME_INPUT : PREPARE_OUTPUT_SCAN;
	}

	if(mState == CONSUME_INPUT) {
		int retval;

		do {
			retval = jpeg_consume_input(&mDecompress);
		} while (retval != JPEG_SUSPENDED && retval != JPEG_REACHED_EOI
			&& (retval != JPEG_REACHED_SOS || mSourceManager.decoder_timestamp.elapsed() < MAX_CONSUMING_TIME));

		if( mSourceManager.final_pass
			|| mSourceManager.decoder_timestamp.elapsed() >= MAX_CONSUMING_TIME
			|| retval == JPEG_REACHED_EOI
			|| retval == JPEG_REACHED_SOS) {
			mState = PREPARE_OUTPUT_SCAN;
		}
	}

	if(mState == PREPARE_OUTPUT_SCAN) {
		if ( jpeg_start_output(&mDecompress, mDecompress.input_scan_number) ) {
			mState = DO_OUTPUT_SCAN;
		}
	}

	if(mState == DO_OUTPUT_SCAN) {
		if(image.isNull() || mSourceManager.decoding_done) {
#ifdef JPEG_DEBUG
			qDebug("complete in doOutputscan, eating..");
#endif
			return consumed;
		}
		uchar** lines = image.jumpTable();
		int oldoutput_scanline = mDecompress.output_scanline;

		while(mDecompress.output_scanline < mDecompress.output_height &&
			jpeg_read_scanlines(&mDecompress, lines+mDecompress.output_scanline, mDecompress.output_height))
			; // here happens all the magic of decoding

		int completed_scanlines = mDecompress.output_scanline - oldoutput_scanline;
#ifdef JPEG_DEBUG
		qDebug("completed now %d scanlines", completed_scanlines);
#endif

		if ( mDecompress.output_components == 3 ) {
			// Expand 24->32 bpp.
			for (int j=oldoutput_scanline; j<oldoutput_scanline+completed_scanlines; j++) {
				uchar *in = image.scanLine(j) + mDecompress.output_width * 3;
				QRgb *out = (QRgb*)image.scanLine(j);

				for (uint i=mDecompress.output_width; i--; ) {
					in-=3;
					out[i] = qRgb(in[0], in[1], in[2]);
				}
			}
		}

		if(consumer && completed_scanlines) {
			QRect r(0, oldoutput_scanline, mDecompress.output_width, completed_scanlines);
#ifdef JPEG_DEBUG
			qDebug("changing %d/%d %d/%d", r.x(), r.y(), r.width(), r.height());
#endif
			mSourceManager.change_rect |= r;

			if ( mSourceManager.decoder_timestamp.elapsed() >= MAX_CONSUMING_TIME ) {
				if( !mSourceManager.old_change_rect.isEmpty()) {
					consumer->changed(mSourceManager.old_change_rect);
					mSourceManager.old_change_rect = QRect();
				}
				consumer->changed(mSourceManager.change_rect);
				mSourceManager.change_rect = QRect();
				mSourceManager.decoder_timestamp.restart();
			}
		}

		if(mDecompress.output_scanline >= mDecompress.output_height) {
			if ( mSourceManager.do_progressive ) {
				jpeg_finish_output(&mDecompress);
				mSourceManager.final_pass = jpeg_input_complete(&mDecompress);
				mSourceManager.decoding_done = mSourceManager.final_pass && mDecompress.input_scan_number == mDecompress.output_scan_number;
				if ( !mSourceManager.decoding_done ) {
					mSourceManager.old_change_rect |= mSourceManager.change_rect;
					mSourceManager.change_rect =  QRect();
				}
			} else {
				mSourceManager.decoding_done = true;
			}
#ifdef JPEG_DEBUG
			qDebug("one pass is completed, final_pass = %d, dec_done: %d, complete: %d",
				mSourceManager.final_pass, mSourceManager.decoding_done, jpeg_input_complete(&mDecompress));
#endif
			if(!mSourceManager.decoding_done) {
#ifdef JPEG_DEBUG
				qDebug("starting another one, input_scan_number is %d/%d", mDecompress.input_scan_number,
					mDecompress.output_scan_number);
#endif
				mSourceManager.decoder_timestamp.restart();
				mState = DECOMPRESS_STARTED;
				// don't return until necessary!
				goto again;
			}
		}

		if(mState == DO_OUTPUT_SCAN && mSourceManager.decoding_done) {
#ifdef JPEG_DEBUG
			qDebug("input is complete, cleaning up, returning..");
#endif
			if ( consumer && !mSourceManager.change_rect.isEmpty() ) {
				consumer->changed( mSourceManager.change_rect );
			}

			if(consumer) consumer->end();

			mSourceManager.at_eof = true;

			(void) jpeg_finish_decompress(&mDecompress);
			(void) jpeg_destroy_decompress(&mDecompress);

			mState = READ_DONE;

			return 0;
		}
	}

#ifdef BUFFER_DEBUG
	qDebug("valid_buffer_length is now %d", mSourceManager.valid_buffer_length);
	qDebug("bytes_in_buffer is now %d", mSourceManager.bytes_in_buffer);
	qDebug("consumed %d bytes", consumed);
#endif
	if(mSourceManager.bytes_in_buffer
		&& mSourceManager.jpeg_buffer != mSourceManager.next_input_byte) {
		memmove(mSourceManager.jpeg_buffer,
			mSourceManager.next_input_byte,
			mSourceManager.bytes_in_buffer);
	}
	mSourceManager.valid_buffer_length = mSourceManager.bytes_in_buffer;
	return consumed;
}


//-----------------------------------------------------------------------------
//
// JPEGFormatType
//
//-----------------------------------------------------------------------------
QImageFormat* JPEGFormatType::decoderFor(const uchar* buffer, int length) {
	if(length < 3) return 0;

	if(buffer[0] == 0377 &&
	   buffer[1] == 0330 &&
	   buffer[2] == 0377) {
		return new JPEGFormat;
	}

	return 0;
}

const char* JPEGFormatType::formatName() const {
	return "JPEG";
}

} // namespace
