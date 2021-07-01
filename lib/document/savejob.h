// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2010 Aurélien Gâteau <agateau@kde.org>

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
#ifndef SAVEJOB_H
#define SAVEJOB_H

#include <lib/gwenviewlib_export.h>

// Qt

// KF

// Local
#include <lib/document/documentjob.h>

class QByteArray;
class QUrl;

namespace Gwenview
{
class DocumentLoadedImpl;

struct SaveJobPrivate;
class GWENVIEWLIB_EXPORT SaveJob : public DocumentJob
{
    Q_OBJECT
public:
    SaveJob(DocumentLoadedImpl *impl, const QUrl &url, const QByteArray &format);
    ~SaveJob() override;
    void saveInternal();

    QUrl oldUrl() const;
    QUrl newUrl() const;

protected Q_SLOTS:
    void doStart() override;
    void slotResult(KJob *) override;

protected:
    bool doKill() override;

private Q_SLOTS:
    void finishSave();

private:
    SaveJobPrivate *const d;
};

} // namespace

#endif /* SAVEJOB_H */
