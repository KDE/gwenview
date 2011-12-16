// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2007 Aurélien Gâteau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
// Self
#include "exiv2imageloader.h"

// Qt
#include <QByteArray>
#include <QString>

// KDE

// Exiv2
#include <exiv2/error.hpp>
#include <exiv2/types.hpp>

// Local

namespace Gwenview
{

struct Exiv2ImageLoaderPrivate
{
    Exiv2::Image::AutoPtr mImage;
    QString mErrorMessage;
};

Exiv2ImageLoader::Exiv2ImageLoader()
: d(new Exiv2ImageLoaderPrivate)
{
}

Exiv2ImageLoader::~Exiv2ImageLoader()
{
    delete d;
}

bool Exiv2ImageLoader::load(const QByteArray& data)
{
    try {
        d->mImage = Exiv2::ImageFactory::open((unsigned char*)data.constData(), data.size());
        d->mImage->readMetadata();
#if EXIV2_VERSION >= EXIV2_MAKE_VERSION(0, 14, 0)
        // For some unknown reason, trying to catch Exiv2::Error fails with Exiv2
        // >=0.14. For now, just catch std::exception. I would welcome any
        // explanation.
    } catch (const std::exception& error) {
        d->mErrorMessage = error.what();
#else
        // In libexiv2 0.12, Exiv2::Error::what() returns an std::string.
    } catch (const Exiv2::Error& error) {
        d->mErrorMessage = error.what().c_str();
#endif
        return false;
    }
    return true;
}

QString Exiv2ImageLoader::errorMessage() const
{
    return d->mErrorMessage;
}

Exiv2::Image::AutoPtr Exiv2ImageLoader::popImage()
{
    return d->mImage;
}

} // namespace
