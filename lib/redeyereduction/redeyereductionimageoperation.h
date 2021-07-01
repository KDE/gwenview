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
#ifndef REDEYEREDUCTIONIMAGEOPERATION_H
#define REDEYEREDUCTIONIMAGEOPERATION_H

#include <lib/gwenviewlib_export.h>

// Qt

// KF

// Local
#include <lib/abstractimageoperation.h>

class QRectF;

namespace Gwenview
{
struct RedEyeReductionImageOperationPrivate;
class GWENVIEWLIB_EXPORT RedEyeReductionImageOperation : public AbstractImageOperation
{
public:
    RedEyeReductionImageOperation(const QRectF &);
    ~RedEyeReductionImageOperation() override;

    void redo() override;
    void undo() override;

    static void apply(QImage *img, const QRectF &rectF);

private:
    RedEyeReductionImageOperationPrivate *const d;
};

} // namespace

#endif /* REDEYEREDUCTIONIMAGEOPERATION_H */
