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
// Self
#include "urlutils.h"

// Qt
#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QUrl>

// KF
#include <KIO/StatJob>
#include <KJobWidgets>
#include <KMountPoint>
#include <KProtocolManager>

// Local
#include <archiveutils.h>
#include <mimetypeutils.h>

namespace Gwenview
{
namespace UrlUtils
{
bool urlIsFastLocalFile(const QUrl &url)
{
    if (!url.isLocalFile()) {
        return false;
    }

    KMountPoint::List list = KMountPoint::currentMountPoints();
    KMountPoint::Ptr mountPoint = list.findByPath(url.toLocalFile());
    if (!mountPoint) {
        // We couldn't find a mount point for the url. We are probably in a
        // chroot. Assume everything is fast then.
        return true;
    }

    return !mountPoint->probablySlow();
}

bool urlIsDirectory(const QUrl &url)
{
    if (url.fileName().isEmpty()) {
        return true; // file:/somewhere/<nothing here>
    }

    // Do direct stat instead of using KIO if the file is local (faster)
    if (UrlUtils::urlIsFastLocalFile(url)) {
        return QFileInfo(url.toLocalFile()).isDir();
    }

    QWidgetList list = QApplication::topLevelWidgets();
    QWidget *parent;
    if (!list.isEmpty()) {
        parent = list[0];
    } else {
        parent = nullptr;
    }
    KIO::StatJob *job = KIO::stat(url);
    KJobWidgets::setWindow(job, parent);
    if (job->exec()) {
        return job->statResult().isDir();
    }
    return false;
}

QUrl fixUserEnteredUrl(const QUrl &in)
{
    if (!in.isRelative() && !in.isLocalFile()) {
        return in;
    }

    QFileInfo info(in.toLocalFile());
    QString path = info.absoluteFilePath();

    QUrl out = QUrl::fromLocalFile(path);
    QString mimeType = MimeTypeUtils::urlMimeType(out);

    const QString protocol = ArchiveUtils::protocolForMimeType(mimeType);

    if (!protocol.isEmpty()) {
        QUrl tmp = out;
        tmp.setScheme(protocol);
        if (KProtocolManager::supportsListing(tmp)) {
            out = tmp;
        }
    }
    return out;
}

} // namespace

} // namespace
