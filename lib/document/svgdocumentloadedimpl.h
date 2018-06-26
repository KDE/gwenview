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
#ifndef SVGDOCUMENTLOADEDIMPL_H
#define SVGDOCUMENTLOADEDIMPL_H

// Qt

// KDE

// Local
#include <lib/document/abstractdocumentimpl.h>

namespace Gwenview
{

struct SvgDocumentLoadedImplPrivate;
class SvgDocumentLoadedImpl : public AbstractDocumentImpl
{
    Q_OBJECT
public:
    SvgDocumentLoadedImpl(Document*, const QByteArray&);
    ~SvgDocumentLoadedImpl() Q_DECL_OVERRIDE;

    void init() Q_DECL_OVERRIDE;

    Document::LoadingState loadingState() const Q_DECL_OVERRIDE;

    virtual void setImage(const QImage&);

    QByteArray rawData() const Q_DECL_OVERRIDE;

    QSvgRenderer* svgRenderer() const Q_DECL_OVERRIDE;

private:
    SvgDocumentLoadedImplPrivate* const d;
};

} // namespace

#endif /* SVGDOCUMENTLOADEDIMPL_H */
