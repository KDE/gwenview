// This code is ImageMagick's resize code, adapted for QImage, with
// fastfloat class added as an optimization.
// The original license text follows.

/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                 RRRR   EEEEE  SSSSS  IIIII  ZZZZZ  EEEEE                    %
%                 R   R  E      SS       I       ZZ  E                        %
%                 RRRR   EEE     SSS     I     ZZZ   EEE                      %
%                 R R    E         SS    I    ZZ     E                        %
%                 R  R   EEEEE  SSSSS  IIIII  ZZZZZ  EEEEE                    %
%                                                                             %
%                     ImageMagick Image Resize Methods                        %
%                                                                             %
%                                                                             %
%                              Software Design                                %
%                                John Cristy                                  %
%                                 July 1992                                   %
%                                                                             %
%                                                                             %
%  Copyright (C) 2003 ImageMagick Studio, a non-profit organization dedicated %
%  to making software imaging solutions freely available.                     %
%                                                                             %
%  Permission is hereby granted, free of charge, to any person obtaining a    %
%  copy of this software and associated documentation files ("ImageMagick"),  %
%  to deal in ImageMagick without restriction, including without limitation   %
%  the rights to use, copy, modify, merge, publish, distribute, sublicense,   %
%  and/or sell copies of ImageMagick, and to permit persons to whom the       %
%  ImageMagick is furnished to do so, subject to the following conditions:    %
%                                                                             %
%  The above copyright notice and this permission notice shall be included in %
%  all copies or substantial portions of ImageMagick.                         %
%                                                                             %
%  The software is provided "as is", without warranty of any kind, express or %
%  implied, including but not limited to the warranties of merchantability,   %
%  fitness for a particular purpose and noninfringement.  In no event shall   %
%  ImageMagick Studio be liable for any claim, damages or other liability,    %
%  whether in an action of contract, tort or otherwise, arising from, out of  %
%  or in connection with ImageMagick or the use or other dealings in          %
%  ImageMagick.                                                               %
%                                                                             %
%  Except as contained in this notice, the name of the ImageMagick Studio     %
%  shall not be used in advertising or otherwise to promote the sale, use or  %
%  other dealings in ImageMagick without prior written authorization from the %
%  ImageMagick Studio.                                                        %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%
*/

// Qt
#include <qimage.h>
#include <qcolor.h>

// Local
#include "gvimageutils.h"

// everything in namespace
namespace GVImageUtils {


#define Max QMAX
#define Min QMIN

// mustn't be less than used precision (i.e. 1/fastfloat::RATIO)
#define MagickEpsilon 0.0002

// fastfloat begin
// this class stores floating point numbers as integers, with BITS shift,
// i.e. value XYZ is stored as XYZ * RATIO
struct fastfloat
    {
    private:
        enum { BITS = 12, RATIO = 4096 };
    public:
        fastfloat() {}
        fastfloat( long v ) : value( v << BITS ) {}
        fastfloat( int v ) : value( v << BITS ) {}
        fastfloat( double v ) : value( static_cast< long >( v * RATIO + 0.5 )) {}
        double toDouble() const { return static_cast< double >( value ) / RATIO; }
        long toLong() const { return value >> BITS; }
        fastfloat& operator += ( fastfloat r ) { value += r.value; return *this; }
        fastfloat& operator -= ( fastfloat r ) { value -= r.value; return *this; }
        fastfloat& operator *= ( fastfloat r ) { value = static_cast< long long >( value ) * r.value >> BITS; return *this; }
        fastfloat& operator /= ( fastfloat r ) { value = ( static_cast< long long >( value ) << BITS ) / r.value; return *this; }
        bool operator< ( fastfloat r ) const { return value < r.value; }
        bool operator<= ( fastfloat r ) const { return value <= r.value; }
        bool operator> ( fastfloat r ) const { return value > r.value; }
        bool operator>= ( fastfloat r ) const { return value >= r.value; }
        bool operator== ( fastfloat r ) const { return value == r.value; }
        bool operator!= ( fastfloat r ) const { return value != r.value; }
        fastfloat operator-() const { return fastfloat( -value, false ); }
    private:
        fastfloat( long v, bool ) : value( v ) {} // for operator-()
        long value;
    };

inline fastfloat operator+ ( fastfloat l, fastfloat r ) { return fastfloat( l ) += r; }
inline fastfloat operator- ( fastfloat l, fastfloat r ) { return fastfloat( l ) -= r; }
inline fastfloat operator* ( fastfloat l, fastfloat r ) { return fastfloat( l ) *= r; }
inline fastfloat operator/ ( fastfloat l, fastfloat r ) { return fastfloat( l ) /= r; }

inline bool operator< ( fastfloat l, double r ) { return l < fastfloat( r ); }
inline bool operator<= ( fastfloat l, double r ) { return l <= fastfloat( r ); }
inline bool operator> ( fastfloat l, double r ) { return l > fastfloat( r ); }
inline bool operator>= ( fastfloat l, double r ) { return l >= fastfloat( r ); }
inline bool operator== ( fastfloat l, double r ) { return l == fastfloat( r ); }
inline bool operator!= ( fastfloat l, double r ) { return l != fastfloat( r ); }

inline bool operator< ( double l, fastfloat r ) { return fastfloat( l ) < r ; }
inline bool operator<= ( double l, fastfloat r ) { return fastfloat( l ) <= r ; }
inline bool operator> ( double l, fastfloat r ) { return fastfloat( l ) > r ; }
inline bool operator>= ( double l, fastfloat r ) { return fastfloat( l ) >= r ; }
inline bool operator== ( double l, fastfloat r ) { return fastfloat( l ) == r ; }
inline bool operator!= ( double l, fastfloat r ) { return fastfloat( l ) != r ; }

inline double fasttodouble( fastfloat v ) { return v.toDouble(); }
inline long fasttolong( fastfloat v ) { return v.toLong(); }

#if 1  // change to 0 to turn fastfloat usage off
static const fastfloat fasthalf( 0.5 );
#define half fasthalf
#else
#define fastfloat double
#define fasttodouble( v ) double( v )
#define fasttolong( v ) long( v )
#define half 0.5
#endif

//fastfloat end


typedef fastfloat (*Filter)(const fastfloat, const fastfloat);

typedef struct _ContributionInfo
{
  fastfloat
    weight;

  long
    pixel;
} ContributionInfo;


/*
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%                                                                             %
%                                                                             %
%                                                                             %
%   R e s i z e I m a g e                                                     %
%                                                                             %
%                                                                             %
%                                                                             %
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%
%  ResizeImage() scales an image to the desired dimensions with one of these
%  filters:
%
%    Bessel   Blackman   Box
%    Catrom   Cubic      Gaussian
%    Hanning  Hermite    Lanczos
%    Mitchell Point      Quandratic
%    Sinc     Triangle
%
%  Most of the filters are FIR (finite impulse response), however, Bessel,
%  Gaussian, and Sinc are IIR (infinite impulse response).  Bessel and Sinc
%  are windowed (brought down to zero) with the Blackman filter.
%
%  ResizeImage() was inspired by Paul Heckbert's zoom program.
%
%  The format of the ResizeImage method is:
%
%      Image *ResizeImage(Image *image,const unsigned long columns,
%        const unsigned long rows,const FilterTypes filter,const double blur,
%        ExceptionInfo *exception)
%
%  A description of each parameter follows:
%
%    o image: The image.
%
%    o columns: The number of columns in the scaled image.
%
%    o rows: The number of rows in the scaled image.
%
%    o filter: Image filter to use.
%
%    o blur: The blur factor where > 1 is blurry, < 1 is sharp.
%
%    o exception: Return any errors or warnings in this structure.
%
%
*/

#if 0
static fastfloat Bessel(const fastfloat x,const fastfloat)
{
  if (x == 0.0)
    return(MagickPI/4.0);
  return(BesselOrderOne(MagickPI*x)/(2.0*x));
}

static fastfloat Sinc(const fastfloat x,const fastfloat)
{
  if (x == 0.0)
    return(1.0);
  return(sin(MagickPI*x)/(MagickPI*x));
}

static fastfloat Blackman(const fastfloat x,const fastfloat)
{
  return(0.42+0.5*cos(MagickPI*x)+0.08*cos(2*MagickPI*x));
}

static fastfloat BlackmanBessel(const fastfloat x,const fastfloat)
{
  return(Blackman(x/support,support)*Bessel(x,support));
}

static fastfloat BlackmanSinc(const fastfloat x,const fastfloat)
{
  return(Blackman(x/support,support)*Sinc(x,support));
}
#endif

static fastfloat Box(const fastfloat x,const fastfloat)
{
  if (x < -half)
    return(0.0);
  if (x < half)
    return(1.0);
  return(0.0);
}

#if 0
static fastfloat Catrom(const fastfloat x,const fastfloat)
{
  if (x < -2.0)
    return(0.0);
  if (x < -1.0)
    return(0.5*(4.0+x*(8.0+x*(5.0+x))));
  if (x < 0.0)
    return(0.5*(2.0+x*x*(-5.0-3.0*x)));
  if (x < 1.0)
    return(0.5*(2.0+x*x*(-5.0+3.0*x)));
  if (x < 2.0)
    return(0.5*(4.0+x*(-8.0+x*(5.0-x))));
  return(0.0);
}

static fastfloat Cubic(const fastfloat x,const fastfloat)
{
  if (x < -2.0)
    return(0.0);
  if (x < -1.0)
    return((2.0+x)*(2.0+x)*(2.0+x)/6.0);
  if (x < 0.0)
    return((4.0+x*x*(-6.0-3.0*x))/6.0);
  if (x < 1.0)
    return((4.0+x*x*(-6.0+3.0*x))/6.0);
  if (x < 2.0)
    return((2.0-x)*(2.0-x)*(2.0-x)/6.0);
  return(0.0);
}

static fastfloat Gaussian(const fastfloat x,const fastfloat)
{
  return(exp(-2.0*x*x)*sqrt(2.0/MagickPI));
}

static fastfloat Hanning(const fastfloat x,const fastfloat)
{
  return(0.5+0.5*cos(MagickPI*x));
}

static fastfloat Hamming(const fastfloat x,const fastfloat)
{
  return(0.54+0.46*cos(MagickPI*x));
}

static fastfloat Hermite(const fastfloat x,const fastfloat)
{
  if (x < -1.0)
    return(0.0);
  if (x < 0.0)
    return((2.0*(-x)-3.0)*(-x)*(-x)+1.0);
  if (x < 1.0)
    return((2.0*x-3.0)*x*x+1.0);
  return(0.0);
}

static fastfloat Lanczos(const fastfloat x,const fastfloat support)
{
  if (x < -3.0)
    return(0.0);
  if (x < 0.0)
    return(Sinc(-x,support)*Sinc(-x/3.0,support));
  if (x < 3.0)
    return(Sinc(x,support)*Sinc(x/3.0,support));
  return(0.0);
}
#endif

static fastfloat Mitchell(const fastfloat x,const fastfloat)
{
#define B   (1.0/3.0)
#define C   (1.0/3.0)
#define P0  ((  6.0- 2.0*B       )/6.0)
#define P2  ((-18.0+12.0*B+ 6.0*C)/6.0)
#define P3  (( 12.0- 9.0*B- 6.0*C)/6.0)
#define Q0  ((       8.0*B+24.0*C)/6.0)
#define Q1  ((     -12.0*B-48.0*C)/6.0)
#define Q2  ((       6.0*B+30.0*C)/6.0)
#define Q3  ((     - 1.0*B- 6.0*C)/6.0)

  if (x < -2.0)
    return(0.0);
  if (x < -1.0)
    return(Q0-x*(Q1-x*(Q2-x*Q3)));
  if (x < 0.0)
    return(P0+x*x*(P2-x*P3));
  if (x < 1.0)
    return(P0+x*x*(P2+x*P3));
  if (x < 2.0)
    return(Q0+x*(Q1+x*(Q2+x*Q3)));
  return(0.0);
}

#if 0
static fastfloat Quadratic(const fastfloat x,const fastfloat)
{
  if (x < -1.5)
    return(0.0);
  if (x < -0.5)
    return(0.5*(x+1.5)*(x+1.5));
  if (x < 0.5)
    return(0.75-x*x);
  if (x < 1.5)
    return(0.5*(x-1.5)*(x-1.5));
  return(0.0);
}
#endif

static fastfloat Triangle(const fastfloat x,const fastfloat)
{
  if (x < -1.0)
    return(0.0);
  if (x < 0.0)
    return(1.0+x);
  if (x < 1.0)
    return(1.0-x);
  return(0.0);
}

static void HorizontalFilter(const QImage& source,QImage& destination,
  const fastfloat x_factor,const fastfloat blur,
  ContributionInfo *contribution, Filter filter, fastfloat filtersupport)
{
  fastfloat
    center,
    density,
    scale,
    support;

  long
    n,
    start,
    stop,
    y;

  register long
    i,
    x;

  /*
    Apply filter to resize horizontally from source to destination.
  */
  scale=blur*Max(1.0/x_factor,1.0);
  support=scale* filtersupport;
  if (support <= half)
    {
      /*
        Reduce to point sampling.
      */
      support=half+MagickEpsilon;
      scale=1.0;
    }
  scale=1.0/scale;
  for (x=0; x < (long) destination.width(); x++)
  {
    center=(fastfloat) (x+half)/x_factor;
    start= fasttolong(Max(center-support+half,0));
    stop= fasttolong(Min(center+support+half,source.width()));
    density=0.0;
    for (n=0; n < (stop-start); n++)
    {
      contribution[n].pixel=start+n;
      contribution[n].weight=
        filter (scale*(start+n-center+half), filtersupport );
      density+=contribution[n].weight;
    }
    if ((density != 0.0) && (density != 1.0))
      {
        /*
          Normalize.
        */
        density=1.0/density;
        for (i=0; i < n; i++)
          contribution[i].weight*=density;
      }
//    p=AcquireImagePixels(source,contribution[0].pixel,0,contribution[n-1].pixel-
//      contribution[0].pixel+1,source->rows,exception);
//    q=SetImagePixels(destination,x,0,1,destination->rows);
    for (y=0; y < (long) destination.height(); y++)
    {
      fastfloat red = 0;
      fastfloat green = 0;
      fastfloat blue = 0;
      fastfloat alpha = 0;
      for (i=0; i < n; i++)
      {
        int px = contribution[i].pixel;
        int py = y;
        QRgb p = reinterpret_cast< QRgb* >( source.jumpTable()[ py ])[ px ];
        red+=contribution[i].weight*qRed(p);
        green+=contribution[i].weight*qGreen(p);
        blue+=contribution[i].weight*qBlue(p);
        alpha+=contribution[i].weight*qAlpha(p);
      }
      QRgb pix = qRgba(
          fasttolong( red < 0 ? 0 : red > 255 ? 255 : red + half ),
          fasttolong( green < 0 ? 0 : green > 255 ? 255 : green + half ),
          fasttolong( blue < 0 ? 0 : blue > 255 ? 255 : blue + half ),
          fasttolong( alpha < 0 ? 0 : alpha > 255 ? 255 : alpha + half ));
      reinterpret_cast< QRgb* >( destination.jumpTable()[ y ])[ x ] = pix;
    }
  }
}

static void VerticalFilter(const QImage& source,QImage& destination,
  const fastfloat y_factor,const fastfloat blur,
  ContributionInfo *contribution, Filter filter, fastfloat filtersupport )
{
  fastfloat
    center,
    density,
    scale,
    support;

  long
    n,
    start,
    stop,
    x;

  register long
    i,
    y;

  /*
    Apply filter to resize vertically from source to destination.
  */
  scale=blur*Max(1.0/y_factor,1.0);
  support=scale* filtersupport;
  if (support <= half)
    {
      /*
        Reduce to point sampling.
      */
      support=half+MagickEpsilon;
      scale=1.0;
    }
  scale=1.0/scale;
  for (y=0; y < (long) destination.height(); y++)
  {
    center=(fastfloat) (y+half)/y_factor;
    start= fasttolong(Max(center-support+half,0));
    stop= fasttolong(Min(center+support+half,source.height()));
    density=0.0;
    for (n=0; n < (stop-start); n++)
    {
      contribution[n].pixel=start+n;
      contribution[n].weight=
        filter (scale*(start+n-center+half), filtersupport);
      density+=contribution[n].weight;
    }
    if ((density != 0.0) && (density != 1.0))
      {
        /*
          Normalize.
        */
        density=1.0/density;
        for (i=0; i < n; i++)
          contribution[i].weight*=density;
      }
//    p=AcquireImagePixels(source,0,contribution[0].pixel,source->columns,
//      contribution[n-1].pixel-contribution[0].pixel+1,exception);
//    q=SetImagePixels(destination,0,y,destination->columns,1);
    for (x=0; x < (long) destination.width(); x++)
    {
      fastfloat red = 0;
      fastfloat green = 0;
      fastfloat blue = 0;
      fastfloat alpha = 0;
      for (i=0; i < n; i++)
      {
        int px = x;
        int py = contribution[i].pixel;
        QRgb p = reinterpret_cast< QRgb* >( source.jumpTable()[ py ])[ px ];
        red+=contribution[i].weight*qRed(p);
        green+=contribution[i].weight*qGreen(p);
        blue+=contribution[i].weight*qBlue(p);
        alpha+=contribution[i].weight*qAlpha(p);
      }
      QRgb pix = qRgba(
          fasttolong( red < 0 ? 0 : red > 255 ? 255 : red + half ),
          fasttolong( green < 0 ? 0 : green > 255 ? 255 : green + half ),
          fasttolong( blue < 0 ? 0 : blue > 255 ? 255 : blue + half ),
          fasttolong( alpha < 0 ? 0 : alpha > 255 ? 255 : alpha + half ));
      reinterpret_cast< QRgb* >( destination.jumpTable()[ y ])[ x ] = pix;
    }
  }
}

QImage ResizeImage(const QImage& image,const int columns,
  const int rows, Filter filter, fastfloat filtersupport, double blur)
{
  ContributionInfo
    *contribution;

  fastfloat
    support,
    x_factor,
    x_support,
    y_factor,
    y_support;

  /*
    Initialize resize image attributes.
  */
  if ((columns == image.width()) && (rows == image.height()) && (blur == 1.0))
    return image.copy();
  QImage resize_image( columns, rows, 32 );
  resize_image.setAlphaBuffer( image.hasAlphaBuffer());
  resize_image.fill( Qt::red.rgb() );
  /*
    Allocate filter contribution info.
  */
  x_factor=(fastfloat) resize_image.width()/image.width();
  y_factor=(fastfloat) resize_image.height()/image.height();
//  i=(long) LanczosFilter;
//  if (image->filter != UndefinedFilter)
//    i=(long) image->filter;
//  else
//    if ((image->storage_class == PseudoClass) || image->matte ||
//        ((x_factor*y_factor) > 1.0))
//      i=(long) MitchellFilter;
  x_support=blur*Max(1.0/x_factor,1.0)*filtersupport;
  y_support=blur*Max(1.0/y_factor,1.0)*filtersupport;
  support=Max(x_support,y_support);
  if (support < filtersupport)
    support=filtersupport;
  contribution=new ContributionInfo[ fasttolong( 2.0*Max(support,half)+3 ) ];
  Q_CHECK_PTR( contribution );
  /*
    Resize image.
  */
  if (((fastfloat) columns*(image.height()+rows)) >
      ((fastfloat) rows*(image.width()+columns)))
    {
      QImage source_image( columns, image.height(), 32 );
      source_image.setAlphaBuffer( image.hasAlphaBuffer());
      source_image.fill( Qt::yellow.rgb() );
      HorizontalFilter(image,source_image,x_factor,blur,
        contribution,filter,filtersupport);
      VerticalFilter(source_image,resize_image,y_factor,
        blur,contribution,filter,filtersupport);
    }
  else
    {
      QImage source_image( image.width(), rows, 32 );
      source_image.setAlphaBuffer( image.hasAlphaBuffer());
      source_image.fill( Qt::yellow.rgb() );
      VerticalFilter(image,source_image,y_factor,blur,
        contribution,filter,filtersupport);
      HorizontalFilter(source_image,resize_image,x_factor,
        blur,contribution,filter,filtersupport);
    }
  /*
    Free allocated memory.
  */
  delete[] contribution;
  return(resize_image);
}


#undef Max
#undef Min
#undef MagickEpsilon


// filters and their matching support values
#if 0
  static const FilterInfo
    filters[SincFilter+1] =
    {
      { Box, 0.0 },
      { Box, 0.0 },
      { Box, 0.5 },
      { Triangle, 1.0 },
      { Hermite, 1.0 },
      { Hanning, 1.0 },
      { Hamming, 1.0 },
      { Blackman, 1.0 },
      { Gaussian, 1.25 },
      { Quadratic, 1.5 },
      { Cubic, 2.0 },
      { Catrom, 2.0 },
      { Mitchell, 2.0 },
      { Lanczos, 3.0 },
      { BlackmanBessel, 3.2383 },
      { BlackmanSinc, 4.0 }
    };
#endif


// the only public function here
QImage scale(const QImage& image, int width, int height,
	SmoothAlgorithm alg, QImage::ScaleMode mode, double blur )
{
	if( image.isNull()) return image.copy();
	
	QSize newSize( image.size() );
	newSize.scale( QSize( width, height ), (QSize::ScaleMode)mode ); // ### remove cast in Qt 4.0
	newSize = newSize.expandedTo( QSize( 1, 1 )); // make sure it doesn't become null

	if ( newSize == image.size() ) return image.copy();
	
	width = newSize.width();
	height = newSize.height();
	Filter filter = NULL;
	fastfloat filtersupport;
	
	switch( alg ) {
	case SMOOTH_NONE:
		filter = NULL;
		filtersupport = 0.0;
		break;
	case SMOOTH_FAST:
		filter = Box;
		filtersupport = 0.5;
		break;
	case SMOOTH_NORMAL:
		filter = Triangle;
		filtersupport = 1.0;
		break;
	case SMOOTH_BEST:
		filter = Mitchell;
		filtersupport = 2.0;
		break;
	}
	
	if( filter == Box && width > image.width() && height > image.height() && blur == 1.0 ) {
		filter = NULL; // Box doesn't really smooth when enlarging
	}
	if( filter == NULL ) {
		return image.scale( width, height );
	}

	if ( image.depth() == 32 ) {
		// 32-bpp to 32-bpp
		return ResizeImage( image, width, height, filter, filtersupport, blur );
	} else if ( image.depth() != 16 && image.allGray() && !image.hasAlphaBuffer() ) {
		// Inefficient
		return ResizeImage( image.convertDepth(32), width, height, filter, filtersupport, blur ).convertDepth(8);
	} else {
	// Inefficient
		return ResizeImage( image.convertDepth(32), width, height, filter, filtersupport, blur );
	}
}


} // namespace
