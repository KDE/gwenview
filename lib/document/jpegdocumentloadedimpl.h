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
#ifndef JPEGDOCUMENTLOADEDIMPL_H
#define JPEGDOCUMENTLOADEDIMPL_H

// Qt

// KF

// Local
#include <lib/document/documentloadedimpl.h>

class QByteArray;

namespace Gwenview
{
class JpegContent;

struct JpegDocumentLoadedImplPrivate;
class JpegDocumentLoadedImpl : public DocumentLoadedImpl
{
    Q_OBJECT
public:
    JpegDocumentLoadedImpl(Document *, JpegContent *);
    ~JpegDocumentLoadedImpl() override;
    QByteArray rawData() const override;

protected:
    bool saveInternal(QIODevice *device, const QByteArray &format) override;

    // AbstractDocumentEditor
    void setImage(const QImage &) override;
    void applyTransformation(Orientation orientation) override;
    //

private:
    JpegDocumentLoadedImplPrivate *const d;
};

} // namespace

#endif /* JPEGDOCUMENTLOADEDIMPL_H */
