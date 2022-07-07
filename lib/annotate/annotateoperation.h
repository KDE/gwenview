/*
Gwenview: an image viewer
Copyright 2022 Ilya Pominov <ipominov@astralinux.ru>

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
#ifndef ANNOTATEOPERATION_H
#define ANNOTATEOPERATION_H

#include <lib/gwenviewlib_export.h>

// Local
#include <lib/abstractimageoperation.h>

namespace Gwenview
{
class AnnotateOperationPrivate;

class GWENVIEWLIB_EXPORT AnnotateOperation : public AbstractImageOperation
{
public:
    explicit AnnotateOperation(const QImage &image);
    ~AnnotateOperation() override;

    void redo() override;
    void undo() override;

private:
    AnnotateOperationPrivate *const d;
};

} // namespace

#endif /* ANNOTATEOPERATION_H */
