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
#ifndef BCGWIDGET_H
#define BCGWIDGET_H

#include <lib/gwenviewlib_export.h>

// Qt
#include <QWidget>

// KF

// Local
#include <lib/document/document.h>

namespace Gwenview
{
struct BCGWidgetPrivate;
class GWENVIEWLIB_EXPORT BCGWidget : public QWidget
{
    Q_OBJECT
public:
    BCGWidget();
    ~BCGWidget() override;

    int brightness() const;
    int contrast() const;
    int gamma() const;

Q_SIGNALS:
    void bcgChanged();
    void reset();
    void done(bool accept);

private:
    BCGWidgetPrivate *const d;
};

} // namespace

#endif /* BCGWIDGET_H */
