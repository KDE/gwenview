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
#include <QFile>

// KF

// Exiv2
#include <exiv2/exiv2.hpp>

// Local
#include "gwenview_exiv2_debug.h"

namespace Gwenview
{

struct Exiv2ImageLoaderPrivate
{
    std::unique_ptr<Exiv2::Image> mImage;
    QString mErrorMessage;
};

struct Exiv2LogHandler {
    static void handleMessage(int level, const char *message) {
        switch(level) {
        case Exiv2::LogMsg::debug:
            qCDebug(GWENVIEW_EXIV2_LOG) << message;
            break;
        case Exiv2::LogMsg::info:
            qCInfo(GWENVIEW_EXIV2_LOG) << message;
            break;
        case Exiv2::LogMsg::warn:
        case Exiv2::LogMsg::error:
        case Exiv2::LogMsg::mute:
            qCWarning(GWENVIEW_EXIV2_LOG) << message;
            break;
        default:
            qCWarning(GWENVIEW_EXIV2_LOG) << "unhandled log level" << level << message;
            break;
        }
    }

    Exiv2LogHandler() {
        Exiv2::LogMsg::setHandler(&Exiv2LogHandler::handleMessage);
    }
};


Exiv2ImageLoader::Exiv2ImageLoader()
: d(new Exiv2ImageLoaderPrivate)
{
    // This is a threadsafe way to ensure that we only register it once
    static Exiv2LogHandler handler;
}

Exiv2ImageLoader::~Exiv2ImageLoader()
{
    delete d;
}

bool Exiv2ImageLoader::load(const QString& filePath)
{
    QByteArray filePathByteArray = QFile::encodeName(filePath);
    try {
        d->mImage.reset(Exiv2::ImageFactory::open(filePathByteArray.constData()).release());
        d->mImage->readMetadata();
    } catch (const Exiv2::Error& error) {
        d->mErrorMessage = QString::fromUtf8(error.what());
        return false;
    }
    return true;
}

bool Exiv2ImageLoader::load(const QByteArray& data)
{
    try {
        d->mImage.reset(Exiv2::ImageFactory::open((unsigned char*)data.constData(), data.size()).release());
        d->mImage->readMetadata();
    } catch (const Exiv2::Error& error) {
        d->mErrorMessage = QString::fromUtf8(error.what());
        return false;
    }
    return true;
}

QString Exiv2ImageLoader::errorMessage() const
{
    return d->mErrorMessage;
}

std::unique_ptr<Exiv2::Image> Exiv2ImageLoader::popImage()
{
    return std::move(d->mImage);
}

} // namespace
