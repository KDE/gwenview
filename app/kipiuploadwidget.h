// vim: set tabstop=4 shiftwidth=4 noexpandtab
/*
Gwenview - A simple image viewer for KDE

Copyright  2008      Angelo Naselli

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
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef KIPIUPLOADWIDGET_H
#define KIPIUPLOADWIDGET_H

// libKipi Includes.

#include <libkipi/imagecollection.h>
#include <libkipi/uploadwidget.h>

// Local includes

namespace Gwenview
{
class KipiInterface;
class KipiUploadWidgetPriv;

class KipiUploadWidget : public KIPI::UploadWidget
{
    Q_OBJECT

public:

    KipiUploadWidget(KIPIInterface *iface, QWidget *parent=0);
    ~KipiUploadWidget();

    KIPI::ImageCollection selectedImageCollection() const;


private slots: 

    void selectUrl(const KUrl &);

private:

    KipiUploadWidgetPriv *d;
};

}  // namespace

#endif  // KIPIUPLOADWIDGET_H
