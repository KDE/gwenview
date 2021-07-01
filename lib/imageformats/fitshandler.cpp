// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2008 Aurélien Gâteau <agateau@kde.org>

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

#include "fitshandler.h"

#include "imageformats/fitsformat/fitsdata.h"

#include "gwenview_lib_debug.h"
#include <QImage>
#include <QSize>
#include <QVariant>

namespace Gwenview
{
bool FitsHandler::canRead() const
{
    if (!device()) {
        return false;
    }

    device()->seek(0);
    if (device()->peek(6) != "SIMPLE" && device()->peek(8) != "XTENSION") {
        return false;
    }

    FITSData fitsLoader;

    if (fitsLoader.loadFITS(*device())) {
        setFormat("fits");
        return true;
    }
    return false;
}

bool FitsHandler::read(QImage *image)
{
    if (!device()) {
        return false;
    }

    *image = FITSData::FITSToImage(*device());
    return true;
}

bool FitsHandler::supportsOption(ImageOption option) const
{
    return option == Size;
}

QVariant FitsHandler::option(ImageOption option) const
{
    if (option == Size && device()) {
        FITSData fitsLoader;

        if (fitsLoader.loadFITS(*device())) {
            return QSize((int)fitsLoader.getWidth(), (int)fitsLoader.getHeight());
        }
    }
    return QVariant();
}

} // namespace
