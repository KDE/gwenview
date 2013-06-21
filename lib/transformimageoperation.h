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
#ifndef TRANSFORMIMAGEOPERATION_H
#define TRANSFORMIMAGEOPERATION_H

#include <lib/gwenviewlib_export.h>

// Qt

// KDE

// Local
#include <lib/abstractimageoperation.h>
#include <lib/document/documentjob.h>

#include <lib/orientation.h>

namespace Gwenview
{

class TransformJob : public ThreadedDocumentJob
{
    Q_OBJECT
public:
    TransformJob(Orientation orientation);
    void threadedStart(); // reimp

private:
    Orientation mOrientation;
};

struct TransformImageOperationPrivate;
class GWENVIEWLIB_EXPORT TransformImageOperation : public AbstractImageOperation
{
public:
    TransformImageOperation(Orientation);
    ~TransformImageOperation();

    virtual void redo();
    virtual void undo();

private:
    TransformImageOperationPrivate* const d;
};

} // namespace

#endif /* TRANSFORMIMAGEOPERATION_H */
