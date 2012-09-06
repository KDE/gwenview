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
#include "cmsprofile.h"

// Local
#include <gvdebug.h>

// KDE
#include <KDebug>

// Qt
#include <QBuffer>

// lcms
#include <lcms2.h>

// libpng
#include <png.h>

// X11
#ifdef Q_WS_X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <fixx11h.h>
#include <QX11Info>
#endif

namespace Gwenview
{

namespace Cms
{

//- Png ------------------------------------------------------------------------
static void readPngChunk(png_structp png_ptr, png_bytep data, png_size_t length)
{
    QIODevice *in = (QIODevice *)png_get_io_ptr(png_ptr);

    while (length) {
        int nr = in->read((char*)data, length);
        if (nr <= 0) {
            png_error(png_ptr, "Read Error");
            return;
        }
        length -= nr;
    }
}

static cmsHPROFILE loadFromPngData(const QByteArray& data)
{
    QBuffer buffer;
    buffer.setBuffer(const_cast<QByteArray*>(&data));
    buffer.open(QIODevice::ReadOnly);

    // Initialize the internal structures
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    GV_RETURN_VALUE_IF_FAIL(png_ptr, 0);

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        kWarning() << "Could not create info_struct";
        return 0;
    }

    png_infop end_info = png_create_info_struct(png_ptr);
    if (!end_info) {
        png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
        kWarning() << "Could not create info_struct2";
        return 0;
    }

    // Catch errors
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
        kWarning() << "Error decoding png file";
        return 0;
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

    cmsHPROFILE profile = 0;
    if (png_get_iCCP(png_ptr, info_ptr, &profile_name, &compression_type, &profile_data, &proflen)) {
        profile = cmsOpenProfileFromMem(profile_data, proflen);
    } else {
        kWarning() << "No ICC profile in this file";
    }
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    return profile;
}


//- Profile class --------------------------------------------------------------
struct ProfilePrivate
{
    cmsHPROFILE mProfile;

    void reset()
    {
        if (mProfile) {
            cmsCloseProfile(mProfile);
        }
        mProfile = 0;
    }

    QString readInfo(cmsInfoType info)
    {
        GV_RETURN_VALUE_IF_FAIL(mProfile, QString());
        wchar_t buffer[1024];
        int size = cmsGetProfileInfo(mProfile, info, "en", "US", buffer, 1024);
        return QString::fromWCharArray(buffer, size);
    }
};

Profile::Profile()
: d(new ProfilePrivate)
{
    d->mProfile = 0;
}

Profile::Profile(cmsHPROFILE hProfile)
: d(new ProfilePrivate)
{
    d->mProfile = hProfile;
}

Profile::~Profile()
{
    d->reset();
    delete d;
}

Profile::Ptr Profile::loadFromImageData(const QByteArray& data, const QByteArray& format)
{
    Profile::Ptr ptr;
    cmsHPROFILE hProfile;
    if (format == "png") {
        hProfile = loadFromPngData(data);
    }
    if (hProfile) {
        ptr = new Profile(hProfile);
    }
    return ptr;
}

cmsHPROFILE Profile::handle() const
{
    return d->mProfile;
}

QString Profile::copyright() const
{
    return d->readInfo(cmsInfoCopyright);
}

QString Profile::description() const
{
    return d->readInfo(cmsInfoDescription);
}

QString Profile::manufacturer() const
{
    return d->readInfo(cmsInfoManufacturer);
}

QString Profile::model() const
{
    return d->readInfo(cmsInfoModel);
}

Profile::Ptr Profile::getMonitorProfile()
{
    cmsHPROFILE hProfile = 0;
    // Get the profile from you config file if the user has set it.
    // if the user allows override through the atom, do this:
#ifdef Q_WS_X11

    // get the current screen...
    int screen = -1;

    Atom type;
    int format;
    unsigned long nitems;
    unsigned long bytes_after;
    quint8 *str;

    static Atom icc_atom = XInternAtom(QX11Info::display(), "_ICC_PROFILE", True);

    if (XGetWindowProperty(QX11Info::display(),
                            QX11Info::appRootWindow(screen),
                            icc_atom,
                            0,
                            INT_MAX,
                            False,
                            XA_CARDINAL,
                            &type,
                            &format,
                            &nitems,
                            &bytes_after,
                            (unsigned char **) &str) == Success
    ) {
        hProfile = cmsOpenProfileFromMem((void*)str, nitems);
    }
#endif
    return hProfile ? Profile::Ptr(new Profile(hProfile)) : Profile::Ptr();
}

} // namespace Cms

} // namespace Gwenview
