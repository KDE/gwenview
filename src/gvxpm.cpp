/* This file is based on qt-3.3.2/src/qimage.cpp , plus it includes
 * a fix from qt-bugs@ issue #49934. Original copyright follows.
 */
/****************************************************************************
** 
**
** Implementation of QImage and QImageIO classes
**
** Created : 950207
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

/*

 This code is xpm loader copied from qimage.cpp, with changes making it
 possible to use this class from another thread. The problem is that
 QColor::setNamedColor() isn't reentrant without thread-safe Xlib,
 i.e. without XInitThreads() called. Current KDE applications
 don't call XInitThreads(), and since it needs to be the very first
 Xlib function called, and Gwenview is also a KPart, it's not possible
 to rely on Xlib being thread-safe.
 
 Changes are marked using #ifdef GV_XPM_CHANGES.

*/

#define GV_XPM_CHANGES

#include "gvxpm.h"

#include <qimage.h>
#include <qmap.h>
#include <qregexp.h>

#include "gvthreadgate.h"


// Qt code start ---------------------------
static QString fbname( const QString &fileName ) // get file basename (sort of)
{
    QString s = fileName;
    if ( !s.isEmpty() ) {
	int i;
	if ( (i = s.findRev('/')) >= 0 )
	    s = s.mid( i );
	if ( (i = s.findRev('\\')) >= 0 )
	    s = s.mid( i );
	QRegExp r( QString::fromLatin1("[a-zA-Z][a-zA-Z0-9_]*") );
	int p = r.search( s );
	if ( p == -1 )
	    s.truncate( 0 );
	else
	    s = s.mid( p, r.matchedLength() );
    }
    if ( s.isEmpty() )
	s = QString::fromLatin1( "dummy" );
    return s;
}

/*****************************************************************************
  XPM image read/write functions
 *****************************************************************************/


// Skip until ", read until the next ", return the rest in *buf
// Returns FALSE on error, TRUE on success

static bool read_xpm_string( QCString &buf, QIODevice *d,
			     const char * const *source, int &index )
{
    if ( source ) {
	buf = source[index++];
	return TRUE;
    }

    if ( buf.size() < 69 )	    //# just an approximation
	buf.resize( 123 );

    buf[0] = '\0';
    int c;
    int i;
    while ( (c=d->getch()) != EOF && c != '"' ) { }
    if ( c == EOF ) {
	return FALSE;
    }
    i = 0;
    while ( (c=d->getch()) != EOF && c != '"' ) {
	if ( i == (int)buf.size() )
	    buf.resize( i*2+42 );
	buf[i++] = c;
    }
    if ( c == EOF ) {
	return FALSE;
    }

    if ( i == (int)buf.size() ) // always use a 0 terminator
	buf.resize( i+1 );
    buf[i] = '\0';
    return TRUE;
}



static int nextColorSpec(const QCString & buf)
{
    int i = buf.find(" c ");
    if (i < 0)
        i = buf.find(" g ");
    if (i < 0)
        i = buf.find(" g4 ");
    if (i < 0)
        i = buf.find(" m ");
    if (i < 0)
        i = buf.find(" s ");
    return i;
}

//
// INTERNAL
//
// Reads an .xpm from either the QImageIO or from the QString *.
// One of the two HAS to be 0, the other one is used.
//

static void read_xpm_image_or_array( QImageIO * iio, const char * const * source,
				     QImage & image)
{
    QCString buf;
    QIODevice *d = 0;
    buf.resize( 200 );

    int i, cpp, ncols, w, h, index = 0;

    if ( iio ) {
	iio->setStatus( 1 );
	d = iio ? iio->ioDevice() : 0;
	d->readLine( buf.data(), buf.size() );	// "/* XPM */"
	QRegExp r( QString::fromLatin1("/\\*.XPM.\\*/") );
	if ( buf.find(r) == -1 )
	    return;					// bad magic
    } else if ( !source ) {
	return;
    }

    if ( !read_xpm_string( buf, d, source, index ) )
	return;

    if ( sscanf( buf, "%d %d %d %d", &w, &h, &ncols, &cpp ) < 4 )
	return;					// < 4 numbers parsed

    if ( cpp > 15 )
	return;

    if ( ncols > 256 ) {
	image.create( w, h, 32 );
    } else {
	image.create( w, h, 8, ncols );
    }

    if (image.isNull())
	return;

    QMap<QString, int> colorMap;
    int currentColor;

    for( currentColor=0; currentColor < ncols; ++currentColor ) {
	if ( !read_xpm_string( buf, d, source, index ) ) {
#if defined(QT_CHECK_RANGE)
	    qWarning( "QImage: XPM color specification missing");
#endif
	    return;
	}
	QString index;
	index = buf.left( cpp );
	buf = buf.mid( cpp ).simplifyWhiteSpace().lower();
	buf.prepend( " " );
	i = nextColorSpec(buf);
	if ( i < 0 ) {
#if defined(QT_CHECK_RANGE)
	    qWarning( "QImage: XPM color specification is missing: %s", buf.data());
#endif
	    return;	// no c/g/g4/m/s specification at all
	}
	buf = buf.mid( i+3 );
	// Strip any other colorspec
	int end = nextColorSpec(buf);
	if (end != -1)
	    buf.truncate(end);
	buf = buf.stripWhiteSpace();
	if ( buf == "none" ) {
	    image.setAlphaBuffer( TRUE );
	    int transparentColor = currentColor;
	    if ( image.depth() == 8 ) {
		image.setColor( transparentColor,
				RGB_MASK & qRgb(198,198,198) );
		colorMap.insert( index, transparentColor );
	    } else {
		QRgb rgb = RGB_MASK & qRgb(198,198,198);
		colorMap.insert( index, rgb );
	    }
	} else {
	    if ( ((buf.length()-1) % 3) && (buf[0] == '#') ) {
		buf.truncate (((buf.length()-1) / 4 * 3) + 1); // remove alpha channel left by imagemagick
	    }
#ifdef GV_XPM_CHANGES
	    QColor c = GVThreadGate::instance()->color( buf.data());
#else
	    QColor c( buf.data() );
#endif
	    if ( image.depth() == 8 ) {
		image.setColor( currentColor, 0xff000000 | c.rgb() );
		colorMap.insert( index, currentColor );
	    } else {
		QRgb rgb = 0xff000000 | c.rgb();
		colorMap.insert( index, rgb );
	    }
	}
    }

    // Read pixels
    for( int y=0; y<h; y++ ) {
	if ( !read_xpm_string( buf, d, source, index ) ) {
#if defined(QT_CHECK_RANGE)
	    qWarning( "QImage: XPM pixels missing on image line %d", y);
#endif
	    return;
	}
	if ( image.depth() == 8 ) {
	    uchar *p = image.scanLine(y);
	    uchar *d = (uchar *)buf.data();
	    uchar *end = d + buf.length();
	    int x;
	    if ( cpp == 1 ) {
		char b[2];
		b[1] = '\0';
		for ( x=0; x<w && d<end; x++ ) {
		    b[0] = *d++;
		    *p++ = (uchar)colorMap[b];
		}
	    } else {
		char b[16];
		b[cpp] = '\0';
		for ( x=0; x<w && d<end; x++ ) {
		    strncpy( b, (char *)d, cpp );
		    *p++ = (uchar)colorMap[b];
		    d += cpp;
		}
	    }
	} else {
	    QRgb *p = (QRgb*)image.scanLine(y);
	    uchar *d = (uchar *)buf.data();
	    uchar *end = d + buf.length();
	    int x;
	    char b[16];
	    b[cpp] = '\0';
	    for ( x=0; x<w && d<end; x++ ) {
		strncpy( b, (char *)d, cpp );
		*p++ = (QRgb)colorMap[b];
		d += cpp;
	    }
	}
    }
    if ( iio ) {
	iio->setImage( image );
	iio->setStatus( 0 );			// image ok
    }
}


static void read_xpm_image( QImageIO * iio )
{
    QImage i;
    (void)read_xpm_image_or_array( iio, 0, i );
    return;
}


static const char* xpm_color_name( int cpp, int index )
{
    static char returnable[5];
    static const char code[] = ".#abcdefghijklmnopqrstuvwxyzABCD"
			       "EFGHIJKLMNOPQRSTUVWXYZ0123456789";
    // cpp is limited to 4 and index is limited to 64^cpp
    if ( cpp > 1 ) {
	if ( cpp > 2 ) {
	    if ( cpp > 3 ) {
		returnable[3] = code[index % 64];
		index /= 64;
	    } else
		returnable[3] = '\0';
	    returnable[2] = code[index % 64];
	    index /= 64;
	} else
	    returnable[2] = '\0';
	// the following 4 lines are a joke!
	if ( index == 0 )
	    index = 64*44+21;
	else if ( index == 64*44+21 )
	    index = 0;
	returnable[1] = code[index % 64];
	index /= 64;
    } else
	returnable[1] = '\0';
    returnable[0] = code[index];

    return returnable;
}


// write XPM image data
static void write_xpm_image( QImageIO * iio )
{
    if ( iio )
	iio->setStatus( 1 );
    else
	return;

    // ### 8-bit case could be made faster
    QImage image;
    if ( iio->image().depth() != 32 )
	image = iio->image().convertDepth( 32 );
    else
	image = iio->image();

    QMap<QRgb, int> colorMap;

    int w = image.width(), h = image.height(), ncolors = 0;
    int x, y;

    // build color table
    for( y=0; y<h; y++ ) {
	QRgb * yp = (QRgb *)image.scanLine( y );
	for( x=0; x<w; x++ ) {
	    QRgb color = *(yp + x);
	    if ( !colorMap.contains(color) )
		colorMap.insert( color, ncolors++ );
	}
    }

    // number of 64-bit characters per pixel needed to encode all colors
    int cpp = 1;
    for ( int k = 64; ncolors > k; k *= 64 ) {
	++cpp;
	// limit to 4 characters per pixel
	// 64^4 colors is enough for a 4096x4096 image
	 if ( cpp > 4)
	    break;
    }

    QString line;

    // write header
    QTextStream s( iio->ioDevice() );
    s << "/* XPM */" << endl
      << "static char *" << fbname(iio->fileName()) << "[]={" << endl
      << "\"" << w << " " << h << " " << ncolors << " " << cpp << "\"";

    // write palette
    QMap<QRgb, int>::Iterator c = colorMap.begin();
    while ( c != colorMap.end() ) {
	QRgb color = c.key();
	if ( image.hasAlphaBuffer() && color == (color & RGB_MASK) )
	    line.sprintf( "\"%s c None\"",
			  xpm_color_name(cpp, *c) );
	else
	    line.sprintf( "\"%s c #%02x%02x%02x\"",
			  xpm_color_name(cpp, *c),
			  qRed(color),
			  qGreen(color),
			  qBlue(color) );
	++c;
	s << "," << endl << line;
    }

    // write pixels, limit to 4 characters per pixel
    line.truncate( cpp*w );
    for( y=0; y<h; y++ ) {
	QRgb * yp = (QRgb *) image.scanLine( y );
	int cc = 0;
	for( x=0; x<w; x++ ) {
	    int color = (int)(*(yp + x));
	    QCString chars = xpm_color_name( cpp, colorMap[color] );
	    line[cc++] = chars[0];
	    if ( cpp > 1 ) {
		line[cc++] = chars[1];
		if ( cpp > 2 ) {
		    line[cc++] = chars[2];
		    if ( cpp > 3 ) {
			line[cc++] = chars[3];
		    }
		}
	    }
	}
	s << "," << endl << "\"" << line << "\"";
    }
    s << "};" << endl;

    iio->setStatus( 0 );
}

// Qt code end ---------------------------

GVXPM::GVXPM()
{
        QImageIO::inputFormats(); // trigger registration of Qt's handlers
	QImageIO::defineIOHandler( "XPM", "/\\*.XPM.\\*/", "T",
				   read_xpm_image, write_xpm_image );
}
