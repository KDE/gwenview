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
#ifndef ABSTRACTTHUMBNAILVIEWHELPER_H
#define ABSTRACTTHUMBNAILVIEWHELPER_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QObject>
#include <QUrl>

// KF

// Local

namespace Gwenview
{

/**
 * This class is used by the ThumbnailView to request various things.
 */
class GWENVIEWLIB_EXPORT AbstractThumbnailViewHelper : public QObject
{
    Q_OBJECT
public:
    explicit AbstractThumbnailViewHelper(QObject* parent);
    ~AbstractThumbnailViewHelper() override;

    virtual void showContextMenu(QWidget* parent) = 0;

    virtual void showMenuForUrlDroppedOnViewport(QWidget* parent, const QList<QUrl>&) = 0;

    virtual void showMenuForUrlDroppedOnDir(QWidget* parent, const QList<QUrl>&, const QUrl&) = 0;
};

} // namespace

#endif /* ABSTRACTTHUMBNAILVIEWHELPER_H */
