// vim: set tabstop=4 shiftwidth=4 noexpandtab
/* -*- c++ -*-
 * gimp.h: Header for a Qt 3 plug-in for reading GIMP XCF image files
 * Copyright (C) 2001 lignum Computing, Inc. <allen@lignumcomputing.com>
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

/*
 * These are the constants and functions I extracted from The GIMP source
 * code. If the reader fails to work, this is probably the place to start
 * looking for discontinuities.
 */

// From GIMP "tile.h" v1.2

const uint TILE_WIDTH = 64;	//!< Width of a tile in the XCF file.
const uint TILE_HEIGHT = 64;	//!< Height of a tile in the XCF file.

// From GIMP "paint_funcs.c" v1.2

const int RANDOM_TABLE_SIZE = 4096; //!< Size of dissolve random number table.
const int RANDOM_SEED = 314159265; //!< Seed for dissolve random number table.
const double EPSILON = 0.0001;	//!< Roundup in alpha blending.

// From GIMP "paint_funcs.h" v1.2

const uchar OPAQUE_OPACITY = 255; //!< Opaque value for 8-bit alpha component.

// From GIMP "apptypes.h" v1.2

//! Basic GIMP image type. QImage converter may produce a deeper image
//! than is specified here. For example, a grayscale image with an
//! alpha channel must (currently) use a 32-bit Qt image.

typedef enum
{
  RGB,
  GRAY,
  INDEXED
} GimpImageBaseType;

//! Type of individual layers in an XCF file.

typedef enum
{
  RGB_GIMAGE,
  RGBA_GIMAGE,
  GRAY_GIMAGE,
  GRAYA_GIMAGE,
  INDEXED_GIMAGE,
  INDEXEDA_GIMAGE
} GimpImageType;

//! Effect to apply when layers are merged together.

typedef enum 
{
  NORMAL_MODE,
  DISSOLVE_MODE,
  BEHIND_MODE,
  MULTIPLY_MODE,
  SCREEN_MODE,
  OVERLAY_MODE,
  DIFFERENCE_MODE,
  ADDITION_MODE,
  SUBTRACT_MODE,
  DARKEN_ONLY_MODE,
  LIGHTEN_ONLY_MODE,
  HUE_MODE,
  SATURATION_MODE,
  COLOR_MODE,
  VALUE_MODE,
  DIVIDE_MODE,
  ERASE_MODE,
  REPLACE_MODE,
  ANTI_ERASE_MODE
} LayerModeEffects;

// From GIMP "xcf.c" v1.2

//! Properties which can be stored in an XCF file.

typedef enum
{
  PROP_END = 0,
  PROP_COLORMAP = 1,
  PROP_ACTIVE_LAYER = 2,
  PROP_ACTIVE_CHANNEL = 3,
  PROP_SELECTION = 4,
  PROP_FLOATING_SELECTION = 5,
  PROP_OPACITY = 6,
  PROP_MODE = 7,
  PROP_VISIBLE = 8,
  PROP_LINKED = 9,
  PROP_PRESERVE_TRANSPARENCY = 10,
  PROP_APPLY_MASK = 11,
  PROP_EDIT_MASK = 12,
  PROP_SHOW_MASK = 13,
  PROP_SHOW_MASKED = 14,
  PROP_OFFSETS = 15,
  PROP_COLOR = 16,
  PROP_COMPRESSION = 17,
  PROP_GUIDES = 18,
  PROP_RESOLUTION = 19,
  PROP_TATTOO = 20,
  PROP_PARASITES = 21,
  PROP_UNIT = 22,
  PROP_PATHS = 23,
  PROP_USER_UNIT = 24
} PropType;

// From GIMP "xcf.c" v1.2

//! Compression type used in layer tiles.

typedef enum
{
  COMPRESS_NONE = 0,
  COMPRESS_RLE = 1,
  COMPRESS_ZLIB = 2,
  COMPRESS_FRACTAL = 3  /* Unused. */
} CompressionType;


