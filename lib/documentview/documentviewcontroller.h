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
#ifndef DOCUMENTVIEWCONTROLLER_H
#define DOCUMENTVIEWCONTROLLER_H

#include <lib/gwenviewlib_export.h>

// Local

// KDE

// Qt
#include <QObject>

class KActionCollection;

namespace Gwenview
{

class DocumentView;
class SlideContainer;
class ZoomWidget;

struct DocumentViewControllerPrivate;

/**
 * Handles all DocumentView specific actions like zooming. Calls the
 * corresponding code on its view, if any.
 */
class GWENVIEWLIB_EXPORT DocumentViewController : public QObject
{
    Q_OBJECT
public:
    explicit DocumentViewController(KActionCollection*, QObject* parent = nullptr);
    ~DocumentViewController() override;

    DocumentView* view() const;
    ZoomWidget* zoomWidget() const;

    void setView(DocumentView*);
    void setZoomWidget(ZoomWidget* widget);

    void setToolContainer(SlideContainer* container);

    void reset();

private Q_SLOTS:
    void slotAdapterChanged();
    void updateZoomToFitActionFromView();
    void updateZoomToFillActionFromView();
    void updateTool();

private:
    DocumentViewControllerPrivate* const d;
};

} // namespace

#endif /* DOCUMENTVIEWCONTROLLER_H */
