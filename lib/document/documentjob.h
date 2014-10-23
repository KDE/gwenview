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
#ifndef DOCUMENTJOB_H
#define DOCUMENTJOB_H

#include <lib/gwenviewlib_export.h>

// Qt

// KDE
#include <KCompositeJob>

// Local
#include <lib/document/document.h>

namespace Gwenview
{

struct DocumentJobPrivate;

/**
 * Represent an asynchronous task to be executed on a document
 * The task must be created on the heap and passed to an instance of Document
 * via Document::enqueueJob()
 *
 * The task behavior must be implemented in run()
 *
 * Tasks are always started from the GUI thread, and are never parallelized.
 * You can of course use threading inside your task implementation to speed it
 * up.
 */
class GWENVIEWLIB_EXPORT DocumentJob : public KCompositeJob
{
    Q_OBJECT
public:
    enum {
        NoDocumentEditorError = UserDefinedError + 1
    };
    DocumentJob();
    ~DocumentJob();

    Document::Ptr document() const;

    virtual void start() Q_DECL_OVERRIDE;

protected Q_SLOTS:
    /**
     * Implement this method to provide the task behavior.
     * You *must* emit the result() signal when your work is finished, but it
     * does not have to be finished when run() returns.
     * If you are not emitting it from the GUI thread, then use a queued
     * connection to emit it.
     */
    virtual void doStart() = 0;

    /**
     * slot-ification of emitResult()
     */
    void emitResult()
    {
        KJob::emitResult();
    }

protected:
    /**
     * Convenience method which checks document()->editor() is valid
     * and set the job error properties if it's not.
     * @return true if the editor is valid.
     */
    bool checkDocumentEditor();

private:
    void setDocument(const Document::Ptr&);

    DocumentJobPrivate* const d;

    friend class Document;
};

/**
 * A document job whose action is started in a separate thread
 */
class ThreadedDocumentJob : public DocumentJob
{
public:
    /**
     * Must be reimplemented to apply the action to the document.
     * This method is never called from the GUI thread.
     */
    virtual void threadedStart() = 0;

protected:
    virtual void doStart() Q_DECL_OVERRIDE;
};

} // namespace

#endif /* DOCUMENTJOB_H */
