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
#include "mimetypeutils_p.h"

// Qt
#include <QApplication>
#include <QStringList>
#include <QDebug>
#include <QFileInfo>
#include <QUrl>
#include <QMimeData>
#include <QMimeDatabase>
#include <QImageReader>

// KDE
#include <KFileItem>
#include <KIO/Job>
#include <kio/jobclasses.h>

// Local
#include <archiveutils.h>
#include <lib/document/documentfactory.h>
#include <gvdebug.h>

namespace Gwenview
{

namespace MimeTypeUtils
{

static inline QString resolveAlias(const QString& name)
{
    QMimeDatabase db;
    return db.mimeTypeForName(name).name();
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
    *list += "image/x-panasonic-rw";
    *list += "image/x-panasonic-rw2";
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
        const auto supported = QImageReader::supportedMimeTypes();
        for (auto mime: supported) {
            list << resolveAlias(QString::fromUtf8(mime));
        }
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

QString urlMimeType(const QUrl &url)
{
    if (url.isEmpty()) {
        return "unknown";
    }

    QMimeDatabase db;
    return db.mimeTypeForUrl(url).name();
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

Kind urlKind(const QUrl &url)
{
    return mimeTypeKind(urlMimeType(url));
}

QMimeData* selectionMimeData(const KFileItemList& selectedFiles)
{
    QMimeData* mimeData = new QMimeData;

    if (selectedFiles.count() == 1) {
        const QUrl url = selectedFiles.first().url();
        const MimeTypeUtils::Kind mimeKind = MimeTypeUtils::urlKind(url);
        bool documentIsModified = false;

        if (mimeKind == MimeTypeUtils::KIND_RASTER_IMAGE || mimeKind == MimeTypeUtils::KIND_SVG_IMAGE) {
            const Document::Ptr doc = DocumentFactory::instance()->load(url);
            doc->waitUntilLoaded();
            documentIsModified = doc->isModified();

            QString suggestedFileName;

            if (mimeKind == MimeTypeUtils::KIND_RASTER_IMAGE) {
                mimeData->setImageData(doc->image());

                // Set the filename extension to PNG, as it is the first
                // entry in the combobox when pasting to Dolphin
                suggestedFileName = QFileInfo(url.fileName()).completeBaseName() + QStringLiteral(".png");
            } else {
                mimeData->setData(MimeTypeUtils::urlMimeType(url), doc->rawData());
                suggestedFileName = url.fileName();
            }

            mimeData->setData(QStringLiteral("application/x-kde-suggestedfilename"),
                              QFile::encodeName(suggestedFileName));
        }

        // Don't set the URL to support pasting edited images to
        // applications preferring the URL otherwise, e.g. Dolphin
        if (!documentIsModified) {
            mimeData->setUrls({url});
        }
    } else {
        mimeData->setUrls(selectedFiles.urlList());
    }

    return mimeData;
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
