// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2009 Aurélien Gâteau <agateau@kde.org>

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
#ifndef FILEUTILS_H
#define FILEUTILS_H

class QWidget;
class QUrl;

namespace Gwenview
{
namespace FileUtils
{
enum RenameResult {
    RenamedOK, /** Renamed without problem */
    RenamedUnderNewName, /** Destination already existed, so rename() added a suffix to make the name unique */
    Skipped, /** Destination already existed and contained the same data as source, so rename() just removed the source */
    RenameFailed, /** Rename failed */
};

/**
 * Compare content of two urls, returns whether they are the same
 */
bool contentsAreIdentical(const QUrl &url1, const QUrl &url2, QWidget *authWindow = nullptr);

/**
 * Rename src to dst, returns RenameResult
 */
RenameResult rename(const QUrl &src, const QUrl &dst, QWidget *authWindow = nullptr);

} // namespace
} // namespace

#endif /* FILEUTILS_H */
