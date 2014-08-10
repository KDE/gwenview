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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Cambridge, MA 02110-1301, USA.

*/
#ifndef FILEOPERATIONS_H
#define FILEOPERATIONS_H

#include <QUrl>

class QWidget;

namespace Gwenview
{

namespace FileOperations
{

void copyTo(const QList<QUrl>& urlList, QWidget* parent);
void moveTo(const QList<QUrl>& urlList, QWidget* parent);
void linkTo(const QList<QUrl>& urlList, QWidget* parent);
void trash(const QList<QUrl>& urlList, QWidget* parent);
void del(const QList<QUrl>& urlList, QWidget* parent);
void rename(const QUrl &url, QWidget* parent);

void showMenuForDroppedUrls(QWidget* parent, const QList<QUrl>& urlList, const QUrl &destUrl);

} // namespace

} // namespace
#endif /* FILEOPERATIONS_H */
