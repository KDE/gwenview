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
#include <config-gwenview.h>

// Local
#include <cms/cmsprofile_png.h>
#include <gvdebug.h>
#include <iodevicejpegsourcemanager.h>
#include <jpegerrormanager.h>

extern "C" {
#include <cms/iccjpeg.h>
}

// KF

// Qt
#include <QBuffer>
#include <QtGlobal>

// lcms
#include <lcms2.h>

// Exiv2
#include <exiv2/exiv2.hpp>

// X11
#ifdef HAVE_X11
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <fixx11h.h>
#include <QtX11Extras/QX11Info>
#endif

// local
#include "gwenview_lib_debug.h"
#include <lib/gwenviewconfig.h>

namespace Gwenview
{

#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) //qCDebug(GWENVIEW_LIB_LOG) << x
#else
#define LOG(x) ;
#endif

namespace Cms
{

//- JPEG -----------------------------------------------------------------------
static cmsHPROFILE loadFromJpegData(const QByteArray& data)
{
    cmsHPROFILE profile = nullptr;
    struct jpeg_decompress_struct srcinfo;

    JPEGErrorManager srcErrorManager;
    srcinfo.err = &srcErrorManager;
    jpeg_create_decompress(&srcinfo);
    if (setjmp(srcErrorManager.jmp_buffer)) {
        qCCritical(GWENVIEW_LIB_LOG) << "libjpeg error in src\n";
        return nullptr;
    }

    QBuffer buffer(const_cast<QByteArray*>(&data));
    buffer.open(QIODevice::ReadOnly);
    IODeviceJpegSourceManager::setup(&srcinfo, &buffer);

    setup_read_icc_profile(&srcinfo);
    jpeg_read_header(&srcinfo, true);
    jpeg_start_decompress(&srcinfo);

    uchar* profile_data;
    uint profile_len;
    if (read_icc_profile(&srcinfo, &profile_data, &profile_len)) {
        LOG("Found a profile, length:" << profile_len);
        profile = cmsOpenProfileFromMem(profile_data, profile_len);
    }

    jpeg_destroy_decompress(&srcinfo);
    free(profile_data);

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
        mProfile = nullptr;
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
    d->mProfile = nullptr;
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
    cmsHPROFILE hProfile = nullptr;
    if (format == "png") {
        hProfile = loadFromPngData(data);
    } else if (format == "jpeg") {
        hProfile = loadFromJpegData(data);
    }
    if (hProfile) {
        ptr = new Profile(hProfile);
    }
    return ptr;
}

Profile::Ptr Profile::loadFromExiv2Image(const Exiv2::Image* image)
{
    Profile::Ptr ptr;
    cmsHPROFILE hProfile = nullptr;

    const Exiv2::ExifData& exifData = image->exifData();
    Exiv2::ExifKey key("Exif.Image.InterColorProfile");
    Exiv2::ExifData::const_iterator it = exifData.findKey(key);
    if (it == exifData.end()) {
        LOG("No profile found");
        return ptr;
    }
    int size = it->size();
    LOG("size:" << size);

    QByteArray data;
    data.resize(size);
    it->copy(reinterpret_cast<Exiv2::byte*>(data.data()), Exiv2::invalidByteOrder);
    hProfile = cmsOpenProfileFromMem(data, size);

    if (hProfile) {
        ptr = new Profile(hProfile);
    }
    return ptr;
}

Profile::Ptr Profile::loadFromICC(const QByteArray &data)
{
    Profile::Ptr ptr;
    int size = data.size();

    if (size > 0) {
        cmsHPROFILE hProfile = cmsOpenProfileFromMem(data, size);

        if (hProfile) {
            ptr = new Profile(hProfile);
        }
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
    cmsHPROFILE hProfile = nullptr;
    // Get the profile from you config file if the user has set it.
    // if the user allows override through the atom, do this:
#ifdef HAVE_X11
    if (QX11Info::isPlatformX11() && !GwenviewConfig::noMonitorICC()) {
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
    }
#endif
    if (!hProfile) {
        return getSRgbProfile();
    }
    return Profile::Ptr(new Profile(hProfile));
}

Profile::Ptr Profile::getSRgbProfile()
{
    return Profile::Ptr(new Profile(cmsCreate_sRGBProfile()));
}

} // namespace Cms

} // namespace Gwenview
