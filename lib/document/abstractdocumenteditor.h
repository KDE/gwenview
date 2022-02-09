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
#ifndef ABSTRACTDOCUMENTEDITOR_H
#define ABSTRACTDOCUMENTEDITOR_H

#include <lib/gwenviewlib_export.h>

// Qt

// KF

// Local
#include <lib/orientation.h>

class QImage;

namespace Gwenview
{
/**
 * An interface which can be returned by some implementations of
 * AbstractDocumentImpl if they support edition.
 */
class GWENVIEWLIB_EXPORT AbstractDocumentEditor
{
public:
    virtual ~AbstractDocumentEditor() = default;

    /**
     * Replaces the current image with image.
     * Calling this while the document is loaded won't do anything.
     *
     * This method should only be called from a subclass of
     * AbstractImageOperation and applied through Document::undoStack().
     */
    virtual void setImage(const QImage &) = 0;

    /**
     * Apply a transformation to the document image.
     *
     * Transformations are handled by the Document class because it can be
     * done in a lossless way by some Document implementations.
     *
     * This method should only be called from a subclass of
     * AbstractImageOperation and applied through Document::undoStack().
     */
    virtual void applyTransformation(Orientation) = 0;
};

} // namespace

#endif /* ABSTRACTDOCUMENTEDITOR_H */
