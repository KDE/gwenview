// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2012 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
// Self
#include "cmsprofile_png.h"

// Local
#include "gwenview_lib_debug.h"
#include <gvdebug.h>

// KF

// Qt
#include <QBuffer>

// lcms
#include <lcms2.h>

// libpng
#include <png.h>

namespace Gwenview
{

namespace Cms
{

static void readPngChunk(png_structp png_ptr, png_bytep data, png_size_t length)
{
    auto in = (QIODevice *)png_get_io_ptr(png_ptr);

    while (length) {
        int nr = in->read((char *)data, length);
        if (nr <= 0) {
            png_error(png_ptr, "Read Error");
            return;
        }
        length -= nr;
    }
}

cmsHPROFILE loadFromPngData(const QByteArray &data)
{
    QBuffer buffer;
    buffer.setBuffer(const_cast<QByteArray *>(&data));
    buffer.open(QIODevice::ReadOnly);

    // Initialize the internal structures
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    GV_RETURN_VALUE_IF_FAIL(png_ptr, nullptr);

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, (png_infopp) nullptr, (png_infopp) nullptr);
        qCWarning(GWENVIEW_LIB_LOG) << "Could not create info_struct";
        return nullptr;
    }

    png_infop end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) nullptr);
        qCWarning(GWENVIEW_LIB_LOG) << "Could not create info_struct2";
        return nullptr;
    }

    // Catch errors
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        qCWarning(GWENVIEW_LIB_LOG) << "Error decoding png file";
        return nullptr;
    }

    // Initialize the special
    png_set_read_fn(png_ptr, &buffer, readPngChunk);

    // read all PNG info up to image data
    png_read_info(png_ptr, info_ptr);

    // Get profile
    png_charp profile_name;
#if PNG_LIBPNG_VER_MAJOR >= 1 && PNG_LIBPNG_VER_MINOR >= 5
    png_bytep profile_data;
#else
    png_charp profile_data;
#endif
    int compression_type;
    png_uint_32 proflen;

    cmsHPROFILE profile = nullptr;
    if (png_get_iCCP(png_ptr, info_ptr, &profile_name, &compression_type, &profile_data, &proflen)) {
        profile = cmsOpenProfileFromMem(profile_data, proflen);
    }
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    return profile;
}

} // namespace Cms
} // namespace Gwenview
