/*  This file is part of the KDE libraries
    Copyright (C) 2003 Fredrik Höglund <fredrik@kde.org>
    Copyright (C) 2005 Lubos Lunak <l.lunak@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include <config.h>

#include <assert.h>

#include "xcursor.h"

#include <qimage.h>
#include <kdebug.h>
#include <qvaluevector.h>

#ifdef GV_HAVE_XCURSOR
#include <X11/Xlib.h>
#include <X11/Xcursor/Xcursor.h>
#include <fixx11h.h>
#endif

namespace Gwenview {

#ifdef GV_HAVE_XCURSOR
class XCursorFormat : public QImageFormat {
public:
    XCursorFormat();
    virtual int decode(QImage& img, QImageConsumer* consumer,
	    const uchar* buffer, int length);
    QByteArray array;
    int pos;
    bool was_seek_error;
};

extern "C"
int xcursor_read( XcursorFile* file, unsigned char* buf, int len )
{
    XCursorFormat* data = reinterpret_cast< XCursorFormat* >( file->closure );
    if( int( data->array.size()) - data->pos < len )
    {
        data->was_seek_error = true;
        len = data->array.size() - data->pos;
    }
    memcpy( buf, data->array.data() + data->pos, len );
    data->pos += len;
    return len;
}

extern "C"
int xcursor_write( XcursorFile*, unsigned char*, int )
{
    assert( false ); // not write support
    return 0;
}

extern "C"
int xcursor_seek( XcursorFile* file, long offset, int whence )
{
    XCursorFormat* data = reinterpret_cast< XCursorFormat* >( file->closure );
    if( whence == SEEK_CUR )
        offset += data->pos;
    else if( whence == SEEK_END )
        offset = data->array.size() + offset;
    if( offset < 0 || offset >= int( data->array.size()))
    {
        data->was_seek_error = true;
        return -1;
    }
    data->pos = offset;
    return 0;
}

XCursorFormat::XCursorFormat()
{
}

int XCursorFormat::decode(QImage& img, QImageConsumer* cons,
	const uchar* buffer, int length)
{
    if( length > 0 )
    {
        int old_size = array.size();
        array.resize( old_size + length );
        memcpy( array.data() + old_size, buffer, length );
    }
// Qt's image async IO and Xcursor library have so nicely incompatible data reading :-/
// Xcursor can read a file only as a whole. One can provide XcursorFile with functions
// for reading, writing and seeking. And seeking is the stupid part on Xcursor's side,
// as there's no way to suspend reading until more data is available. This means we
// basically have to read the whole file first. However, Qt's image async IO just keeps
// feeding data and when there's end of the data it just does nothing and doesn't tell
// the decoder.
// So the trick will be calling XcursorXcFileLoadImage() but whenever there will be
// a seeking/reading error, then the function will fail and we'll try again when there's
// more data. Also, reading everything first means we can't return without processing
// further data after reaching end of a frame in the input as decode() doc says, but oh well.
    XcursorFile file;
    file.closure = this;
    file.read = xcursor_read;
    file.write = xcursor_write;
    file.seek = xcursor_seek;
    pos = 0;
    was_seek_error = false;
    XcursorImages *cursors = XcursorXcFileLoadImages( &file, 1024 ); // largest possible cursor size
    if ( cursors )
    {
        for( int cur = 0;
             cur < cursors->nimage;
             ++cur )
        {
            XcursorImage* cursor = cursors->images[ cur ];
            img = QImage( reinterpret_cast<uchar *>( cursor->pixels ),
                cursor->width, cursor->height, 32, NULL, 0, QImage::BigEndian );
            img.setAlphaBuffer( true );
            // Convert the image to non-premultiplied alpha
            Q_UINT32 *pixels = reinterpret_cast<Q_UINT32 *>( img.bits() );
            for ( int i = 0; i < (img.width() * img.height()); i++ )
            {
                float alpha = qAlpha( pixels[i] ) / 255.0;
                if ( alpha > 0.0 && alpha < 1.0 )
                    pixels[i] = qRgba( int( qRed(pixels[i]) / alpha ),
                                       int( qGreen(pixels[i]) / alpha ),
                                       int( qBlue(pixels[i]) / alpha ),
                                       qAlpha(pixels[i]) );
	    }
            // Create a deep copy of the image so the image data is preserved
            img = img.copy();
            if( cons )
            {
                if( cur == 0 )
                {
                    cons->setSize( img.width(), img.height());
                    if( cursors->nimage > 1 )
                        cons->setLooping( 0 );
                }
                cons->changed( QRect( QPoint( 0, 0 ), img.size()));
                cons->frameDone();
                cons->setFramePeriod( cursor->delay );
            }
        }
        XcursorImagesDestroy( cursors );
        if( cons )
            cons->end();
        return length;
    }
    else if( was_seek_error ) // try again next time
        return length;
    return -1; // failure
}
#endif

QImageFormat* XCursorFormatType::decoderFor(
    const uchar* buffer, int length)
{
#ifdef GV_HAVE_XCURSOR
    if (length < 4) return 0;
    if (buffer[0]=='X'
     && buffer[1]=='c'
     && buffer[2]=='u'
     && buffer[3]=='r')
	return new XCursorFormat;
#else
    Q_UNUSED( buffer );
    Q_UNUSED( length );
#endif
    return 0;
}

const char* XCursorFormatType::formatName() const
{
    return "XCursor";
}


} // namespace
