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
#ifndef ANIMATEDDOCUMENTLOADEDIMPL_H
#define ANIMATEDDOCUMENTLOADEDIMPL_H

// Qt

// KF

// Local
#include <lib/document/abstractdocumentimpl.h>

namespace Gwenview
{
struct AnimatedDocumentLoadedImplPrivate;
class AnimatedDocumentLoadedImpl : public AbstractDocumentImpl
{
    Q_OBJECT
public:
    AnimatedDocumentLoadedImpl(Document *, const QByteArray &);
    ~AnimatedDocumentLoadedImpl() override;

    void init() override;
    Document::LoadingState loadingState() const override;
    QByteArray rawData() const override;
    bool isAnimated() const override;
    void startAnimation() override;
    void stopAnimation() override;

private Q_SLOTS:
    void slotFrameChanged(int frameNumber);

private:
    AnimatedDocumentLoadedImplPrivate *const d;
};

} // namespace

#endif /* ANIMATEDDOCUMENTLOADEDIMPL_H */
