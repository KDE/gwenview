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
#ifndef ABSTRACTIMAGEOPERATION_H
#define ABSTRACTIMAGEOPERATION_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QUndoCommand>

// KF

// Local
#include <lib/document/document.h>

class KJob;

namespace Gwenview
{
struct AbstractImageOperationPrivate;

/**
 * An operation to apply on a document. This class pushes internal instances of
 * QUndoCommand to the document undo stack, but it only does so if the
 * operation succeeded.
 *
 * Class inheriting from this class should:
 * - Implement redo() and call finish() or finishFromKJob() when done
 * - Implement undo()
 * - Define the operation/command text with setText()
 */
class GWENVIEWLIB_EXPORT AbstractImageOperation : public QObject
{
    Q_OBJECT
public:
    AbstractImageOperation();
    ~AbstractImageOperation() override;

    void applyToDocument(Document::Ptr);
    Document::Ptr document() const;

protected:
    virtual void redo() = 0;
    virtual void undo()
    {
    }
    void setText(const QString &);

    /**
     * Convenience method which can be called from redo() if the operation is
     * implemented as a job
     */
    void redoAsDocumentJob(DocumentJob *job);

protected Q_SLOTS:
    void finish(bool ok);

    /**
     * Convenience slot which call finish() correctly if job succeeded
     */
    void finishFromKJob(KJob *job);

private:
    AbstractImageOperationPrivate *const d;

    friend class ImageOperationCommand;
};

} // namespace

#endif /* ABSTRACTIMAGEOPERATION_H */
