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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

*/
#ifndef LOADINGDOCUMENTIMPL_H
#define LOADINGDOCUMENTIMPL_H

// Qt

// KF

// Local
#include <lib/document/abstractdocumentimpl.h>

class KJob;
namespace KIO
{
class Job;
}

namespace Gwenview
{
struct LoadingDocumentImplPrivate;
class LoadingDocumentImpl : public AbstractDocumentImpl
{
    Q_OBJECT
public:
    LoadingDocumentImpl(Document *);
    ~LoadingDocumentImpl() override;

    void init() override;
    Document::LoadingState loadingState() const override;
    bool isEditable() const override;

    void loadImage(int invertedZoom);

private Q_SLOTS:
    void slotMetaInfoLoaded();
    void slotImageLoaded();
    void slotDataReceived(KIO::Job *, const QByteArray &);
    void slotTransferFinished(KJob *);

private:
    LoadingDocumentImplPrivate *const d;
    friend struct LoadingDocumentImplPrivate;
};

} // namespace

#endif /* LOADINGDOCUMENTIMPL_H */
