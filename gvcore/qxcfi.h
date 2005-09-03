#ifndef QXCFI_H
#define QXCFI_H

#include <qimage.h>
#include <qimageformatplugin.h>
#include <qvaluestack.h>
#include <qvaluevector.h>

#include "gimp.h"
namespace Gwenview {

// Safe readBlock helper functions
class SafeDataStream {
public:
  SafeDataStream(QIODevice* device)
  : mDevice(device), mFailed(false) {}

  bool failed() const { return mFailed; }
  QIODevice* device() const { return mDevice; }

  SafeDataStream& readRawBytes(char* data, uint length) {
	if (mFailed) return *this;
	int read_length=mDevice->readBlock(data, length);
	if (read_length==-1) mFailed=true;
	if ((uint)read_length!=length) mFailed=true;
	return *this;
  }

  SafeDataStream& operator>>(Q_INT8& value) {
	return readRawBytes((char*)&value, 1);
  }

  SafeDataStream& operator>>(Q_UINT32& value) {
	if (mFailed) return *this;
	uchar *p = (uchar *)(&value);
	char b[4];
	if (mDevice->readBlock( b, 4 )==4) {
	  *p++ = b[3];
	  *p++ = b[2];
	  *p++ = b[1];
	  *p   = b[0];
	} else {
	  mFailed=true;
	}
	return *this;
  }

  SafeDataStream& operator>>(Q_INT32& value) {
	return *this >>((Q_UINT32&)value);
  }

  SafeDataStream& operator>>(float& value) {
	return *this >>((Q_UINT32&)value);
  }

  SafeDataStream& operator>>(char*& value) {
	if (mFailed) return *this;
	
	Q_UINT32 len;
	*this >> len;
	if (mFailed) return *this;
	if ( len == 0 ) {
	  value = 0;
	  return *this;
	}
	if (mDevice->atEnd() ) {
	  value = 0;
	  mFailed=true;
	  return *this;
	}
	value = new char[len];
	Q_CHECK_PTR( value );
	if ( !value ) {
	  mFailed=true;
	  return *this;
	}
	return readRawBytes(value, len);
  }

  SafeDataStream& readBytes(char*& data, uint& len) {
	if (mFailed) return *this;

	*this >> len;
	if (mFailed) return *this;
	data=new char[len];
	Q_CHECK_PTR( data );
	if ( !data ) {
	  mFailed=true;
	  return *this;
	}
	return readRawBytes(data, len);
  }

  // This method is usefull to debug with gdb. Do not inline it!
  int at() const;

private:
  QIODevice* mDevice;
  bool mFailed;
};

//! Plug-in for loading a GIMP XCF image file directly.
/*!
 * This class uses the Qt 3.0 Image format plug-in loader to provide
 * the ability to read The GIMP XCF image files. This plug-in will
 * be dynamically loaded as needed.
 */
class XCFImageFormat : public QImageFormatPlugin {

  /*!
   * Each layer in an XCF file is stored as a matrix of
   * 64-pixel by 64-pixel images. The GIMP has a sophisticated
   * method of handling very large images as well as implementing
   * parallel processing on a tile-by-tile basis. Here, though,
   * we just read them in en-masse and store them in a matrix.
   */
  typedef QValueVector< QValueVector< QImage > > Tiles;

  /*!
   * Each GIMP image is composed of one or more layers. A layer can
   * be one of any three basic types: RGB, grayscale or indexed. With an
   * optional alpha channel, there are six possible types altogether.
   *
   * Note: there is only ever one instance of this structure. The
   * layer info is discarded after it is merged into the final QImage.
   */
  struct Layer {
    Q_UINT32 width;		//!< Width of the layer
    Q_UINT32 height;		//!< Height of the layer
    Q_INT32 type;		//!< Type of the layer (GimpImageType)
    char* name;			//!< Name of the layer
    Q_UINT32 hierarchy_offset;	//!< File position of Tile hierarchy
    Q_UINT32 mask_offset;	//!< File position of mask image

    uint nrows;			//!< Number of rows of tiles (y direction)
    uint ncols;			//!< Number of columns of tiles (x direction)

    Tiles image_tiles;		//!< The basic image
    //! For Grayscale and Indexed images, the alpha channel is stored
    //! separately (in this data structure, anyway).
    Tiles alpha_tiles;
    Tiles mask_tiles;		//!< The layer mask (optional)

    //! Additional information about a layer mask.
    struct {
      Q_UINT32 opacity;
      Q_UINT32 visible;
      Q_UINT32 show_masked;
      uchar red, green, blue;
      Q_UINT32 tattoo;
    } mask_channel;

    bool active;		//!< Is this layer the active layer?
    Q_UINT32 opacity;		//!< The opacity of the layer
    Q_UINT32 visible;		//!< Is the layer visible?
    Q_UINT32 linked;		//!< Is this layer linked (geometrically)
    Q_UINT32 preserve_transparency; //!< Preserve alpha when drawing on layer?
    Q_UINT32 apply_mask;	//!< Apply the layer mask?
    Q_UINT32 edit_mask;		//!< Is the layer mask the being edited?
    Q_UINT32 show_mask;		//!< Show the layer mask rather than the image?
    Q_INT32 x_offset;		//!< x offset of the layer relative to the image
    Q_INT32 y_offset;		//!< y offset of the layer relative to the image
    Q_UINT32 mode;		//!< Combining mode of layer (LayerModeEffects)
    Q_UINT32 tattoo;		//!< (unique identifier?)

    //! As each tile is read from the file, it is buffered here.
    uchar tile[TILE_WIDTH * TILE_HEIGHT * sizeof(QRgb)];

    //! The data from tile buffer is copied to the Tile by this
    //! method.  Depending on the type of the tile (RGB, Grayscale,
    //! Indexed) and use (image or mask), the bytes in the buffer are
    //! copied in different ways.
    void (*assignBytes)( Layer& layer, uint i, uint j );

    //! Construct a layer.
    Layer ( void ) : name( 0 ) {}
    //! Destruct the layer.
    ~Layer ( void ) { if ( name != 0 ) delete[] name; }
  };

  /*!
   * The in-memory representation of the XCF Image. It contains a few
   * metadata items, but is mostly a container for the layer information.
   */
  struct XCFImage {
    Q_UINT32 width;		//!< width of the XCF image
    Q_UINT32 height;		//!< height of the XCF image
    Q_INT32 type;		//!< type of the XCF image (GimpImageBaseType)

    Q_UINT8 compression;	//!< tile compression method (CompressionType)
    float x_resolution;		//!< x resolution in dots per inch
    float y_resolution;		//!< y resolution in dots per inch
    Q_INT32 tattoo;		//!< (unique identifier?)
    Q_UINT32 unit;		//!< Units of The GIMP (inch, mm, pica, etc...)
    Q_INT32 num_colors;		//!< number of colors in an indexed image
    QValueVector< QRgb > palette; //!< indexed image color palette

    int num_layers;		//!< number of layers
    Layer layer;		//!< most recently read layer

    bool initialized;		//!< Is the QImage initialized?
    QImage image;		//!< final QImage

    //! Simple constructor.
    XCFImage ( void ) : initialized( false ) {}
  };

  //! The bottom-most layer is copied into the final QImage by this
  //! routine.
  typedef void (*PixelCopyOperation) ( Layer& layer, uint i, uint j, int k, int l,
				       QImage& image, int m, int n );

  //! Higher layers are merged into the the final QImage by this routine.
  typedef void (*PixelMergeOperation) ( Layer& layer, uint i, uint j, int k, int l,
					QImage& image, int m, int n );

  //! In layer DISSOLVE mode, a random number is chosen to compare to a
  //! pixel's alpha. If the alpha is greater than the random number, the
  //! pixel is drawn. This table merely contains the random number seeds
  //! for each ROW of an image. Therefore, the random numbers chosen
  //! are consistent from run to run.

  static int random_table[RANDOM_TABLE_SIZE];

  //! This table provides the add_pixel saturation values (i.e. 250 + 250 = 255).

  static int add_lut[256][256];

  //! Layer mode static data.
  typedef struct {
    bool affect_alpha;		//!< Does this mode affect the source alpha?
  } LayerModes;

  //! Array of layer mode structures for the modes described by
  //! LayerModeEffects.
  static LayerModes layer_modes[];

public:
  /*!
   * The constructor for the XCF image loader. This initializes the
   * tables used in the layer merging routines.
   */
  XCFImageFormat ();
  

  /*!
   * The image loader makes no (direct) use of dynamic memory
   * and the Qt infrastructure takes care of constructing and destructing
   * the loader so there is not much to do here.
   */
  ~XCFImageFormat () {}

  /*!
   * You can query Qt about the types of image file formats it knows about
   * via QImage::inputFormats or QImage::inputFormatList().
   * This method returns "xcf".
   */
  QStringList keys () const {
    return QStringList() << "XCF";
  }

  /*!
   * This method installs the XCF reader on demand.
   */
  bool installIOHandler ( const QString& ); 

  static void registerFormat();

private:
  static void readXCF ( QImageIO* image_io );
#ifdef TMP_WRITE
  static void writeXCF ( QImageIO* ) {}
#endif
  static void initializeImage ( XCFImage& xcf_image );
  static void composeTiles ( XCFImage& xcf_image );
  static bool loadImageProperties ( SafeDataStream& xcf_io, XCFImage& image );
  static bool loadLayer ( SafeDataStream& xcf_io, XCFImage& xcf_image );
  static bool loadLayerProperties ( SafeDataStream& xcf_io, Layer& layer );
  static bool loadChannelProperties ( SafeDataStream& xcf_io, Layer& layer );
  static bool loadHierarchy ( SafeDataStream& xcf_io, Layer& layer );
  static bool loadMask ( SafeDataStream& xcf_io, Layer& layer );
  static bool loadLevel ( SafeDataStream& xcf_io, Layer& layer, Q_INT32 bpp );
  static bool loadTileRLE ( SafeDataStream& xcf_io, uchar* tile, int size,
			    int data_length, Q_INT32 bpp );
  static bool loadProperty ( SafeDataStream& xcf_io, PropType& type,
			     QByteArray& bytes );
  static void setGrayPalette ( QImage& image );
  static void setPalette ( XCFImage& xcf_image, QImage& image );
  static void assignImageBytes ( Layer& layer, uint i, uint j );
  static void assignMaskBytes ( Layer& layer, uint i, uint j );

  static void copyLayerToImage ( XCFImage& xcf_image );
  static void copyRGBToRGB ( Layer& layer, uint i, uint j, int k, int l,
			     QImage& image, int m, int n );
  static void copyGrayToGray ( Layer& layer, uint i, uint j, int k, int l,
			       QImage& image, int m, int n );
  static void copyGrayToRGB ( Layer& layer, uint i, uint j, int k, int l,
			      QImage& image, int m, int n );
  static void copyGrayAToRGB ( Layer& layer, uint i, uint j, int k, int l,
			       QImage& image, int m, int n );
  static void copyIndexedToIndexed ( Layer& layer, uint i, uint j, int k, int l,
				     QImage& image, int m, int n );
  static void copyIndexedAToIndexed ( Layer& layer, uint i, uint j, int k, int l,
				      QImage& image, int m, int n );
  static void copyIndexedAToRGB ( Layer& layer, uint i, uint j, int k, int l,
				  QImage& image, int m, int n );

  static void mergeLayerIntoImage ( XCFImage& xcf_image );
  static void mergeRGBToRGB ( Layer& layer, uint i, uint j, int k, int l,
			      QImage& image, int m, int n );
  static void mergeGrayToGray ( Layer& layer, uint i, uint j, int k, int l,
				QImage& image, int m, int n );
  static void mergeGrayAToGray ( Layer& layer, uint i, uint j, int k, int l,
				 QImage& image, int m, int n );
  static void mergeGrayToRGB ( Layer& layer, uint i, uint j, int k, int l,
			       QImage& image, int m, int n );
  static void mergeGrayAToRGB ( Layer& layer, uint i, uint j, int k, int l,
				QImage& image, int m, int n );
  static void mergeIndexedToIndexed ( Layer& layer, uint i, uint j, int k, int l,
				      QImage& image, int m, int n );
  static void mergeIndexedAToIndexed ( Layer& layer, uint i, uint j, int k, int l,
				       QImage& image, int m, int n );
  static void mergeIndexedAToRGB ( Layer& layer, uint i, uint j, int k, int l,
				   QImage& image, int m, int n );

  static void dissolveRGBPixels ( QImage& image, int x, int y );
  static void dissolveAlphaPixels ( QImage& image, int x, int y );
};

} // namespace
#endif

