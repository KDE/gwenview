// vim: set tabstop=4 shiftwidth=4 noexpandtab:
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
#ifndef DOCUMENTVIEWCONTAINER_H
#define DOCUMENTVIEWCONTAINER_H

#include <lib/gwenviewlib_export.h>

// Local

// KDE

// Qt
#include <QGraphicsView>

namespace Gwenview
{

class DocumentView;

class DocumentViewContainerPrivate;
/**
 * A container for DocumentViews which will arrange them to make best use of
 * available space.
 */
class GWENVIEWLIB_EXPORT DocumentViewContainer : public QGraphicsView
{
    Q_OBJECT
public:
    DocumentViewContainer(QWidget* parent = 0);
    ~DocumentViewContainer();

    /**
     * Create a DocumentView in the DocumentViewContainer scene
     */
    DocumentView* createView();

    /**
     * Note: this method takes ownership of the view and will delete it
     */
    void removeView(DocumentView* view);

public Q_SLOTS:
    void updateLayout();

protected:
    void showEvent(QShowEvent*);
    void resizeEvent(QResizeEvent*);

private:
    friend class ViewItem;
    DocumentViewContainerPrivate* const d;

private Q_SLOTS:
    void slotViewAnimationFinished(DocumentView*);
};

} // namespace

#endif /* DOCUMENTVIEWCONTAINER_H */
