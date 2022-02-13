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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/
#include "documentfactory.h"

// Qt
#include <QByteArray>
#include <QDateTime>
#include <QMap>
#include <QUndoGroup>
#include <QUrl>

// KF

// Local
#include "gwenview_lib_debug.h"
#include <gvdebug.h>

namespace Gwenview
{
#undef ENABLE_LOG
#undef LOG
//#define ENABLE_LOG
#ifdef ENABLE_LOG
#define LOG(x) qCDebug(GWENVIEW_LIB_LOG) << x
#else
#define LOG(x) ;
#endif

inline int getMaxUnreferencedImages()
{
    int defaultValue = 3;
    QByteArray ba = qgetenv("GV_MAX_UNREFERENCED_IMAGES");
    if (ba.isEmpty()) {
        return defaultValue;
    }
    LOG("Custom value for max unreferenced images:" << ba);
    bool ok;
    const int value = ba.toInt(&ok);
    return ok ? value : defaultValue;
}

static const int MAX_UNREFERENCED_IMAGES = getMaxUnreferencedImages();

/**
 * This internal structure holds the document and the last time it has been
 * accessed. This access time is used to "garbage collect" the loaded
 * documents.
 */
struct DocumentInfo {
    Document::Ptr mDocument;
    QDateTime mLastAccess;
};

/**
 * Our collection of DocumentInfo instances. We keep them as pointers to avoid
 * altering DocumentInfo::mDocument refcount, since we rely on it to garbage
 * collect documents.
 */
using DocumentMap = QMap<QUrl, DocumentInfo *>;

struct DocumentFactoryPrivate {
    DocumentMap mDocumentMap;
    QUndoGroup mUndoGroup;

    /**
     * Removes items in a map if they are no longer referenced elsewhere
     */
    void garbageCollect(DocumentMap &map)
    {
        // Build a map of all unreferenced images. We use a MultiMap because in
        // rare cases documents may get accessed at the same millisecond.
        // See https://bugs.kde.org/show_bug.cgi?id=296401
        using UnreferencedImages = QMultiMap<QDateTime, QUrl>;
        UnreferencedImages unreferencedImages;

        DocumentMap::Iterator it = map.begin(), end = map.end();
        for (; it != end; ++it) {
            DocumentInfo *info = it.value();
            if (info->mDocument->ref == 1 && !info->mDocument->isModified()) {
                unreferencedImages.insert(info->mLastAccess, it.key());
            }
        }

        // Remove oldest unreferenced images. Since the map is sorted by key,
        // the oldest one is always unreferencedImages.begin().
        for (UnreferencedImages::Iterator unreferencedIt = unreferencedImages.begin(); unreferencedImages.count() > MAX_UNREFERENCED_IMAGES;
             unreferencedIt = unreferencedImages.erase(unreferencedIt)) {
            const QUrl url = unreferencedIt.value();
            LOG("Collecting" << url);
            it = map.find(url);
            Q_ASSERT(it != map.end());
            delete it.value();
            map.erase(it);
        }

#ifdef ENABLE_LOG
        logDocumentMap(map);
#endif
    }

    void logDocumentMap(const DocumentMap &map)
    {
        LOG("map:");
        DocumentMap::ConstIterator it = map.constBegin(), end = map.constEnd();
        for (; it != end; ++it) {
            LOG("-" << it.key() << "refCount=" << it.value()->mDocument.count() << "lastAccess=" << it.value()->mLastAccess);
        }
    }

    QList<QUrl> mModifiedDocumentList;
};

DocumentFactory::DocumentFactory()
    : d(new DocumentFactoryPrivate)
{
}

DocumentFactory::~DocumentFactory()
{
    qDeleteAll(d->mDocumentMap);
    delete d;
}

DocumentFactory *DocumentFactory::instance()
{
    static DocumentFactory factory;
    return &factory;
}

Document::Ptr DocumentFactory::getCachedDocument(const QUrl &url) const
{
    const DocumentInfo *info = d->mDocumentMap.value(url);
    return info ? info->mDocument : Document::Ptr();
}

Document::Ptr DocumentFactory::load(const QUrl &url)
{
    GV_RETURN_VALUE_IF_FAIL(!url.isEmpty(), Document::Ptr());
    DocumentInfo *info = nullptr;

    DocumentMap::Iterator it = d->mDocumentMap.find(url);

    if (it != d->mDocumentMap.end()) {
        LOG(url.fileName() << "url in mDocumentMap");
        info = it.value();
        info->mLastAccess = QDateTime::currentDateTime();
        return info->mDocument;
    }

    // At this point we couldn't find the document in the map

    // Start loading the document
    LOG(url.fileName() << "loading");
    auto doc = new Document(url);
    connect(doc, &Document::loaded, this, &DocumentFactory::slotLoaded);
    connect(doc, &Document::saved, this, &DocumentFactory::slotSaved);
    connect(doc, &Document::modified, this, &DocumentFactory::slotModified);
    connect(doc, &Document::busyChanged, this, &DocumentFactory::slotBusyChanged);

    // Make sure that an url passed as command line argument is loaded
    // and shown before a possibly long running dirlister on a slow
    // network device is started. So start the dirlister after url is
    // loaded or failed to load.
    connect(doc, &Document::loaded, [this, url]() {
        Q_EMIT readyForDirListerStart(url);
    });
    connect(doc, &Document::loadingFailed, [this, url]() {
        Q_EMIT readyForDirListerStart(url);
    });
    connect(doc, &Document::downSampledImageReady, [this, url]() {
        Q_EMIT readyForDirListerStart(url);
    });

    doc->reload();

    // Create DocumentInfo instance
    info = new DocumentInfo;
    Document::Ptr docPtr(doc);
    info->mDocument = docPtr;
    info->mLastAccess = QDateTime::currentDateTime();

    // Place DocumentInfo in the map
    d->mDocumentMap[url] = info;

    d->garbageCollect(d->mDocumentMap);

    return docPtr;
}

QList<QUrl> DocumentFactory::modifiedDocumentList() const
{
    return d->mModifiedDocumentList;
}

bool DocumentFactory::hasUrl(const QUrl &url) const
{
    return d->mDocumentMap.contains(url);
}

void DocumentFactory::clearCache()
{
    qDeleteAll(d->mDocumentMap);
    d->mDocumentMap.clear();
    d->mModifiedDocumentList.clear();
}

void DocumentFactory::slotLoaded(const QUrl &url)
{
    if (d->mModifiedDocumentList.contains(url)) {
        d->mModifiedDocumentList.removeAll(url);
        Q_EMIT modifiedDocumentListChanged();
        Q_EMIT documentChanged(url);
    }
}

void DocumentFactory::slotSaved(const QUrl &oldUrl, const QUrl &newUrl)
{
    const bool oldIsNew = oldUrl == newUrl;
    const bool oldUrlWasModified = d->mModifiedDocumentList.removeOne(oldUrl);
    bool newUrlWasModified = false;
    if (!oldIsNew) {
        newUrlWasModified = d->mModifiedDocumentList.removeOne(newUrl);
        DocumentInfo *info = d->mDocumentMap.take(oldUrl);
        d->mDocumentMap.insert(newUrl, info);
    }
    d->garbageCollect(d->mDocumentMap);
    if (oldUrlWasModified || newUrlWasModified) {
        Q_EMIT modifiedDocumentListChanged();
    }
    if (oldUrlWasModified) {
        Q_EMIT documentChanged(oldUrl);
    }
    if (!oldIsNew) {
        Q_EMIT documentChanged(newUrl);
    }
}

void DocumentFactory::slotModified(const QUrl &url)
{
    if (!d->mModifiedDocumentList.contains(url)) {
        d->mModifiedDocumentList << url;
        Q_EMIT modifiedDocumentListChanged();
    }
    Q_EMIT documentChanged(url);
}

void DocumentFactory::slotBusyChanged(const QUrl &url, bool busy)
{
    Q_EMIT documentBusyStateChanged(url, busy);
}

QUndoGroup *DocumentFactory::undoGroup()
{
    return &d->mUndoGroup;
}

void DocumentFactory::forget(const QUrl &url)
{
    DocumentInfo *info = d->mDocumentMap.take(url);
    if (!info) {
        return;
    }
    delete info;

    if (d->mModifiedDocumentList.contains(url)) {
        d->mModifiedDocumentList.removeAll(url);
        Q_EMIT modifiedDocumentListChanged();
    }
}

} // namespace
