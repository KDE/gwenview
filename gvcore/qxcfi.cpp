/*
 * qxcfi.cpp: A Qt 3 plug-in for reading GIMP XCF image files
 * Copyright (C) 2001 lignum Computing, Inc. <allen@lignumcomputing.com>
 * $Id$
 *
 * This plug-in is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <qiodevice.h>
#include <kdeversion.h>
#include <stdlib.h>
#include "qxcfi.h"


// Change a QRgb value's alpha only. (an optimization)
inline QRgb qRgba ( QRgb rgb, int a )
{
  return ( ( a & 0xff ) << 24 | ( rgb & RGB_MASK ) );
}


namespace Gwenview {

int SafeDataStream::at() const {
  return mDevice->at();
}

const float INCHESPERMETER = (100. / 2.54);

// Static global values

int XCFImageFormat::random_table[RANDOM_TABLE_SIZE];

int XCFImageFormat::add_lut[256][256];

XCFImageFormat::LayerModes XCFImageFormat::layer_modes[] = {
  { true },			// NORMAL_MODE
  { true },			// DISSOLVE_MODE
  { true },			// BEHIND_MODE
  { false },			// MULTIPLY_MODE
  { false },			// SCREEN_MODE
  { false },			// OVERLAY_MODE
  { false },			// DIFFERENCE_MODE
  { false },			// ADDITION_MODE
  { false },			// SUBTRACT_MODE
  { false },			// DARKEN_ONLY_MODE
  { false },			// LIGHTEN_ONLY_MODE
  { false },			// HUE_MODE
  { false },			// SATURATION_MODE
  { false },			// COLOR_MODE
  { false },			// VALUE_MODE
  { false },			// DIVIDE_MODE
  { true },			// ERASE_MODE
  { true },			// REPLACE_MODE
  { true },			// ANTI_ERASE_MODE
};

//////////////////////////////////////////////////////////////////////////////////
// From GIMP "paint_funcs.c" v1.2

/*!
 * Multiply two color components. Really expects the arguments to be
 * 8-bit quantities.
 * \param a first minuend.
 * \param b second minuend.
 * \return product of arguments.
 */
inline int INT_MULT ( int a, int b )
{
  int c = a * b + 0x80;
  return ( ( c >> 8 ) + c ) >> 8;
}

/*!
 * Blend the two color components in the proportion alpha:
 *
 * result = alpha a + ( 1 - alpha b)
 *
 * \param a first component.
 * \param b second component.
 * \param alpha blend proportion.
 * \return blended color components.
 */

inline int INT_BLEND ( int a, int b, int alpha )
{
  return INT_MULT( a - b, alpha ) + b;
}

// Actually from GLIB

inline int MIN ( int a, int b )
{
  return ( a < b ? a : b );
}

inline int MAX ( int a, int b )
{
  return ( a > b ? a : b );
}

// From GIMP "gimpcolorspace.c" v1.2

/*!
 * Convert a color in RGB space to HSV space (Hue, Saturation, Value).
 * \param red the red component (modified in place).
 * \param green the green component (modified in place).
 * \param blue the blue component (modified in place).
 */
void RGBTOHSV ( uchar& red, uchar& green, uchar& blue )
{
  int r, g, b;
  double h, s, v;
  int min, max;

  h = 0.;

  r = red;
  g = green;
  b = blue;

  if ( r > g ) {
    max = MAX( r, b );
    min = MIN( g, b );
  }
  else {
    max = MAX( g, b );
    min = MIN( r, b );
  }

  v = max;

  if ( max != 0 )
    s = ( ( max - min ) * 255 ) / (double)max;
  else
    s = 0;

  if ( s == 0 )
    h = 0;
  else {
    int delta = max - min;
    if ( r == max )
      h = ( g - b ) / (double)delta;
    else if ( g == max )
      h = 2 + ( b - r ) / (double)delta;
    else if ( b == max )
      h = 4 + ( r - g ) / (double)delta;
    h *= 42.5;

    if ( h < 0 )
      h += 255;
    if ( h > 255 )
      h -= 255;
  }

  red   = (uchar)h;
  green = (uchar)s;
  blue  = (uchar)v;
}

/*!
 * Convert a color in HSV space to RGB space.
 * \param hue the hue component (modified in place).
 * \param saturation the saturation component (modified in place).
 * \param value the value component (modified in place).
 */
void HSVTORGB ( uchar& hue, uchar& saturation, uchar& value )
{
  if ( saturation == 0 ) {
    hue        = value;
    saturation = value;
    value      = value;
  }
  else {
    double h = hue * 6. / 255.;
    double s = saturation / 255.;
    double v = value / 255.;

    double f = h - (int)h;
    double p = v * ( 1. - s );
    double q = v * ( 1. - ( s * f ) );
    double t = v * ( 1. - ( s * ( 1. - f ) ) );

    // Worth a note here that gcc 2.96 will generate different results
    // depending on optimization mode on i386.

    switch ((int)h) {
    case 0:
      hue        = (uchar)( v * 255 );
      saturation = (uchar)( t * 255 );
      value      = (uchar)( p * 255 );
      break;
    case 1:
      hue        = (uchar)( q * 255 );
      saturation = (uchar)( v * 255 );
      value      = (uchar)( p * 255 );
      break;
    case 2:
      hue        = (uchar)( p * 255 );
      saturation = (uchar)( v * 255 );
      value      = (uchar)( t * 255 );
      break;
    case 3:
      hue        = (uchar)( p * 255 );
      saturation = (uchar)( q * 255 );
      value      = (uchar)( v * 255 );
      break;
    case 4:
      hue        = (uchar)( t * 255 );
      saturation = (uchar)( p * 255 );
      value      = (uchar)( v * 255 );
      break;
    case 5:
      hue        = (uchar)( v * 255 );
      saturation = (uchar)( p * 255 );
      value      = (uchar)( q * 255 );
    }
  }
}

/*!
 * Convert a color in RGB space to HLS space (Hue, Lightness, Saturation).
 * \param red the red component (modified in place).
 * \param green the green component (modified in place).
 * \param blue the blue component (modified in place).
 */
void RGBTOHLS ( uchar& red, uchar& green, uchar& blue )
{
  int r = red;
  int g = green;
  int b = blue;

  int min, max;

  if ( r > g ) {
    max = MAX( r, b );
    min = MIN( g, b );
  }
  else {
    max = MAX( g, b );
    min = MIN( r, b );
  }

  double h;
  double l = ( max + min ) / 2.;
  double s;

  if ( max == min ) {
    s = 0.;
    h = 0.;
  }
  else {
    int delta = max - min;

    if ( l < 128 )
      s = 255 * (double)delta / (double)( max + min );
    else
      s = 255 * (double)delta / (double)( 511 - max - min );

    if ( r == max )
      h = ( g - b ) / (double)delta;
    else if ( g == max )
      h = 2 + ( b - r ) / (double)delta;
    else
      h = 4 + ( r - g ) / (double)delta;

    h *= 42.5;

    if ( h < 0 )
      h += 255;
    else if ( h > 255 )
      h -= 255;
  }

  red   = (uchar)h;
  green = (uchar)l;
  blue  = (uchar)s;
}

/*!
 * Implement the HLS "double hex-cone".
 * \param n1 lightness fraction (?)
 * \param n2 saturation fraction (?)
 * \param hue hue "angle".
 * \return HLS value.
 */
int HLSVALUE ( double n1, double n2, double hue )
{
  double value;

  if ( hue > 255 )
    hue -= 255;
  else if ( hue < 0 )
    hue += 255;

  if ( hue < 42.5 )
    value = n1 + ( n2 - n1 ) * ( hue / 42.5 );
  else if ( hue < 127.5 )
    value = n2;
  else if ( hue < 170 )
    value = n1 + ( n2 - n1 ) * ( ( 170 - hue ) / 42.5 );
  else
    value = n1;

  return (int)( value * 255 );
}

/*!
 * Convert a color in HLS space to RGB space.
 * \param hue the hue component (modified in place).
 * \param lightness the lightness component (modified in place).
 * \param saturation the saturation component (modified in place).
 */
void HLSTORGB ( uchar& hue, uchar& lightness, uchar& saturation )
{
  double h = hue;
  double l = lightness;
  double s = saturation;

  if ( s == 0 ) {
    hue        = (uchar)l;
    lightness  = (uchar)l;
    saturation = (uchar)l;
  }
  else {
    double m1, m2;

    if ( l < 128 )
      m2 = ( l * ( 255 + s ) ) / 65025.;
    else
      m2 = ( l + s - ( l * s ) / 255. ) / 255.;

    m1 = ( l / 127.5 ) - m2;

    hue        = HLSVALUE( m1, m2, h + 85 );
    lightness  = HLSVALUE( m1, m2, h );
    saturation = HLSVALUE( m1, m2, h - 85 );
  }
}
//////////////////////////////////////////////////////////////////////////////////


XCFImageFormat::XCFImageFormat() {
    // From GIMP "paint_funcs.c" v1.2
    srand( RANDOM_SEED );

    for ( int i = 0; i < RANDOM_TABLE_SIZE; i++ )
      random_table[i] = rand();

    for ( int i = 0; i < RANDOM_TABLE_SIZE; i++ ) {
      int tmp;
      int swap = i + rand() % ( RANDOM_TABLE_SIZE - i );
      tmp = random_table[i];
      random_table[i] = random_table[swap];
      random_table[swap] = tmp;
    }

    for ( int j = 0; j < 256; j++ ) {
      for ( int k = 0; k < 256; k++ ) {
	int tmp_sum = j + k;
	if ( tmp_sum > 255 )
	  tmp_sum = 255;
	add_lut[j][k] = tmp_sum;
      }
    }
  }


bool XCFImageFormat::installIOHandler ( const QString& ) {
    QImageIO::defineIOHandler( "XCF", "gimp xcf", 0,
			       &XCFImageFormat::readXCF,
#ifdef TMP_WRITE
			       &XCFImageFormat::writeXCF );
#else
			       0 );
#endif
    return true;
}


void XCFImageFormat::registerFormat() {
	QImageIO::defineIOHandler( "XCF","^gimp xcf",
		0,XCFImageFormat::readXCF,0L);
}


/*!
 * The Qt QImageIO architecture invokes this routine to read the image.
 * The file (or other data stream) is already open and the
 * initial string indicating a XCF file has been matched (but the stream
 * is positioned at its beginning).
 *
 * The XCF file is binary and is stored in big endian format. The
 * SafeDataStream class is used to read the file. Even though the XCF file
 * was not written with SafeDataStream, there is still a good match. At least
 * in version 001 of XCF and version 4 of SafeDataStream. Any other combination
 * is suspect.
 *
 * Any failures while reading the XCF image are reported by the
 * QImage::status() method.
 *
 * \param image_io the QImageIO object connected to the XCF image.
 */
void XCFImageFormat::readXCF ( QImageIO* image_io )
{
  XCFImage xcf_image;

  // The XCF data is stored in big endian format, which SafeDataStream handles
  // very well.

  SafeDataStream xcf_io( image_io->ioDevice() );

  char tag[14];
  xcf_io.readRawBytes( tag, sizeof(tag) );

  if ( xcf_io.failed() ) {
    qDebug( "XCF: read failure on header tag" );
    return;
  }

  xcf_io >> xcf_image.width >> xcf_image.height >> xcf_image.type;

  if ( xcf_io.failed() ) {
    qDebug( "XCF: read failure on image info" );
    return;
  }

  if ( !loadImageProperties( xcf_io, xcf_image ) ) return;

  // The layers appear to be stored in top-to-bottom order. This is
  // the reverse of how a merged image must be computed. So, the layer
  // offsets are pushed onto a LIFO stack (thus, we don't have to load
  // all the data of all layers before beginning to construct the
  // merged image).

  QValueStack< Q_INT32 > layer_offsets;

  while ( true ) {
    Q_INT32 layer_offset;

    xcf_io >> layer_offset;

    if ( xcf_io.failed() ) {
      qDebug( "XCF: read failure on layer offsets" );
      return;
    }

    if ( layer_offset == 0 ) break;

    layer_offsets.push( layer_offset );
  }

  xcf_image.num_layers = layer_offsets.size();

  if ( layer_offsets.size() == 0 ) {
    qDebug( "XCF: no layers!" );
    return;
  }

  // Load each layer and add it to the image

  while ( !layer_offsets.isEmpty() ) {
    Q_INT32 layer_offset = layer_offsets.pop();

    xcf_io.device()->at( layer_offset );

    if ( !loadLayer( xcf_io, xcf_image ) ) return;
  }

  if ( !xcf_image.initialized ) {
    qDebug( "XCF: no visible layers!" );
    return;
  }

  image_io->setImage( xcf_image.image );
  image_io->setStatus( 0 );
}

/*!
 * Construct the QImage which will eventually be returned to the QImage
 * loader.
 *
 * There are a couple of situations which require that the QImage is not
 * exactly the same as The GIMP's representation. The full table is:
 * \verbatim
 *  Grayscale  opaque      :  8 bpp indexed
 *  Grayscale  translucent : 32 bpp + alpha
 *  Indexed    opaque      :  1 bpp if num_colors <= 2
 *                         :  8 bpp indexed otherwise
 *  Indexed    translucent :  8 bpp indexed + alpha if num_colors < 256
 *                         : 32 bpp + alpha otherwise
 *  RGB        opaque      : 32 bpp
 *  RGBA       translucent : 32 bpp + alpha
 * \endverbatim
 * Whether the image is translucent or not is determined by the bottom layer's
 * alpha channel. However, even if the bottom layer lacks an alpha channel,
 * it can still have an opacity < 1. In this case, the QImage is promoted
 * to 32-bit. (Note this is different from the output from the GIMP image
 * exporter, which seems to ignore this attribute.)
 *
 * Independently, higher layers can be translucent, but the background of
 * the image will not show through if the bottom layer is opaque.
 *
 * For indexed images, translucency is an all or nothing effect.
 * \param xcf_image contains image info and bottom-most layer.
 */
void XCFImageFormat::initializeImage ( XCFImage& xcf_image )
{
  // (Aliases to make the code look a little better.)
  Layer& layer( xcf_image.layer );
  QImage& image( xcf_image.image );

  switch ( layer.type ) {
  case RGB_GIMAGE:
    if ( layer.opacity == OPAQUE_OPACITY ) {
      image.create( xcf_image.width, xcf_image.height, 32 );
      image.fill( qRgb( 255, 255, 255 ) );
      break;
    } // else, fall through to 32-bit representation

  case RGBA_GIMAGE:
    image.create( xcf_image.width, xcf_image.height, 32 );
    image.fill( qRgba( 255, 255, 255, 0 ) );
    // Turning this on prevents fill() from affecting the alpha channel,
    // by the way.
    image.setAlphaBuffer( true );
    break;

  case GRAY_GIMAGE:
    if ( layer.opacity == OPAQUE_OPACITY ) {
      image.create( xcf_image.width, xcf_image.height, 8, 256 );
      setGrayPalette( image );
      image.fill( 255 );
      break;
    } // else, fall through to 32-bit representation

  case GRAYA_GIMAGE:
    image.create( xcf_image.width, xcf_image.height, 32 );
    image.fill( qRgba( 255, 255, 255, 0 ) );
    image.setAlphaBuffer( true );
    break;

  case INDEXED_GIMAGE:
    // As noted in the table above, there are quite a few combinations
    // which are possible with indexed images, depending on the
    // presence of transparency (note: not translucency, which is not
    // supported by The GIMP for indexed images) and the number of
    // individual colors.

    // Note: Qt treats a bitmap with a Black and White color palette
    // as a mask, so only the "on" bits are drawn, regardless of the
    // order color table entries. Otherwise (i.e., at least one of the
    // color table entries is not black or white), it obeys the one-
    // or two-color palette. Have to ask about this...

    if ( xcf_image.num_colors <= 2 ) {
      image.create( xcf_image.width, xcf_image.height,
		    1, xcf_image.num_colors,
		    QImage::LittleEndian );
      image.fill( 0 );
      setPalette( xcf_image, image );
    }

    else if ( xcf_image.num_colors <= 256 ) {
      image.create( xcf_image.width, xcf_image.height,
		    8, xcf_image.num_colors,
		    QImage::LittleEndian );
      image.fill( 0 );
      setPalette( xcf_image, image );
    }

    break;

  case INDEXEDA_GIMAGE:

    if ( xcf_image.num_colors == 1 ) {

      // Plenty(!) of room to add a transparent color

      xcf_image.num_colors++;
      xcf_image.palette.resize( xcf_image.num_colors );
      xcf_image.palette[1] = xcf_image.palette[0];
      xcf_image.palette[0] = qRgba( 255, 255, 255, 0 );

      image.create( xcf_image.width, xcf_image.height,
		    1, xcf_image.num_colors,
		    QImage::LittleEndian );
      image.fill( 0 );
      setPalette( xcf_image, image );
      image.setAlphaBuffer( true );
    }

    else if ( xcf_image.num_colors < 256 ) {

      // Plenty of room to add a transparent color

      xcf_image.num_colors++;
      xcf_image.palette.resize( xcf_image.num_colors );
      for ( int c = xcf_image.num_colors-1; c >= 1; c-- )
	xcf_image.palette[c] = xcf_image.palette[c-1];
      xcf_image.palette[0] = qRgba( 255, 255, 255, 0 );

      image.create( xcf_image.width, xcf_image.height,
		    8, xcf_image.num_colors );
      image.fill( 0 );
      setPalette( xcf_image, image );
      image.setAlphaBuffer( true );
    }

    else {
      // No room for a transparent color, so this has to be promoted to
      // true color. (There is no equivalent PNG representation output
      // from The GIMP as of v1.2.)
      image.create( xcf_image.width, xcf_image.height, 32 );
      image.fill( qRgba( 255, 255, 255, 0 ) );
      image.setAlphaBuffer( true );
    }

    break;
  }

  image.setDotsPerMeterX( (int)( xcf_image.x_resolution * INCHESPERMETER ) );
  image.setDotsPerMeterY( (int)( xcf_image.y_resolution * INCHESPERMETER ) );
}

/*!
 * Compute the number of tiles in the current layer and allocate
 * QImage structures for each of them.
 * \param xcf_image contains the current layer.
 */
void XCFImageFormat::composeTiles ( XCFImage& xcf_image )
{
  Layer& layer( xcf_image.layer );

  layer.nrows = ( layer.height + TILE_HEIGHT - 1 ) / TILE_HEIGHT;
  layer.ncols = ( layer.width + TILE_WIDTH - 1 ) / TILE_WIDTH;

  layer.image_tiles.resize( layer.nrows );

  if ( layer.type == GRAYA_GIMAGE || layer.type == INDEXEDA_GIMAGE )
    layer.alpha_tiles.resize( layer.nrows );

  if ( layer.mask_offset != 0 )
    layer.mask_tiles.resize( layer.nrows );

  for ( uint j = 0; j < layer.nrows; j++ ) {
    layer.image_tiles[j].resize( layer.ncols );

    if ( layer.type == GRAYA_GIMAGE || layer.type == INDEXEDA_GIMAGE )
      layer.alpha_tiles[j].resize( layer.ncols );

    if ( layer.mask_offset != 0 )
      layer.mask_tiles[j].resize( layer.ncols );
  }

  for ( uint j = 0; j < layer.nrows; j++ ) {
    for ( uint i = 0; i < layer.ncols; i++ ) {

      uint tile_width = (i+1) * TILE_WIDTH <= layer.width ?
	TILE_WIDTH : layer.width - i*TILE_WIDTH;

      uint tile_height = (j+1) * TILE_HEIGHT <= layer.height ?
	TILE_HEIGHT : layer.height - j*TILE_HEIGHT;

      // Try to create the most appropriate QImage (each GIMP layer
      // type is treated slightly differently)

      switch ( layer.type ) {
      case RGB_GIMAGE:
	layer.image_tiles[j][i] = QImage( tile_width, tile_height, 32, 0 );
	layer.image_tiles[j][i].setAlphaBuffer( false );
	break;

      case RGBA_GIMAGE:
	layer.image_tiles[j][i] = QImage( tile_width, tile_height, 32, 0 );
	layer.image_tiles[j][i].setAlphaBuffer( true );
	break;

      case GRAY_GIMAGE:
	layer.image_tiles[j][i] = QImage( tile_width, tile_height, 8, 256 );
	setGrayPalette( layer.image_tiles[j][i] );
	break;

      case GRAYA_GIMAGE:
	layer.image_tiles[j][i] = QImage( tile_width, tile_height, 8, 256 );
	setGrayPalette( layer.image_tiles[j][i] );

	layer.alpha_tiles[j][i] = QImage( tile_width, tile_height, 8, 256 );
	setGrayPalette( layer.alpha_tiles[j][i] );
	break;

      case INDEXED_GIMAGE:
	layer.image_tiles[j][i] = QImage( tile_width, tile_height, 8,
					  xcf_image.num_colors );
	setPalette( xcf_image, layer.image_tiles[j][i] );
	break;

      case INDEXEDA_GIMAGE:
	layer.image_tiles[j][i] = QImage( tile_width, tile_height, 8,
					  xcf_image.num_colors );
	setPalette( xcf_image, layer.image_tiles[j][i] );

	layer.alpha_tiles[j][i] = QImage( tile_width, tile_height, 8, 256 );
	setGrayPalette( layer.alpha_tiles[j][i] );
      }

      if ( layer.mask_offset != 0 ) {
	layer.mask_tiles[j][i] = QImage( tile_width, tile_height, 8, 256 );
	setGrayPalette( layer.mask_tiles[j][i] );
      }
    }
  }
}

/*!
 * Apply a grayscale palette to the QImage. Note that Qt does not distinguish
 * between grayscale and indexed images. A grayscale image is just
 * an indexed image with a 256-color, grayscale palette.
 * \param image image to set to a grayscale palette.
 */
void XCFImageFormat::setGrayPalette ( QImage& image )
{
  for ( int i = 0; i < 256; i++ )
    image.setColor( i, qRgb(i,i,i) );
}

/*!
 * Copy the indexed palette from the XCF image into the QImage.
 * \param xcf_image XCF image containing the palette read from the data stream.
 * \param image image to apply the palette to.
 */
void XCFImageFormat::setPalette ( XCFImage& xcf_image, QImage& image )
{
  for ( int i = 0; i < xcf_image.num_colors; i++ )
    image.setColor( i, xcf_image.palette[i] );
}

/*!
 * An XCF file can contain an arbitrary number of properties associated
 * with the image (and layer and mask).
 * \param xcf_io the data stream connected to the XCF image
 * \param xcf_image XCF image data.
 * \return true if there were no I/O errors.
 */
bool XCFImageFormat::loadImageProperties ( SafeDataStream& xcf_io,
					   XCFImage& xcf_image )
{
  while ( true ) {
    PropType type;
    QByteArray bytes;

    if ( !loadProperty( xcf_io, type, bytes ) ) {
      qDebug( "XCF: error loading global image properties" );
      return false;
    }

    QDataStream property( bytes, IO_ReadOnly );

    switch ( type ) {
    case PROP_END:
      return true;

    case PROP_COMPRESSION:
      property >> xcf_image.compression;
      break;

    case PROP_GUIDES:
      // This property is ignored.
      break;
	  
    case PROP_RESOLUTION:
      property >> xcf_image.x_resolution >> xcf_image.y_resolution;
      break;
      
    case PROP_TATTOO:
      property >> xcf_image.tattoo;
      break;

    case PROP_PARASITES:
      while ( !property.atEnd() ) {
 char* tag;
 Q_UINT32 size;

 property.readBytes( tag, size );

 Q_UINT32 flags;
 char* data;
 property >> flags >> data;

 if ( strcmp( tag, "gimp-comment" ) == 0 )
   xcf_image.image.setText( "Comment", 0, data );

 delete[] tag;
 delete[] data;
      }
      break;

    case PROP_UNIT:
      property >> xcf_image.unit;
      break;

    case PROP_PATHS:
      // This property is ignored.
      break;

    case PROP_USER_UNIT:
      // This property is ignored.
      break;

    case PROP_COLORMAP:
      property >> xcf_image.num_colors;

      xcf_image.palette.reserve( xcf_image.num_colors );

      for ( int i = 0; i < xcf_image.num_colors; i++ ) {
	uchar r, g, b;
	property >> r >> g >> b;
	xcf_image.palette.push_back( qRgb(r,g,b) );
      }
      break;

    default:
      qDebug( "XCF: unimplemented image property %d, size %d", type, bytes.size() );
    }
  }
}

/*!
 * Load a layer from the XCF file. The data stream must be positioned at
 * the beginning of the layer data.
 * \param xcf_io the image file data stream.
 * \param xcf_image contains the layer and the color table
 * (if the image is indexed).
 * \return true if there were no I/O errors.
 */
bool XCFImageFormat::loadLayer ( SafeDataStream& xcf_io, XCFImage& xcf_image )
{
  Layer& layer( xcf_image.layer );

  if ( layer.name != 0 ) delete[] layer.name;

  xcf_io >> layer.width >> layer.height >> layer.type >> layer.name;

  if ( xcf_io.failed() ) {
    qDebug( "XCF: read failure on layer" );
    return false;
  }

  if ( !loadLayerProperties( xcf_io, layer ) ) return false;
#if 0
  cout << "layer: \"" << layer.name << "\", size: " << layer.width << " x "
       << layer.height << ", type: " << layer.type << ", mode: " << layer.mode
       << ", opacity: " << layer.opacity << ", visible: " << layer.visible
       << ", offset: " << layer.x_offset << ", " << layer.y_offset << endl;
#endif
  // Skip reading the rest of it if it is not visible. Typically, when
  // you export an image from the The GIMP it flattens (or merges) only
  // the visible layers into the output image.

  if ( layer.visible == 0 ) return true;

  // If there are any more layers, merge them into the final QImage.

  xcf_io >> layer.hierarchy_offset >> layer.mask_offset;

  if ( xcf_io.failed() ) {
    qDebug( "XCF: read failure on layer image offsets" );
    return false;
  }

  // Allocate the individual tile QImages based on the size and type
  // of this layer.

  composeTiles( xcf_image );

  xcf_io.device()->at( layer.hierarchy_offset );

  // As tiles are loaded, they are copied into the layers tiles by
  // this routine. (loadMask(), below, uses a slightly different
  // version of assignBytes().)

  layer.assignBytes = assignImageBytes;

  if ( !loadHierarchy( xcf_io, layer ) ) return false;

  if ( layer.mask_offset != 0 ) {
    xcf_io.device()->at( layer.mask_offset );

    if ( !loadMask( xcf_io, layer ) ) return false;
  }

  // Now we should have enough information to initialize the final
  // QImage. The first visible layer determines the attributes
  // of the QImage.

  if ( !xcf_image.initialized ) {
    initializeImage( xcf_image );

    copyLayerToImage( xcf_image );

    xcf_image.initialized = true;
  }
  else
    mergeLayerIntoImage( xcf_image );

  return true;
}

/*!
 * An XCF file can contain an arbitrary number of properties associated
 * with a layer.
 * \param xcf_io the data stream connected to the XCF image.
 * \param layer layer to collect the properties.
 * \return true if there were no I/O errors.
 */
bool XCFImageFormat::loadLayerProperties ( SafeDataStream& xcf_io, Layer& layer )
{
  while ( true ) {
    PropType type;
    QByteArray bytes;

    if ( !loadProperty( xcf_io, type, bytes ) ) {
      qDebug( "XCF: error loading layer properties" );
      return false;
    }

    QDataStream property( bytes, IO_ReadOnly );

    switch ( type ) {
    case PROP_END:
      return true;

    case PROP_ACTIVE_LAYER:
      layer.active = true;
      break;

    case PROP_OPACITY:
      property >> layer.opacity;
      break;

    case PROP_VISIBLE:
      property >> layer.visible;
      break;

    case PROP_LINKED:
      property >> layer.linked;
      break;

    case PROP_PRESERVE_TRANSPARENCY:
      property >> layer.preserve_transparency;
      break;

    case PROP_APPLY_MASK:
      property >> layer.apply_mask;
      break;

    case PROP_EDIT_MASK:
      property >> layer.edit_mask;
      break;

    case PROP_SHOW_MASK:
      property >> layer.show_mask;
      break;

    case PROP_OFFSETS:
      property >> layer.x_offset >> layer.y_offset;
      break;

    case PROP_MODE:
      property >> layer.mode;
      break;

    case PROP_TATTOO:
      property >> layer.tattoo;
      break;

    default:
      qDebug( "XCF: unimplemented layer property %d, size %d", type, bytes.size() );
    }
  }
}

/*!
 * An XCF file can contain an arbitrary number of properties associated
 * with a channel. Note that this routine only reads mask channel properties.
 * \param xcf_io the data stream connected to the XCF image.
 * \param layer layer containing the mask channel to collect the properties.
 * \return true if there were no I/O errors.
 */
bool XCFImageFormat::loadChannelProperties ( SafeDataStream& xcf_io, Layer& layer )
{
  while ( true ) {
    PropType type;
    QByteArray bytes;

    if ( !loadProperty( xcf_io, type, bytes ) ) {
      qDebug( "XCF: error loading channel properties" );
      return false;
    }

    QDataStream property( bytes, IO_ReadOnly );

    switch ( type ) {
    case PROP_END:
      return true;

    case PROP_OPACITY:
      property >> layer.mask_channel.opacity;
      break;

    case PROP_VISIBLE:
      property >> layer.mask_channel.visible;
      break;

    case PROP_SHOW_MASKED:
      property >> layer.mask_channel.show_masked;
      break;

    case PROP_COLOR:
      property >> layer.mask_channel.red >> layer.mask_channel.green
	       >> layer.mask_channel.blue;
      break;
      
    case PROP_TATTOO:
      property >> layer.mask_channel.tattoo;
      break;

    default:
      qDebug( "XCF: unimplemented channel property %d, size %d", type, bytes.size() );
    }
  }
}

/*!
 * The GIMP stores images in a "mipmap"-like hierarchy. As far as the QImage
 * is concerned, however, only the top level (i.e., the full resolution image)
 * is used.
 * \param xcf_io the data stream connected to the XCF image.
 * \param layer the layer to collect the image.
 * \return true if there were no I/O errors.
 */
bool XCFImageFormat::loadHierarchy ( SafeDataStream& xcf_io, Layer& layer )
{
  Q_INT32 width;
  Q_INT32 height;
  Q_INT32 bpp;
  Q_UINT32 offset;

  xcf_io >> width >> height >> bpp >> offset;

  if ( xcf_io.failed() ) {
    qDebug( "XCF: read failure on layer %s image header", layer.name );
    return false;
  }

  // GIMP stores images in a "mipmap"-like format (multiple levels of
  // increasingly lower resolution). Only the top level is used here,
  // however.

  Q_UINT32 junk;
  do {
    xcf_io >> junk;

    if ( xcf_io.failed() ) {
      qDebug( "XCF: read failure on layer %s level offsets", layer.name );
      return false;
    }
  } while ( junk != 0 );

  QIODevice::Offset saved_pos = xcf_io.device()->at();

  xcf_io.device()->at( offset );

  if ( !loadLevel( xcf_io, layer, bpp ) ) return false;

  xcf_io.device()->at( saved_pos );

  return true;
}

/*!
 * Load one level of the image hierarchy (but only the top level is ever used).
 * \param xcf_io the data stream connected to the XCF image.
 * \param layer the layer to collect the image.
 * \param bpp the number of bytes in a pixel.
 * \return true if there were no I/O errors.
 * \sa loadTileRLE().
 */
bool XCFImageFormat::loadLevel ( SafeDataStream& xcf_io, Layer& layer, Q_INT32 bpp )
{
  Q_INT32 width;
  Q_INT32 height;
  Q_UINT32 offset;

  xcf_io >> width >> height >> offset;

  if ( xcf_io.failed() ) {
    qDebug( "XCF: read failure on layer %s level info", layer.name );
    return false;
  }

  if ( offset == 0 ) return true;

  for ( uint j = 0; j < layer.nrows; j++ ) {
    for ( uint i = 0; i < layer.ncols; i++ ) {

      if ( offset == 0 ) {
	qDebug( "XCF: incorrect number of tiles in layer %s", layer.name );
	return false;
      }

      QIODevice::Offset saved_pos = xcf_io.device()->at();

      Q_UINT32 offset2;

      xcf_io >> offset2;

      if ( xcf_io.failed() ) {
	qDebug( "XCF: read failure on layer %s level offset look-ahead",
		layer.name );
	return false;
      }

      // Evidently, RLE can occasionally expand a tile instead of compressing it!

      if ( offset2 == 0 )
	offset2 = offset + (uint)( TILE_WIDTH * TILE_HEIGHT * 4 * 1.5 );

      xcf_io.device()->at( offset );

      int size = layer.image_tiles[j][i].width() * layer.image_tiles[j][i].height();

      if ( !loadTileRLE( xcf_io, layer.tile, size, offset2 - offset, bpp ) )
	return false;

      // The bytes in the layer tile are juggled differently depending on
      // the target QImage. The caller has set layer.assignBytes to the
      // appropriate routine.

      layer.assignBytes( layer, i, j );

      xcf_io.device()->at( saved_pos );
      
      xcf_io >> offset;

      if ( xcf_io.failed() ) {
	qDebug( "XCF: read failure on layer %s level offset", layer.name );
	return false;
      }
    }
  }

  return true;
}

/*!
 * A layer can have a one channel image which is used as a mask.
 * \param xcf_io the data stream connected to the XCF image.
 * \param layer the layer to collect the mask image.
 * \return true if there were no I/O errors.
 */
bool XCFImageFormat::loadMask ( SafeDataStream& xcf_io, Layer& layer )
{
  Q_INT32 width;
  Q_INT32 height;
  char* name;

  xcf_io >> width >> height >> name;

  if ( xcf_io.failed() ) {
    qDebug( "XCF: read failure on mask info" );
    return false;
  }

  delete name;

  if ( !loadChannelProperties( xcf_io, layer ) ) return false;

  Q_UINT32 hierarchy_offset;

  xcf_io >> hierarchy_offset;

  if ( xcf_io.failed() ) {
    qDebug( "XCF: read failure on mask image offset" );
    return false;
  }

  xcf_io.device()->at( hierarchy_offset );

  layer.assignBytes = assignMaskBytes;

  if ( !loadHierarchy( xcf_io, layer ) ) return false;

  return true;
}

/*!
 * This is the routine for which all the other code is simply
 * infrastructure. Read the image bytes out of the file and
 * store them in the tile buffer. This is passed a full 32-bit deep
 * buffer, even if bpp is smaller. The caller can figure out what to
 * do with the bytes.
 *
 * The tile is stored in "channels", i.e. the red component of all
 * pixels, then the green component of all pixels, then blue then
 * alpha, or, for indexed images, the color indices of all pixels then
 * the alpha of all pixels.
 *
 * The data is compressed with "run length encoding". Some simple data
 * integrity checks are made.
 *
 * \param xcf_io the data stream connected to the XCF image.
 * \param tile the buffer to expand the RLE into.
 * \param image_size number of bytes expected to be in the image tile.
 * \param data_length number of bytes expected in the RLE.
 * \param bpp number of bytes per pixel.
 * \return true if there were no I/O errors and no obvious corruption of
 * the RLE data.
 */
bool XCFImageFormat::loadTileRLE ( SafeDataStream& xcf_io, uchar* tile, int image_size,
				   int data_length, Q_INT32 bpp )
{
  uchar* data;

  uchar* xcfdata;
  uchar* xcfodata;
  uchar* xcfdatalimit;

  xcfdata = xcfodata = new uchar[data_length];

  int read_length=xcf_io.device()->readBlock( (char*)xcfdata, data_length  );

  if ( read_length<=0 ) {
    delete[] xcfodata;
    qDebug( "XCF: read failure on tile" );
    return false;
  }

  xcfdatalimit = &xcfodata[read_length-1];

  for ( int i = 0; i < bpp; ++i ) {

    data = tile + i;

    int count = 0;
    int size = image_size;

    while ( size > 0 ) {
      if ( xcfdata > xcfdatalimit )
	goto bogus_rle;

      uchar val = *xcfdata++;

      uint length = val;

      if ( length >= 128 ) {
	length = 255 - ( length - 1 );
	if ( length == 128 ) {
	  if ( xcfdata >= xcfdatalimit )
	    goto bogus_rle;

	  length = ( *xcfdata << 8 ) + xcfdata[1];

	  xcfdata += 2;
	}

	count += length;
	size -= length;

	if ( size < 0 )
	  goto bogus_rle;

	if ( &xcfdata[length-1] > xcfdatalimit )
	  goto bogus_rle;

	while ( length-- > 0 ) {
	  *data = *xcfdata++;
	  data += sizeof(QRgb);
	}
      }
      else {
	length += 1;
	if ( length == 128 ) {

	  if ( xcfdata >= xcfdatalimit )
	    goto bogus_rle;

	  length = ( *xcfdata << 8 ) + xcfdata[1];
	  xcfdata += 2;
	}

	count += length;
	size -= length;

	if ( size < 0 )
	  goto bogus_rle;

	if ( xcfdata > xcfdatalimit )
	  goto bogus_rle;

	val = *xcfdata++;

	while ( length-- > 0 ) {
	  *data = val;
	  data += sizeof(QRgb);
	}
      }
    }
  }

  delete[] xcfodata;
  return true;

 bogus_rle:

  qDebug( "The run length encoding could not be decoded properly" );
  delete[] xcfodata;
  return false;
}

/*!
 * Copy the bytes from the tile buffer into the image tile QImage, taking into
 * account all the myriad different modes.
 * \param layer layer containing the tile buffer and the image tile matrix.
 * \param i column index of current tile.
 * \param j row index of current tile.
 */
void XCFImageFormat::assignImageBytes ( Layer& layer, uint i, uint j )
{
  uchar* tile = layer.tile;

  switch ( layer.type ) {
  case RGB_GIMAGE:
    for ( int l = 0; l < layer.image_tiles[j][i].height(); l++ ) {
      for ( int k = 0; k < layer.image_tiles[j][i].width(); k++ ) {
	layer.image_tiles[j][i].setPixel( k, l, qRgb( tile[0], tile[1], tile[2] ) );
	tile += sizeof(QRgb);
      }
    }
    break;

  case RGBA_GIMAGE:
    for ( int l = 0; l < layer.image_tiles[j][i].height(); l++ ) {
      for ( int k = 0; k < layer.image_tiles[j][i].width(); k++ ) {
	layer.image_tiles[j][i].setPixel( k, l,
					  qRgba( tile[0], tile[1], tile[2], tile[3] ) );
	tile += sizeof(QRgb);
      }
    }
    break;

  case GRAY_GIMAGE:
  case INDEXED_GIMAGE:
    for ( int l = 0; l < layer.image_tiles[j][i].height(); l++ ) {
      for ( int k = 0; k < layer.image_tiles[j][i].width(); k++ ) {
	layer.image_tiles[j][i].setPixel( k, l, tile[0] );
	tile += sizeof(QRgb);
      }
    }
    break;

  case GRAYA_GIMAGE:
  case INDEXEDA_GIMAGE:
    for ( int l = 0; l < layer.image_tiles[j][i].height(); l++ ) {
      for ( int k = 0; k < layer.image_tiles[j][i].width(); k++ ) {

	// The "if" here should not be necessary, but apparently there
	// are some cases where the image can contain larger indices
	// than there are colors in the palette. (A bug in The GIMP?)

	if ( tile[0] < layer.image_tiles[j][i].numColors() )
	  layer.image_tiles[j][i].setPixel( k, l, tile[0] );

	layer.alpha_tiles[j][i].setPixel( k, l, tile[1] );
	tile += sizeof(QRgb);
      }
    }
    break;
  }
}

/*!
 * Copy the bytes from the tile buffer into the mask tile QImage.
 * \param layer layer containing the tile buffer and the mask tile matrix.
 * \param i column index of current tile.
 * \param j row index of current tile.
 */
void XCFImageFormat::assignMaskBytes ( Layer& layer, uint i, uint j )
{
  uchar* tile = layer.tile;

  for ( int l = 0; l < layer.image_tiles[j][i].height(); l++ ) {
    for ( int k = 0; k < layer.image_tiles[j][i].width(); k++ ) {
      layer.mask_tiles[j][i].setPixel( k, l, tile[0] );
      tile += sizeof(QRgb);
    }
  }
}

/*!
 * Read a single property from the image file. The property type is returned
 * in type and the data is returned in bytes.
 * \param xcf the image file data stream.
 * \param type returns with the property type.
 * \param bytes returns with the property data.
 * \return true if there were no IO errors.  */
bool XCFImageFormat::loadProperty ( SafeDataStream& xcf_io, PropType& type,
				    QByteArray& bytes )
{
  Q_UINT32 tmp;
  xcf_io >> tmp;
  type=static_cast<PropType>(tmp);

  if ( xcf_io.failed() ) {
    qDebug( "XCF: read failure on property type" );
    return false;
  }

  char* data;
  Q_UINT32 size;

  // The COLORMAP property is tricky: in version of GIMP older than 2.0.2, the
  // property size was wrong (it was 4 + ncolors instead of 4 + 3*ncolors).
  // This has been fixed in 2.0.2 (*), but the XCF format version has not been
  // increased, so we can't rely on the property size. The UINT32 after the
  // property size is the number of colors, which has always been correct, so
  // we read it, compute the size from it and put it back in the stream.
  //
  // * See http://bugzilla.gnome.org/show_bug.cgi?id=142149 and
  // gimp/app/xcf-save.c, revision 1.42
  if ( type == PROP_COLORMAP ) {
    Q_UINT32 ignoredSize, ncolors;
    xcf_io >> ignoredSize;
    if ( xcf_io.failed() ) {
      qDebug( "XCF: read failure on property %d size", type );
      return false;
    }

    xcf_io >> ncolors;
    if ( xcf_io.failed() ) {
      qDebug( "XCF: read failure on property %d size", type );
      return false;
    }
    xcf_io.device()->ungetch( ncolors & 0xff);
    xcf_io.device()->ungetch( (ncolors>> 8) & 0xff );
    xcf_io.device()->ungetch( (ncolors>>16) & 0xff );
    xcf_io.device()->ungetch( (ncolors>>24) & 0xff );
    
    size=4 + 3 * ncolors;
    data = new char[size];

    xcf_io.readRawBytes( data, size );
  }

  // The USER UNIT property size is not correct. I'm not sure why, though.

  else if ( type == PROP_USER_UNIT ) {
    float factor;
    Q_INT32 digits;
    char* unit_strings;

    xcf_io >> size >> factor >> digits;

    if ( xcf_io.failed() ) {
      qDebug( "XCF: read failure on property %d", type );
      return false;
    }

    for ( int i = 0; i < 5; i++ ) {
      xcf_io >> unit_strings;

      if ( xcf_io.failed() ) {
	qDebug( "XCF: read failure on property %d", type );
	return false;
      }

      delete[] unit_strings;
    }

    size = 0;
  }

  else
    xcf_io.readBytes( data, size );

  if ( xcf_io.failed() ) {
    qDebug( "XCF: read failure on property %d data, size %d", type, size );
    return false;
  }

  if ( size != 0 ) {
    bytes.resize( size );

    for ( uint i = 0; i < size; i++ ) bytes[i] = data[i];

    delete[] data;
  }

  return true;
}

/*!
 * Copy a layer into an image, taking account of the manifold modes. The
 * contents of the image are replaced.
 * \param xcf_image contains the layer and image to be replaced.
 */
void XCFImageFormat::copyLayerToImage ( XCFImage& xcf_image )
{
  Layer& layer( xcf_image.layer );
  QImage& image( xcf_image.image );

  PixelCopyOperation copy = 0;

  switch ( layer.type ) {
  case RGB_GIMAGE:
  case RGBA_GIMAGE:
    copy = copyRGBToRGB; break;
  case GRAY_GIMAGE:
    if ( layer.opacity == OPAQUE_OPACITY )
      copy = copyGrayToGray;
    else
      copy = copyGrayToRGB;
    break;
  case GRAYA_GIMAGE:
    copy = copyGrayAToRGB; break;
  case INDEXED_GIMAGE:
    copy = copyIndexedToIndexed; break;
  case INDEXEDA_GIMAGE:
    if ( xcf_image.image.depth() <= 8 )
      copy = copyIndexedAToIndexed;
    else
      copy = copyIndexedAToRGB;
  }

  // For each tile...

  for ( uint j = 0; j < layer.nrows; j++ ) {
    uint y = j * TILE_HEIGHT;
      
    for ( uint i = 0; i < layer.ncols; i++ ) {
      uint x = i * TILE_WIDTH;

      // This seems the best place to apply the dissolve because it
      // depends on the global position of each tile's
      // pixels. Apparently it's the only mode which can apply to a
      // single layer.

      if ( layer.mode == DISSOLVE_MODE ) {
	if ( layer.type == RGBA_GIMAGE )
	  dissolveRGBPixels( layer.image_tiles[j][i], x, y );

	else if ( layer.type == GRAYA_GIMAGE )
	  dissolveAlphaPixels( layer.alpha_tiles[j][i], x, y );
      }

      for ( int l = 0; l < layer.image_tiles[j][i].height(); l++ ) {
	for ( int k = 0; k < layer.image_tiles[j][i].width(); k++ ) {

	  int m = x + k + layer.x_offset;
	  int n = y + l + layer.y_offset;

	  if ( m < 0 || m >= image.width() || n < 0 || n >= image.height() )
	    continue;

	  (*copy)( layer, i, j, k, l, image, m, n );
	}
      }
    }
  }
}

/*!
 * Copy an RGB pixel from the layer to the RGB image. Straight-forward.
 * The only thing this has to take account of is the opacity of the
 * layer. Evidently, the GIMP exporter itself does not actually do this.
 * \param layer source layer.
 * \param i x tile index.
 * \param j y tile index.
 * \param k x pixel index of tile i,j.
 * \param l y pixel index of tile i,j.
 * \param image destination image.
 * \param m x pixel of destination image.
 * \param n y pixel of destination image.
 */
void XCFImageFormat::copyRGBToRGB ( Layer& layer, uint i, uint j, int k, int l,
				    QImage& image, int m, int n )
{
  QRgb src = layer.image_tiles[j][i].pixel( k, l );

  uchar src_a = layer.opacity;

  if ( layer.type == RGBA_GIMAGE )
    src_a = INT_MULT( src_a, qAlpha( src ) );

  // Apply the mask (if any)

  if ( layer.apply_mask == 1 && layer.mask_tiles.size() > j &&
       layer.mask_tiles[j].size() > i )
    src_a = INT_MULT( src_a, layer.mask_tiles[j][i].pixelIndex( k, l ) );

  image.setPixel( m, n, qRgba( src, src_a ) );
}

/*!
 * Copy a Gray pixel from the layer to the Gray image. Straight-forward.
 * \param layer source layer.
 * \param i x tile index.
 * \param j y tile index.
 * \param k x pixel index of tile i,j.
 * \param l y pixel index of tile i,j.
 * \param image destination image.
 * \param m x pixel of destination image.
 * \param n y pixel of destination image.
 */
void XCFImageFormat::copyGrayToGray ( Layer& layer, uint i, uint j, int k, int l,
				      QImage& image, int m, int n )
{
  int src = layer.image_tiles[j][i].pixelIndex( k, l );

  image.setPixel( m, n, src );
}

/*!
 * Copy a Gray pixel from the layer to an RGB image. Straight-forward.
 * The only thing this has to take account of is the opacity of the
 * layer. Evidently, the GIMP exporter itself does not actually do this.
 * \param layer source layer.
 * \param i x tile index.
 * \param j y tile index.
 * \param k x pixel index of tile i,j.
 * \param l y pixel index of tile i,j.
 * \param image destination image.
 * \param m x pixel of destination image.
 * \param n y pixel of destination image.
 */
void XCFImageFormat::copyGrayToRGB ( Layer& layer, uint i, uint j, int k, int l,
				     QImage& image, int m, int n )
{
  QRgb src = layer.image_tiles[j][i].pixel( k, l );

  uchar src_a = layer.opacity;

  image.setPixel( m, n, qRgba( src, src_a ) );
}

/*!
 * Copy a GrayA pixel from the layer to an RGB image. Straight-forward.
 * The only thing this has to take account of is the opacity of the
 * layer. Evidently, the GIMP exporter itself does not actually do this.
 * \param layer source layer.
 * \param i x tile index.
 * \param j y tile index.
 * \param k x pixel index of tile i,j.
 * \param l y pixel index of tile i,j.
 * \param image destination image.
 * \param m x pixel of destination image.
 * \param n y pixel of destination image.
 */
void XCFImageFormat::copyGrayAToRGB ( Layer& layer, uint i, uint j, int k, int l,
				      QImage& image, int m, int n )
{
  QRgb src = layer.image_tiles[j][i].pixel( k, l );

  uchar src_a = layer.alpha_tiles[j][i].pixelIndex( k, l );

  src_a = INT_MULT( src_a, layer.opacity );

  // Apply the mask (if any)

  if ( layer.apply_mask == 1 && layer.mask_tiles.size() > j &&
       layer.mask_tiles[j].size() > i )
    src_a = INT_MULT( src_a, layer.mask_tiles[j][i].pixelIndex( k, l ) );

  image.setPixel( m, n, qRgba( src, src_a ) );
}

/*!
 * Copy an Indexed pixel from the layer to the Indexed image. Straight-forward.
 * \param layer source layer.
 * \param i x tile index.
 * \param j y tile index.
 * \param k x pixel index of tile i,j.
 * \param l y pixel index of tile i,j.
 * \param image destination image.
 * \param m x pixel of destination image.
 * \param n y pixel of destination image.
 */
void XCFImageFormat::copyIndexedToIndexed ( Layer& layer, uint i,uint j,int k,int l,
					    QImage& image, int m, int n )
{
  int src = layer.image_tiles[j][i].pixelIndex( k, l );

  image.setPixel( m, n, src );
}

/*!
 * Copy an IndexedA pixel from the layer to the Indexed image. Straight-forward.
 * \param layer source layer.
 * \param i x tile index.
 * \param j y tile index.
 * \param k x pixel index of tile i,j.
 * \param l y pixel index of tile i,j.
 * \param image destination image.
 * \param m x pixel of destination image.
 * \param n y pixel of destination image.
 */
void XCFImageFormat::copyIndexedAToIndexed ( Layer& layer,uint i,uint j,int k,int l,
					     QImage& image, int m, int n )
{
  uchar src = layer.image_tiles[j][i].pixelIndex( k, l );

  uchar src_a = layer.alpha_tiles[j][i].pixelIndex( k, l );

  src_a = INT_MULT( src_a, layer.opacity );

  if ( layer.apply_mask == 1 &&
       layer.mask_tiles.size() > j &&
       layer.mask_tiles[j].size() > i )
    src_a = INT_MULT( src_a,
		      layer.mask_tiles[j][i].pixelIndex( k, l ) );

  if ( src_a > 127 )
    src++;
  else
    src = 0;

  image.setPixel( m, n, src );
}

/*!
 * Copy an IndexedA pixel from the layer to an RGB image. Straight-forward.
 * The only thing this has to take account of is the opacity of the
 * layer. Evidently, the GIMP exporter itself does not actually do this.
 * \param layer source layer.
 * \param i x tile index.
 * \param j y tile index.
 * \param k x pixel index of tile i,j.
 * \param l y pixel index of tile i,j.
 * \param image destination image.
 * \param m x pixel of destination image.
 * \param n y pixel of destination image.
 */
void XCFImageFormat::copyIndexedAToRGB ( Layer& layer, uint i, uint j, int k, int l,
					 QImage& image, int m, int n )
{
  QRgb src = layer.image_tiles[j][i].pixel( k, l );

  uchar src_a = layer.alpha_tiles[j][i].pixelIndex( k, l );

  src_a = INT_MULT( src_a, layer.opacity );

  // Apply the mask (if any)

  if ( layer.apply_mask == 1 && layer.mask_tiles.size() > j &&
       layer.mask_tiles[j].size() > i )
    src_a = INT_MULT( src_a, layer.mask_tiles[j][i].pixelIndex( k, l ) );

  // This is what appears in the GIMP window

  if ( src_a <= 127 )
    src_a = 0;
  else
    src_a = OPAQUE_OPACITY;

  image.setPixel( m, n, qRgba( src, src_a ) );
}

/*!
 * Merge a layer into an image, taking account of the manifold modes.
 * \param xcf_image contains the layer and image to merge.
 */
void XCFImageFormat::mergeLayerIntoImage ( XCFImage& xcf_image )
{
  Layer& layer( xcf_image.layer );
  QImage& image( xcf_image.image );

  PixelMergeOperation merge = 0;

  switch ( layer.type ) {
  case RGB_GIMAGE:
  case RGBA_GIMAGE:
    merge = mergeRGBToRGB; break;
  case GRAY_GIMAGE:
    if ( layer.opacity == OPAQUE_OPACITY )
      merge = mergeGrayToGray;
    else
      merge = mergeGrayToRGB;
    break;
  case GRAYA_GIMAGE:
    if ( xcf_image.image.depth() <= 8 )
      merge = mergeGrayAToGray;
    else
      merge = mergeGrayAToRGB;
    break;
  case INDEXED_GIMAGE:
    merge = mergeIndexedToIndexed; break;
  case INDEXEDA_GIMAGE:
    if ( xcf_image.image.depth() <= 8 )
      merge = mergeIndexedAToIndexed;
    else
      merge = mergeIndexedAToRGB;
  }

  for ( uint j = 0; j < layer.nrows; j++ ) {
    uint y = j * TILE_HEIGHT;
      
    for ( uint i = 0; i < layer.ncols; i++ ) {
      uint x = i * TILE_WIDTH;

      // This seems the best place to apply the dissolve because it
      // depends on the global position of each tile's
      // pixels. Apparently it's the only mode which can apply to a
      // single layer.

      if ( layer.mode == DISSOLVE_MODE ) {
	if ( layer.type == RGBA_GIMAGE )
	  dissolveRGBPixels( layer.image_tiles[j][i], x, y );

	else if ( layer.type == GRAYA_GIMAGE )
	  dissolveAlphaPixels( layer.alpha_tiles[j][i], x, y );
      }

      for ( int l = 0; l < layer.image_tiles[j][i].height(); l++ ) {
	for ( int k = 0; k < layer.image_tiles[j][i].width(); k++ ) {

	  int m = x + k + layer.x_offset;
	  int n = y + l + layer.y_offset;

	  if ( m < 0 || m >= image.width() || n < 0 || n >= image.height() )
	    continue;

	  (*merge)( layer, i, j, k, l, image, m, n );
	}
      }
    }
  }
}

/*!
 * Merge an RGB pixel from the layer to the RGB image. Straight-forward.
 * The only thing this has to take account of is the opacity of the
 * layer. Evidently, the GIMP exporter itself does not actually do this.
 * \param layer source layer.
 * \param i x tile index.
 * \param j y tile index.
 * \param k x pixel index of tile i,j.
 * \param l y pixel index of tile i,j.
 * \param image destination image.
 * \param m x pixel of destination image.
 * \param n y pixel of destination image.
 */
void XCFImageFormat::mergeRGBToRGB ( Layer& layer, uint i, uint j, int k, int l,
				     QImage& image, int m, int n )
{
  QRgb src = layer.image_tiles[j][i].pixel( k, l );
  QRgb dst = image.pixel( m, n );

  uchar src_r = qRed( src );
  uchar src_g = qGreen( src );
  uchar src_b = qBlue( src );
  uchar src_a = qAlpha( src );

  uchar dst_r = qRed( dst );
  uchar dst_g = qGreen( dst );
  uchar dst_b = qBlue( dst );
  uchar dst_a = qAlpha( dst );

  switch ( layer.mode ) {
  case MULTIPLY_MODE: {
    src_r = INT_MULT( src_r, dst_r );
    src_g = INT_MULT( src_g, dst_g );
    src_b = INT_MULT( src_b, dst_b );
    src_a = MIN( src_a, dst_a );
  }
  break;
  case DIVIDE_MODE: {
    src_r = MIN( ( dst_r * 256 ) / ( 1 + src_r ), 255 );
    src_g = MIN( ( dst_g * 256 ) / ( 1 + src_g ), 255 );
    src_b = MIN( ( dst_b * 256 ) / ( 1 + src_b ), 255 );
    src_a = MIN( src_a, dst_a );
  }
  break;
  case SCREEN_MODE: {
    src_r = 255 - INT_MULT( 255 - dst_r, 255 - src_r );
    src_g = 255 - INT_MULT( 255 - dst_g, 255 - src_g );
    src_b = 255 - INT_MULT( 255 - dst_b, 255 - src_b );
    src_a = MIN( src_a, dst_a );
  }
  break;
  case OVERLAY_MODE: {
    src_r = INT_MULT( dst_r, dst_r + INT_MULT( 2 * src_r, 255 - dst_r ) );
    src_g = INT_MULT( dst_g, dst_g + INT_MULT( 2 * src_g, 255 - dst_g ) );
    src_b = INT_MULT( dst_b, dst_b + INT_MULT( 2 * src_b, 255 - dst_b ) );
    src_a = MIN( src_a, dst_a );
  }
  break;
  case DIFFERENCE_MODE: {
    src_r = dst_r > src_r ? dst_r - src_r : src_r - dst_r;
    src_g = dst_g > src_g ? dst_g - src_g : src_g - dst_g;
    src_b = dst_b > src_b ? dst_b - src_b : src_b - dst_b;
    src_a = MIN( src_a, dst_a );
  }
  break;
  case ADDITION_MODE: {
    src_r = add_lut[dst_r][src_r];
    src_g = add_lut[dst_g][src_g];
    src_b = add_lut[dst_b][src_b];
    src_a = MIN( src_a, dst_a );
  }
  break;
  case SUBTRACT_MODE: {
    src_r = dst_r > src_r ? dst_r - src_r : 0;
    src_g = dst_g > src_g ? dst_g - src_g : 0;
    src_b = dst_b > src_b ? dst_b - src_b : 0;
    src_a = MIN( src_a, dst_a );
  }
  break;
  case DARKEN_ONLY_MODE: {
    src_r = dst_r < src_r ? dst_r : src_r;
    src_g = dst_g < src_g ? dst_g : src_g;
    src_b = dst_b < src_b ? dst_b : src_b;
    src_a = MIN( src_a, dst_a );
  }
  break;
  case LIGHTEN_ONLY_MODE: {
    src_r = dst_r < src_r ? src_r : dst_r;
    src_g = dst_g < src_g ? src_g : dst_g;
    src_b = dst_b < src_b ? src_b : dst_b;
    src_a = MIN( src_a, dst_a );
  }
  break;
  case HUE_MODE: {
    uchar new_r = dst_r;
    uchar new_g = dst_g;
    uchar new_b = dst_b;

    RGBTOHSV( src_r, src_g, src_b );
    RGBTOHSV( new_r, new_g, new_b );

    new_r = src_r;

    HSVTORGB( new_r, new_g, new_b );

    src_r = new_r;
    src_g = new_g;
    src_b = new_b;
    src_a = MIN( src_a, dst_a );
  }
  break;
  case SATURATION_MODE: {
    uchar new_r = dst_r;
    uchar new_g = dst_g;
    uchar new_b = dst_b;

    RGBTOHSV( src_r, src_g, src_b );
    RGBTOHSV( new_r, new_g, new_b );

    new_g = src_g;

    HSVTORGB( new_r, new_g, new_b );

    src_r = new_r;
    src_g = new_g;
    src_b = new_b;
    src_a = MIN( src_a, dst_a );
  }
  break;
  case VALUE_MODE: {
    uchar new_r = dst_r;
    uchar new_g = dst_g;
    uchar new_b = dst_b;

    RGBTOHSV( src_r, src_g, src_b );
    RGBTOHSV( new_r, new_g, new_b );

    new_b = src_b;

    HSVTORGB( new_r, new_g, new_b );

    src_r = new_r;
    src_g = new_g;
    src_b = new_b;
    src_a = MIN( src_a, dst_a );
  }
  break;
  case COLOR_MODE: {
    uchar new_r = dst_r;
    uchar new_g = dst_g;
    uchar new_b = dst_b;

    RGBTOHLS( src_r, src_g, src_b );
    RGBTOHLS( new_r, new_g, new_b );

    new_r = src_r;
    new_b = src_b;

    HLSTORGB( new_r, new_g, new_b );

    src_r = new_r;
    src_g = new_g;
    src_b = new_b;
    src_a = MIN( src_a, dst_a );
  }
  break;
  }

  src_a = INT_MULT( src_a, layer.opacity );

  // Apply the mask (if any)

  if ( layer.apply_mask == 1 && layer.mask_tiles.size() > j &&
       layer.mask_tiles[j].size() > i )
    src_a = INT_MULT( src_a, layer.mask_tiles[j][i].pixelIndex( k, l ) );

  uchar new_r, new_g, new_b, new_a;

  new_a = dst_a + INT_MULT( OPAQUE_OPACITY - dst_a, src_a );

  float src_ratio = (float)src_a / new_a;
  float dst_ratio = 1. - src_ratio;

  new_r = (uchar)( src_ratio * src_r + dst_ratio * dst_r + EPSILON );
  new_g = (uchar)( src_ratio * src_g + dst_ratio * dst_g + EPSILON );
  new_b = (uchar)( src_ratio * src_b + dst_ratio * dst_b + EPSILON );

  if ( !layer_modes[layer.mode].affect_alpha )
    new_a = dst_a;

  image.setPixel( m, n, qRgba( new_r, new_g, new_b, new_a ) );
}

/*!
 * Merge a Gray pixel from the layer to the Gray image. Straight-forward.
 * \param layer source layer.
 * \param i x tile index.
 * \param j y tile index.
 * \param k x pixel index of tile i,j.
 * \param l y pixel index of tile i,j.
 * \param image destination image.
 * \param m x pixel of destination image.
 * \param n y pixel of destination image.
 */
void XCFImageFormat::mergeGrayToGray ( Layer& layer, uint i, uint j, int k, int l,
				       QImage& image, int m, int n )
{
  int src = layer.image_tiles[j][i].pixelIndex( k, l );

  image.setPixel( m, n, src );
}

/*!
 * Merge a GrayA pixel from the layer to the Gray image. Straight-forward.
 * \param layer source layer.
 * \param i x tile index.
 * \param j y tile index.
 * \param k x pixel index of tile i,j.
 * \param l y pixel index of tile i,j.
 * \param image destination image.
 * \param m x pixel of destination image.
 * \param n y pixel of destination image.
 */
void XCFImageFormat::mergeGrayAToGray ( Layer& layer, uint i, uint j, int k, int l,
					QImage& image, int m, int n )
{
  int src = qGray( layer.image_tiles[j][i].pixel( k, l ) );
  int dst = image.pixelIndex( m, n );

  uchar src_a = layer.alpha_tiles[j][i].pixelIndex( k, l );

  switch ( layer.mode ) {
  case MULTIPLY_MODE: {
    src = INT_MULT( src, dst );
  }
  break;
  case DIVIDE_MODE: {
    src = MIN( ( dst * 256 ) / ( 1 + src ), 255 );
  }
  break;
  case SCREEN_MODE: {
    src = 255 - INT_MULT( 255 - dst, 255 - src );
  }
  break;
  case OVERLAY_MODE: {
    src = INT_MULT( dst, dst + INT_MULT( 2 * src, 255 - dst ) );
  }
  break;
  case DIFFERENCE_MODE: {
    src = dst > src ? dst - src : src - dst;
  }
  break;
  case ADDITION_MODE: {
    src = add_lut[dst][src];
  }
  break;
  case SUBTRACT_MODE: {
    src = dst > src ? dst - src : 0;
  }
  break;
  case DARKEN_ONLY_MODE: {
    src = dst < src ? dst : src;
  }
  break;
  case LIGHTEN_ONLY_MODE: {
    src = dst < src ? src : dst;
  }
  break;
  }

  src_a = INT_MULT( src_a, layer.opacity );

  // Apply the mask (if any)

  if ( layer.apply_mask == 1 && layer.mask_tiles.size() > j &&
       layer.mask_tiles[j].size() > i )
    src_a = INT_MULT( src_a, layer.mask_tiles[j][i].pixelIndex( k, l ) );

  uchar new_a = OPAQUE_OPACITY;

  float src_ratio = (float)src_a / new_a;
  float dst_ratio = 1. - src_ratio;

  uchar new_g = (uchar)( src_ratio * src + dst_ratio * dst + EPSILON );

  image.setPixel( m, n, new_g );
}

/*!
 * Merge a Gray pixel from the layer to an RGB image. Straight-forward.
 * The only thing this has to take account of is the opacity of the
 * layer. Evidently, the GIMP exporter itself does not actually do this.
 * \param layer source layer.
 * \param i x tile index.
 * \param j y tile index.
 * \param k x pixel index of tile i,j.
 * \param l y pixel index of tile i,j.
 * \param image destination image.
 * \param m x pixel of destination image.
 * \param n y pixel of destination image.
 */
void XCFImageFormat::mergeGrayToRGB ( Layer& layer, uint i, uint j, int k, int l,
				      QImage& image, int m, int n )
{
  QRgb src = layer.image_tiles[j][i].pixel( k, l );

  uchar src_a = layer.opacity;

  image.setPixel( m, n, qRgba( src, src_a ) );
}

/*!
 * Merge a GrayA pixel from the layer to an RGB image. Straight-forward.
 * The only thing this has to take account of is the opacity of the
 * layer. Evidently, the GIMP exporter itself does not actually do this.
 * \param layer source layer.
 * \param i x tile index.
 * \param j y tile index.
 * \param k x pixel index of tile i,j.
 * \param l y pixel index of tile i,j.
 * \param image destination image.
 * \param m x pixel of destination image.
 * \param n y pixel of destination image.
 */
void XCFImageFormat::mergeGrayAToRGB ( Layer& layer, uint i, uint j, int k, int l,
				       QImage& image, int m, int n )
{
  int src = qGray( layer.image_tiles[j][i].pixel( k, l ) );
  int dst = qGray( image.pixel( m, n ) );

  uchar src_a = layer.alpha_tiles[j][i].pixelIndex( k, l );
  uchar dst_a = qAlpha( image.pixel( m, n ) );

  switch ( layer.mode ) {
  case MULTIPLY_MODE: {
    src = INT_MULT( src, dst );
    src_a = MIN( src_a, dst_a );
  }
  break;
  case DIVIDE_MODE: {
    src = MIN( ( dst * 256 ) / ( 1 + src ), 255 );
    src_a = MIN( src_a, dst_a );
  }
  break;
  case SCREEN_MODE: {
    src = 255 - INT_MULT( 255 - dst, 255 - src );
    src_a = MIN( src_a, dst_a );
  }
  break;
  case OVERLAY_MODE: {
    src = INT_MULT( dst, dst + INT_MULT( 2 * src, 255 - dst ) );
    src_a = MIN( src_a, dst_a );
  }
  break;
  case DIFFERENCE_MODE: {
    src = dst > src ? dst - src : src - dst;
    src_a = MIN( src_a, dst_a );
  }
  break;
  case ADDITION_MODE: {
    src = add_lut[dst][src];
    src_a = MIN( src_a, dst_a );
  }
  break;
  case SUBTRACT_MODE: {
    src = dst > src ? dst - src : 0;
    src_a = MIN( src_a, dst_a );
  }
  break;
  case DARKEN_ONLY_MODE: {
    src = dst < src ? dst : src;
    src_a = MIN( src_a, dst_a );
  }
  break;
  case LIGHTEN_ONLY_MODE: {
    src = dst < src ? src : dst;
    src_a = MIN( src_a, dst_a );
  }
  break;
  }

  src_a = INT_MULT( src_a, layer.opacity );

  // Apply the mask (if any)

  if ( layer.apply_mask == 1 && layer.mask_tiles.size() > j &&
       layer.mask_tiles[j].size() > i )
    src_a = INT_MULT( src_a, layer.mask_tiles[j][i].pixelIndex( k, l ) );

  uchar new_a = dst_a + INT_MULT( OPAQUE_OPACITY - dst_a, src_a );

  float src_ratio = (float)src_a / new_a;
  float dst_ratio = 1. - src_ratio;

  uchar new_g = (uchar)( src_ratio * src + dst_ratio * dst + EPSILON );

  if ( !layer_modes[layer.mode].affect_alpha )
    new_a = dst_a;

  image.setPixel( m, n, qRgba( new_g, new_g, new_g, new_a ) );
}

/*!
 * Merge an Indexed pixel from the layer to the Indexed image. Straight-forward.
 * \param layer source layer.
 * \param i x tile index.
 * \param j y tile index.
 * \param k x pixel index of tile i,j.
 * \param l y pixel index of tile i,j.
 * \param image destination image.
 * \param m x pixel of destination image.
 * \param n y pixel of destination image.
 */
void XCFImageFormat::mergeIndexedToIndexed ( Layer& layer, uint i,uint j,int k,int l,
					     QImage& image, int m, int n )
{
  int src = layer.image_tiles[j][i].pixelIndex( k, l );

  image.setPixel( m, n, src );
}

/*!
 * Merge an IndexedA pixel from the layer to the Indexed image. Straight-forward.
 * \param layer source layer.
 * \param i x tile index.
 * \param j y tile index.
 * \param k x pixel index of tile i,j.
 * \param l y pixel index of tile i,j.
 * \param image destination image.
 * \param m x pixel of destination image.
 * \param n y pixel of destination image.
 */
void XCFImageFormat::mergeIndexedAToIndexed ( Layer& layer,uint i,uint j,int k,int l,
					      QImage& image, int m, int n )
{
  uchar src = layer.image_tiles[j][i].pixelIndex( k, l );

  uchar src_a = layer.alpha_tiles[j][i].pixelIndex( k, l );

  src_a = INT_MULT( src_a, layer.opacity );

  if ( layer.apply_mask == 1 &&
       layer.mask_tiles.size() > j &&
       layer.mask_tiles[j].size() > i )
    src_a = INT_MULT( src_a,
		      layer.mask_tiles[j][i].pixelIndex( k, l ) );

  if ( src_a > 127 ) {
    src++;
    image.setPixel( m, n, src );
  }
}

/*!
 * Merge an IndexedA pixel from the layer to an RGB image. Straight-forward.
 * The only thing this has to take account of is the opacity of the
 * layer. Evidently, the GIMP exporter itself does not actually do this.
 * \param layer source layer.
 * \param i x tile index.
 * \param j y tile index.
 * \param k x pixel index of tile i,j.
 * \param l y pixel index of tile i,j.
 * \param image destination image.
 * \param m x pixel of destination image.
 * \param n y pixel of destination image.
 */
void XCFImageFormat::mergeIndexedAToRGB ( Layer& layer, uint i, uint j, int k, int l,
					  QImage& image, int m, int n )
{
  QRgb src = layer.image_tiles[j][i].pixel( k, l );

  uchar src_a = layer.alpha_tiles[j][i].pixelIndex( k, l );

  src_a = INT_MULT( src_a, layer.opacity );

  // Apply the mask (if any)

  if ( layer.apply_mask == 1 && layer.mask_tiles.size() > j &&
       layer.mask_tiles[j].size() > i )
    src_a = INT_MULT( src_a, layer.mask_tiles[j][i].pixelIndex( k, l ) );

  // This is what appears in the GIMP window

  if ( src_a <= 127 )
    src_a = 0;
  else
    src_a = OPAQUE_OPACITY;

  image.setPixel( m, n, qRgba( src, src_a ) );
}

/*!
 * Dissolving pixels: pick a random number between 0 and 255. If the pixel's
 * alpha is less than that, make it transparent.
 * \param image the image tile to dissolve.
 * \param x the global x position of the tile.
 * \param y the global y position of the tile.
 */
void XCFImageFormat::dissolveRGBPixels ( QImage& image, int x, int y )
{
  // The apparently spurious rand() calls are to wind the random
  // numbers up to the same point for each tile.

  for ( int l = 0; l < image.height(); l++ ) {
    srand( random_table[( l + y ) % RANDOM_TABLE_SIZE] );

    for ( int k = 0; k < x; k++ )
      rand();

    for ( int k = 0; k < image.width(); k++ ) {
      int rand_val = rand() & 0xff;
      QRgb pixel = image.pixel( k, l );

      if ( rand_val > qAlpha( pixel ) ) {
	image.setPixel( k, l, qRgba( pixel, 0 ) );
      }
    }
  }
}

/*!
 * Dissolving pixels: pick a random number between 0 and 255. If the pixel's
 * alpha is less than that, make it transparent. This routine works for
 * the GRAYA and INDEXEDA image types where the pixel alpha's are stored
 * separately from the pixel themselves.
 * \param image the alpha tile to dissolve.
 * \param x the global x position of the tile.
 * \param y the global y position of the tile.
 */
void XCFImageFormat::dissolveAlphaPixels ( QImage& image, int x, int y )
{
  // The apparently spurious rand() calls are to wind the random
  // numbers up to the same point for each tile.

  for ( int l = 0; l < image.height(); l++ ) {
    srand( random_table[( l + y ) % RANDOM_TABLE_SIZE] );

    for ( int k = 0; k < x; k++ )
      rand();

    for ( int k = 0; k < image.width(); k++ ) {
      int rand_val = rand() & 0xff;
      uchar alpha = image.pixelIndex( k, l );

      if ( rand_val > alpha ) {
 image.setPixel( k, l, 0 );
      }
    }
  }
}

KDE_Q_EXPORT_PLUGIN( XCFImageFormat )

} // namespace
