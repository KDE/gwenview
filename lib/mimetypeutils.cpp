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
#include <QFileInfo>
#include <QUrl>
#include <QMimeData>
#include <QMimeDatabase>
#include <QImageReader>

// KF
#include <KFileItem>
#include <KIO/Job>
#include <kio/jobclasses.h>

// Local
#include "gwenview_lib_debug.h"
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

static inline QStringList rawMimeTypes()
{
    // need to invent more intelligent way to whitelist raws
    return {
        QStringLiteral("image/x-nikon-nef"),
        QStringLiteral("image/x-nikon-nrw"),
        QStringLiteral("image/x-canon-cr2"),
        QStringLiteral("image/x-canon-crw"),
        QStringLiteral("image/x-pentax-pef"),
        QStringLiteral("image/x-adobe-dng"),
        QStringLiteral("image/x-sony-arw"),
        QStringLiteral("image/x-minolta-mrw"),
        QStringLiteral("image/x-panasonic-raw"),
        QStringLiteral("image/x-panasonic-raw2"),
        QStringLiteral("image/x-panasonic-rw"),
        QStringLiteral("image/x-panasonic-rw2"),
        QStringLiteral("image/x-samsung-srw"),
        QStringLiteral("image/x-olympus-orf"),
        QStringLiteral("image/x-fuji-raf"),
        QStringLiteral("image/x-kodak-dcr"),
        QStringLiteral("image/x-sigma-x3f")
    };
}

const QStringList& rasterImageMimeTypes()
{
    static QStringList list;
    if (list.isEmpty()) {
        const auto supported = QImageReader::supportedMimeTypes();
        for (const auto &mime: supported) {
            const auto resolved = resolveAlias(QString::fromUtf8(mime));
            if (resolved.isEmpty()) {
                qCWarning(GWENVIEW_LIB_LOG) << "Unresolved mime type " << mime;
            } else {
                list << resolved;
            }
        }
        // We don't want svg images to be considered as raster images
        const QStringList svgImageMimeTypesList = svgImageMimeTypes();
        for (const QString& mimeType : svgImageMimeTypesList) {
            list.removeOne(mimeType);
        }
        for (const QString &rawMimetype : rawMimeTypes()) {
            const auto resolved = resolveAlias(rawMimetype);
            if (resolved.isEmpty()) {
                qCWarning(GWENVIEW_LIB_LOG) << "Unresolved raw mime type " << rawMimetype;
            } else {
                list << resolved;
            }
        }
    }
    return list;
}

const QStringList& svgImageMimeTypes()
{
    static QStringList list;
    if (list.isEmpty()) {
        list << QStringLiteral("image/svg+xml") << QStringLiteral("image/svg+xml-compressed");
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
        return QStringLiteral("unknown");
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

QMimeData* selectionMimeData(const KFileItemList& selectedFiles, MimeTarget mimeTarget)
{
    auto* mimeData = new QMimeData;

    if (selectedFiles.count() == 1) {

        // When a single file is selected, there are a couple of cases:
        // - Pasting unmodified images: Set both image data and URL
        //   (since some apps only support either image data or URL)
        // - Dragging unmodified images: Only set URL
        //   (otherwise dragging to Chromium or the desktop fails, see https://phabricator.kde.org/D13249#300894)
        // - Dragging or pasting modified images: Only set image data
        //   (otherwise some apps prefer the URL, which would only contain the unmodified image)

        const QUrl url = selectedFiles.first().url();
        const MimeTypeUtils::Kind mimeKind = MimeTypeUtils::urlKind(url);
        bool documentIsModified = false;

        if (mimeKind == MimeTypeUtils::KIND_RASTER_IMAGE || mimeKind == MimeTypeUtils::KIND_SVG_IMAGE) {
            const Document::Ptr doc = DocumentFactory::instance()->load(url);
            doc->waitUntilLoaded();
            documentIsModified = doc->isModified();

            if (mimeTarget == ClipboardTarget || (mimeTarget == DropTarget && documentIsModified)) {
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
        }

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
    connect(job, &KIO::TransferJob::data,
            this, &DataAccumulator::slotDataReceived);
    connect(job, &KJob::result,
            this, &DataAccumulator::slotFinished);
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
