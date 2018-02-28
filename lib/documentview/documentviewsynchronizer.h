// vim: set tabstop=4 shiftwidth=4 expandtab:
/*
Gwenview: an image viewer
Copyright 2011 Aurélien Gâteau <agateau@kde.org>

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
#ifndef DOCUMENTVIEWSYNCHRONIZER_H
#define DOCUMENTVIEWSYNCHRONIZER_H

#include <lib/gwenviewlib_export.h>

// Local

// KDE

// Qt
#include <QObject>


namespace Gwenview
{

class DocumentView;

struct DocumentViewSynchronizerPrivate;
/**
 * A class to synchronize zoom and scroll of DocumentViews
 */
class GWENVIEWLIB_EXPORT DocumentViewSynchronizer : public QObject
{
    Q_OBJECT
public:
    // We pass a pointer to the view list because we don't want to maintain
    // a copy of the list itself
    explicit DocumentViewSynchronizer(const QList<DocumentView*>* views, QObject* parent = 0);
    ~DocumentViewSynchronizer();

    void setCurrentView(DocumentView* view);

public Q_SLOTS:
    void setActive(bool);

private Q_SLOTS:
    void setZoom(qreal zoom);
    void setZoomToFit(bool);
    void setZoomToFill(bool);
    void updatePosition();

private:
    DocumentViewSynchronizerPrivate* const d;
};

} // namespace

#endif /* DOCUMENTVIEWSYNCHRONIZER_H */
