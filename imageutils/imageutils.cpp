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
#include <math.h>

// Qt
#include <qimage.h>
#include <qwmatrix.h>

// KDE
#include <kdebug.h>
#include <kdeversion.h>
#include <kglobal.h>

// Local
#include "imageutils/orientation.h"

namespace ImageUtils {

QImage transform(const QImage& img, Orientation orientation) {
	QWMatrix matrix;
	switch (orientation) {
	case NOT_AVAILABLE:
	case NORMAL:
		return img;

	case HFLIP:
		matrix.scale(-1,1);
		break;

	case ROT_180:
		matrix.rotate(180);
		break;

	case VFLIP:
		matrix.scale(1,-1);
		break;
	
	case TRANSPOSE:
		matrix.scale(-1,1);
		matrix.rotate(90);
		break;
		
	case ROT_90:		
		matrix.rotate(90);
		break;
	
	case TRANSVERSE:
		matrix.scale(1,-1);
		matrix.rotate(90);
		break;
		
	case ROT_270:		
		matrix.rotate(270);
		break;
	}

	return img.xForm(matrix);
}

inline
int changeBrightness( int value, int brightness )
    {
    return KCLAMP( value + brightness * 255 / 100, 0, 255 );
    }

inline
int changeContrast( int value, int contrast )
    {
    return KCLAMP((( value - 127 ) * contrast / 100 ) + 127, 0, 255 );
    }

inline
int changeGamma( int value, int gamma )
    {
    return KCLAMP( int( pow( value / 255.0, 100.0 / gamma ) * 255 ), 0, 255 );
    }

inline
int changeUsingTable( int value, const int table[] )
    {
    return table[ value ];
    }

/*
 Applies either brightness, contrast or gamma conversion on the image.
 If the image is not truecolor, the color table is changed. If it is
 truecolor, every pixel has to be changed. In order to make it as fast
 as possible, alpha value is converted only if necessary. Additionally,
 since color components (red/green/blue/alpha) can have only 256 values
 but images usually have many pixels, a conversion table is first
 created for every color component value, and pixels are converted
 using this table.
*/

template< int operation( int, int ) >
static
QImage changeImage( const QImage& image, int value )
    {
    QImage im = image;
    im.detach();
    if( im.numColors() == 0 ) /* truecolor */
        {
        if( im.depth() != 32 ) /* just in case */
            im = im.convertDepth( 32 );
        int table[ 256 ];
        for( int i = 0;
             i < 256;
             ++i )
            table[ i ] = operation( i, value );
        if( im.hasAlphaBuffer())
            {
            for( int y = 0;
                 y < im.height();
                 ++y )
                {
                QRgb* line = reinterpret_cast< QRgb* >( im.scanLine( y ));
                for( int x = 0;
                     x < im.width();
                     ++x )
                    line[ x ] = qRgba( changeUsingTable( qRed( line[ x ] ), table ),
                        changeUsingTable( qGreen( line[ x ] ), table ),
                        changeUsingTable( qBlue( line[ x ] ), table ),
                        changeUsingTable( qAlpha( line[ x ] ), table ));
                }
            }
        else
            {
            for( int y = 0;
                 y < im.height();
                 ++y )
                {
                QRgb* line = reinterpret_cast< QRgb* >( im.scanLine( y ));
                for( int x = 0;
                     x < im.width();
                     ++x )
                    line[ x ] = qRgb( changeUsingTable( qRed( line[ x ] ), table ),
                        changeUsingTable( qGreen( line[ x ] ), table ),
                        changeUsingTable( qBlue( line[ x ] ), table ));
                }
            }
        }
    else
        {
        QRgb* colors = im.colorTable();
        for( int i = 0;
             i < im.numColors();
             ++i )
            colors[ i ] = qRgb( operation( qRed( colors[ i ] ), value ),
                operation( qGreen( colors[ i ] ), value ),
                operation( qBlue( colors[ i ] ), value ));
        }
    return im;
    }


// brightness is multiplied by 100 in order to avoid floating point numbers
QImage changeBrightness( const QImage& image, int brightness )
    {
    if( brightness == 0 ) // no change
        return image;
    return changeImage< changeBrightness >( image, brightness );
    }


// contrast is multiplied by 100 in order to avoid floating point numbers
QImage changeContrast( const QImage& image, int contrast )
    {
    if( contrast == 100 ) // no change
        return image;
    return changeImage< changeContrast >( image, contrast );
    }

// gamma is multiplied by 100 in order to avoid floating point numbers
QImage changeGamma( const QImage& image, int gamma )
    {
    if( gamma == 100 ) // no change
        return image;
    return changeImage< changeGamma >( image, gamma );
    }

} // Namespace

