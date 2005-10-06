/*

 Copyright (C) 2005 Lubos Lunak <l.lunak@kde.org>

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

#include "croppedqimage.h"

namespace ImageUtils
{

// This class is used in ImageView::performPaint(). Just using QImage::copy( QRect )
// takes a significant time with very large images. So instead of copying the image data
// just create CroppedQImage which fakes a subimage by manipulating its scanline pointers.
// That will of course break if something doesn't use scanlines but accesses the image
// data directly, QImage::copy() being the most notable case. There are two ways
// to handle that: 1) It is possible to manually call normalize() which will make
// CroppedQImage copy the image data and own it, just like proper QImage. 2) CroppedQImage
// has as a data member also QImage holding the original image. This ensures that all
// the original image data are still available for the whole lifetime of CroppedQImage.

CroppedQImage::CroppedQImage( const QImage& im, const QRect& rect )
    : QImage( rect.size(), im.depth(), im.numColors(), im.bitOrder())
    , orig( im )
    {
    if( im.isNull())
        return;
    memcpy( colorTable(), im.colorTable(), im.numColors() * sizeof( QRgb ));
    setAlphaBuffer( im.hasAlphaBuffer());
    setDotsPerMeterX( im.dotsPerMeterX());
    setDotsPerMeterY( im.dotsPerMeterY());
    //data->offset = im.offset();
    // make scanlines point to right places in the original QImage
    for( int i = 0;
         i < height();
         ++i )
        jumpTable()[ i ] = im.scanLine( rect.y() + i ) + rect.x() * ( depth() / 8 );
    }

CroppedQImage& CroppedQImage::operator= ( const QImage& im )
    {
    QImage::operator=( im );
    return *this;
    }

void CroppedQImage::normalize()
    {
    // is it a normal QImage with its own data?
    uchar* firstdata = ( uchar* )( jumpTable() + height());
    if( scanLine( 0 ) == firstdata )
        return;
    // copy the image data to our own data and make scanlines point properly there
    for( int i = 0;
         i < height();
         ++i )
        {
        uchar* oldline = scanLine( i );
        jumpTable()[ i ] = firstdata + i * bytesPerLine();
        memcpy( scanLine( i ), oldline, bytesPerLine());
        }
    }

} // namespace
