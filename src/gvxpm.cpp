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
	i = buf.find( " c " );
	if ( i < 0 )
	    i = buf.find( " g " );
	if ( i < 0 )
	    i = buf.find( " g4 " );
	if ( i < 0 )
	    i = buf.find( " m " );
	if ( i < 0 )
	    i = buf.find( " s " );
	if ( i < 0 ) {
#if defined(QT_CHECK_RANGE)
	    qWarning( "QImage: XPM color specification is missing: %s", buf.data());
#endif
	    return;	// no c/g/g4/m specification at all
	}
	buf = buf.mid( i+3 );
	// Strip any other colorspec
	int endtmp;
	int end = buf.length();
	endtmp = buf.find(" c ");
	if( endtmp > 0 && endtmp < end )
	    end = endtmp;
	endtmp = buf.find(" g ");
	if( endtmp > 0 && endtmp < end )
	    end = endtmp;
	endtmp = buf.find(" g4 ");
	if( endtmp > 0 && endtmp < end )
	    end = endtmp;
	endtmp = buf.find(" m ");
	if( endtmp > 0 && endtmp < end )
	    end = endtmp;
	endtmp = buf.find(" s ");
	if( endtmp > 0 && endtmp < end )
	    end = endtmp;
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

// Qt code end ---------------------------

GVXPM::GVXPM()
{
        QImageIO::inputFormats(); // trigger registration of Qt's handlers
	QImageIO::defineIOHandler( "XPM", "/\\*.XPM.\\*/", "T",
				   read_xpm_image, NULL );
}
