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
#ifndef IMAGEMETAINFODIALOG_H
#define IMAGEMETAINFODIALOG_H

// Qt

// KDE
#include <QDialog>

// Local

namespace Gwenview
{

class ImageMetaInfoModel;

struct ImageMetaInfoDialogPrivate;
class ImageMetaInfoDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ImageMetaInfoDialog(QWidget* parent);
    ~ImageMetaInfoDialog() override;

    /**
     * Defines the image metainfo model and the preferred metainfo key list.
     */
    void setMetaInfo(ImageMetaInfoModel*, const QStringList& list);

    QSize sizeHint() const override;

Q_SIGNALS:
    void preferredMetaInfoKeyListChanged(const QStringList&);

private:
    ImageMetaInfoDialogPrivate* const d;
};

} // namespace

#endif /* IMAGEMETAINFODIALOG_H */
