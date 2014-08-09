// vim: set tabstop=4 shiftwidth=4 expandtab
/*
Gwenview - A simple image viewer for KDE
Copyright 2006 Aurelien Gateau <agateau@kde.org>

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "mimetypeutils.h"
// #include "mimetypeutils_p.moc"

// Qt
#include <QApplication>
#include <QStringList>

// KDE
#include <KApplication>
#include <KDebug>
#include <KFileItem>
#include <KIO/Job>
#include <KIO/JobClasses>
#include <KIO/NetAccess>
#include <KMimeType>
#include <KUrl>

#include <KImageIO>

// Local
#include <archiveutils.h>
#include <gvdebug.h>

namespace Gwenview
{

namespace MimeTypeUtils
{

static inline QString resolveAlias(const QString& name)
{
    KMimeType::Ptr ptr = KMimeType::mimeType(name, KMimeType::ResolveAliases);
    return ptr.isNull() ? name : ptr->name();
}

static void resolveAliasInList(QStringList* list)
{
    QStringList::Iterator
    it = list->begin(),
    end = list->end();
    for (; it != end; ++it) {
        *it = resolveAlias(*it);
    }
}

static void addRawMimeTypes(QStringList* list)
{
    // need to invent more intelligent way to whitelist raws
    *list += "image/x-nikon-nef";
    *list += "image/x-nikon-nrw";
    *list += "image/x-canon-cr2";
    *list += "image/x-canon-crw";
    *list += "image/x-pentax-pef";
    *list += "image/x-adobe-dng";
    *list += "image/x-sony-arw";
    *list += "image/x-minolta-mrw";
    *list += "image/x-panasonic-raw";
    *list += "image/x-panasonic-raw2";
    *list += "image/x-samsung-srw";
    *list += "image/x-olympus-orf";
    *list += "image/x-fuji-raf";
    *list += "image/x-kodak-dcr";
    *list += "image/x-sigma-x3f";
}

const QStringList& rasterImageMimeTypes()
{
    static QStringList list;
    if (list.isEmpty()) {
        list = KImageIO::mimeTypes(KImageIO::Reading);
        resolveAliasInList(&list);
        // We don't want svg images to be considered as raster images
        Q_FOREACH(const QString& mimeType, svgImageMimeTypes()) {
            list.removeOne(mimeType);
        }
        addRawMimeTypes(&list);
    }
    return list;
}

const QStringList& svgImageMimeTypes()
{
    static QStringList list;
    if (list.isEmpty()) {
        list << "image/svg+xml" << "image/svg+xml-compressed";
        resolveAliasInList(&list);
    }
    return list;
}

const QStringList& imageMimeTypes()
{
    static QStringList list;
    if (list.isEmpty()) {
        list = rasterImageMimeTypes();
        list += svgImageMimeTypes();
    }

    return list;
}

QString urlMimeType(const KUrl& url)
{
    if (url.isEmpty()) {
        return "unknown";
    }
    // Try a simple guess, using extension for remote urls
    QString mimeType = KMimeType::findByUrl(url)->name();
    if (mimeType == "application/octet-stream") {
        kDebug() << "KMimeType::findByUrl() failed to find mimetype for" << url << ". Falling back to KIO::NetAccess::mimetype().";
        // No luck, look deeper. This can happens with http urls if the filename
        // does not provide any extension.
        mimeType = KIO::NetAccess::mimetype(url, KApplication::kApplication()->activeWindow());
    }
    return mimeType;
}

QString urlMimeTypeByContent(const KUrl& url)
{
    const int HEADER_SIZE = 30;
    if (url.isLocalFile()) {
        return KMimeType::findByFileContent(url.toLocalFile())->name();
    }

    KIO::TransferJob* job = KIO::get(url);
    DataAccumulator accumulator(job);
    while (!accumulator.finished() && accumulator.data().size() < HEADER_SIZE) {
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
    }
    return KMimeType::findByContent(accumulator.data())->name();
}

Kind mimeTypeKind(const QString& mimeType)
{
    if (rasterImageMimeTypes().contains(mimeType)) {
        return KIND_RASTER_IMAGE;
    }
    if (svgImageMimeTypes().contains(mimeType)) {
        return KIND_SVG_IMAGE;
    }
    if (mimeType.startsWith(QLatin1String("video/"))) {
        return KIND_VIDEO;
    }
    if (mimeType.startsWith(QLatin1String("inode/directory"))) {
        return KIND_DIR;
    }
    if (!ArchiveUtils::protocolForMimeType(mimeType).isEmpty()) {
        return KIND_ARCHIVE;
    }

    return KIND_FILE;
}

Kind fileItemKind(const KFileItem& item)
{
    GV_RETURN_VALUE_IF_FAIL(!item.isNull(), KIND_UNKNOWN);
    return mimeTypeKind(item.mimetype());
}

Kind urlKind(const KUrl& url)
{
    return mimeTypeKind(urlMimeType(url));
}

DataAccumulator::DataAccumulator(KIO::TransferJob* job)
: QObject()
, mFinished(false)
{
    connect(job, SIGNAL(data(KIO::Job*,QByteArray)),
            SLOT(slotDataReceived(KIO::Job*,QByteArray)));
    connect(job, SIGNAL(result(KJob*)),
            SLOT(slotFinished()));
}

void DataAccumulator::slotDataReceived(KIO::Job*, const QByteArray& data)
{
    mData += data;
}

void DataAccumulator::slotFinished()
{
    mFinished = true;
}

} // namespace MimeTypeUtils
} // namespace Gwenview
