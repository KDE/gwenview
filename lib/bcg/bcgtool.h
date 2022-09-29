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
#ifndef BCGTOOL_H
#define BCGTOOL_H

#include <lib/gwenviewlib_export.h>

// Qt

// KF

// Local
#include <lib/documentview/abstractrasterimageviewtool.h>

namespace Gwenview
{
class AbstractImageOperation;

struct BCGToolPrivate;
class GWENVIEWLIB_EXPORT BCGTool : public AbstractRasterImageViewTool
{
    Q_OBJECT
public:
    BCGTool(RasterImageView *parent);
    ~BCGTool() override;

    void paint(QPainter *painter) override;
    void keyPressEvent(QKeyEvent *event) override;
    QWidget *widget() const override;

Q_SIGNALS:
    void done();
    void imageOperationRequested(AbstractImageOperation *);

private Q_SLOTS:
    void slotBCGRequested();

private:
    BCGToolPrivate *const d;
};

} // namespace

#endif /* BCGTOOL_H */
