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
#ifndef MIMETYPEUTILS_P_H
#define MIMETYPEUTILS_P_H

// Qt
#include <QByteArray>
#include <QObject>

namespace KIO
{
class Job;
class TransferJob;
}

namespace Gwenview
{
namespace MimeTypeUtils
{
/**
 * A simple helper class used to determine mime type in some extreme cases
 * where we need to download the url header
 */
class DataAccumulator : public QObject
{
    Q_OBJECT
public:
    DataAccumulator(KIO::TransferJob *job);

    const QByteArray &data() const
    {
        return mData;
    }

    bool finished() const
    {
        return mFinished;
    }

private Q_SLOTS:
    void slotDataReceived(KIO::Job *, const QByteArray &data);
    void slotFinished();

private:
    QByteArray mData;
    bool mFinished;
};

} // namespace MimeTypeUtils

} // namespace Gwenview

#endif /* MIMETYPEUTILS_P_H */
